// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   cmm_asic_connac3x.c
*    \brief  Internal driver stack will export the required procedures here for
* GLUE Layer.
*
*    This file contains all routines which are exported from MediaTek 802.11
* Wireless
*    LAN driver stack to GLUE Layer.
*/

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

#if (CFG_SUPPORT_CONNAC3X == 1)

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#include "precomp.h"
#include "wlan_lib.h"
#include "wlan_pinctrl.h"
#include "gl_coredump.h"

#if CFG_MTK_WIFI_DEVAPC
#include <linux/soc/mediatek/devapc_public.h>
#endif

#if CFG_MTK_MDDP_SUPPORT
#include "mddp.h"
#endif /* CFG_MTK_MDDP_SUPPORT */

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define CONNAC3X_HIF_DMASHDL_BASE 0x52000000

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
#if CFG_MTK_WIFI_DEVAPC
u_int8_t g_fgIsRegDevapcCb;
#endif

#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
u_int8_t g_fgWlanOnOffHoldRtnlLock;
#endif

#define USB_ACCESS_RETRY_LIMIT           1

#if defined(_HIF_USB)
#define MAX_POLLING_LOOP 2
uint32_t g_au4UsbPollAddrTbl[] = {
	CONNAC3X_U3D_RX4CSR0,
	CONNAC3X_U3D_RX5CSR0,
	CONNAC3X_U3D_RX6CSR0,
	CONNAC3X_U3D_RX7CSR0,
	CONNAC3X_U3D_RX8CSR0,
	CONNAC3X_U3D_RX9CSR0,
	CONNAC3X_UDMA_WLCFG_0,
	CONNAC3X_UDMA_WL_TX_SCH_ADDR,
	CONNAC3X_UDMA_AR_CMD_FIFO_ADDR,
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT2_CSR_TX_DROP_MODE_TEST_ADDR,
	WF_WFDMA_EXT_WRAP_CSR_WFDMA_HIF_MISC_HIF_BUSY_ADDR,
	CONNAC3X_UDMA_WL_STOP_DP_OUT_ADDR
};
uint32_t g_au4UsbPollMaskTbl[] = {
	CONNAC3X_U3D_RX_FIFOEMPTY,
	CONNAC3X_U3D_RX_FIFOEMPTY,
	CONNAC3X_U3D_RX_FIFOEMPTY,
	CONNAC3X_U3D_RX_FIFOEMPTY,
	CONNAC3X_U3D_RX_FIFOEMPTY,
	CONNAC3X_U3D_RX_FIFOEMPTY,
	CONNAC3X_UDMA_WLCFG_0_WL_TX_BUSY_MASK,
	CONNAC3X_UDMA_WL_TX_SCH_MASK,
	CONNAC3X_UDMA_AR_CMD_FIFO_MASK,
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT2_CSR_TX_DROP_MODE_TEST_MASK,
	WF_WFDMA_EXT_WRAP_CSR_WFDMA_HIF_MISC_HIF_BUSY_MASK,
	CONNAC3X_UDMA_WL_STOP_DP_OUT_DROP_MASK
};
uint32_t g_au4UsbPollValueTbl[] = {
	CONNAC3X_U3D_RX_FIFOEMPTY,
	CONNAC3X_U3D_RX_FIFOEMPTY,
	CONNAC3X_U3D_RX_FIFOEMPTY,
	CONNAC3X_U3D_RX_FIFOEMPTY,
	CONNAC3X_U3D_RX_FIFOEMPTY,
	CONNAC3X_U3D_RX_FIFOEMPTY,
	0,
	CONNAC3X_UDMA_WL_TX_SCH_IDLE,
	CONNAC3X_UDMA_AR_CMD_FIFO_MASK,
	0,
	0,
	0
};
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
void asicConnac3xCapInit(
	struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo = NULL;
	uint32_t u4HostWpdamBase = 0;

	ASSERT(prAdapter);

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	u4HostWpdamBase = prChipInfo->u4HostWfdmaBaseAddr;
	if (u4HostWpdamBase == 0) {
		DBGLOG(HAL, ERROR, "HostWfdmaBaseAddr is not set\n");
		return;
	}

	prChipInfo->u2HifTxdSize = 0;
	prChipInfo->u2TxInitCmdPort = 0;
	prChipInfo->u2TxFwDlPort = 0;
	prChipInfo->fillHifTxDesc = NULL;
	prChipInfo->u2CmdTxHdrSize = sizeof(struct CONNAC3X_WIFI_CMD);
	prChipInfo->asicFillInitCmdTxd = asicConnac3xFillInitCmdTxd;
	prChipInfo->asicFillCmdTxd = asicConnac3xFillCmdTxd;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	prChipInfo->u2UniCmdTxHdrSize = sizeof(struct CONNAC3X_WIFI_UNI_CMD);
	prChipInfo->asicFillUniCmdTxd = asicConnac3xFillUniCmdTxd;
#endif
	prChipInfo->u2RxSwPktBitMap = CONNAC3X_RX_STATUS_PKT_TYPE_SW_BITMAP;
	prChipInfo->u2RxSwPktEvent = CONNAC3X_RX_STATUS_PKT_TYPE_SW_EVENT;
	prChipInfo->u2RxSwPktFrame = CONNAC3X_RX_STATUS_PKT_TYPE_SW_FRAME;
	prChipInfo->u4ExtraTxByteCount = 0;
	asicConnac3xInitTxdHook(prAdapter, prChipInfo->prTxDescOps);
	asicConnac3xInitRxdHook(prAdapter, prChipInfo->prRxDescOps);
#if (CFG_SUPPORT_MSP == 1)
	prChipInfo->asicRxProcessRxvforMSP = asicConnac3xRxProcessRxvforMSP;
#endif /* CFG_SUPPORT_MSP == 1 */
	prChipInfo->asicRxGetRcpiValueFromRxv =
			asicConnac3xRxGetRcpiValueFromRxv;
	prChipInfo->asicRxGetRxModeValueFromRxv =
			asicConnac3xRxGetRxModeValueFromRxv;
#if (CFG_SUPPORT_PERF_IND == 1)
	prChipInfo->asicRxPerfIndProcessRXV = asicConnac3xRxPerfIndProcessRXV;
#endif
#if (CFG_CHIP_RESET_SUPPORT == 1) && (CFG_WMT_RESET_API_SUPPORT == 0)
	prChipInfo->rst_L0_notify_step2 = conn2_rst_L0_notify_step2;
#endif
#if CFG_SUPPORT_WIFI_SYSDVT
	prAdapter->u2TxTest = TX_TEST_UNLIMITIED;
	prAdapter->u2TxTestCount = 0;
	prAdapter->ucTxTestUP = TX_TEST_UP_UNDEF;
#endif /* CFG_SUPPORT_WIFI_SYSDVT */

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		prAdapter->fgEnShowHETrigger = FALSE;
		heRlmInitHeHtcACtrlOMAndUPH(prAdapter);
	}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	ehtRlmInit(prAdapter);
#endif

	switch (prGlueInfo->u4InfType) {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	case MT_DEV_INF_PCIE:
	case MT_DEV_INF_AXI:

		prChipInfo->u2TxInitCmdPort =
			TX_RING_CMD; /* Ring17 for CMD */
		prChipInfo->u2TxFwDlPort =
			TX_RING_FWDL; /* Ring16 for FWDL */
		prChipInfo->ucPacketFormat = TXD_PKT_FORMAT_TXD;
		prChipInfo->u4HifDmaShdlBaseAddr = CONNAC3X_HIF_DMASHDL_BASE;

		if (prBusInfo && prBusInfo->lowPowerOwnInit)
			prBusInfo->lowPowerOwnInit(prAdapter);
		else
			HAL_MCR_WR(prAdapter,
				   CONNAC3X_BN0_IRQ_ENA_ADDR,
				   BIT(0));

		if (prChipInfo->is_support_asic_lp) {
			HAL_MCR_WR(prAdapter,
				CONNAC3X_WPDMA_MCU2HOST_SW_INT_MASK
					(u4HostWpdamBase),
				BITS(0, 15));
		}
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

#if (CFG_ENABLE_FW_DOWNLOAD == 1)
		prChipInfo->asicEnableFWDownload = asicConnac3xEnableUsbFWDL;
#endif /* CFG_ENABLE_FW_DOWNLOAD == 1 */
		break;
#endif /* _HIF_USB */
	default:
		break;
	}
}

static void asicConnac3xFillInitCmdTxdInfo(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	u_int8_t *pucSeqNum)
{
	struct INIT_HIF_TX_HEADER *prInitHifTxHeader;
	struct HW_MAC_CONNAC3X_TX_DESC *prInitHifTxHeaderPadding;
	uint32_t u4TxdLen = sizeof(struct HW_MAC_CONNAC3X_TX_DESC);

	prInitHifTxHeaderPadding =
		(struct HW_MAC_CONNAC3X_TX_DESC *) (prCmdInfo->pucInfoBuffer);
	prInitHifTxHeader = (struct INIT_HIF_TX_HEADER *)
		(prCmdInfo->pucInfoBuffer + u4TxdLen);

	HAL_MAC_CONNAC3X_TXD_SET_TX_BYTE_COUNT(prInitHifTxHeaderPadding,
		prCmdInfo->u2InfoBufLen);
	if (!prCmdInfo->ucCID)
		HAL_MAC_CONNAC3X_TXD_SET_PKT_FORMAT(prInitHifTxHeaderPadding,
						INIT_PKT_FT_PDA_FWDL)
	else
		HAL_MAC_CONNAC3X_TXD_SET_PKT_FORMAT(prInitHifTxHeaderPadding,
						INIT_PKT_FT_CMD)
	HAL_MAC_CONNAC3X_TXD_SET_HEADER_FORMAT(prInitHifTxHeaderPadding,
						HEADER_FORMAT_COMMAND);
	HAL_MAC_CONNAC3X_TXD_SET_QUEUE_INDEX(prInitHifTxHeaderPadding, 0);

	prInitHifTxHeader->rInitWifiCmd.ucCID = prCmdInfo->ucCID;
	prInitHifTxHeader->rInitWifiCmd.ucPktTypeID = prCmdInfo->ucPktTypeID;
	prInitHifTxHeader->rInitWifiCmd.ucSeqNum =
		nicIncreaseCmdSeqNum(prAdapter);
	prInitHifTxHeader->u2TxByteCount =
		prCmdInfo->u2InfoBufLen - u4TxdLen;

	if (pucSeqNum)
		*pucSeqNum = prInitHifTxHeader->rInitWifiCmd.ucSeqNum;

	DBGLOG(INIT, INFO, "TX CMD: ID[0x%02X] SEQ[%u] LEN[%u]\n",
			prInitHifTxHeader->rInitWifiCmd.ucCID,
			prInitHifTxHeader->rInitWifiCmd.ucSeqNum,
			prCmdInfo->u2InfoBufLen);
}

static void asicConnac3xFillCmdTxdInfo(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	u_int8_t *pucSeqNum)
{
	struct CONNAC3X_WIFI_CMD *prWifiCmd;
	uint32_t u4TxdLen = sizeof(struct HW_MAC_CONNAC3X_TX_DESC);

	prWifiCmd = (struct CONNAC3X_WIFI_CMD *)prCmdInfo->pucInfoBuffer;

	HAL_MAC_CONNAC3X_TXD_SET_TX_BYTE_COUNT(
		(struct HW_MAC_CONNAC3X_TX_DESC *)prWifiCmd,
		prCmdInfo->u2InfoBufLen);
	HAL_MAC_CONNAC3X_TXD_SET_PKT_FORMAT(
		(struct HW_MAC_CONNAC3X_TX_DESC *)prWifiCmd,
		INIT_PKT_FT_CMD);
	HAL_MAC_CONNAC3X_TXD_SET_HEADER_FORMAT(
		(struct HW_MAC_CONNAC3X_TX_DESC *)prWifiCmd,
		HEADER_FORMAT_COMMAND);
	HAL_MAC_CONNAC3X_TXD_SET_QUEUE_INDEX(
		(struct HW_MAC_CONNAC3X_TX_DESC *)prWifiCmd, 0);

	prWifiCmd->ucCID = prCmdInfo->ucCID;
	prWifiCmd->ucExtenCID = prCmdInfo->ucExtCID;
	prWifiCmd->ucPktTypeID = prCmdInfo->ucPktTypeID;
	prWifiCmd->ucSetQuery = prCmdInfo->ucSetQuery;
	prWifiCmd->ucSeqNum = nicIncreaseCmdSeqNum(prAdapter);
	prWifiCmd->ucS2DIndex = prCmdInfo->ucS2DIndex;
	prWifiCmd->u2Length = prCmdInfo->u2InfoBufLen - u4TxdLen;

	if (pucSeqNum)
		*pucSeqNum = prWifiCmd->ucSeqNum;

	if (aucDebugModule[DBG_TX_IDX] & DBG_CLASS_TRACE)
		DBGLOG(TX, TRACE,
			"TX CMD: ID[0x%02X] SEQ[%u] SET[%u] LEN[%u]\n",
			prWifiCmd->ucCID, prWifiCmd->ucSeqNum,
			prWifiCmd->ucSetQuery, prCmdInfo->u2InfoBufLen);
	else
		DBGLOG_LIMITED(TX, INFO,
			"TX CMD: ID[0x%02X] SEQ[%u] SET[%u] LEN[%u]\n",
			prWifiCmd->ucCID, prWifiCmd->ucSeqNum,
			prWifiCmd->ucSetQuery, prCmdInfo->u2InfoBufLen);
}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
static void asicConnac3xFillUniCmdTxdInfo(
	struct ADAPTER *prAdapter,
	struct WIFI_UNI_CMD_INFO *prCmdInfo,
	u_int8_t *pucSeqNum)
{
	struct CONNAC3X_WIFI_UNI_CMD *prWifiCmd;
	uint32_t u4TxdLen = sizeof(struct HW_MAC_CONNAC3X_TX_DESC);

	prWifiCmd = (struct CONNAC3X_WIFI_UNI_CMD *)prCmdInfo->pucInfoBuffer;

	HAL_MAC_CONNAC3X_TXD_SET_TX_BYTE_COUNT(
		(struct HW_MAC_CONNAC3X_TX_DESC *)prWifiCmd,
		prCmdInfo->u2InfoBufLen);
	HAL_MAC_CONNAC3X_TXD_SET_PKT_FORMAT(
		(struct HW_MAC_CONNAC3X_TX_DESC *)prWifiCmd,
		INIT_PKT_FT_CMD);
	HAL_MAC_CONNAC3X_TXD_SET_HEADER_FORMAT(
		(struct HW_MAC_CONNAC3X_TX_DESC *)prWifiCmd,
		HEADER_FORMAT_COMMAND);
	HAL_MAC_CONNAC3X_TXD_SET_QUEUE_INDEX(
		(struct HW_MAC_CONNAC3X_TX_DESC *)prWifiCmd, 0);

	prWifiCmd->u2CID = prCmdInfo->u2CID;
	prWifiCmd->ucPktTypeID = prCmdInfo->ucPktTypeID;
	prWifiCmd->ucSeqNum = nicIncreaseCmdSeqNum(prAdapter);
	prWifiCmd->ucS2DIndex = prCmdInfo->ucS2DIndex;
	prWifiCmd->u2Length = prCmdInfo->u2InfoBufLen - u4TxdLen;
	prWifiCmd->ucOption = prCmdInfo->ucOption;

	if (pucSeqNum)
		*pucSeqNum = prWifiCmd->ucSeqNum;

	DBGLOG(TX, TRACE, "TX CMD: ID[0x%04X] SEQ[%u] OPT[0x%02X] LEN[%u]\n",
			prWifiCmd->u2CID, prWifiCmd->ucSeqNum,
			prWifiCmd->ucOption, prCmdInfo->u2InfoBufLen);
}

#endif

void asicConnac3xFillInitCmdTxd(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	u_int16_t *pu2BufInfoLen,
	u_int8_t *pucSeqNum, void **pCmdBuf)
{
	struct INIT_HIF_TX_HEADER *prInitHifTxHeader;
	uint32_t u4TxdLen = sizeof(struct HW_MAC_CONNAC3X_TX_DESC);

	/* We don't need to append TXD while sending fw frames. */
	if (!prCmdInfo->ucCID && pCmdBuf)
		*pCmdBuf = prCmdInfo->pucInfoBuffer;
	else {
		prInitHifTxHeader = (struct INIT_HIF_TX_HEADER *)
			(prCmdInfo->pucInfoBuffer + u4TxdLen);
		asicConnac3xFillInitCmdTxdInfo(
			prAdapter,
			prCmdInfo,
			pucSeqNum);
		if (pCmdBuf)
			*pCmdBuf =
				&prInitHifTxHeader->rInitWifiCmd.aucBuffer[0];
	}
}

u_int8_t asicConnac3xWfdmaDummyCrRead(
	struct ADAPTER *prAdapter,
	uint32_t *pu4Value)
{
	u_int32_t u4Addr = CONNAC3X_WFDMA_DUMMY_CR;
	u_int8_t fgRet = FALSE;

	HAL_MCR_EMI_RD(prAdapter, u4Addr, pu4Value, &fgRet);
	if (!fgRet)
		HAL_RMCR_RD(HIF_READ, prAdapter, u4Addr, pu4Value);

	return (*pu4Value & CONNAC3X_WFDMA_NEED_REINIT_BIT) == 0 ? TRUE : FALSE;
}


void asicConnac3xWfdmaDummyCrWrite(
	struct ADAPTER *prAdapter)
{
#if !CFG_MTK_WIFI_WFDMA_TX_RING_BK_RS
	u_int32_t u4RegValue = 0;

	asicConnac3xWfdmaDummyCrRead(prAdapter, &u4RegValue);
	u4RegValue |= CONNAC3X_WFDMA_NEED_REINIT_BIT;

	HAL_MCR_WR(prAdapter,
		CONNAC3X_WFDMA_DUMMY_CR,
		u4RegValue);
#endif /* CFG_MTK_WIFI_WFDMA_BK_RS */
}

u_int8_t asicConnac3xWfdmaIsNeedReInit(
	struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	u_int32_t u4RegValue = 0;
	u_int8_t fgNeedReInit;

	prChipInfo = prAdapter->chip_info;

	if (prChipInfo->asicWfdmaReInit == NULL)
		return FALSE;

#if 0
	/* for bus hang debug purpose */
	if (prAdapter->chip_info->checkbushang)
		prAdapter->chip_info->checkbushang((void *) prAdapter, TRUE);
#endif

	fgNeedReInit = asicConnac3xWfdmaDummyCrRead(prAdapter, &u4RegValue);

	return fgNeedReInit;
}

static void asicConnac3xWfdmaReInitImpl(struct ADAPTER *prAdapter)
{
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
#if CFG_MTK_WIFI_WFDMA_BK_RS
	{
		struct BUS_INFO *prBusInfo;
		struct GL_HIF_INFO *prHifInfo;
		uint32_t u4Idx;

		prBusInfo = prAdapter->chip_info->bus_info;

		DBGLOG(INIT, TRACE,
			"WFDMA reinit after bk/sr(deep sleep)\n");
		prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
		for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
			prHifInfo->TxRing[u4Idx].TxSwUsedIdx = 0;
			prHifInfo->TxRing[u4Idx].u4UsedCnt = 0;
			prHifInfo->TxRing[u4Idx].TxCpuIdx = 0;
		}

		if (halWpdmaGetRxDmaDoneCnt(prAdapter->prGlueInfo,
					    RX_RING_EVT)) {
			KAL_SET_BIT(RX_RING_EVT,
				prAdapter->ulNoMoreRfb);
		}
	}
#else /* CFG_MTK_WIFI_WFDMA_BK_RS */
	DBGLOG(INIT, INFO, "WFDMA reinit due to deep sleep\n");
	halWpdmaInitRing(prAdapter->prGlueInfo, true);
#endif /* CFG_MTK_WIFI_WFDMA_BK_RS */
#elif defined(_HIF_USB)
	{
		struct mt66xx_chip_info *prChipInfo = NULL;

		prChipInfo = prAdapter->chip_info;
		if (prChipInfo->is_support_asic_lp &&
		    prChipInfo->asicUsbInit)
			prChipInfo->asicUsbInit(prAdapter, prChipInfo);
	}
#endif
}

void asicConnac3xWfdmaReInit(
	struct ADAPTER *prAdapter)
{
#if !CFG_MTK_WIFI_WFDMA_TX_RING_BK_RS
	u_int8_t fgResult = FALSE;
	uint32_t u4Val = 0;

	/*WFDMA re-init flow after chip deep sleep*/
	fgResult = asicConnac3xWfdmaDummyCrRead(prAdapter, &u4Val);
	if (fgResult) {
		asicConnac3xWfdmaReInitImpl(prAdapter);
		asicConnac3xWfdmaDummyCrWrite(prAdapter);
	}
#endif /* CFG_MTK_WIFI_WFDMA_TX_RING_BK_RS */
}

void asicConnac3xFillCmdTxd(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	u_int8_t *pucSeqNum,
	void **pCmdBuf)
{
	struct CONNAC3X_WIFI_CMD *prWifiCmd;

	/* 2. Setup common CMD Info Packet */
	prWifiCmd = (struct CONNAC3X_WIFI_CMD *)prCmdInfo->pucInfoBuffer;
	asicConnac3xFillCmdTxdInfo(prAdapter, prCmdInfo, pucSeqNum);
	if (pCmdBuf)
		*pCmdBuf = &prWifiCmd->aucBuffer[0];
}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
void asicConnac3xFillUniCmdTxd(
	struct ADAPTER *prAdapter,
	struct WIFI_UNI_CMD_INFO *prCmdInfo,
	u_int8_t *pucSeqNum,
	void **pCmdBuf)
{
	struct CONNAC3X_WIFI_UNI_CMD *prWifiCmd;

	/* 2. Setup common CMD Info Packet */
	prWifiCmd = (struct CONNAC3X_WIFI_UNI_CMD *)prCmdInfo->pucInfoBuffer;
	asicConnac3xFillUniCmdTxdInfo(prAdapter, prCmdInfo, pucSeqNum);
	if (pCmdBuf)
		*pCmdBuf = &prWifiCmd->aucBuffer[0];
}
#endif

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
uint32_t asicConnac3xWfdmaCfgAddrGet(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t ucDmaIdx)
{
	struct BUS_INFO *prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	if (ucDmaIdx == 0)
		return CONNAC3X_WPDMA_GLO_CFG(prBusInfo->host_dma0_base);
	else
		return CONNAC3X_WPDMA_GLO_CFG(prBusInfo->host_dma1_base);
}

uint32_t asicConnac3xWfdmaIntRstDtxPtrAddrGet(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t ucDmaIdx)
{
	struct BUS_INFO *prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	if (ucDmaIdx == 0)
		return CONNAC3X_WPDMA_RST_DTX_PTR(prBusInfo->host_dma0_base);
	else
		return CONNAC3X_WPDMA_RST_DTX_PTR(prBusInfo->host_dma1_base);
}

uint32_t asicConnac3xWfdmaIntRstDrxPtrAddrGet(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t ucDmaIdx)
{
	struct BUS_INFO *prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	if (ucDmaIdx == 0)
		return CONNAC3X_WPDMA_RST_DRX_PTR(prBusInfo->host_dma0_base);
	else
		return CONNAC3X_WPDMA_RST_DRX_PTR(prBusInfo->host_dma1_base);
}

uint32_t asicConnac3xWfdmaHifRstAddrGet(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t ucDmaIdx)
{
	struct BUS_INFO *prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	if (ucDmaIdx == 0)
		return CONNAC3X_WPDMA_HIF_RST(prBusInfo->host_dma0_base);
	else
		return CONNAC3X_WPDMA_HIF_RST(prBusInfo->host_dma1_base);
}

void asicConnac3xWfdmaStop(struct GLUE_INFO *prGlueInfo, u_int8_t enable)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	union WPDMA_GLO_CFG_STRUCT GloCfg;
	uint32_t u4DmaCfgCr;

	u4DmaCfgCr = asicConnac3xWfdmaCfgAddrGet(prGlueInfo, 0);
	HAL_RMCR_RD(HIF_READ, prAdapter, u4DmaCfgCr, &GloCfg.word);

	if (enable == TRUE) {
		GloCfg.field_conn3x.tx_dma_en = 0;
		GloCfg.field_conn3x.rx_dma_en = 0;
	} else {
		GloCfg.field_conn3x.tx_dma_en = 1;
		GloCfg.field_conn3x.rx_dma_en = 1;
	}

	HAL_MCR_WR(prAdapter, u4DmaCfgCr, GloCfg.word);
}

u_int8_t asicConnac3xWfdmaPollingAllIdle(struct GLUE_INFO *prGlueInfo)
{
	if (!asicConnac3xWfdmaWaitIdle(prGlueInfo, 0, 100, 1000)) {
		DBGLOG(HAL, WARN,
		       "Polling PDMA idle Timeout!!\n");
		return FALSE;
	}

	return TRUE;
}

void asicConnac3xWfdmaControl(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t ucDmaIdx,
	u_int8_t enable)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	union WPDMA_GLO_CFG_STRUCT *prGloCfg = &prHifInfo->GloCfg;
	uint32_t u4DmaCfgCr;

	ASSERT(ucDmaIdx < CONNAC3X_MAX_WFDMA_COUNT);
	u4DmaCfgCr = asicConnac3xWfdmaCfgAddrGet(prGlueInfo, ucDmaIdx);

	if (enable) {
		prGloCfg->field_conn3x.pdma_bt_size = 3;
		prGloCfg->field_conn3x.tx_wb_ddone = 1;
		prGloCfg->field_conn3x.csr_axi_bufrdy_byp = 1;
		prGloCfg->field_conn3x.fifo_little_endian = 1;
		prGloCfg->field_conn3x.csr_rx_wb_ddone = 1;
		prGloCfg->field_conn3x.csr_disp_base_ptr_chain_en = 1;
		prGloCfg->field_conn3x.csr_lbk_rx_q_sel_en = 1;
		prGloCfg->field_conn3x.omit_rx_info_pfet2 = 1;
		if (ucDmaIdx == 1)
			prGloCfg->field_conn3x.omit_rx_info = 1;
		prGloCfg->field_conn3x.omit_tx_info = 1;
		prGloCfg->field_conn3x.clk_gate_dis = 1;
	} else {
		prGloCfg->field_conn3x.tx_dma_en = 0;
		prGloCfg->field_conn3x.rx_dma_en = 0;
		prGloCfg->field_conn3x.csr_disp_base_ptr_chain_en = 0;
		prGloCfg->field_conn3x.omit_tx_info = 0;
		prGloCfg->field_conn3x.omit_rx_info = 0;
		prGloCfg->field_conn3x.omit_rx_info_pfet2 = 0;
	}
	HAL_MCR_WR(prAdapter, u4DmaCfgCr, prGloCfg->word);

#if !defined(_HIF_PCIE)
	if (!enable) {
		uint32_t u4DmaRstDtxPtrCr, u4DmaRstDrxPtrCr;

		u4DmaRstDtxPtrCr = asicConnac3xWfdmaIntRstDtxPtrAddrGet(
			prGlueInfo, ucDmaIdx);
		u4DmaRstDrxPtrCr = asicConnac3xWfdmaIntRstDrxPtrAddrGet(
			prGlueInfo, ucDmaIdx);

		asicConnac3xWfdmaWaitIdle(prGlueInfo, ucDmaIdx, 100, 1000);
		/* Reset DMA Index */
		HAL_MCR_WR(prAdapter, u4DmaRstDtxPtrCr, 0xFFFFFFFF);
		HAL_MCR_WR(prAdapter, u4DmaRstDrxPtrCr, 0xFFFFFFFF);
	}
#endif
}

u_int8_t asicConnac3xWfdmaWaitIdle(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t index,
	uint32_t round,
	uint32_t wait_us)
{
	uint32_t i = 0;
	uint32_t u4RegAddr = 0;
	union WPDMA_GLO_CFG_STRUCT GloCfg = {0};
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

	if (index == 0 || index == 1)
		u4RegAddr = asicConnac3xWfdmaCfgAddrGet(prGlueInfo, index);
	else {
		DBGLOG(HAL, ERROR, "Unknown wfdma index(=%d)\n", index);
		return FALSE;
	}

	do {
		HAL_RMCR_RD(HIF_READ, prAdapter, u4RegAddr, &GloCfg.word);
		if ((GloCfg.field_conn3x.tx_dma_busy == 0) &&
		    (GloCfg.field_conn3x.rx_dma_busy == 0)) {
			DBGLOG(HAL, TRACE, "==>  DMAIdle, GloCfg=0x%x\n",
			       GloCfg.word);
			return TRUE;
		}
		kalUdelay(wait_us);
	} while ((i++) < round);

	DBGLOG(HAL, INFO, "==>  DMABusy, GloCfg=0x%x\n", GloCfg.word);

	return FALSE;
}

void asicConnac3xWfdmaTxRingBasePtrExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *tx_ring,
	u_int32_t index,
	u_int32_t u4DefVal)
{
	struct BUS_INFO *prBusInfo;
	struct ADAPTER *prAdapter;
	uint32_t phy_addr_ext = 0;

	prAdapter = prGlueInfo->prAdapter;
	prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->u4DmaMask <= 32)
		goto end;

	phy_addr_ext = (((uint64_t)tx_ring->Cell[0].AllocPa >>
			DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK) << 16;

end:
	HAL_MCR_WR(prAdapter, tx_ring->hw_cnt_addr,
		   u4DefVal | phy_addr_ext);

	DBGLOG(HAL, TRACE, "phy_addr_ext=0x%x\n", phy_addr_ext);
}

void asicConnac3xWfdmaRxRingBasePtrExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	u_int32_t index,
	u_int32_t u4DefVal)
{
	struct BUS_INFO *prBusInfo;
	struct ADAPTER *prAdapter;
	uint32_t phy_addr_ext = 0;

	prAdapter = prGlueInfo->prAdapter;
	prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->u4DmaMask <= 32)
		goto end;

	phy_addr_ext = (((uint64_t)rx_ring->Cell[0].AllocPa >>
			DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK) << 16;

end:
	HAL_MCR_WR(prAdapter, rx_ring->hw_cnt_addr,
		   u4DefVal | phy_addr_ext);

	DBGLOG(HAL, TRACE, "phy_addr_ext=0x%x\n", phy_addr_ext);
}


void asicConnac3xWfdmaTxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *tx_ring,
	u_int32_t index)
{
	struct BUS_INFO *prBusInfo;
	uint32_t ext_offset = 0;
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	if (index == TX_RING_CMD)
		ext_offset = prBusInfo->tx_ring_cmd_idx * 4;
	else if (index == TX_RING_FWDL)
		ext_offset = prBusInfo->tx_ring_fwdl_idx * 4;
	else if (prChipInfo->is_support_wacpu) {
		if (index == TX_RING_DATA0)
			ext_offset = prBusInfo->tx_ring0_data_idx * 4;
		if (index == TX_RING_DATA1)
			ext_offset = prBusInfo->tx_ring1_data_idx * 4;
		if (index == TX_RING_DATA2)
			ext_offset = prBusInfo->tx_ring2_data_idx * 4;
		if (index == TX_RING_DATA3)
			ext_offset = prBusInfo->tx_ring3_data_idx * 4;
		if (index == TX_RING_DATA_PRIO)
			ext_offset = prBusInfo->tx_prio_data_idx * 4;
		if (index == TX_RING_WA_CMD)
			ext_offset = prBusInfo->tx_ring_wa_cmd_idx * 4;
	} else {
		if (index == TX_RING_DATA0)
			ext_offset = prBusInfo->tx_ring0_data_idx * 4;
		else if (index == TX_RING_DATA1)
			ext_offset = prBusInfo->tx_ring1_data_idx * 4;
		else if (index == TX_RING_DATA2)
			ext_offset = prBusInfo->tx_ring2_data_idx * 4;
		else if (index == TX_RING_DATA3)
			ext_offset = prBusInfo->tx_ring3_data_idx * 4;
		else if (index == TX_RING_DATA_PRIO)
			ext_offset = prBusInfo->tx_prio_data_idx * 4;
		else if (index == TX_RING_DATA_ALTX)
			ext_offset = prBusInfo->tx_altx_data_idx * 4;
		else
			ext_offset = index * 4;
	}

	tx_ring->hw_desc_base_ext =
		prBusInfo->host_tx_ring_ext_ctrl_base + ext_offset;
	HAL_MCR_WR(prAdapter, tx_ring->hw_desc_base_ext,
		   CONNAC3X_TX_RING_DISP_MAX_CNT);
	asicConnac3xWfdmaTxRingBasePtrExtCtrl(
		prGlueInfo, tx_ring, index, tx_ring->u4RingSize);
}

void asicConnac3xWfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	u_int32_t index)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	uint32_t ext_offset;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	switch (index) {
	case RX_RING_EVT:
		ext_offset = 0;
		break;
	case RX_RING_DATA0:
		ext_offset = (index + 2) * 4;
		break;
	case RX_RING_DATA1:
	case RX_RING_TXDONE0:
	case RX_RING_TXDONE1:
		ext_offset = (index + 1) * 4;
		break;
	case RX_RING_WAEVT0:
	case RX_RING_WAEVT1:
		ext_offset = (index - 4) * 4;
		break;
	default:
		DBGLOG(RX, ERROR, "Error index=%d\n", index);
		return;
	}

	rx_ring->hw_desc_base_ext =
		prBusInfo->host_rx_ring_ext_ctrl_base + ext_offset;

	HAL_MCR_WR(prAdapter, rx_ring->hw_desc_base_ext,
		   CONNAC3X_RX_RING_DISP_MAX_CNT);
	asicConnac3xWfdmaRxRingBasePtrExtCtrl(
		prGlueInfo, rx_ring, index, rx_ring->u4RingSize);
}

#if CFG_MTK_WIFI_WFDMA_WB
void asicConnac3xAllocWfdmaWbBuffer(struct GLUE_INFO *prGlueInfo,
				    bool fgAllocMem)
{
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_DMABUF *prBuf;
	struct RTMP_DMABUF *prRingDmyRd, *prRingDmyWr;

	prHifInfo = &prGlueInfo->rHifInfo;
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prMemOps = &prHifInfo->rMemOps;

	if (!prChipInfo->is_support_wfdma_write_back)
		return;

	if (!prMemOps->allocExtBuf)
		return;

	prRingDmyRd = &prHifInfo->rRingDmyRd;
	prRingDmyWr = &prHifInfo->rRingDmyWr;
	prRingDmyRd->AllocSize = sizeof(uint32_t);
	prRingDmyWr->AllocSize = sizeof(uint32_t);
	if (fgAllocMem) {
		prMemOps->allocExtBuf(prHifInfo, prRingDmyRd,
				      WFDMA_WB_MEMORY_ALIGNMENT);
		prMemOps->allocExtBuf(prHifInfo, prRingDmyWr,
				      WFDMA_WB_MEMORY_ALIGNMENT);
	}
	kalMemZero(prRingDmyRd->AllocVa, prRingDmyRd->AllocSize);
	kalMemZero(prRingDmyWr->AllocVa, prRingDmyWr->AllocSize);

	/* add debug dummy memory */
	if (prChipInfo->wb_dmy_dbg_size) {
		prBuf = &prHifInfo->rRingDmyDbg;
		prBuf->AllocSize = prChipInfo->wb_dmy_dbg_size;
		if (fgAllocMem)
			prMemOps->allocExtBuf(prHifInfo, prBuf,
					      WFDMA_WB_MEMORY_ALIGNMENT);
		kalMemZero(prBuf->AllocVa, prBuf->AllocSize);
	}

	if (prChipInfo->wb_int_sta_size) {
		prBuf = &prHifInfo->rRingIntSta;
		prBuf->AllocSize = prChipInfo->wb_int_sta_size;
		if (fgAllocMem)
			prMemOps->allocExtBuf(prHifInfo, prBuf,
					      WFDMA_WB_MEMORY_ALIGNMENT);
		kalMemZero(prBuf->AllocVa, prBuf->AllocSize);
	}
	if (prChipInfo->wb_didx_size) {
		prBuf = &prHifInfo->rRingDidx;
		prBuf->AllocSize = prChipInfo->wb_didx_size;
		if (fgAllocMem)
			prMemOps->allocExtBuf(prHifInfo, prBuf,
					      WFDMA_WB_MEMORY_ALIGNMENT);
		kalMemZero(prBuf->AllocVa, prBuf->AllocSize);
	}
	if (prChipInfo->wb_cidx_size) {
		prBuf = &prHifInfo->rRingCidx;
		prBuf->AllocSize = prChipInfo->wb_cidx_size;
		if (fgAllocMem)
			prMemOps->allocExtBuf(prHifInfo, prBuf,
					      WFDMA_WB_MEMORY_ALIGNMENT);
		kalMemZero(prBuf->AllocVa, prBuf->AllocSize);
	}
	if (prChipInfo->wb_hw_done_flag_size) {
		prBuf = &prHifInfo->rHwDoneFlag;
		prBuf->AllocSize = prChipInfo->wb_hw_done_flag_size;
		if (fgAllocMem)
			prMemOps->allocExtBuf(prHifInfo, prBuf,
					      WFDMA_WB_MEMORY_ALIGNMENT);
		kalMemZero(prBuf->AllocVa, prBuf->AllocSize);
	}
	if (prChipInfo->wb_sw_done_flag_size) {
		prBuf = &prHifInfo->rSwDoneFlag;
		prBuf->AllocSize = prChipInfo->wb_sw_done_flag_size;
		if (fgAllocMem)
			prMemOps->allocExtBuf(prHifInfo, prBuf,
					      WFDMA_WB_MEMORY_ALIGNMENT);
		kalMemZero(prBuf->AllocVa, prBuf->AllocSize);
	}

	if (prChipInfo->wb_md_int_sta_size) {
		prBuf = &prHifInfo->rRingMdIntSta;
		prBuf->AllocSize = prChipInfo->wb_md_int_sta_size;
		if (fgAllocMem)
			prMemOps->allocExtBuf(prHifInfo, prBuf,
					      WFDMA_WB_MEMORY_ALIGNMENT);
		kalMemZero(prBuf->AllocVa, prBuf->AllocSize);
	}
	if (prChipInfo->wb_md_didx_size) {
		prBuf = &prHifInfo->rRingMdDidx;
		prBuf->AllocSize = prChipInfo->wb_md_didx_size;
		if (fgAllocMem)
			prMemOps->allocExtBuf(prHifInfo, prBuf,
					      WFDMA_WB_MEMORY_ALIGNMENT);
		kalMemZero(prBuf->AllocVa, prBuf->AllocSize);
	}
}

void asicConnac3xFreeWfdmaWbBuffer(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct HIF_MEM_OPS *prMemOps;

	prHifInfo = &prGlueInfo->rHifInfo;
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prMemOps = &prHifInfo->rMemOps;

	if (!prChipInfo->is_support_wfdma_write_back)
		return;

	if (!prMemOps->freeExtBuf)
		return;

	prMemOps->freeExtBuf(prHifInfo, &prHifInfo->rRingDmyRd);
	prMemOps->freeExtBuf(prHifInfo, &prHifInfo->rRingDmyWr);

	if (prChipInfo->wb_dmy_dbg_size)
		prMemOps->freeExtBuf(prHifInfo, &prHifInfo->rRingDmyDbg);
	if (prChipInfo->wb_int_sta_size)
		prMemOps->freeExtBuf(prHifInfo, &prHifInfo->rRingIntSta);
	if (prChipInfo->wb_didx_size)
		prMemOps->freeExtBuf(prHifInfo, &prHifInfo->rRingDidx);
	if (prChipInfo->wb_cidx_size)
		prMemOps->freeExtBuf(prHifInfo, &prHifInfo->rRingCidx);
	if (prChipInfo->wb_hw_done_flag_size)
		prMemOps->freeExtBuf(prHifInfo, &prHifInfo->rHwDoneFlag);
	if (prChipInfo->wb_sw_done_flag_size)
		prMemOps->freeExtBuf(prHifInfo, &prHifInfo->rSwDoneFlag);

	if (prChipInfo->wb_md_int_sta_size)
		prMemOps->freeExtBuf(prHifInfo, &prHifInfo->rRingMdIntSta);
	if (prChipInfo->wb_md_didx_size)
		prMemOps->freeExtBuf(prHifInfo, &prHifInfo->rRingMdDidx);
}
#endif /* CFG_MTK_WIFI_WFDMA_WB */

void asicConnac3xEnablePlatformIRQ(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_VAR *prWifiVar;

	ASSERT(prAdapter);

	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
#if defined(_HIF_PCIE)
	mtk_pci_enable_irq(prAdapter->prGlueInfo);
#else
	enable_irq(prHifInfo->u4IrqId);
	KAL_CLR_BIT(0, prHifInfo->ulHifIntEnBits);
#endif

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	/* IrqId_1 is MAWD interrupt */
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd) &&
	    KAL_TEST_AND_CLEAR_BIT(1, prHifInfo->ulHifIntEnBits))
		enable_irq(prHifInfo->u4IrqId_1);
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
}

void asicConnac3xDisablePlatformIRQ(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_VAR *prWifiVar;

	ASSERT(prAdapter);

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;
#if defined(_HIF_PCIE)
	mtk_pci_disable_irq(prAdapter->prGlueInfo);
#else
	disable_irq_nosync(prHifInfo->u4IrqId);
	KAL_SET_BIT(0, prHifInfo->ulHifIntEnBits);
#endif
}

#if defined(_HIF_AXI)
void asicConnac3xDisablePlatformSwIRQ(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = NULL;

	ASSERT(prAdapter);

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	disable_irq_nosync(prHifInfo->u4IrqId_1);
}
#endif

void asicConnac3xLowPowerOwnRead(
	struct ADAPTER *prAdapter,
	u_int8_t *pfgResult)
{
	struct mt66xx_chip_info *prChipInfo;

	prChipInfo = prAdapter->chip_info;

	if (prChipInfo->is_support_asic_lp) {
		u_int32_t u4RegValue = 0;

		HAL_RMCR_RD(LPOWN_READ, prAdapter,
				CONNAC3X_BN0_LPCTL_ADDR,
				&u4RegValue);
		*pfgResult = (u4RegValue &
				PCIE_LPCR_AP_HOST_OWNER_STATE_SYNC)
				== 0 ? TRUE : FALSE;
	} else
		*pfgResult = TRUE;
}

#if IS_ENABLED(CFG_MTK_WIFI_DRV_OWN_INT_MODE)
void asicConnac3xLowPowerOwnSet(
	struct ADAPTER *prAdapter,
	u_int8_t *pfgResult)
{
	struct mt66xx_chip_info *prChipInfo;

	prChipInfo = prAdapter->chip_info;

	if (prChipInfo->is_support_asic_lp) {

		HAL_MCR_WR(prAdapter,
			CONNAC3X_BN0_LPCTL_ADDR,
			PCIE_LPCR_HOST_SET_OWN);

#if defined(_HIF_PCIE)
		if (prChipInfo->bus_info->hwControlVote)
			prChipInfo->bus_info->hwControlVote(prAdapter,
				TRUE, PCIE_VOTE_USER_DRVOWN);
#endif

	}
	*pfgResult = TRUE;
}

void asicConnac3xLowPowerOwnClear(
	struct ADAPTER *prAdapter,
	u_int8_t *pfgResult)
{
	struct mt66xx_chip_info *prChipInfo;

	prChipInfo = prAdapter->chip_info;

	if (prChipInfo->is_support_asic_lp) {

#if defined(_HIF_PCIE)
		if (prChipInfo->bus_info->hwControlVote)
			prChipInfo->bus_info->hwControlVote(prAdapter,
				FALSE, PCIE_VOTE_USER_DRVOWN);
#endif
#if CFG_MTK_WIFI_PCIE_SUPPORT
		mtk_pcie_dump_link_info(0);
#endif

		if (prChipInfo->setCrypto) {
			DBGLOG(HAL, TRACE, "setCrypto\n");
			prChipInfo->setCrypto(prAdapter);
		}

		clear_bit(GLUE_FLAG_DRV_OWN_INT_BIT,
			&prAdapter->prGlueInfo->ulFlag);

		HAL_MCR_WR(prAdapter,
			CONNAC3X_BN0_LPCTL_ADDR,
			PCIE_LPCR_HOST_CLR_OWN);
	}

	*pfgResult = TRUE;
}

#else /* !IS_ENABLED(CFG_MTK_WIFI_DRV_OWN_INT_MODE) */

void asicConnac3xLowPowerOwnSet(
	struct ADAPTER *prAdapter,
	u_int8_t *pfgResult)
{
	struct mt66xx_chip_info *prChipInfo;
	u_int32_t u4RegValue = 0;

	prChipInfo = prAdapter->chip_info;

	if (prChipInfo->is_support_asic_lp) {

		HAL_MCR_WR(prAdapter,
			CONNAC3X_BN0_LPCTL_ADDR,
			PCIE_LPCR_HOST_SET_OWN);

		HAL_RMCR_RD(LPOWN_READ, prAdapter,
			CONNAC3X_BN0_LPCTL_ADDR,
			&u4RegValue);

		*pfgResult = (u4RegValue &
			PCIE_LPCR_AP_HOST_OWNER_STATE_SYNC) == 0x4;

#if defined(_HIF_PCIE)
		if (prChipInfo->bus_info->hwControlVote)
			prChipInfo->bus_info->hwControlVote(prAdapter,
				TRUE, PCIE_VOTE_USER_DRVOWN);
#endif

	} else
		*pfgResult = TRUE;
}

void asicConnac3xLowPowerOwnClear(
	struct ADAPTER *prAdapter,
	u_int8_t *pfgResult)
{
	u_int32_t u4RegValue = 0;
	struct mt66xx_chip_info *prChipInfo;

	prChipInfo = prAdapter->chip_info;

	if (prChipInfo->is_support_asic_lp) {

#if defined(_HIF_PCIE)
		if (prChipInfo->bus_info->hwControlVote)
			prChipInfo->bus_info->hwControlVote(prAdapter,
				FALSE, PCIE_VOTE_USER_DRVOWN);
#endif

		HAL_MCR_WR(prAdapter,
			CONNAC3X_BN0_LPCTL_ADDR,
			PCIE_LPCR_HOST_CLR_OWN);

		HAL_RMCR_RD(LPOWN_READ, prAdapter,
			CONNAC3X_BN0_LPCTL_ADDR,
			&u4RegValue);

		*pfgResult = (u4RegValue &
				PCIE_LPCR_AP_HOST_OWNER_STATE_SYNC) == 0;

	} else
		*pfgResult = TRUE;
}
#endif /* !IS_ENABLED(CFG_MTK_WIFI_DRV_OWN_INT_MODE) */

void asicConnac3xProcessSoftwareInterrupt(
	struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct ERR_RECOVERY_CTRL_T *prErrRecoveryCtrl;
	struct HIF_MEM *prMem = NULL;
	uint32_t u4Status = 0, u4Addr = 0, u4Idx;
	uint32_t u4HostWpdamBase = 0;
	u_int8_t fgRet = FALSE;

	if (prAdapter->prGlueInfo == NULL) {
		DBGLOG(HAL, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prChipInfo = prAdapter->chip_info;
	prErrRecoveryCtrl = &prHifInfo->rErrRecoveryCtl;
	u4HostWpdamBase = prChipInfo->u4HostWfdmaBaseAddr;
	if (u4HostWpdamBase == 0) {
		DBGLOG(HAL, ERROR, "HostWfdmaBaseAddr is not set\n");
		return;
	}

	if (prMemOps->getWifiMiscRsvEmi) {
		prMem = prMemOps->getWifiMiscRsvEmi(
			prChipInfo, WIFI_MISC_MEM_BLOCK_SER_STATUS);
	}

	if (prMem && prMem->va) {
		struct SER_EMI_STATUS *prEmiSta =
			(struct SER_EMI_STATUS *)prMem->va;

		for (u4Idx = 0; u4Idx < HIF_EMI_SER_STATUS_SIZE; u4Idx++) {
			if (prEmiSta->ucStatus[u4Idx]) {
				u4Status |= BIT(u4Idx);
				prEmiSta->ucStatus[u4Idx] = 0;
			}
		}

		if (u4Status)
			DBGLOG(HAL, INFO, "STA[0x%08x]\n", u4Status);
	} else {
		u4Addr = CONNAC3X_WPDMA_MCU2HOST_SW_INT_STA(u4HostWpdamBase);
		HAL_MCR_EMI_RD(prAdapter, u4Addr, &u4Status, &fgRet);
		if (!fgRet)
			HAL_RMCR_RD(SER_READ, prAdapter, u4Addr, &u4Status);
	}

	if (u4Status) {
		u4Addr = CONNAC3X_WPDMA_MCU2HOST_SW_INT_STA(u4HostWpdamBase);
		HAL_MCR_WR(prAdapter, u4Addr, u4Status);
	}

	prErrRecoveryCtrl->u4BackupStatus = u4Status;
	if (u4Status & ERROR_DETECT_SUBSYS_BUS_TIMEOUT) {
		DBGLOG(INIT, ERROR, "[SER][L0.5] wfsys timeout!!\n");
		GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_SUBSYS_BUS_HANG);
	} else if (u4Status & ERROR_DETECT_MASK) {
		prErrRecoveryCtrl->u4Status = u4Status;
		halHwRecoveryFromError(prAdapter);
	} else
		DBGLOG(HAL, TRACE, "undefined SER status[0x%x].\n", u4Status);
}

void asicConnac3xSoftwareInterruptMcu(
	struct ADAPTER *prAdapter, u_int32_t intrBitMask)
{
	struct GLUE_INFO *prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4McuWpdamBase = 0;

	if (prAdapter == NULL || prAdapter->prGlueInfo == NULL) {
		DBGLOG(HAL, ERROR, "prAdapter or prGlueInfo is NULL\n");
		return;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	u4McuWpdamBase = prChipInfo->u4McuWfdmaBaseAddr;
	if (u4McuWpdamBase == 0) {
		DBGLOG(HAL, ERROR, "McuWfdmaBaseAddr is not set\n");
		return;
	}

	kalDevRegWrite(prGlueInfo,
		CONNAC3X_WPDMA_HOST2MCU_SW_INT_SET(u4McuWpdamBase),
		intrBitMask);
}

uint32_t asicConnac3xGetMdSoftwareInterruptStatus(
	struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct ERR_RECOVERY_CTRL_T *prErrRecoveryCtrl;
	uint32_t u4Status = 0, u4Addr = 0;
	uint32_t u4HostWpdamBase = 0;
	u_int8_t fgRet = FALSE;

	if (prAdapter->prGlueInfo == NULL) {
		DBGLOG(HAL, ERROR, "prGlueInfo is NULL\n");
		return 0;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prChipInfo = prAdapter->chip_info;
	prErrRecoveryCtrl = &prHifInfo->rErrRecoveryCtl;
	u4HostWpdamBase = prChipInfo->u4HostWfdmaBaseAddr;
	if (u4HostWpdamBase == 0) {
		DBGLOG(HAL, ERROR, "HostWfdmaBaseAddr is not set\n");
		return 0;
	}

	u4Addr = CONNAC3X_WPDMA_MCU2MD_SW_INT_STA(u4HostWpdamBase);
	HAL_MCR_EMI_RD(prAdapter, u4Addr, &u4Status, &fgRet);
	if (!fgRet)
		HAL_RMCR_RD(SER_READ, prAdapter, u4Addr, &u4Status);

	return u4Status & BITS(0, 15);
}

void asicConnac3xHifRst(
	struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4HifRstCr;

	u4HifRstCr = asicConnac3xWfdmaHifRstAddrGet(prGlueInfo, 0);
	/* Reset dmashdl and wpdma */
	kalDevRegWrite(prGlueInfo, u4HifRstCr, 0x00000000);
	kalDevRegWrite(prGlueInfo, u4HifRstCr, 0x00000030);

	u4HifRstCr = asicConnac3xWfdmaHifRstAddrGet(prGlueInfo, 1);
	/* Reset dmashdl and wpdma */
	kalDevRegWrite(prGlueInfo, u4HifRstCr, 0x00000000);
	kalDevRegWrite(prGlueInfo, u4HifRstCr, 0x00000030);
}
#endif /* _HIF_PCIE */

#if defined(_HIF_USB)
/*
 * tx_ring config
 * 1.1tx_ring_ext_ctrl
 * 7c025600[31:0]: 0x00800004 (ring 0 BASE_PTR & max_cnt for EP4)
 * 7c025604[31:0]: 0x00c00004 (ring 1 BASE_PTR & max_cnt for EP5)
 * 7c025608[31:0]: 0x01000004 (ring 2 BASE_PTR & max_cnt for EP6)
 * 7c02560c[31:0]: 0x01400004 (ring 3 BASE_PTR & max_cnt for EP7)
 * 7c025610[31:0]: 0x01800004 (ring 4 BASE_PTR & max_cnt for EP9)
 * 7c025640[31:0]: 0x02800004 (ring 16 BASE_PTR & max_cnt for EP4/FWDL)
 * 7c02563C[31:0]: 0x02c00004 (ring 15 BASE_PTR & max_cnt for EP8/WMCPU)
 * 7c025650[31:0]: 0x03800004 (ring 20 BASE_PTR & max_cnt for EP8/WACPU)
 *
 * WFDMA_GLO_CFG Setting
 * 2.1 WFDMA_GLO_CFG: 7c025208[28][27]=2'b11;
 * 2.2 WFDMA_GLO_CFG: 7c025208[20]=1'b1;
 * 2.3 WFDMA_GLO_CFG: 7c025208[9]=1'b1;
 *
 * 3.	trx_dma_en:
 * 3.1 WFDMA_GLO_CFG: 7c025208[2][0]=1'b1;
 */
void asicConnac3xWfdmaInitForUSB(
	struct ADAPTER *prAdapter,
	struct mt66xx_chip_info *prChipInfo)
{
	struct BUS_INFO *prBusInfo;
	uint32_t idx;
	uint32_t u4WfdmaAddr, u4WfdmaCr;

	prBusInfo = prChipInfo->bus_info;

	if (prChipInfo->u4HostWfdmaBaseAddr == 0) {
		DBGLOG(HAL, ERROR, "WfdmaBaseAddr is not set\n");
		return;
	}
	if (prChipInfo->u4DmaShdlBaseAddr == 0) {
		DBGLOG(HAL, ERROR, "DmaShdlBaseAddr is not set\n");
		return;
	}

	/* HAL_RMCR_RD(HIF_USB, prAdapter, 0x7c00e400, &u4WfdmaCr); */
	/* HAL_MCR_WR(prAdapter, 0x7c00e400, 0xFF); */

	HAL_RMCR_RD(HIF_USB, prAdapter, 0x7c021100, &u4WfdmaCr);
	HAL_MCR_WR(prAdapter, 0x7c021100, 0x0);

	u4WfdmaAddr = CONNAC3X_TX_RING_EXT_CTRL_BASE(
		prChipInfo->u4HostWfdmaBaseAddr);
	/*
	 * HOST_DMA1_WPDMA_TX_RING0_EXT_CTRL ~ HOST_DMA1_WPDMA_TX_RING4_EXT_CTRL
	 */
	for (idx = 0; idx < USB_TX_EPOUT_NUM; idx++) {
		HAL_RMCR_RD(HIF_USB, prAdapter,
			       u4WfdmaAddr + (idx*4), &u4WfdmaCr);
		u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_MAX_CNT_MASK;
		u4WfdmaCr |= CONNAC3X_TX_RING_DISP_MAX_CNT;
		u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_BASE_PTR_MASK;
		u4WfdmaCr |= (0x008 + 0x4 * idx)<<20;
		HAL_MCR_WR(prAdapter, u4WfdmaAddr + (idx*4), u4WfdmaCr);
	}

	/* HOST_DMA1_WPDMA_TX_RING16_EXT_CTRL_ADDR */
	HAL_RMCR_RD(HIF_USB, prAdapter, u4WfdmaAddr + 0x40, &u4WfdmaCr);
	u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_MAX_CNT_MASK;
	u4WfdmaCr |= CONNAC3X_TX_RING_DISP_MAX_CNT;
	u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_BASE_PTR_MASK;
	u4WfdmaCr |= 0x02800000;
	HAL_MCR_WR(prAdapter, u4WfdmaAddr + 0x40, u4WfdmaCr);

	/* HOST_DMA1_WPDMA_TX_RING15_EXT_CTRL_ADDR */
	HAL_RMCR_RD(HIF_USB, prAdapter, u4WfdmaAddr + 0x3c, &u4WfdmaCr);
	u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_MAX_CNT_MASK;
	u4WfdmaCr |= CONNAC3X_TX_RING_DISP_MAX_CNT;
	u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_BASE_PTR_MASK;
	u4WfdmaCr |= 0x02c00000;
	HAL_MCR_WR(prAdapter, u4WfdmaAddr + 0x3c, u4WfdmaCr);

	u4WfdmaAddr = CONNAC3X_WPDMA_GLO_CFG(
		prChipInfo->u4HostWfdmaBaseAddr);
	HAL_RMCR_RD(HIF_USB, prAdapter, u4WfdmaAddr, &u4WfdmaCr);
	u4WfdmaCr &= ~(CONNAC3X_WPDMA1_GLO_CFG_OMIT_RX_INFO);
	u4WfdmaCr |=
		(CONNAC3X_WPDMA1_GLO_CFG_OMIT_TX_INFO |
		 CONNAC3X_WPDMA1_GLO_CFG_OMIT_RX_INFO_PFET2 |
		 CONNAC3X_WPDMA1_GLO_CFG_FW_DWLD_Bypass_dmashdl |
		 CONNAC3X_WPDMA1_GLO_CFG_RX_DMA_EN |
		 CONNAC3X_WPDMA1_GLO_CFG_TX_DMA_EN);
	HAL_MCR_WR(prAdapter, u4WfdmaAddr, u4WfdmaCr);

	prChipInfo->is_support_dma_shdl = wlanCfgGetUint32(prAdapter,
				    "DmaShdlEnable",
				    FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	if (!prChipInfo->is_support_dma_shdl) {
		/*
		 *	To disable 0x7C0252B0[6] DMASHDL
		 */
		u4WfdmaAddr = CONNAC3X_WPDMA_GLO_CFG_EXT0(
			prChipInfo->u4HostWfdmaBaseAddr);
		HAL_RMCR_RD(HIF_USB, prAdapter, u4WfdmaAddr, &u4WfdmaCr);
		u4WfdmaCr &= ~CONNAC3X_WPDMA1_GLO_CFG_EXT0_TX_DMASHDL_EN;
		HAL_MCR_WR(prAdapter, u4WfdmaAddr, u4WfdmaCr);

		/*
		 *	[28]DMASHDL_BYPASS
		 *	DMASHDL host ask and quota control function bypass
		 *	0: Disable
		 *	1: Enable
		 */
		u4WfdmaAddr = CONNAC3X_HOST_DMASHDL_SW_CONTROL(
			prChipInfo->u4DmaShdlBaseAddr);
		HAL_RMCR_RD(HIF_USB, prAdapter, u4WfdmaAddr, &u4WfdmaCr);
		u4WfdmaCr |= CONNAC3X_HIF_DMASHDL_BYPASS_EN;
		HAL_MCR_WR(prAdapter, u4WfdmaAddr, u4WfdmaCr);
	}

	if (prChipInfo->asicUsbInit_ic_specific)
		prChipInfo->asicUsbInit_ic_specific(prAdapter, prChipInfo);
}

uint8_t asicConnac3xUsbEventEpDetected(struct ADAPTER *prAdapter)
{
	return USB_DATA_EP_IN;
}

#if CFG_ENABLE_FW_DOWNLOAD
void asicConnac3xEnableUsbFWDL(
	struct ADAPTER *prAdapter,
	u_int8_t fgEnable)
{
	struct GLUE_INFO *prGlueInfo;
	struct BUS_INFO *prBusInfo;
	struct mt66xx_chip_info *prChipInfo;

	uint32_t u4Value = 0;

	ASSERT(prAdapter);

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	HAL_RMCR_RD(HIF_USB, prAdapter, prBusInfo->u4UdmaTxQsel, &u4Value);
	if (fgEnable)
		u4Value |= FW_DL_EN;
	else
		u4Value &= ~FW_DL_EN;

	HAL_MCR_WR(prAdapter, prBusInfo->u4UdmaTxQsel, u4Value);
}
#endif /* CFG_ENABLE_FW_DOWNLOAD */

void asicConnac3xUdmaRxFlush(
	struct ADAPTER *prAdapter,
	u_int8_t bEnable)
{
	struct BUS_INFO *prBusInfo;
	uint32_t u4Value;

	prBusInfo = prAdapter->chip_info->bus_info;

	HAL_RMCR_RD(HIF_USB, prAdapter, prBusInfo->u4UdmaWlCfg_0_Addr,
		   &u4Value);
	if (bEnable)
		u4Value |= UDMA_WLCFG_0_RX_FLUSH_MASK;
	else
		u4Value &= ~UDMA_WLCFG_0_RX_FLUSH_MASK;
	HAL_MCR_WR(prAdapter, prBusInfo->u4UdmaWlCfg_0_Addr,
		   u4Value);
}

u_int8_t asicConnac3xUsbResume(
	struct ADAPTER *prAdapter,
	struct GLUE_INFO *prGlueInfo)
{
	uint8_t count = 0;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct BUS_INFO *prBusInfo;
	uint32_t u4Value, u4Idx, u4Loop;
	uint32_t u4PollingFail = FALSE;
	struct CHIP_DBG_OPS *prDbgOps;

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prDbgOps = prChipInfo->prDebugOps;

#if 0 /* enable it if need to do bug fixing by vender request */
	/* NOTE: USB bus may not really do suspend and resume*/
	ret = usb_control_msg(prGlueInfo->rHifInfo.udev,
			  usb_sndctrlpipe(prGlueInfo->rHifInfo.udev, 0),
			  VND_REQ_FEATURE_SET,
			  prBusInfo->u4device_vender_request_out,
			  FEATURE_SET_WVALUE_RESUME, 0, NULL, 0,
			  VENDOR_TIMEOUT_MS);
	if (ret)
		DBGLOG(HAL, ERROR,
		"VendorRequest FeatureSetResume ERROR: %x\n",
		(unsigned int)ret);
#endif

	glUsbSetState(&prGlueInfo->rHifInfo, USB_STATE_PRE_RESUME);

	if (asicConnac3xWfdmaIsNeedReInit(prAdapter)) {
		DBGLOG(INIT, INFO,
		       "Deep sleep happens in suspend\n");

		/* reinit USB because LP could clear WFDMA's CR */
		if (prChipInfo->asicWfdmaReInit)
			prChipInfo->asicWfdmaReInit(prAdapter);
	}

	for (u4Loop = 0; u4Loop < MAX_POLLING_LOOP; u4Loop++) {
		for (u4Idx = 0; u4Idx < ARRAY_SIZE(g_au4UsbPollAddrTbl);
			u4Idx++) {
			HAL_RMCR_RD(HIF_USB, prAdapter,
				g_au4UsbPollAddrTbl[u4Idx], &u4Value);
			if ((u4Value & g_au4UsbPollMaskTbl[u4Idx])
				!= g_au4UsbPollValueTbl[u4Idx]) {
				DBGLOG(HAL, ERROR,
					"Polling [0x%08x] VALUE [0x%08x]\n",
					g_au4UsbPollAddrTbl[u4Idx], u4Value);
				u4PollingFail = TRUE;
			}
		}
		if (u4PollingFail == TRUE) {
			if (u4Loop == (MAX_POLLING_LOOP - 1)) {
				if (prDbgOps && prDbgOps->showPdmaInfo)
					prDbgOps->showPdmaInfo(prAdapter);
			} else {
				u4PollingFail = FALSE;
				msleep(100);
			}
		} else
			break;
	}

	/* To trigger CR4 path */
	wlanSendDummyCmd(prAdapter, FALSE);
	halEnableInterrupt(prAdapter);

	/* using inband cmd to inform FW instead of vendor request */
	/* All Resume operations move to FW */
	halUSBPreResumeCmd(prAdapter);

	while (prGlueInfo->rHifInfo.state != USB_STATE_LINK_UP) {
		if (count > 50) {
			DBGLOG(HAL, ERROR, "pre_resume timeout\n");

			/* If coredump is on-going, then we shall wait
			 * until coredump is finished and subsequent
			 * reset would be selected at that tiime.
			 */
#if (CFG_CE_ASSERT_DUMP == 1)
			if (!prAdapter->fgN9AssertDumpOngoing)
				GL_DEFAULT_RESET_TRIGGER(prAdapter,
							 RST_CMD_EVT_FAIL);
#endif
			break;
		}
		msleep(20);
		count++;
	}

	DBGLOG(HAL, STATE, "pre_resume event check(count %d)\n", count);

	wlanResumePmHandle(prGlueInfo);

	return TRUE;
}

uint16_t asicConnac3xUsbRxByteCount(
	struct ADAPTER *prAdapter,
	struct BUS_INFO *prBusInfo,
	uint8_t *pRXD)
{

	uint16_t u2RxByteCount;
	uint8_t ucPacketType;

	ucPacketType = HAL_MAC_CONNAC3X_RX_STATUS_GET_PKT_TYPE(
		(struct HW_MAC_CONNAC3X_RX_DESC *)pRXD);
	u2RxByteCount = HAL_MAC_CONNAC3X_RX_STATUS_GET_RX_BYTE_CNT(
		(struct HW_MAC_CONNAC3X_RX_DESC *)pRXD);

	u2RxByteCount = ALIGN_16(u2RxByteCount) + LEN_USB_RX_PADDING_CSO;

	return u2RxByteCount;
}

#endif /* _HIF_USB */

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
void fillConnac3xNicTxDescAppendWithSdo(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint8_t *prTxDescBuffer)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	union HW_MAC_TX_DESC_APPEND *prHwTxDescAppend;

	/* Fill TxD append */
	prHwTxDescAppend = (union HW_MAC_TX_DESC_APPEND *)prTxDescBuffer;
	kalMemZero(prHwTxDescAppend, prChipInfo->txd_append_size);
	prHwTxDescAppend->CR4_APPEND.u2PktFlags =
		HIF_PKT_FLAGS_CT_INFO_STA_APPLY_OVERRIDE;
#if (CFG_TX_MGMT_BY_DATA_Q == 1)
	if (prMsduInfo->fgMgmtUseDataQ)
		prHwTxDescAppend->CR4_APPEND.u2PktFlags =
			HIF_PKT_FLAGS_CT_INFO_APPLY_TXD;
#endif
#if CFG_SUPPORT_WED_PROXY
	prHwTxDescAppend->CR4_APPEND.u2PktFlags =
		HIF_PKT_FLAGS_CT_INFO_APPLY_TXD;
#endif
	prHwTxDescAppend->CR4_APPEND.ucBssIndex =
		prMsduInfo->ucBssIndex;
	prHwTxDescAppend->CR4_APPEND.ucWtblIndex =
		prMsduInfo->ucWlanIndex;
}

void fillConnac3xNicTxDescAppendWithSdoV2(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint8_t *prTxDescBuffer)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	union HW_MAC_TX_DESC_APPEND *prHwTxDescAppend;

	/* Fill TxD append */
	prHwTxDescAppend = (union HW_MAC_TX_DESC_APPEND *)prTxDescBuffer;
	kalMemZero(prHwTxDescAppend, prChipInfo->txd_append_size);
	prHwTxDescAppend->CR4_APPEND.u2PktFlags =
		HIF_PKT_FLAGS_CT_INFO_APPLY_OVERRIDE;
#if (CFG_TX_MGMT_BY_DATA_Q == 1)
	if (prMsduInfo->fgMgmtUseDataQ)
		prHwTxDescAppend->CR4_APPEND.u2PktFlags =
			HIF_PKT_FLAGS_CT_INFO_APPLY_TXD;
#endif
	prHwTxDescAppend->CR4_APPEND.ucBssIndex =
		prMsduInfo->ucBssIndex;
	prHwTxDescAppend->CR4_APPEND.ucWtblIndex =
		prMsduInfo->ucWlanIndex;
}

void fillConnac3xTxDescAppendBySdo(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint16_t u4MsduId,
	phys_addr_t rDmaAddr, uint32_t u4Idx,
	u_int8_t fgIsLast,
	uint8_t *pucBuffer)
{
	union HW_MAC_TX_DESC_APPEND *prHwTxDescAppend;
	uint32_t u4AddrExt = ((uint64_t)rDmaAddr >> DMA_BITS_OFFSET) &
		DMA_HIGHER_4BITS_MASK;

	prHwTxDescAppend = (union HW_MAC_TX_DESC_APPEND *)
		(pucBuffer + NIC_TX_DESC_LONG_FORMAT_LENGTH);

	prHwTxDescAppend->CR4_APPEND.u2MsduToken = u4MsduId;
	prHwTxDescAppend->CR4_APPEND.ucBufNum = 1;
	prHwTxDescAppend->CR4_APPEND.au4BufPtr[0] = rDmaAddr;
	prHwTxDescAppend->CR4_APPEND.au2BufLen[0] =
		prMsduInfo->u2FrameLength | (u4AddrExt << 12);
}

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
uint32_t u4MawdPacketCnt;
void fillConnac3xTxDescAppendByMawdSdo(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint16_t u4MsduId,
	dma_addr_t rDmaAddr,
	uint32_t u4Idx,
	u_int8_t fgIsLast,
	uint8_t *pucBuffer)
{
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_TX_RING *prTxRing;
	struct RTMP_DMACB *pTxCell;
	struct HW_MAC_CONNAC3X_TX_DESC *prTxDesc;
	uint32_t u4AddrExt = ((uint64_t)rDmaAddr >> DMA_BITS_OFFSET) &
		DMA_HIGHER_4BITS_MASK;
	union HW_MAC_TX_DESC_APPEND *prHwTxDescAppend;
	uint8_t *pucData;
	struct BUS_INFO *prBusInfo;
	union mawd_l2tbl rL2Tbl = {0};
	uint16_t u2EtherTypeLen = 0, u2Port = TX_RING_DATA0;
	uint8_t ucType = 0;
	uint8_t ucIpVer, u4TxDSize;

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	u2Port = halTxRingDataSelect(prAdapter, prMsduInfo);

	u4TxDSize = NIC_TX_DESC_LONG_FORMAT_LENGTH +
		prChipInfo->hif_txd_append_size;
	pucData = pucBuffer + u4TxDSize;
	ucIpVer = (pucData[ETH_HLEN] & IP_VERSION_MASK) >>
		      IP_VERSION_OFFSET;
	WLAN_GET_FIELD_BE16(&pucData[ETHER_HEADER_LEN - ETHER_TYPE_LEN],
			    &u2EtherTypeLen);

	DBGLOG(HAL, INFO, "EtherType [0x%08x]\n", u2EtherTypeLen);
	if (ucIpVer == IP_VERSION_4) {
		DBGLOG(HAL, INFO, "is IPV4[0x%08x]\n", u2EtherTypeLen);
		ucType = 1;
	}
	if (ucIpVer == IP_VERSION_6) {
		DBGLOG(HAL, INFO, "is IPV6[0x%08x]\n", u2EtherTypeLen);
		ucType = 2;
	}
	if (ucType) {
		if (ucType == 1) {
			for (u4Idx = 0; u4Idx < 4; u4Idx++)
				rL2Tbl.sram.key_ip[u4Idx] =
					pucData[ETH_HLEN + 12 + u4Idx];
		} else if (ucType == 2) {
			for (u4Idx = 0; u4Idx < 16; u4Idx++)
				rL2Tbl.sram.key_ip[u4Idx] =
					pucData[ETH_HLEN + 8 + u4Idx];
		}
		for (u4Idx = 0; u4Idx < MAC_ADDR_LEN; u4Idx++) {
			rL2Tbl.sram.d_mac[u4Idx] = pucData[u4Idx];
			rL2Tbl.sram.s_mac[u4Idx] =
				pucData[MAC_ADDR_LEN + u4Idx];
		}
		rL2Tbl.sram.wlan_id = prMsduInfo->ucWlanIndex;
		rL2Tbl.sram.bss_id = prMsduInfo->ucBssIndex;
		prHifInfo->u4MawdL2TblCnt = 2;
		halMawdUpdateL2Tbl(prAdapter->prGlueInfo, rL2Tbl, ucType - 1);
	}

	prTxRing = &prHifInfo->MawdTxRing[u2Port];
	pTxCell = &prTxRing->Cell[prTxRing->TxCpuIdx];

	kalMemCopy(pTxCell->AllocVa, pucBuffer, u4TxDSize);

	prTxDesc = (struct HW_MAC_CONNAC3X_TX_DESC *) pTxCell->AllocVa;
#if CFG_ENABLE_MAWD_MD_RING
	HAL_MAC_CONNAC3X_TXD_SET_PACKET_SOURCE(prTxDesc, 1);
#endif /* CFG_ENABLE_MAWD_MD_RING */

	prHwTxDescAppend = (union HW_MAC_TX_DESC_APPEND *)
		((uint8_t *)pTxCell->AllocVa + NIC_TX_DESC_LONG_FORMAT_LENGTH);

	prHwTxDescAppend->CR4_APPEND.u2PktFlags = 0;
	if (ucType) {
		prHwTxDescAppend->CR4_APPEND.u2PktFlags |=
			HIF_PKT_FLAGS_CT_INFO_APPLY_OVERRIDE |
			HIF_PKT_FLAGS_CT_INFO_STA_APPLY_OVERRIDE |
			HIF_PKT_FLAGS_CT_INFO_MAWD_OFLD;
	}

	prHwTxDescAppend->CR4_APPEND.ucBssIndex = prMsduInfo->ucBssIndex;
	prHwTxDescAppend->CR4_APPEND.ucWtblIndex = prMsduInfo->ucWlanIndex;

	prHwTxDescAppend->CR4_APPEND.u2MsduToken = u4MsduId;
	prHwTxDescAppend->CR4_APPEND.ucBufNum = 2;
	prHwTxDescAppend->CR4_APPEND.au4BufPtr[0] = rDmaAddr;
	/* eth header */
	prHwTxDescAppend->CR4_APPEND.au2BufLen[0] =
		ETH_HLEN | (u4AddrExt << 12);
	prHwTxDescAppend->CR4_APPEND.au4BufPtr[1] = (rDmaAddr + ETH_HLEN);
	prHwTxDescAppend->CR4_APPEND.au2BufLen[1] =
		(prMsduInfo->u2FrameLength - ETH_HLEN) | (u4AddrExt << 12);

	DBGLOG(HAL, INFO, "Fill HIF TXD + payload[%d]\n", u4MawdPacketCnt++);
	DBGLOG_MEM32(HAL, INFO, pTxCell->AllocVa,
		     NIC_TX_DESC_AND_PADDING_LENGTH +
		     prChipInfo->txd_append_size);
}
#endif

void fillConnac3xTxDescTxByteCountWithSdo(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	void *prTxDesc)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4TxByteCount = NIC_TX_DESC_LONG_FORMAT_LENGTH;

	prChipInfo = prAdapter->chip_info;
	u4TxByteCount += prMsduInfo->u2FrameLength;

	if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_DATA)
		u4TxByteCount += prChipInfo->u4ExtraTxByteCount;

	HAL_MAC_CONNAC3X_TXD_SET_HIF_VERSION(
		(struct HW_MAC_CONNAC3X_TX_DESC *)prTxDesc, 1);

	/* Calculate Tx byte count */
	HAL_MAC_CONNAC3X_TXD_SET_TX_BYTE_COUNT(
		(struct HW_MAC_CONNAC3X_TX_DESC *)prTxDesc, u4TxByteCount);
}
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

static u_int8_t IsPacketGoThruDataPath(struct MSDU_INFO *prMsduInfo)
{
	if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_DATA)
		return TRUE;

#if (CFG_TX_MGMT_BY_DATA_Q == 1)
	if (prMsduInfo->fgMgmtUseDataQ)
		return TRUE;
#endif

	return FALSE;
}

void fillConnac3xTxDescTxByteCount(
	struct ADAPTER *prAdapter,
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

	if (IsPacketGoThruDataPath(prMsduInfo))
		u4TxByteCount += prChipInfo->u4ExtraTxByteCount;

	/* Calculate Tx byte count */
	HAL_MAC_CONNAC3X_TXD_SET_TX_BYTE_COUNT(
		(struct HW_MAC_CONNAC3X_TX_DESC *)prTxDesc, u4TxByteCount);
}

void asicConnac3xInitTxdHook(
	struct ADAPTER *prAdapter,
	struct TX_DESC_OPS_T *prTxDescOps)
{
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_VAR *prWifiVar;

	ASSERT(prTxDescOps);
	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;

	prTxDescOps->nic_txd_long_format_op = nic_txd_v3_long_format_op;
	prTxDescOps->nic_txd_tid_op = nic_txd_v3_tid_op;
	prTxDescOps->nic_txd_pkt_format_op = nic_txd_v3_pkt_format_op;
	prTxDescOps->nic_txd_queue_idx_op = nic_txd_v3_queue_idx_op;
#if (CFG_TCP_IP_CHKSUM_OFFLOAD == 1)
	prTxDescOps->nic_txd_chksum_op = nic_txd_v3_chksum_op;
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD == 1 */
	prTxDescOps->nic_txd_header_format_op = nic_txd_v3_header_format_op;
	prTxDescOps->nic_txd_fill_by_pkt_option = nic_txd_v3_fill_by_pkt_option;
	prTxDescOps->nic_txd_compose = nic_txd_v3_compose;
	prTxDescOps->nic_txd_set_pkt_fixed_rate_option =
		nic_txd_v3_set_pkt_fixed_rate_option;
	prTxDescOps->nic_txd_set_hw_amsdu_template =
		nic_txd_v3_set_hw_amsdu_template;

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableSdo)) {
		prChipInfo->txd_append_size =
			prChipInfo->hif_txd_append_size;
		if (prTxDescOps->fillNicSdoAppend)
			prTxDescOps->fillNicAppend =
				prTxDescOps->fillNicSdoAppend;
		prTxDescOps->fillHifAppend =
			fillConnac3xTxDescAppendBySdo;
		prTxDescOps->fillTxByteCount =
			fillConnac3xTxDescTxByteCountWithSdo;
	}

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawdTx)) {
		prChipInfo->txd_append_size =
			prChipInfo->hif_txd_append_size;
		if (prTxDescOps->fillNicSdoAppend)
			prTxDescOps->fillNicAppend =
				prTxDescOps->fillNicSdoAppend;
		prTxDescOps->fillHifAppend =
			fillConnac3xTxDescAppendByMawdSdo;
		prTxDescOps->fillTxByteCount =
			fillConnac3xTxDescTxByteCountWithSdo;
	}
#endif
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
}

void asicConnac3xInitRxdHook(
	struct ADAPTER *prAdapter,
	struct RX_DESC_OPS_T *prRxDescOps)
{
	ASSERT(prRxDescOps);
	prRxDescOps->nic_rxd_get_rx_byte_count = nic_rxd_v3_get_rx_byte_count;
	prRxDescOps->nic_rxd_get_pkt_type = nic_rxd_v3_get_packet_type;
	prRxDescOps->nic_rxd_get_wlan_idx = nic_rxd_v3_get_wlan_idx;
	prRxDescOps->nic_rxd_get_sec_mode = nic_rxd_v3_get_sec_mode;
	prRxDescOps->nic_rxd_get_sw_class_error_bit =
		nic_rxd_v3_get_sw_class_error_bit;
	prRxDescOps->nic_rxd_get_ch_num = nic_rxd_v3_get_ch_num;
	prRxDescOps->nic_rxd_get_rf_band = nic_rxd_v3_get_rf_band;
	prRxDescOps->nic_rxd_get_tcl = nic_rxd_v3_get_tcl;
	prRxDescOps->nic_rxd_get_ofld = nic_rxd_v3_get_ofld;
	prRxDescOps->nic_rxd_get_HdrTrans = nic_rxd_v3_get_HdrTrans;
	prRxDescOps->nic_rxd_fill_rfb = nic_rxd_v3_fill_rfb;
	prRxDescOps->nic_rxd_sanity_check = nic_rxd_v3_sanity_check;
#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
	prRxDescOps->nic_rxd_check_wakeup_reason =
		nic_rxd_v3_check_wakeup_reason;
#endif
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	prRxDescOps->nic_rxd_fill_radiotap = nic_rxd_v3_fill_radiotap;
#endif
	prRxDescOps->nic_rxd_handle_host_rpt =
		nic_rxd_v3_handle_host_rpt;
}

#if (CFG_SUPPORT_MSP == 1)
void asicConnac3xRxProcessRxvforMSP(struct ADAPTER *prAdapter,
	  struct SW_RFB *prRetSwRfb)
{
	struct HW_MAC_RX_STS_GROUP_3_V2 *prGroup3;
	struct STA_RECORD *prStaRec;
	uint8_t ucStaRecIdx;
	uint32_t *prRxV = NULL; /* pointer to destination buffer to store RxV */

	if (prRetSwRfb->ucStaRecIdx >= CFG_STA_REC_NUM) {
		DBGLOG(RX, LOUD,
		"prRetSwRfb->ucStaRecIdx(%d) >= CFG_STA_REC_NUM(%d)\n",
			prRetSwRfb->ucStaRecIdx, CFG_STA_REC_NUM);
		return;
	}

	/* reduce rxv processing */
	if (!IS_RX_MPDU_BEGIN(prRetSwRfb->ucPayloadFormat))
		return;

	if (prRetSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3)) {
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		prStaRec = mldGetStaRecByBandIdx(prAdapter,
				&prAdapter->arStaRec[prRetSwRfb->ucStaRecIdx],
				prRetSwRfb->ucHwBandIdx);
		if (!prStaRec)
			return;
		ucStaRecIdx = prStaRec->ucIndex;
#else
		ucStaRecIdx = prRetSwRfb->ucStaRecIdx;
#endif
		prRxV = prAdapter->arStaRec[ucStaRecIdx].au4RxV;
		kalMemZero(prRxV, sizeof(uint32_t) * RXV_NUM);

		prGroup3 = prRetSwRfb->prRxStatusGroup3;

		/* P-RXV0[0:31] in RXD Group3 */
		prRxV[0] = CONNAC3X_HAL_RX_VECTOR_GET_RX_VECTOR(prGroup3, 0);

		/* P-RXV0[32:63] in RXD Group3 */
		prRxV[1] = CONNAC3X_HAL_RX_VECTOR_GET_RX_VECTOR(prGroup3, 1);

		/* DW22 for MODE, STBC, GI, DBW; rate(MCS), NSTS in RXV0 */
		prRxV[2] = prGroup3->u2RxInfo;

		/* RXD Group3, DW23 */
		prRxV[3] = prGroup3->u4Rcpi;

		nicRxProcessRxvLinkStats(prAdapter, prRetSwRfb, prRxV);
	}
}
#endif /* CFG_SUPPORT_MSP == 1 */

uint8_t asicConnac3xRxGetRcpiValueFromRxv(
	uint8_t ucRcpiMode,
	struct SW_RFB *prSwRfb)
{
	uint8_t ucRcpi0, ucRcpi1, ucRcpi2, ucRcpi3;
	uint8_t ucRcpiValue = 0;
	struct HW_MAC_RX_STS_GROUP_3_V2 *prGroup3 = NULL;

	ASSERT(prSwRfb);

	if (ucRcpiMode >= RCPI_MODE_NUM) {
		DBGLOG(RX, WARN,
		       "Rcpi Mode = %d is invalid for getting uint8_t value from RXV\n",
		       ucRcpiMode);
		return 0;
	}

	if ((prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3)) == 0) {
		DBGLOG(RX, WARN, "%s, RXD group 3 is not valid\n", __func__);
		return 0;
	}

	prGroup3 = (struct HW_MAC_RX_STS_GROUP_3_V2 *)
				prSwRfb->prRxStatusGroup3;

	ucRcpi0 = CONNAC3X_HAL_RX_VECTOR_GET_RCPI0(prGroup3);
	ucRcpi1 = CONNAC3X_HAL_RX_VECTOR_GET_RCPI1(prGroup3);
	ucRcpi2 = CONNAC3X_HAL_RX_VECTOR_GET_RCPI2(prGroup3);
	ucRcpi3 = CONNAC3X_HAL_RX_VECTOR_GET_RCPI3(prGroup3);

	/*If Rcpi is not available, set it to zero*/
	if (ucRcpi0 == RCPI_MEASUREMENT_NOT_AVAILABLE)
		ucRcpi0 = RCPI_LOW_BOUND;
	if (ucRcpi1 == RCPI_MEASUREMENT_NOT_AVAILABLE)
		ucRcpi1 = RCPI_LOW_BOUND;
	DBGLOG(RX, TRACE, "RCPI WF0:%d WF1:%d WF2:%d WF3:%d\n",
	       ucRcpi0, ucRcpi1, ucRcpi2, ucRcpi3);

	switch (ucRcpiMode) {
	case RCPI_MODE_WF0:
		ucRcpiValue = ucRcpi0;
		break;

	case RCPI_MODE_WF1:
		ucRcpiValue = ucRcpi1;
		break;

	case RCPI_MODE_WF2:
		ucRcpiValue = ucRcpi2;
		break;

	case RCPI_MODE_WF3:
		ucRcpiValue = ucRcpi3;
		break;

	case RCPI_MODE_AVG: /*Not recommended for CBW80+80*/
		ucRcpiValue = (ucRcpi0 + ucRcpi1) / 2;
		break;

	case RCPI_MODE_MAX:
		ucRcpiValue =
			(ucRcpi0 > ucRcpi1) ? (ucRcpi0) : (ucRcpi1);
		break;

	case RCPI_MODE_MIN:
		ucRcpiValue =
			(ucRcpi0 < ucRcpi1) ? (ucRcpi0) : (ucRcpi1);
		break;

	default:
		break;
	}

	return ucRcpiValue;
}

uint8_t asicConnac3xRxGetRxModeValueFromRxv(struct SW_RFB *prSwRfb)
{
	uint8_t ucRxModeValue = 0;
	struct HW_MAC_RX_STS_GROUP_3_V2 *prGroup3 = NULL;

	if (!prSwRfb) {
		DBGLOG(RX, WARN, "prSwRfb is NULL\n");
		return 0xFF;
	}

	if ((prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3)) == 0) {
		DBGLOG(RX, WARN, "RXD group 3 is not valid\n");
		return 0xFF;
	}

	prGroup3 = (struct HW_MAC_RX_STS_GROUP_3_V2 *)
				prSwRfb->prRxStatusGroup3;
	ucRxModeValue = CONNAC3X_HAL_RX_VECTOR_GET_RX_MODE(prGroup3);

	return ucRxModeValue;
}

#if (CFG_SUPPORT_PERF_IND == 1)
void asicConnac3xRxPerfIndProcessRXV(struct ADAPTER *prAdapter,
			       struct SW_RFB *prSwRfb,
			       uint8_t ucBssIndex)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_PERF_IND_INFO *prPerfIndInfo;
	struct HW_MAC_RX_STS_GROUP_3_V2 *prGroup3 = NULL;
	uint8_t ucRCPI0 = 0, ucRCPI1 = 0;
	uint32_t u4PhyRate;
	uint16_t u2Rate = 0; /* Unit 500 Kbps */
	struct RxRateInfo rRxRateInfo = {0};
	int status;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prGlueInfo = prAdapter->prGlueInfo;
	status = wlanGetRxRateByBssid(prGlueInfo, ucBssIndex, &u4PhyRate, NULL,
				&rRxRateInfo);
	if (status < 0 || u4PhyRate == 0)
		return;

	if (prSwRfb->prRxStatusGroup3 == NULL)
		return;

	prPerfIndInfo = &prGlueInfo->PerfIndCache;

	if (rRxRateInfo.u4Nss == 1) {
		if (prPerfIndInfo->ucCurRxNss[ucBssIndex] < 0xff)
			prPerfIndInfo->ucCurRxNss[ucBssIndex]++;
	} else if (rRxRateInfo.u4Nss == 2) {
		if (prPerfIndInfo->ucCurRxNss2[ucBssIndex] < 0xff)
			prPerfIndInfo->ucCurRxNss2[ucBssIndex]++;
	}

	/* ucRate(500kbs) = u4PhyRate(100kbps) */
	u2Rate = u4PhyRate / 5;

	/* RCPI */
	prGroup3 = prSwRfb->prRxStatusGroup3;
	ucRCPI0 = CONNAC3X_HAL_RX_VECTOR_GET_RCPI0(prGroup3);
	ucRCPI1 = CONNAC3X_HAL_RX_VECTOR_GET_RCPI1(prGroup3);

	/* Record peak rate to Traffic Indicator*/
	if (u2Rate > prPerfIndInfo->u2CurRxRate[ucBssIndex]) {
		prPerfIndInfo->u2CurRxRate[ucBssIndex] = u2Rate;
		prPerfIndInfo->ucCurRxRCPI0[ucBssIndex] = ucRCPI0;
		prPerfIndInfo->ucCurRxRCPI1[ucBssIndex] = ucRCPI1;
	}

	DBGLOG(SW4, TEMP, "rate=[%u], nss=[%u], cnt_nss1=[%d], cnt_nss2=[%d]\n",
		u2Rate,
		rRxRateInfo.u4Nss,
		prPerfIndInfo->ucCurRxNss[ucBssIndex],
		prPerfIndInfo->ucCurRxNss2[ucBssIndex]);
}
#endif

#if (CFG_CHIP_RESET_SUPPORT == 1) && (CFG_WMT_RESET_API_SUPPORT == 0)
u_int8_t conn2_rst_L0_notify_step2(void)
{
	if (glGetRstReason() == RST_BT_TRIGGER) {
		typedef void (*p_bt_fun_type) (void);
		p_bt_fun_type bt_func;
		char *bt_func_name = "WF_rst_L0_notify_BT_step2";

		bt_func = (p_bt_fun_type)(uintptr_t)
				GLUE_LOOKUP_FUN(bt_func_name);
		if (bt_func) {
			bt_func();
		} else {
			DBGLOG(INIT, WARN, "[SER][L0] %s does not exist\n",
							bt_func_name);
			return FALSE;
		}
	} else {
		/* if wifi trigger, then wait bt ready notify */
		DBGLOG(INIT, WARN, "[SER][L0] not support..\n");
	}

	return TRUE;
}
#endif

void asicConnac3xDmashdlLiteSetTotalPlePsePageSize(
	struct ADAPTER *prAdapter,
	uint16_t u2PlePageSize, uint16_t u2PsePageSize)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	u4Val |= (u2PlePageSize << prCfg->rPleTotalPageSize.u4Shift) &
		prCfg->rPleTotalPageSize.u4Mask;
	u4Val |= (u2PsePageSize << prCfg->rPseTotalPageSize.u4Shift) &
		prCfg->rPseTotalPageSize.u4Mask;

	HAL_MCR_WR(prAdapter, prCfg->rPleTotalPageSize.u4Addr, u4Val);
}

void asicConnac3xDmashdlLiteSetQueueMapping(
	struct ADAPTER *prAdapter, uint8_t ucQueue, uint8_t ucGroup)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr, u4Mask, u4Shft;
	uint32_t u4Val = 0, u4Idx = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	if (ucQueue >= 64)
		return;

	if (ucGroup >= prCfg->u4GroupNum)
		return;

	u4Idx = ucQueue >> 2;
	u4Addr = prCfg->rQueueMapping0Queue0.u4Addr + (u4Idx << 2);
	u4Mask = prCfg->rQueueMapping0Queue0.u4Mask << ((ucQueue % 4) << 3);
	u4Shft = (ucQueue % 4) << 3;

	u4Val = prCfg->u4Queue2Group[u4Idx];
	u4Val &= ~u4Mask;
	u4Val |= (ucGroup << u4Shft) & u4Mask;
	prCfg->u4Queue2Group[u4Idx] = u4Val;

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

void asicConnac3xDmashdlSetPlePsePktMaxPage(
	struct ADAPTER *prAdapter,
	uint16_t u2MaxPlePage, uint16_t u2MaxPsePage)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	u4Val |= (u2MaxPlePage << prCfg->rPlePacketMaxSize.u4Shift) &
		prCfg->rPlePacketMaxSize.u4Mask;
	u4Val |= (u2MaxPsePage << prCfg->rPsePacketMaxSize.u4Shift) &
		 prCfg->rPsePacketMaxSize.u4Mask;

	HAL_MCR_WR(prAdapter, prCfg->rPlePacketMaxSize.u4Addr, u4Val);
}

void asicConnac3xDmashdlSetRefill(struct ADAPTER *prAdapter, uint8_t ucGroup,
			       u_int8_t fgEnable)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Val = 0, u4Mask;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	if (ucGroup >= prCfg->u4GroupNum)
		ASSERT(0);

	u4Mask = prCfg->rGroup0RefillDisable.u4Mask << ucGroup;

	u4Val = prCfg->u4RefillCtrl;
	if (fgEnable)
		u4Val &= ~u4Mask;
	else
		u4Val |= u4Mask;
	prCfg->u4RefillCtrl = u4Val;

	HAL_MCR_WR(prAdapter, prCfg->rGroup0RefillDisable.u4Addr, u4Val);
}

void asicConnac3xDmashdlSetMinMaxQuota(
	struct ADAPTER *prAdapter, uint8_t ucGroup,
	uint16_t u2MinQuota, uint16_t u2MaxQuota)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr;
	uint32_t u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	if (ucGroup >= prCfg->u4GroupNum)
		ASSERT(0);

	u4Addr = prCfg->rGroup0ControlMinQuota.u4Addr + (ucGroup << 2);

	u4Val &= ~prCfg->rGroup0ControlMinQuota.u4Mask;
	u4Val |= (u2MinQuota << prCfg->rGroup0ControlMinQuota.u4Shift) &
		prCfg->rGroup0ControlMinQuota.u4Mask;

	u4Val &= ~prCfg->rGroup0ControlMaxQuota.u4Mask;
	u4Val |= (u2MaxQuota << prCfg->rGroup0ControlMaxQuota.u4Shift) &
		prCfg->rGroup0ControlMaxQuota.u4Mask;

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

void asicConnac3xDmashdlSetQueueMapping(
	struct ADAPTER *prAdapter, uint8_t ucQueue, uint8_t ucGroup)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr, u4Mask, u4Shft;
	uint32_t u4Val = 0, u4Idx = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	if (ucQueue >= 32)
		ASSERT(0);

	if (ucGroup >= prCfg->u4GroupNum)
		ASSERT(0);

	u4Idx = ucQueue >> 3;
	if (u4Idx >= 4)
		return;

	u4Addr = prCfg->rQueueMapping0Queue0.u4Addr + (u4Idx << 2);
	u4Mask = prCfg->rQueueMapping0Queue0.u4Mask << ((ucQueue % 8) << 2);
	u4Shft = (ucQueue % 8) << 2;

	u4Val = prCfg->u4Queue2Group[u4Idx];
	u4Val &= ~u4Mask;
	u4Val |= (ucGroup << u4Shft) & u4Mask;
	prCfg->u4Queue2Group[u4Idx] = u4Val;

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

void asicConnac3xDmashdlSetUserDefinedPriority(
	struct ADAPTER *prAdapter, uint8_t ucPriority, uint8_t ucGroup)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr, u4Mask, u4Shft;
	uint32_t u4Val = 0, u4Idx = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	ASSERT(ucPriority < 16);
	ASSERT(ucGroup < prCfg->u4GroupNum);

	u4Idx = ucPriority >> 3;
	if (u4Idx >= 2)
		return;

	u4Addr = prCfg->rSchdulerSetting0Priority0Group.u4Addr + (u4Idx << 2);
	u4Mask = prCfg->rSchdulerSetting0Priority0Group.u4Mask <<
		((ucPriority % 8) << 2);
	u4Shft = (ucPriority % 8) << 2;

	u4Val = prCfg->u4Priority2Group[u4Idx];
	u4Val &= ~u4Mask;
	u4Val |= (ucGroup << u4Shft) & u4Mask;
	prCfg->u4Priority2Group[u4Idx] = u4Val;

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

void asicConnac3xDmashdlSetSlotArbiter(
	struct ADAPTER *prAdapter, u_int8_t fgEnable, uint32_t u4DefVal)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Val = u4DefVal;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	if (fgEnable)
		u4Val |= prCfg->rPageSettingGroupSeqOrderType.u4Mask;
	else
		u4Val &= ~prCfg->rPageSettingGroupSeqOrderType.u4Mask;

	HAL_MCR_WR(prAdapter, prCfg->rPageSettingGroupSeqOrderType.u4Addr,
		   u4Val);
}

void asicConnac3xDmashdlSetOptionalControl(
	struct ADAPTER *prAdapter,
	uint16_t u2HifAckCntTh, uint16_t u2HifGupActMap, uint32_t u4DefVal)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr, u4Val = u4DefVal;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	u4Addr = prCfg->rOptionalControlCrHifAckCntTh.u4Addr;

	u4Val &= ~prCfg->rOptionalControlCrHifAckCntTh.u4Mask;
	u4Val |= (u2HifAckCntTh <<
		  prCfg->rOptionalControlCrHifAckCntTh.u4Shift);

	u4Val &= ~prCfg->rOptionalControlCrHifGupActMap.u4Mask;
	u4Val |= (u2HifGupActMap <<
		  prCfg->rOptionalControlCrHifGupActMap.u4Shift);

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

#if (CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1)
uint32_t asicConnac3xDynamicDmashdlGetInUsedMaxQuota(
	struct ADAPTER *prAdapter,
	uint32_t u4GroupIdx,
	uint32_t u4DefMaxQuota)
{
	struct WMM_QUOTA_STATUS *prWmmStatus;
	uint8_t ucWmmIdx = u4GroupIdx / 4, ucAc = u4GroupIdx % 4;

	prWmmStatus = &prAdapter->rWmmQuotaStatus[ucWmmIdx];
	if (ucAc == WMM_AC_VO_INDEX || !prWmmStatus->fgIsUsed)
		return u4DefMaxQuota;

	return prWmmStatus->u4Quota;
}

uint32_t asicConnac3xUpdateDynamicDmashdlQuota(
	struct ADAPTER *prAdapter,
	uint8_t ucWmmIndex,
	uint32_t u4MaxQuota)
{
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint8_t ucGroupIdx, ucAcIdx;
	uint32_t u4Idx;
	uint16_t u2MaxQuotaFinal, u2MinQuota = 0;
	u_int8_t fgIsMaxQuotaInvalid = FALSE;
	uint32_t u4BufSize = 512, u4Pos = 0;
	char *aucBuf;

	aucBuf = (char *)kalMemAlloc(u4BufSize, VIR_MEM_TYPE);
	if (aucBuf)
		kalMemZero(aucBuf, u4BufSize);

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;
	u2MinQuota = prChipInfo->u4DefaultMinQuota;

	if (u4MaxQuota > (DMASHDL_MAX_QUOTA_MASK >> DMASHDL_MAX_QUOTA_OFFSET))
		fgIsMaxQuotaInvalid = TRUE;

	for (u4Idx = 0; u4Idx < WMM_AC_INDEX_NUM; u4Idx++) {
		if (u4Idx == WMM_AC_VO_INDEX)
			continue;

		ucAcIdx = u4Idx + (ucWmmIndex * WMM_AC_INDEX_NUM);
		ucGroupIdx = prCfg->aucQueue2Group[ucAcIdx];
		u2MaxQuotaFinal = u4MaxQuota;
		/* Set quota to default */
		if (fgIsMaxQuotaInvalid)
			u2MaxQuotaFinal = prChipInfo->u4DefaultMaxQuota;

		if (u2MaxQuotaFinal == 0 ||
		    (prCfg->au2MaxQuota[ucGroupIdx] == u2MaxQuotaFinal))
			continue;

		if (u2MaxQuotaFinal < u2MinQuota)
			u2MaxQuotaFinal = u2MinQuota;

		prCfg->au2MaxQuota[ucGroupIdx] = u2MaxQuotaFinal;
		asicConnac3xDmashdlSetMinMaxQuota(
			prAdapter,
			ucGroupIdx,
			u2MinQuota,
			u2MaxQuotaFinal);

		if (aucBuf) {
			u4Pos += kalSnprintf(
				aucBuf + u4Pos, u4BufSize - u4Pos,
				"WmmIndex[%u] Ac[%u] Group[%u] MaxQuota[0x%x]",
				ucWmmIndex, ucAcIdx, ucGroupIdx,
				u2MaxQuotaFinal);
		}
	}

	if (aucBuf) {
		if (u4Pos)
			DBGLOG(HAL, INFO, "%s\n", aucBuf);
		kalMemFree(aucBuf, VIR_MEM_TYPE, u4BufSize);
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t asicConnac3xDynamicDmashdlQuotaDecision(
	struct ADAPTER *prAdapter,
	uint8_t ucWmmIndex)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	enum ENUM_MBMC_BN eHwBand = ENUM_BAND_AUTO;
	enum ENUM_BAND eBand = BAND_NULL;
	uint32_t u4Band = 0, u4Quota = 0;

	if (ucWmmIndex >= HW_WMM_NUM)
		return 0;

	eHwBand = prAdapter->rWmmQuotaReqCS[ucWmmIndex].eHwBand;
	eBand = prAdapter->rWmmQuotaReqCS[ucWmmIndex].eBand;
	if (eHwBand >= ENUM_BAND_NUM)
		return 0;

	/* MLO use Hw band */
	if (prAdapter->rWmmQuotaReqCS[ucWmmIndex].fgIsMldMulti)
		return prChipInfo->au4DmaMaxQuotaBand[eHwBand];

	if (eBand == BAND_NULL || eBand >= BAND_NUM)
		return prChipInfo->u4DefaultMaxQuota;

	u4Band = (uint32_t)eBand - 1;
	u4Quota = prChipInfo->au4DmaMaxQuotaRfBand[u4Band];
	if (u4Quota > prChipInfo->au4DmaMaxQuotaBand[eHwBand])
		u4Quota = prChipInfo->au4DmaMaxQuotaBand[eHwBand];

	DBGLOG(HAL, TRACE, "WmmIdx[%u] HwBand[%u] Band[%u] MaxQuota[0x%x]\n",
	       ucWmmIndex, eHwBand, eBand, u4Quota);

	return u4Quota;
}
#endif /* CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1 */

#if CFG_WMT_RESET_API_SUPPORT
static void handle_wfsys_reset(struct ADAPTER *prAdapter)
{
	struct CHIP_DBG_OPS *dbg_ops = prAdapter->chip_info->prDebugOps;

	wifi_coredump_set_enable(TRUE);
	if (kalIsResetting()) {
		DBGLOG(HAL, INFO,
			"Wi-Fi Driver trigger, need do complete.\n");
		reset_done_trigger_completion();
	} else if (fgIsDrvTriggerWholeChipReset) {
		DBGLOG(HAL, INFO,
			"Ignore fw assert due to whole chip reset ongoing.\n");
	} else if (kalGetShutdownState()) {
		DBGLOG(HAL, INFO,
			"Ignore fw assert due to device shutdown.\n");
	} else {
		if (prAdapter->fgIsSkipFWL05) {
			DBGLOG(HAL, ERROR,
				"Ignore fw assert due to before rom patch dl\n");
			/* To do: send msg to do dfd dump by rom cmd */
			return;
		}
		DBGLOG(HAL, ERROR, "FW trigger assert.\n");
		g_Coredump_source = COREDUMP_SOURCE_WF_FW;

		glSetRstReason(RST_FW_ASSERT);

		prAdapter->u4HifDbgFlag |= DEG_HIF_DEFAULT_DUMP;
		kalSetHifDbgEvent(prAdapter->prGlueInfo);

		glResetUpdateFlag(TRUE);

#if CFG_MTK_MDDP_SUPPORT
#if (CFG_PCIE_GEN_SWITCH == 1)
		mddpNotifyMDGenSwitchEnd(prAdapter);
#endif /* CFG_PCIE_GEN_SWITCH */
#endif /* CFG_MTK_MDDP_SUPPORT */

		if (dbg_ops && dbg_ops->dumpBusHangCr)
			dbg_ops->dumpBusHangCr(prAdapter);

		kalSetRstFwNotifyL05Event(TRUE);
	}
}

static void handle_whole_chip_reset(struct ADAPTER *prAdapter)
{
	struct CHIP_DBG_OPS *dbg_ops = prAdapter->chip_info->prDebugOps;

	if (kalGetShutdownState()) {
		DBGLOG(HAL, INFO,
			"Ignore fw assert due to device shutdown.\n");
		return;
	}

	DBGLOG(HAL, ERROR,
		"FW trigger whole chip reset.\n");

	wifi_coredump_set_enable(TRUE);
	glResetUpdateFlag(TRUE);
	g_IsWfsysBusHang = TRUE;
	glResetUpdateFwAsserted(TRUE);

#if CFG_MTK_MDDP_SUPPORT
#if (CFG_PCIE_GEN_SWITCH == 1)
	mddpNotifyMDGenSwitchEnd(prAdapter);
#endif /* CFG_PCIE_GEN_SWITCH */
#endif /* CFG_MTK_MDDP_SUPPORT */

	if (dbg_ops && dbg_ops->dumpBusHangCr)
		dbg_ops->dumpBusHangCr(prAdapter);

	kalSetRstFwNotifyTriggerL0Event(TRUE);
}
#endif

u_int8_t asicConnac3xSwIntHandler(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
#if defined(_HIF_PCIE)
	struct BUS_INFO *prBusInfo = NULL;
#endif
	uint32_t u4Status = 0;
	u_int8_t fgRet = TRUE;

	if (!prAdapter)
		return TRUE;

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
#if defined(_HIF_PCIE)
	prBusInfo = prChipInfo->bus_info;
#endif

#if defined(_HIF_PCIE)
	if (prBusInfo->hwControlVote)
		prBusInfo->hwControlVote(prAdapter,
					 FALSE,
					 PCIE_VOTE_USER_LOG_RESET);
#endif

	if (!prChipInfo->get_sw_interrupt_status)
		goto exit;

	fgRet = prChipInfo->get_sw_interrupt_status(prAdapter, &u4Status);
	if (fgRet == FALSE || u4Status == 0)
		goto exit;

#if (DBG_DISABLE_ALL_INFO == 0)
	if (!(prGlueInfo->ulFlag & GLUE_FLAG_HALT) &&
	    (u4Status & BIT(SW_INT_FW_LOG)))
		fw_log_handler();
#endif

#if CFG_WMT_RESET_API_SUPPORT
	if (u4Status & BIT(SW_INT_SUBSYS_RESET))
		handle_wfsys_reset(prAdapter);

	if (u4Status & BIT(SW_INT_WHOLE_RESET))
		handle_whole_chip_reset(prAdapter);

#if defined(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (u4Status & BIT(SW_INT_PMIC_RESET))
		connv3_trigger_pmic_irq(CONNV3_DRV_TYPE_WIFI,
			"fw trigger PMIC reset");
#endif
#endif

#if defined(_HIF_PCIE)
	if (prBusInfo->hwControlVote)
		prBusInfo->hwControlVote(prAdapter,
					 TRUE,
					 PCIE_VOTE_USER_LOG_RESET);
#endif

exit:
	return fgRet;
}

uint32_t asicConnac3xQueryPmicInfo(struct ADAPTER *prAdapter)
{
	struct INIT_CMD_QUERY_INFO rCmd = {0};
	struct INIT_EVENT_QUERY_INFO *prEvent;
	struct INIT_EVENT_TLV_GENERAL *prTlv;
	struct INIT_EVENT_QUERY_INFO_PMIC *prPmicEvent;
	uint32_t u4EventSize;
	uint32_t u4Status = WLAN_STATUS_FAILURE;

	rCmd.u4QueryBitmap = BIT(INIT_CMD_QUERY_TYPE_PMIC_INFO);

	u4EventSize = CFG_RX_MAX_PKT_SIZE;
	prEvent = kalMemAlloc(u4EventSize, VIR_MEM_TYPE);
	if (!prEvent) {
		DBGLOG(INIT, ERROR, "Allocate event packet FAILED.\n");
		goto exit;
	}
	kalMemZero(prEvent, u4EventSize);

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_QUERY_INFO, &rCmd, sizeof(rCmd),
		TRUE, FALSE,
		INIT_EVENT_ID_QUERY_INFO_RESULT, prEvent, u4EventSize);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	if (prEvent->u2TotalElementNum != 1) {
		DBGLOG(INIT, ERROR, "Unexpected element num: %d.\n",
			prEvent->u2TotalElementNum);
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	prTlv = (struct INIT_EVENT_TLV_GENERAL *)&prEvent->aucTlvBuffer[0];
	if (prTlv->u2Tag != INIT_CMD_QUERY_TYPE_PMIC_INFO) {
		DBGLOG(INIT, ERROR, "Unexpected tag id: %d.\n",
			prTlv->u2Tag);
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	prPmicEvent = (struct INIT_EVENT_QUERY_INFO_PMIC *)
		&prTlv->aucBuffer[0];
	DBGLOG(INIT, TRACE, "PMIC ID: 0x%x.\n", prPmicEvent->u4PmicId);
	DBGLOG_MEM32(INIT, TRACE, &prPmicEvent->aucPMICCoreDumpbuf[0],
		prPmicEvent->u4Length);
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	connv3_update_pmic_state(CONNV3_DRV_TYPE_WIFI,
		&prPmicEvent->aucPMICCoreDumpbuf[0],
		prPmicEvent->u4Length);
#endif
	u4Status = WLAN_STATUS_SUCCESS;

exit:
	if (prEvent)
		kalMemFree(prEvent, VIR_MEM_TYPE, u4EventSize);

	return u4Status;
}

const struct DEBUG_DUMP_REGIOM debug_info_region[NUM_OF_SYSTEM_DEBUG_INFO] = {
	{DEBUG_INFO_PMIC_DUMP,
		DEBUG_INFO_PMIC_DUMP_LENG}, /*PMIC INFO*/
	{DEBUG_INFO_DFD_CB_INFRA_INFO,
		DEBUG_INFO_DFD_CB_INFRA_INFO_LENG}, /*CB_INFRA INFO*/
	{DEBUG_INFO_DFD_CB_INFRA_SRAM,
		DEBUG_INFO_DFD_CB_INFRA_SRAM_LENG}, /*CB_INFRA SRAM*/
	{DEBUG_INFO_DFD_CB_INFRA_WF_SRAM,
		DEBUG_INFO_DFD_CB_INFRA_WF_SRAM_LENG}, /*CB_INFRA WIFI SRAM*/
	{DEBUG_INFO_DFD_CB_INFRA_BT_SRAM,
		DEBUG_INFO_DFD_CB_INFRA_BT_SRAM_LENG}, /*CB_INFRA BT SRAM*/
	{DEBUG_INFO_DFD_CB_INFRA_DEBUG_INFO,
		DEBUG_INFO_DFD_CB_INFRA_DEBUG_INFO_LENG}, /*CB_INFRA DBG INFO*/
	{DEBUG_INFO_DFD_WF_DEBUG_INFO,
		DEBUG_INFO_DFD_WF_DEBUG_INFO_LENG}, /*WIFI DEBUG INFO*/
	{DEBUG_INFO_DFD_BT_DEBUG_INFO,
		DEBUG_INFO_DFD_BT_DEBUG_INFO_LENG}, /*BT DEBUG INFO*/
};

uint32_t asicConnac3xQueryDFDInfo(
	struct ADAPTER *prAdapter, uint32_t u4InfoIdx, uint32_t u4Offset,
	uint32_t u4Length, uint8_t *pBuf)
{
	uint32_t u4DFDInfoLength = 0;
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
	struct INIT_CMD_DFD_INFO_QUERY rCmd = {0};
	struct INIT_EVENT_DFD_INFO_QUERY *prDfdEvent = NULL;
	uint32_t u4EventSize = 0;
	uint32_t u4Status = WLAN_STATUS_FAILURE;

	if (u4Length > debug_info_region[u4InfoIdx].u4Length) {
		DBGLOG(INIT, ERROR, "length %d over [%d]%d\n",
			u4Length, u4InfoIdx,
			debug_info_region[u4InfoIdx].u4Length);
		goto exit;
	}

	rCmd.u4InfoIdx = u4InfoIdx;
	rCmd.u4Offset = u4Offset;
	rCmd.u4Length = u4Length;

	u4EventSize = sizeof(struct INIT_EVENT_DFD_INFO_QUERY);
	prDfdEvent = kalMemAlloc(u4EventSize, VIR_MEM_TYPE);
	if (!prDfdEvent) {
		DBGLOG(INIT, ERROR, "Allocate event packet FAILED.\n");
		goto exit;
	}
	kalMemZero(prDfdEvent, u4EventSize);

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_DFD_INFO_QUERY, &rCmd, sizeof(rCmd),
		TRUE, FALSE,
		INIT_EVENT_ID_CMD_RESULT, prDfdEvent, u4EventSize);
	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "[%d] Get EVT(%d) failed\n",
			u4InfoIdx, prDfdEvent->u4Length);
		goto exit;
	}

	if (prDfdEvent->u4Length > DEBUG_INFO_DFD_MAX_EVENT_LEN ||
	    prDfdEvent->u4Length == 0) {
		DBGLOG(INIT, ERROR, "[%d] Unexpected dump length: %d.\n",
			u4InfoIdx, prDfdEvent->u4Length);
		goto exit;
	}

	kalMemCopy(pBuf, prDfdEvent->aucDFDInfoBuf, prDfdEvent->u4Length);
	u4DFDInfoLength = prDfdEvent->u4Length;
	DBGLOG(INIT, TRACE, "[%d] Get dump length: %d\n",
		u4InfoIdx, u4DFDInfoLength);
	/* DBGLOG_MEM32(INIT, INFO, &prDfdEvent->aucDFDInfoBuf[0], 32); */

exit:
	if (prDfdEvent)
		kalMemFree(prDfdEvent, VIR_MEM_TYPE, u4EventSize);
#endif
	return u4DFDInfoLength;
}

uint32_t asicConnac3xGetFwVer(struct ADAPTER *prAdapter)
{
	uint32_t u4SwVer = 0;

	u4SwVer = nicGetChipSwVer() + 1;

	return u4SwVer;
}

int connsys_power_on(void)
{
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	struct mt66xx_chip_info *chip = NULL;
#endif
	int ret = 0;

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	ret = connv3_pwr_on(CONNV3_DRV_TYPE_WIFI);
	if (ret) {
		DBGLOG(HAL, ERROR, "connv3_pwr_on failed, ret=%d\n",
			ret);
		return ret;
	}
	/* Add delay for pcie phy ready */
	kalMdelay(10);

	ret = halMawdPwrOn();
	if (ret) {
		connv3_pwr_off(CONNV3_DRV_TYPE_WIFI);
		return ret;
	}


	glGetChipInfo((void **)&chip);

	if (!chip)
		DBGLOG(HAL, ERROR, "NULL chip info pwr on.\n");
	else
		wlan_pinctrl_action(chip, WLAN_PINCTRL_MSG_FUNC_PTA_UART_ON);

#endif

	return ret;
}

int connsys_power_done(void)
{
	int ret = 0;

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	ret = connv3_pwr_on_done(CONNV3_DRV_TYPE_WIFI);
	if (ret)
		DBGLOG(HAL, ERROR,
			"connv3_pwr_on_done failed, ret=%d\n",
			ret);
#endif

	return ret;
}

void connsys_power_off(void)
{
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	struct mt66xx_chip_info *chip = NULL;

	glGetChipInfo((void **)&chip);
	if (!chip) {
		DBGLOG(HAL, ERROR, "NULL chip info.\n");
		return;
	}

	halMawdPwrOff();
	wlan_pinctrl_action(chip, WLAN_PINCTRL_MSG_FUNC_PTA_UART_OFF);
	connv3_pwr_off(CONNV3_DRV_TYPE_WIFI);
#endif
}

int wlan_test_mode_on(bool uIsSwtichTestMode)
{
	int32_t ret = 0;
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
	DBGLOG(INIT, INFO, "uIsSwtichTestMode: %d\n", uIsSwtichTestMode);

	if (kalIsResetOnEnd() == TRUE) {
		DBGLOG(INIT, INFO, "now is resetting\n");
		ret = WLAN_STATUS_FAILURE;
		return ret;
	}

	if (!wfsys_trylock()) {
		DBGLOG(INIT, INFO, "now is write processing\n");
		ret = WLAN_STATUS_FAILURE;
		return ret;
	}

	set_wifi_in_switch_mode(1);
	g_fgWlanOnOffHoldRtnlLock = 1;

	wlanFuncOff();
	if (uIsSwtichTestMode)
		set_wifi_test_mode_fwdl(1);
	ret = wlanFuncOn();
	if (uIsSwtichTestMode)
		set_wifi_test_mode_fwdl(0);

	g_fgWlanOnOffHoldRtnlLock = 0;
	set_wifi_in_switch_mode(0);
	wfsys_unlock();
#endif
	return ret;
}

#if CFG_MTK_ANDROID_WMT
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
static int wlan_pre_pwr_on(void)
{
	struct mt66xx_hif_driver_data *prDriverData =
		get_platform_driver_data();
	struct mt66xx_chip_info *prChipInfo = prDriverData->chip_info;

	DBGLOG(INIT, INFO, "wlan_pre_pwr_on\n");

	return wlan_pinctrl_action(prChipInfo, WLAN_PINCTRL_MSG_FUNC_ON);
}

static int wlan_efuse_on(void)
{
	int32_t ret = 0;
	struct task_struct *cutTask = current;

	DBGLOG(INIT, INFO, "wlan_efuse_on.\n");

	/*Setup sub_wifi_thrd run on non X core */
	kalSetRunOnNonXCore(cutTask);

	/* expect unlock wfsys at the end of do_cal_cb */
	wfsys_lock();
	set_cal_enabled(FALSE);
	ret = wlanFuncOnImpl();
	if (ret)
		goto exit;

	wlanFuncOffImpl();

exit:
	set_cal_enabled(TRUE);
	if (ret) {
		DBGLOG(INIT, ERROR, "failed, ret=%d\n", ret);
		wfsys_unlock();
	}

	return ret;
}

static struct work_struct pwr_on_notify_work;
static u_int8_t fgInPwrOnNotifyCb;
static void __wlan_pwr_on_notify(struct work_struct *work)
{
	struct GLUE_INFO *glue = NULL;
	int32_t ret = 0;
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T *prWlanOnOffWakeLock;
#endif
	DBGLOG(INIT, INFO, "__wlan_pwr_on_notify.\n");

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_INIT(NULL,
		prWlanOnOffWakeLock, "WLAN pwr_on");
	KAL_WAKE_LOCK(NULL, prWlanOnOffWakeLock);
#endif
	wfsys_lock();

	WIPHY_PRIV(wlanGetWiphy(), glue);
	if (glue->u4ReadyFlag == 1) {
		DBGLOG(INIT, TRACE, "Skip due to wifi is already on.\n");
		ret = 0;
		goto exit;
	}

	fgInPwrOnNotifyCb = TRUE;
	set_cal_enabled(FALSE);
	ret = wlanFuncOn();
	if (ret)
		goto exit;

	wlanFuncOff();

exit:
	set_cal_enabled(TRUE);
	fgInPwrOnNotifyCb = FALSE;
	if (ret)
		DBGLOG(INIT, ERROR, "failed, ret=%d\n", ret);
	wfsys_unlock();
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_UNLOCK(NULL, prWlanOnOffWakeLock);
	KAL_WAKE_LOCK_DESTROY(NULL, prWlanOnOffWakeLock);
#endif
}

u_int8_t is_pwr_on_notify_processing(void)
{
	return fgInPwrOnNotifyCb;
}

static int wlan_pwr_on_notify(void)
{
	DBGLOG(INIT, INFO, "wlan_pwr_on_notify.\n");

	schedule_work(&pwr_on_notify_work);

	return 0;
}

static int wlan_chip_power_down_notify(unsigned int notify)
{
	while (get_wifi_process_status() == 3) {
		DBGLOG_LIMITED(REQ, WARN,
			"Wi-Fi off process is ongoing, wait here.\n");
		msleep(50);
	}

	if ((!get_wifi_process_status() && !get_wifi_powered_status()) ||
	    (kalGetShutdownState() == 2))
		glNotifyPciePowerDown();

#if CFG_TESTMODE_WMT_WIFI_ON_SUPPORT
	/* prevent turn on wifi by wmt driver before precal finished */
	/* so we register cb function after precal done */
	register_set_wifi_test_mode_fwdl_handler(set_wifi_test_mode_fwdl);
#endif

	return 0;
}

#if CFG_MTK_ANDROID_WMT && CFG_WIFI_PLAT_SHUTDOWN_SUPPORT
static int wlan_pre_fmd(void)
{
#define MAX_WAIT_OFF_DONE 10

	int retry = 0;

	if (kalGetShutdownState()) {
		while (kalGetShutdownState() == 1) {
			if (retry > MAX_WAIT_OFF_DONE) {
				DBGLOG(INIT, WARN,
					"shutdown off over 1s, retry = %d\n",
					retry);
			}
			kalMsleep(100);
			retry++;
		}
	} else {
		wfsys_lock();
		wlanShutdown();
		wfsys_unlock();
	}

	DBGLOG(INIT, INFO, "wifi off success\n");
	return 0;
}

static int wlan_post_fmd(void)
{
	return wlan_chip_power_down_notify(0);
}
#endif

static void unregister_connv3_cbs(void)
{
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	conninfra_sub_drv_ops_unregister(CONNDRV_TYPE_MAWD);
#endif

	connv3_sub_drv_ops_unregister(CONNV3_DRV_TYPE_WIFI);
}

static void register_connv3_cbs(void)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct connv3_sub_drv_ops_cb cb;
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	struct sub_drv_ops_cb conninfra_wf_cb;
#endif
	int ret = 0;
#if CFG_SUPPORT_WIFI_SLEEP_COUNT
	struct mt66xx_chip_info *chip = NULL;

	glGetChipInfo((void **)&chip);
#endif

	kalMemZero(&cb, sizeof(cb));
	cb.pwr_on_cb.pre_power_on = wlan_pre_pwr_on;
	cb.pwr_on_cb.power_on_notify = wlan_pwr_on_notify;
	cb.pwr_on_cb.chip_power_down_notify = wlan_chip_power_down_notify;

	INIT_WORK(&pwr_on_notify_work, __wlan_pwr_on_notify);

#if (CFG_SUPPORT_PRE_ON_PHY_ACTION == 1)
	cb.pre_cal_cb.pre_on_cb = wlan_pre_pwr_on;
	cb.pre_cal_cb.efuse_on_cb = wlan_efuse_on;
	cb.pre_cal_cb.pwr_on_cb = wlan_precal_pwron_v2;
	cb.pre_cal_cb.do_cal_cb = wlan_precal_docal_v2;
	cb.pre_cal_cb.pre_cal_error = wlan_precal_err;
#endif

#if CFG_MTK_ANDROID_WMT && CFG_WIFI_PLAT_SHUTDOWN_SUPPORT
	cb.fmd_cb.pre_fmd_cb = wlan_pre_fmd;
	cb.fmd_cb.post_fmd_cb = wlan_post_fmd;
#endif

#if CFG_CHIP_RESET_SUPPORT
	cb.rst_cb.pre_whole_chip_rst = wlan_pre_whole_chip_rst_v3;
	cb.rst_cb.post_whole_chip_rst = wlan_post_whole_chip_rst_v3;
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
	cb.rst_cb.post_reset_on = wlan_post_reset_on_v3;
#endif
#endif

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);

#if CFG_SUPPORT_HIF_REG_WORK
	cb.cr_cb.priv_data = prGlueInfo;
	cb.cr_cb.read = wf_reg_read_wrapper;
	cb.cr_cb.write = wf_reg_write_wrapper;
	cb.cr_cb.write_mask = wf_reg_write_mask_wrapper;
	cb.hif_dump_cb.hif_dump_start = wf_reg_start_wrapper;
	cb.hif_dump_cb.hif_dump_end = wf_reg_end_wrapper;
#endif

#if CFG_SUPPORT_WIFI_SLEEP_COUNT
	if (!chip) {
		DBGLOG(HAL, ERROR, "NULL chip info.\n");
	} else {
		cb.pwr_dump_cb.power_dump_start =
			chip->bus_info->wf_power_dump_start;
		cb.pwr_dump_cb.power_dump_end =
			chip->bus_info->wf_power_dump_end;
	}
#endif

	ret = connv3_sub_drv_ops_register(CONNV3_DRV_TYPE_WIFI, &cb);
	if (ret)
		DBGLOG(INIT, ERROR,
			"connv3_sub_drv_ops_register failed, ret=%d\n",
			ret);

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	kalMemZero(&conninfra_wf_cb, sizeof(struct sub_drv_ops_cb));
#if CFG_CHIP_RESET_SUPPORT
	conninfra_wf_cb.rst_cb.pre_whole_chip_rst =
		wlan_pre_whole_chip_rst_v2;
	conninfra_wf_cb.rst_cb.post_whole_chip_rst =
		wlan_post_whole_chip_rst_v2;
#endif

	ret = conninfra_sub_drv_ops_register(CONNDRV_TYPE_MAWD,
		&conninfra_wf_cb);
	if (ret)
		DBGLOG(INIT, ERROR,
			"conninfra_sub_drv_ops_register failed, ret=%d\n",
			ret);
#endif
}
#endif

#if CFG_MTK_WIFI_DEVAPC
static void wlan_devapc_debug_dump(void)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct CHIP_DBG_OPS *prDbgOps;

	glGetChipInfo((void **)&prChipInfo);
	if (prChipInfo) {
		prDbgOps = prChipInfo->prDebugOps;
		if (prDbgOps->showDevapcDebugInfo)
			prDbgOps->showDevapcDebugInfo();
	}

	GL_DEFAULT_RESET_TRIGGER(NULL, RST_DEVAPC);
}

static struct devapc_vio_callbacks wlan_devapc_vio_handle = {
	.id = INFRA_SUBSYS_PCIE,
	.debug_dump = wlan_devapc_debug_dump,
};
#endif

void unregister_plat_connsys_cbs(void)
{
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	unregister_connv3_cbs();
#endif

	unregister_chrdev_cbs();
}

void register_plat_connsys_cbs(void)
{
	register_chrdev_cbs();

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	register_connv3_cbs();
#endif

#if CFG_MTK_WIFI_DEVAPC
	if (!g_fgIsRegDevapcCb) {
		register_devapc_vio_callback(&wlan_devapc_vio_handle);
		g_fgIsRegDevapcCb = TRUE;
	}
#endif
}
#endif

#if CFG_NEW_HIF_DEV_REG_IF
static void connac3xInitValidMmioReadReasonAry(
	struct mt66xx_chip_info *prChipInfo)
{
	uint32_t u4Idx, u4Num, u4Size = prChipInfo->u4ValidMmioReadReasonSize;

	if (prChipInfo->fgIsInitValidMmioReadAry)
		return;

	for (u4Idx = 0; u4Idx < u4Size; u4Idx++) {
		u4Num = prChipInfo->prValidMmioReadReason[u4Idx];
		prChipInfo->aucValidMmioReadAry[u4Num] = TRUE;
	}

	u4Size = prChipInfo->u4NoMmioReadReasonSize;
	for (u4Idx = 0; u4Idx < u4Size; u4Idx++) {
		u4Num = prChipInfo->prNoMmioReadReason[u4Idx];
		prChipInfo->aucNoMmioReadReasonAry[u4Num] = TRUE;
	}

	prChipInfo->fgIsInitValidMmioReadAry = TRUE;
}

u_int8_t connac3xIsValidMmioReadReason(
	struct mt66xx_chip_info *prChipInfo, enum HIF_DEV_REG_REASON eReason)
{
	connac3xInitValidMmioReadReasonAry(prChipInfo);

	if (eReason >= HIF_DEV_REG_MAX)
		return FALSE;

	return prChipInfo->aucValidMmioReadAry[eReason];
}

u_int8_t connac3xIsNoMmioReadReason(
	struct mt66xx_chip_info *prChipInfo, enum HIF_DEV_REG_REASON eReason)
{
	if (eReason >= HIF_DEV_REG_MAX)
		return FALSE;

	return prChipInfo->aucNoMmioReadReasonAry[eReason];
}
#endif /* CFG_NEW_HIF_DEV_REG_IF */

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
void connac3xClearEvtRingTillCmdRingEmpty(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_TX_RING *prTxRing;
	uint32_t u4Idx = 0;

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	prTxRing = &prHifInfo->TxRing[TX_RING_CMD];
	for (u4Idx = 0; u4Idx < HIF_CMD_POWER_OFF_RETRY_COUNT; u4Idx++) {
		halWpdmaProcessCmdDmaDone(prAdapter->prGlueInfo, TX_RING_CMD);
		if (prTxRing->u4UsedCnt == 0)
			break;

		kalMsleep(HIF_CMD_POWER_OFF_RETRY_TIME);
		nicProcessISTWithSpecifiedCount(prAdapter, 1);

	}
	if (u4Idx) {
		DBGLOG(HAL, INFO,
		       "try to clear event ring, cmd[%u] retry[%u]\n",
		       prTxRing->u4UsedCnt, u4Idx);
	}
}
#endif /*_HIF_PCIE || _HIF_AXI */
#endif /* CFG_SUPPORT_CONNAC3X == 1 */
