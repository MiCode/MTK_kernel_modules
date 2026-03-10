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
#include "gl_rst.h"

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
#include <linux/soc/mediatek/devapc_public.h>
#endif

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
#if IS_ENABLED(CONFIG_MTK_DEVAPC)
u_int8_t g_fgIsRegDevapcCb;
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
	if (prAdapter->chip_info->is_support_wfdma1)
		u4HostWpdamBase = CONNAC3X_HOST_WPDMA_1_BASE;
	else
		u4HostWpdamBase = CONNAC3X_HOST_WPDMA_0_BASE;

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

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
#if CFG_MTK_WIFI_EN_SW_EMI_READ
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct SW_EMI_RING_INFO *prSwEmiRingInfo = &prBusInfo->rSwEmiRingInfo;
	u_int8_t fgRet = FALSE;

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnSwEmiRead) &&
	    prSwEmiRingInfo->rOps.read) {
		fgRet = prSwEmiRingInfo->rOps.read(
			prAdapter->prGlueInfo, u4Addr, pu4Value);
	}
	if (!fgRet)
#endif
		HAL_MCR_RD(prAdapter, u4Addr, pu4Value);

	return (*pu4Value & CONNAC3X_WFDMA_NEED_REINIT_BIT) == 0 ? TRUE : FALSE;
}


void asicConnac3xWfdmaDummyCrWrite(
	struct ADAPTER *prAdapter)
{
	u_int32_t u4RegValue = 0;

	asicConnac3xWfdmaDummyCrRead(prAdapter, &u4RegValue);
	u4RegValue |= CONNAC3X_WFDMA_NEED_REINIT_BIT;

	HAL_MCR_WR(prAdapter,
		CONNAC3X_WFDMA_DUMMY_CR,
		u4RegValue);
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
	u_int8_t fgResult = FALSE;
	uint32_t u4Val = 0;

	/*WFDMA re-init flow after chip deep sleep*/
	fgResult = asicConnac3xWfdmaDummyCrRead(prAdapter, &u4Val);
	if (fgResult) {
		asicConnac3xWfdmaReInitImpl(prAdapter);
		asicConnac3xWfdmaDummyCrWrite(prAdapter);
	}
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
	HAL_MCR_RD(prAdapter, u4DmaCfgCr, &GloCfg.word);

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
	struct BUS_INFO *prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

	if (index == 0)
		u4RegAddr = prBusInfo->host_dma0_base;
	else if (index == 1)
		u4RegAddr = prBusInfo->host_dma1_base;
	else {
		DBGLOG(HAL, ERROR, "Unknown wfdma index(=%d)\n", index);
		return FALSE;
	}

	do {
		HAL_MCR_RD(prAdapter, u4RegAddr, &GloCfg.word);
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
		if (index == TX_RING_DATA_PRIO)
			ext_offset = prBusInfo->tx_ring2_data_idx * 4;
		if (index == TX_RING_WA_CMD)
			ext_offset = prBusInfo->tx_ring_wa_cmd_idx * 4;
	} else
		ext_offset = index * 4;

	tx_ring->hw_desc_base_ext =
		prBusInfo->host_tx_ring_ext_ctrl_base + ext_offset;
	HAL_MCR_WR(prAdapter, tx_ring->hw_desc_base_ext,
		   CONNAC3X_TX_RING_DISP_MAX_CNT);
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
}

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
	    KAL_TEST_AND_CLEAR_BIT(1, prHifInfo->ulHifIntEnBits)) {
		enable_irq(prHifInfo->u4IrqId_1);
	}
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

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	/* IrqId_1 is MAWD interrupt */
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd)) {
		disable_irq_nosync(prHifInfo->u4IrqId_1);
		KAL_SET_BIT(1, prHifInfo->ulHifIntEnBits);
	}
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
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

void asicConnac3xDisableExtInterrupt(
	struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	union WPDMA_INT_MASK IntMask;

	ASSERT(prAdapter);

	prGlueInfo = prAdapter->prGlueInfo;

	IntMask.word = 0;

	HAL_MCR_WR(prAdapter,
		CONNAC3X_WPDMA_EXT_INT_MASK(CONNAC3X_HOST_EXT_CONN_HIF_WRAP),
		IntMask.word);
	HAL_MCR_RD(prAdapter,
		CONNAC3X_WPDMA_EXT_INT_MASK(CONNAC3X_HOST_EXT_CONN_HIF_WRAP),
		&IntMask.word);

	DBGLOG(HAL, TRACE, "%s\n", __func__);
}

void asicConnac3xLowPowerOwnRead(
	struct ADAPTER *prAdapter,
	u_int8_t *pfgResult)
{
	struct mt66xx_chip_info *prChipInfo;

	prChipInfo = prAdapter->chip_info;

	if (prChipInfo->is_support_asic_lp) {
		u_int32_t u4RegValue = 0;

		HAL_MCR_RD(prAdapter,
				CONNAC3X_BN0_LPCTL_ADDR,
				&u4RegValue);
		*pfgResult = (u4RegValue &
				PCIE_LPCR_AP_HOST_OWNER_STATE_SYNC)
				== 0 ? TRUE : FALSE;
	} else
		*pfgResult = TRUE;
}

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

		if (prChipInfo->setCrypto)
			prChipInfo->setCrypto(prAdapter);

		clear_bit(GLUE_FLAG_DRV_OWN_INT_BIT,
			&prAdapter->prGlueInfo->ulFlag);
		HAL_MCR_WR(prAdapter,
			CONNAC3X_BN0_LPCTL_ADDR,
			PCIE_LPCR_HOST_CLR_OWN);
		if (prAdapter->rWifiVar.u4DrvOwnMode == 1)
			kalMdelay(10);

	}

	*pfgResult = TRUE;
}

void asicConnac3xProcessSoftwareInterrupt(
	struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
#if CFG_MTK_WIFI_EN_SW_EMI_READ
	struct BUS_INFO *prBusInfo;
	struct WIFI_VAR *prWifiVar;
	struct SW_EMI_RING_INFO *prSwEmiRingInfo;
	u_int8_t fgRet = FALSE;
#endif
	struct ERR_RECOVERY_CTRL_T *prErrRecoveryCtrl;
	uint32_t u4Status = 0, u4Addr = 0;
	uint32_t u4HostWpdamBase = 0;

	if (prAdapter->prGlueInfo == NULL) {
		DBGLOG(HAL, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
#if CFG_MTK_WIFI_EN_SW_EMI_READ
	prBusInfo = prAdapter->chip_info->bus_info;
	prWifiVar = &prAdapter->rWifiVar;
	prSwEmiRingInfo = &prBusInfo->rSwEmiRingInfo;
#endif
	prErrRecoveryCtrl = &prHifInfo->rErrRecoveryCtl;

	if (prAdapter->chip_info->is_support_wfdma1)
		u4HostWpdamBase = CONNAC3X_HOST_WPDMA_1_BASE;
	else
		u4HostWpdamBase = CONNAC3X_HOST_WPDMA_0_BASE;

	u4Addr = CONNAC3X_WPDMA_MCU2HOST_SW_INT_STA(u4HostWpdamBase);
#if CFG_MTK_WIFI_EN_SW_EMI_READ
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnSwEmiRead) &&
	    prSwEmiRingInfo->rOps.read) {
		fgRet = prSwEmiRingInfo->rOps.read(
			prGlueInfo, u4Addr, &u4Status);
	}

	if (!fgRet)
#endif
		kalDevRegRead(prGlueInfo, u4Addr, &u4Status);

	prErrRecoveryCtrl->u4BackupStatus = u4Status;
	if (u4Status & ERROR_DETECT_MASK) {
		prErrRecoveryCtrl->u4Status = u4Status;
		kalDevRegWrite(prGlueInfo,
			CONNAC3X_WPDMA_MCU2HOST_SW_INT_STA(u4HostWpdamBase),
			u4Status);
		halHwRecoveryFromError(prAdapter);
	} else {
		kalDevRegWrite(prGlueInfo,
			CONNAC3X_WPDMA_MCU2HOST_SW_INT_STA(u4HostWpdamBase),
			u4Status);
		DBGLOG(HAL, TRACE, "undefined SER status[0x%x].\n", u4Status);
	}
}

void asicConnac3xSoftwareInterruptMcu(
	struct ADAPTER *prAdapter, u_int32_t intrBitMask)
{
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4McuWpdamBase = 0;

	if (prAdapter == NULL || prAdapter->prGlueInfo == NULL) {
		DBGLOG(HAL, ERROR, "prAdapter or prGlueInfo is NULL\n");
		return;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	if (prAdapter->chip_info->is_support_wfdma1)
		u4McuWpdamBase = CONNAC3X_MCU_WPDMA_1_BASE;
	else
		u4McuWpdamBase = CONNAC3X_MCU_WPDMA_0_BASE;
	kalDevRegWrite(prGlueInfo,
		CONNAC3X_WPDMA_HOST2MCU_SW_INT_SET(u4McuWpdamBase),
		intrBitMask);
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

	/* HAL_MCR_RD(prAdapter, 0x7c00e400, &u4WfdmaCr); */
	/* HAL_MCR_WR(prAdapter, 0x7c00e400, 0xFF); */

	HAL_MCR_RD(prAdapter, 0x7c021100, &u4WfdmaCr);
	HAL_MCR_WR(prAdapter, 0x7c021100, 0x0);

	if (prChipInfo->is_support_wfdma1) {
		u4WfdmaAddr =
		CONNAC3X_TX_RING_EXT_CTRL_BASE(CONNAC3X_HOST_WPDMA_1_BASE);
	} else {
		u4WfdmaAddr =
		CONNAC3X_TX_RING_EXT_CTRL_BASE(CONNAC3X_HOST_WPDMA_0_BASE);
	}
	/*
	 * HOST_DMA1_WPDMA_TX_RING0_EXT_CTRL ~ HOST_DMA1_WPDMA_TX_RING4_EXT_CTRL
	 */
	for (idx = 0; idx < USB_TX_EPOUT_NUM; idx++) {
		HAL_MCR_RD(prAdapter, u4WfdmaAddr + (idx*4), &u4WfdmaCr);
		u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_MAX_CNT_MASK;
		u4WfdmaCr |= CONNAC3X_TX_RING_DISP_MAX_CNT;
		u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_BASE_PTR_MASK;
		u4WfdmaCr |= (0x008 + 0x4 * idx)<<20;
		HAL_MCR_WR(prAdapter, u4WfdmaAddr + (idx*4), u4WfdmaCr);
	}

	/* HOST_DMA1_WPDMA_TX_RING16_EXT_CTRL_ADDR */
	HAL_MCR_RD(prAdapter, u4WfdmaAddr + 0x40, &u4WfdmaCr);
	u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_MAX_CNT_MASK;
	u4WfdmaCr |= CONNAC3X_TX_RING_DISP_MAX_CNT;
	u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_BASE_PTR_MASK;
	u4WfdmaCr |= 0x02800000;
	HAL_MCR_WR(prAdapter, u4WfdmaAddr + 0x40, u4WfdmaCr);

	/* HOST_DMA1_WPDMA_TX_RING15_EXT_CTRL_ADDR */
	HAL_MCR_RD(prAdapter, u4WfdmaAddr + 0x3c, &u4WfdmaCr);
	u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_MAX_CNT_MASK;
	u4WfdmaCr |= CONNAC3X_TX_RING_DISP_MAX_CNT;
	u4WfdmaCr &= ~CONNAC3X_WFDMA_DISP_BASE_PTR_MASK;
	u4WfdmaCr |= 0x02c00000;
	HAL_MCR_WR(prAdapter, u4WfdmaAddr + 0x3c, u4WfdmaCr);

	if (prChipInfo->is_support_wfdma1) {
		u4WfdmaAddr =
			CONNAC3X_WPDMA_GLO_CFG(CONNAC3X_HOST_WPDMA_1_BASE);
		HAL_MCR_RD(prAdapter, u4WfdmaAddr, &u4WfdmaCr);
		u4WfdmaCr |=
			(CONNAC3X_WPDMA1_GLO_CFG_OMIT_TX_INFO |
			 CONNAC3X_WPDMA1_GLO_CFG_OMIT_RX_INFO |
			 CONNAC3X_WPDMA1_GLO_CFG_FW_DWLD_Bypass_dmashdl |
			 CONNAC3X_WPDMA1_GLO_CFG_RX_DMA_EN |
			 CONNAC3X_WPDMA1_GLO_CFG_TX_DMA_EN);
		HAL_MCR_WR(prAdapter, u4WfdmaAddr, u4WfdmaCr);

		/* Enable WFDMA0 RX for receiving data frame */
		u4WfdmaAddr =
			CONNAC3X_WPDMA_GLO_CFG(CONNAC3X_HOST_WPDMA_0_BASE);
		HAL_MCR_RD(prAdapter, u4WfdmaAddr, &u4WfdmaCr);
		u4WfdmaCr |=
			(CONNAC3X_WPDMA1_GLO_CFG_RX_DMA_EN);
		HAL_MCR_WR(prAdapter, u4WfdmaAddr, u4WfdmaCr);
	} else {
		u4WfdmaAddr =
			CONNAC3X_WPDMA_GLO_CFG(CONNAC3X_HOST_WPDMA_0_BASE);
		HAL_MCR_RD(prAdapter, u4WfdmaAddr, &u4WfdmaCr);
		u4WfdmaCr &= ~(CONNAC3X_WPDMA1_GLO_CFG_OMIT_RX_INFO);
		u4WfdmaCr |=
			(CONNAC3X_WPDMA1_GLO_CFG_OMIT_TX_INFO |
			 CONNAC3X_WPDMA1_GLO_CFG_OMIT_RX_INFO_PFET2 |
			 CONNAC3X_WPDMA1_GLO_CFG_FW_DWLD_Bypass_dmashdl |
			 CONNAC3X_WPDMA1_GLO_CFG_RX_DMA_EN |
			 CONNAC3X_WPDMA1_GLO_CFG_TX_DMA_EN);
		HAL_MCR_WR(prAdapter, u4WfdmaAddr, u4WfdmaCr);

	}

	prChipInfo->is_support_dma_shdl = wlanCfgGetUint32(prAdapter,
				    "DmaShdlEnable",
				    FEATURE_ENABLED);
	if (!prChipInfo->is_support_dma_shdl) {
		/*
		 *	To disable 0x7C0252B0[6] DMASHDL
		 */
		if (prChipInfo->is_support_wfdma1) {
			u4WfdmaAddr = CONNAC3X_WPDMA_GLO_CFG_EXT0(
					CONNAC3X_HOST_WPDMA_1_BASE);
		} else {
			u4WfdmaAddr = CONNAC3X_WPDMA_GLO_CFG_EXT0(
					CONNAC3X_HOST_WPDMA_0_BASE);
		}
		HAL_MCR_RD(prAdapter, u4WfdmaAddr, &u4WfdmaCr);
		u4WfdmaCr &= ~CONNAC3X_WPDMA1_GLO_CFG_EXT0_TX_DMASHDL_EN;
		HAL_MCR_WR(prAdapter, u4WfdmaAddr, u4WfdmaCr);

		/*
		 *	[28]DMASHDL_BYPASS
		 *	DMASHDL host ask and quota control function bypass
		 *	0: Disable
		 *	1: Enable
		 */
		u4WfdmaAddr = CONNAC3X_HOST_DMASHDL_SW_CONTROL(
					CONNAC3X_HOST_DMASHDL);
		HAL_MCR_RD(prAdapter, u4WfdmaAddr, &u4WfdmaCr);
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

	HAL_MCR_RD(prAdapter, prBusInfo->u4UdmaTxQsel, &u4Value);
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

	HAL_MCR_RD(prAdapter, prBusInfo->u4UdmaWlCfg_0_Addr,
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

	for (u4Loop = 0; u4Loop < MAX_POLLING_LOOP; u4Loop++) {
		for (u4Idx = 0;
			u4Idx < sizeof(g_au4UsbPollAddrTbl)/sizeof(uint32_t);
			u4Idx++) {
			HAL_MCR_RD(prAdapter,
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

	prHwTxDescAppend = (union HW_MAC_TX_DESC_APPEND *)
		(pucBuffer + NIC_TX_DESC_LONG_FORMAT_LENGTH);

	prHwTxDescAppend->CR4_APPEND.u2MsduToken = u4MsduId;
	prHwTxDescAppend->CR4_APPEND.ucBufNum = 1;
	prHwTxDescAppend->CR4_APPEND.au4BufPtr[0] = rDmaAddr;
	prHwTxDescAppend->CR4_APPEND.au2BufLen[0] = prMsduInfo->u2FrameLength;
}

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
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
	union HW_MAC_TX_DESC_APPEND *prHwTxDescAppend;
	uint8_t *pucData;
	struct BUS_INFO *prBusInfo;
	union mawd_l2tbl rL2Tbl = {0};
	uint16_t u2EtherTypeLen = 0, u2Port = TX_RING_DATA0;
	uint8_t ucType = 0;
	uint32_t u4NumIPv4 = 0, u4NumIPv6 = 0;
	uint8_t pucIPv4Addr[IPV4_ADDR_LEN * CFG_PF_ARP_NS_MAX_NUM * 2];
	uint8_t pucIPv6Addr[IPV6_ADDR_LEN * CFG_PF_ARP_NS_MAX_NUM];
	uint8_t ucIpVer, u4TxDSize;

	kalGetIPv4Address(wlanGetNetDev(prAdapter->prGlueInfo, 0),
		CFG_PF_ARP_NS_MAX_NUM, pucIPv4Addr,
			  &u4NumIPv4);
#if IS_ENABLED(CONFIG_IPV6)
	kalGetIPv6Address(wlanGetNetDev(prAdapter->prGlueInfo, 0),
		CFG_PF_ARP_NS_MAX_NUM, pucIPv6Addr,
			  &u4NumIPv6);
#endif

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

	kalMemZero(pTxCell->AllocVa, u4TxDSize);

	kalMemCopy(pTxCell->AllocVa, pucBuffer, u4TxDSize);

	prHwTxDescAppend = (union HW_MAC_TX_DESC_APPEND *)
		(pTxCell->AllocVa + NIC_TX_DESC_LONG_FORMAT_LENGTH);

	prHwTxDescAppend->CR4_APPEND.u2PktFlags = 0;
	if (ucType)
		prHwTxDescAppend->CR4_APPEND.u2PktFlags |=
			HIF_PKT_FLAGS_CT_INFO_STA_APPLY_OVERRIDE |
			HIF_PKT_FLAGS_CT_INFO_MAWD_OFLD;

	prHwTxDescAppend->CR4_APPEND.ucBssIndex = prMsduInfo->ucBssIndex;
	prHwTxDescAppend->CR4_APPEND.ucWtblIndex = prMsduInfo->ucWlanIndex;

	prHwTxDescAppend->CR4_APPEND.u2MsduToken = u4MsduId;
	prHwTxDescAppend->CR4_APPEND.ucBufNum = 2;
	prHwTxDescAppend->CR4_APPEND.au4BufPtr[0] = rDmaAddr;
	/* eth header */
	prHwTxDescAppend->CR4_APPEND.au2BufLen[0] = ETH_HLEN;
	prHwTxDescAppend->CR4_APPEND.au4BufPtr[1] = (rDmaAddr + ETH_HLEN);
	prHwTxDescAppend->CR4_APPEND.au2BufLen[1] =
		prMsduInfo->u2FrameLength - ETH_HLEN;
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
		prTxDescOps->fillNicAppend =
			fillConnac3xNicTxDescAppendWithSdo;
		prTxDescOps->fillHifAppend =
			fillConnac3xTxDescAppendBySdo;
		prTxDescOps->fillTxByteCount =
			fillConnac3xTxDescTxByteCountWithSdo;
	}

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawdTx)) {
		prChipInfo->txd_append_size =
			prChipInfo->hif_txd_append_size;
		prTxDescOps->fillNicAppend =
			fillConnac3xNicTxDescAppendWithSdo;
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
	prRxDescOps->nic_rxd_check_wakeup_reason =
		nic_rxd_v3_check_wakeup_reason;
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
	uint32_t *prRxV = NULL; /* pointer to destination buffer to store RxV */

	if (prRetSwRfb->ucStaRecIdx >= CFG_STA_REC_NUM) {
		DBGLOG(RX, LOUD,
		"prRetSwRfb->ucStaRecIdx(%d) >= CFG_STA_REC_NUM(%d)\n",
			prRetSwRfb->ucStaRecIdx, CFG_STA_REC_NUM);
		return;
	}

	if (prRetSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_3)) {
		prRxV = prAdapter->arStaRec[prRetSwRfb->ucStaRecIdx].au4RxV;
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
	prGroup3 = (struct HW_MAC_RX_STS_GROUP_3_V2 *)prSwRfb->prRxStatusGroup3;
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

void asicConnac3xDmashdlSetPlePktMaxPage(struct ADAPTER *prAdapter,
				      uint16_t u2MaxPage)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	HAL_MCR_RD(prAdapter, prCfg->rPlePacketMaxSize.u4Addr, &u4Val);

	u4Val &= ~prCfg->rPlePacketMaxSize.u4Mask;
	u4Val |= (u2MaxPage << prCfg->rPlePacketMaxSize.u4Shift) &
		prCfg->rPlePacketMaxSize.u4Mask;

	HAL_MCR_WR(prAdapter, prCfg->rPlePacketMaxSize.u4Addr, u4Val);
}

void asicConnac3xDmashdlSetPsePktMaxPage(struct ADAPTER *prAdapter,
				      uint16_t u2MaxPage)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	HAL_MCR_RD(prAdapter, prCfg->rPsePacketMaxSize.u4Addr, &u4Val);

	u4Val &= ~prCfg->rPsePacketMaxSize.u4Mask;
	u4Val |= (u2MaxPage << prCfg->rPsePacketMaxSize.u4Shift) &
		 prCfg->rPsePacketMaxSize.u4Mask;

	HAL_MCR_WR(prAdapter, prCfg->rPsePacketMaxSize.u4Addr, u4Val);
}

void asicConnac3xDmashdlGetPktMaxPage(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Val = 0;
	uint32_t ple_pkt_max_sz;
	uint32_t pse_pkt_max_sz;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	HAL_MCR_RD(prAdapter, prCfg->rPlePacketMaxSize.u4Addr, &u4Val);

	ple_pkt_max_sz = (u4Val & prCfg->rPlePacketMaxSize.u4Mask) >>
		prCfg->rPlePacketMaxSize.u4Shift;
	pse_pkt_max_sz = (u4Val & prCfg->rPsePacketMaxSize.u4Mask) >>
		prCfg->rPsePacketMaxSize.u4Shift;

	DBGLOG(HAL, INFO, "DMASHDL PLE_PACKET_MAX_SIZE (0x%08x): 0x%08x\n",
		prCfg->rPlePacketMaxSize.u4Addr, u4Val);
	DBGLOG(HAL, INFO, "PLE/PSE packet max size=0x%03x/0x%03x\n",
		ple_pkt_max_sz, pse_pkt_max_sz);

}
void asicConnac3xDmashdlSetRefill(struct ADAPTER *prAdapter, uint8_t ucGroup,
			       u_int8_t fgEnable)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Mask;
	uint32_t u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	if (ucGroup >= prCfg->u4GroupNum)
		ASSERT(0);

	u4Mask = prCfg->rGroup0RefillDisable.u4Mask << ucGroup;

	HAL_MCR_RD(prAdapter, prCfg->rGroup0RefillDisable.u4Addr, &u4Val);

	if (fgEnable)
		u4Val &= ~u4Mask;
	else
		u4Val |= u4Mask;

	HAL_MCR_WR(prAdapter, prCfg->rGroup0RefillDisable.u4Addr, u4Val);
}

void asicConnac3xDmashdlGetRefill(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	HAL_MCR_RD(prAdapter, prCfg->rGroup0RefillDisable.u4Addr, &u4Val);
	DBGLOG(HAL, INFO, "DMASHDL ReFill Control (0x%08x): 0x%08x\n",
		prCfg->rGroup0RefillDisable.u4Addr, u4Val);
}

void asicConnac3xDmashdlSetMaxQuota(struct ADAPTER *prAdapter, uint8_t ucGroup,
				 uint16_t u2MaxQuota)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr;
	uint32_t u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	if (ucGroup >= prCfg->u4GroupNum)
		ASSERT(0);

	u4Addr = prCfg->rGroup0ControlMaxQuota.u4Addr + (ucGroup << 2);

	HAL_MCR_RD(prAdapter, u4Addr, &u4Val);

	u4Val &= ~prCfg->rGroup0ControlMaxQuota.u4Mask;
	u4Val |= (u2MaxQuota << prCfg->rGroup0ControlMaxQuota.u4Shift) &
		prCfg->rGroup0ControlMaxQuota.u4Mask;

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

void asicConnac3xDmashdlSetMinQuota(struct ADAPTER *prAdapter, uint8_t ucGroup,
				 uint16_t u2MinQuota)
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

	HAL_MCR_RD(prAdapter, u4Addr, &u4Val);

	u4Val &= ~prCfg->rGroup0ControlMinQuota.u4Mask;
	u4Val |= (u2MinQuota << prCfg->rGroup0ControlMinQuota.u4Shift) &
		prCfg->rGroup0ControlMinQuota.u4Mask;

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

void asicConnac3xDmashdlGetGroupControl(struct ADAPTER *prAdapter,
					uint8_t ucGroup)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr;
	uint32_t u4Val = 0;
	uint32_t max_quota;
	uint32_t min_quota;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	u4Addr = prCfg->rGroup0ControlMaxQuota.u4Addr + (ucGroup << 2);

	HAL_MCR_RD(prAdapter, u4Addr, &u4Val);

	max_quota = GET_DMASHDL_MAX_QUOTA_NUM(u4Val);
	min_quota = GET_DMASHDL_MIN_QUOTA_NUM(u4Val);
	DBGLOG(HAL, INFO, "\tDMASHDL Group%d control(0x%08x): 0x%08x\n",
		ucGroup, u4Addr, u4Val);
	DBGLOG(HAL, INFO, "\tmax/min quota = 0x%03x/ 0x%03x\n",
		max_quota, min_quota);

}
void asicConnac3xDmashdlSetQueueMapping(struct ADAPTER *prAdapter,
					uint8_t ucQueue,
					uint8_t ucGroup)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr, u4Mask, u4Shft;
	uint32_t u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	if (ucQueue >= 32)
		ASSERT(0);

	if (ucGroup >= prCfg->u4GroupNum)
		ASSERT(0);

	u4Addr = prCfg->rQueueMapping0Queue0.u4Addr + ((ucQueue >> 3) << 2);
	u4Mask = prCfg->rQueueMapping0Queue0.u4Mask << ((ucQueue % 8) << 2);
	u4Shft = (ucQueue % 8) << 2;

	HAL_MCR_RD(prAdapter, u4Addr, &u4Val);

	u4Val &= ~u4Mask;
	u4Val |= (ucGroup << u4Shft) & u4Mask;

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

void asicConnac3xDmashdlSetSlotArbiter(struct ADAPTER *prAdapter,
				    u_int8_t fgEnable)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	HAL_MCR_RD(prAdapter, prCfg->rPageSettingGroupSeqOrderType.u4Addr,
		   &u4Val);

	if (fgEnable)
		u4Val |= prCfg->rPageSettingGroupSeqOrderType.u4Mask;
	else
		u4Val &= ~prCfg->rPageSettingGroupSeqOrderType.u4Mask;

	HAL_MCR_WR(prAdapter, prCfg->rPageSettingGroupSeqOrderType.u4Addr,
		   u4Val);
}

void asicConnac3xDmashdlSetUserDefinedPriority(struct ADAPTER *prAdapter,
					    uint8_t ucPriority, uint8_t ucGroup)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr, u4Mask, u4Shft;
	uint32_t u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	ASSERT(ucPriority < 16);
	ASSERT(ucGroup < prCfg->u4GroupNum);

	u4Addr = prCfg->rSchdulerSetting0Priority0Group.u4Addr +
		((ucPriority >> 3) << 2);
	u4Mask = prCfg->rSchdulerSetting0Priority0Group.u4Mask <<
		((ucPriority % 8) << 2);
	u4Shft = (ucPriority % 8) << 2;

	HAL_MCR_RD(prAdapter, u4Addr, &u4Val);

	u4Val &= ~u4Mask;
	u4Val |= (ucGroup << u4Shft) & u4Mask;

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

uint32_t asicConnac3xDmashdlGetRsvCount(struct ADAPTER *prAdapter,
					uint8_t ucGroup)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr;
	uint32_t u4Val = 0;
	uint32_t rsv_cnt = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	u4Addr = prCfg->rStatusRdGp0RsvCnt.u4Addr + (ucGroup << 2);

	HAL_MCR_RD(prAdapter, u4Addr, &u4Val);

	rsv_cnt = (u4Val & prCfg->rStatusRdGp0RsvCnt.u4Mask) >>
		prCfg->rStatusRdGp0RsvCnt.u4Shift;

	DBGLOG(HAL, INFO, "\tDMASHDL Status_RD_GP%d(0x%08x): 0x%08x\n",
		ucGroup, u4Addr, u4Val);
	DBGLOG(HAL, TRACE, "\trsv_cnt = 0x%03x\n", rsv_cnt);
	return rsv_cnt;
}

uint32_t asicConnac3xDmashdlGetSrcCount(struct ADAPTER *prAdapter,
					uint8_t ucGroup)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr;
	uint32_t u4Val = 0;
	uint32_t src_cnt = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	u4Addr = prCfg->rStatusRdGp0SrcCnt.u4Addr + (ucGroup << 2);

	HAL_MCR_RD(prAdapter, u4Addr, &u4Val);

	src_cnt = (u4Val & prCfg->rStatusRdGp0SrcCnt.u4Mask) >>
		prCfg->rStatusRdGp0SrcCnt.u4Shift;

	DBGLOG(HAL, TRACE, "\tsrc_cnt = 0x%03x\n", src_cnt);
	return src_cnt;
}

void asicConnac3xDmashdlGetPKTCount(struct ADAPTER *prAdapter, uint8_t ucGroup)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr;
	uint32_t u4Val = 0;
	uint32_t pktin_cnt = 0;
	uint32_t ask_cnt = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	if ((ucGroup & 0x1) == 0)
		u4Addr = prCfg->rRdGroupPktCnt0.u4Addr + (ucGroup << 1);
	else
		u4Addr = prCfg->rRdGroupPktCnt0.u4Addr + ((ucGroup-1) << 1);

	HAL_MCR_RD(prAdapter, u4Addr, &u4Val);
	DBGLOG(HAL, INFO, "\tDMASHDL RD_group_pkt_cnt_%d(0x%08x): 0x%08x\n",
		ucGroup / 2, u4Addr, u4Val);
	if ((ucGroup & 0x1) == 0) {
		pktin_cnt = GET_EVEN_GROUP_PKT_IN_CNT(u4Val);
		ask_cnt = GET_EVEN_GROUP_ASK_CNT(u4Val);
	} else {
		pktin_cnt = GET_ODD_GROUP_PKT_IN_CNT(u4Val);
		ask_cnt = GET_ODD_GROUP_ASK_CNT(u4Val);
	}
	DBGLOG(HAL, INFO, "\tpktin_cnt = 0x%02x, ask_cnt = 0x%02x",
		pktin_cnt, ask_cnt);
}

void asicConnac3xDmashdlSetOptionalControl(struct ADAPTER *prAdapter,
		uint16_t u2HifAckCntTh, uint16_t u2HifGupActMap)
{
	struct BUS_INFO *prBusInfo;
	struct DMASHDL_CFG *prCfg;
	uint32_t u4Addr, u4Val = 0;

	prBusInfo = prAdapter->chip_info->bus_info;
	prCfg = prBusInfo->prDmashdlCfg;

	u4Addr = prCfg->rOptionalControlCrHifAckCntTh.u4Addr;

	HAL_MCR_RD(prAdapter, u4Addr, &u4Val);

	u4Val &= ~prCfg->rOptionalControlCrHifAckCntTh.u4Mask;
	u4Val |= (u2HifAckCntTh <<
		  prCfg->rOptionalControlCrHifAckCntTh.u4Shift);

	u4Val &= ~prCfg->rOptionalControlCrHifGupActMap.u4Mask;
	u4Val |= (u2HifGupActMap <<
		  prCfg->rOptionalControlCrHifGupActMap.u4Shift);

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

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
	} else {
		DBGLOG(HAL, ERROR, "FW trigger assert.\n");
		g_Coredump_source = COREDUMP_SOURCE_WF_FW;

		glSetRstReason(RST_FW_ASSERT);

		glResetUpdateFlag(TRUE);

		if (dbg_ops && dbg_ops->dumpBusHangCr)
			dbg_ops->dumpBusHangCr(prAdapter);

		kalSetRstEvent(TRUE);
	}
}

static void handle_whole_chip_reset(struct ADAPTER *prAdapter)
{
	struct CHIP_DBG_OPS *dbg_ops = prAdapter->chip_info->prDebugOps;

	DBGLOG(HAL, ERROR,
		"FW trigger whole chip reset.\n");

	wifi_coredump_set_enable(TRUE);
	g_Coredump_source = COREDUMP_SOURCE_WF_FW;
	glResetUpdateFlag(TRUE);
	g_IsWfsysBusHang = TRUE;

	if (dbg_ops && dbg_ops->dumpBusHangCr)
		dbg_ops->dumpBusHangCr(prAdapter);

	kalSetRstEvent(TRUE);
}
#endif

u_int8_t asicConnac3xSwIntHandler(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	uint32_t u4Status = 0;
	u_int8_t fgRet = TRUE;

	if (!prAdapter)
		return TRUE;

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	if (prBusInfo->hwControlVote)
		prBusInfo->hwControlVote(prAdapter,
					 FALSE,
					 PCIE_VOTE_USER_LOG_RESET);

	if (!prChipInfo->get_sw_interrupt_status)
		goto exit;

	fgRet = prChipInfo->get_sw_interrupt_status(prAdapter, &u4Status);
	if (fgRet == FALSE || u4Status == 0)
		goto exit;

	if (!(prGlueInfo->ulFlag & GLUE_FLAG_HALT) &&
	    (u4Status & BIT(SW_INT_FW_LOG)))
		fw_log_handler();

#if CFG_WMT_RESET_API_SUPPORT
	if (u4Status & BIT(SW_INT_SUBSYS_RESET))
		handle_wfsys_reset(prAdapter);

	if (u4Status & BIT(SW_INT_WHOLE_RESET))
		handle_whole_chip_reset(prAdapter);
#endif

	if (prBusInfo->hwControlVote)
		prBusInfo->hwControlVote(prAdapter,
					 TRUE,
					 PCIE_VOTE_USER_LOG_RESET);

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

uint32_t asicConnac3xGetFwVer(struct ADAPTER *prAdapter)
{
	uint32_t u4SwVer = 0;

	u4SwVer = nicGetChipSwVer() + 1;

	return u4SwVer;
}

int connsys_power_on(void)
{
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
	connv3_pwr_off(CONNV3_DRV_TYPE_WIFI);
#endif
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

	DBGLOG(INIT, INFO, "wlan_efuse_on.\n");

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

	DBGLOG(INIT, INFO, "__wlan_pwr_on_notify.\n");

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

	INIT_WORK(&pwr_on_notify_work, __wlan_pwr_on_notify);

#if (CFG_SUPPORT_PRE_ON_PHY_ACTION == 1)
	cb.pre_cal_cb.pre_on_cb = wlan_pre_pwr_on;
	cb.pre_cal_cb.efuse_on_cb = wlan_efuse_on;
	cb.pre_cal_cb.pwr_on_cb = wlan_precal_pwron_v2;
	cb.pre_cal_cb.do_cal_cb = wlan_precal_docal_v2;
	cb.pre_cal_cb.pre_cal_error = wlan_precal_err;
#endif

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	cb.cr_cb.priv_data = prGlueInfo;
	cb.cr_cb.read = wf_reg_read_wrapper;
	cb.cr_cb.write = wf_reg_write_wrapper;
	cb.cr_cb.write_mask = wf_reg_write_mask_wrapper;

	cb.rst_cb.pre_whole_chip_rst = wlan_pre_whole_chip_rst_v3;
	cb.rst_cb.post_whole_chip_rst = wlan_post_whole_chip_rst_v3;

	cb.hif_dump_cb.hif_dump_start = wf_reg_start_wrapper;
	cb.hif_dump_cb.hif_dump_end = wf_reg_end_wrapper;

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
	conninfra_wf_cb.rst_cb.pre_whole_chip_rst =
		wlan_pre_whole_chip_rst_v2;
	conninfra_wf_cb.rst_cb.post_whole_chip_rst =
		wlan_post_whole_chip_rst_v2;

	ret = conninfra_sub_drv_ops_register(CONNDRV_TYPE_MAWD,
		&conninfra_wf_cb);
	if (ret)
		DBGLOG(INIT, ERROR,
			"conninfra_sub_drv_ops_register failed, ret=%d\n",
			ret);
#endif
}
#endif

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
static void wlan_devapc_debug_dump(void)
{
	GL_DEFAULT_RESET_TRIGGER(NULL, RST_DEVAPC);
}

static struct devapc_vio_callbacks wlan_devapc_vio_handle = {
	.id = INFRA_SUBSYS_PCIE,
	.debug_dump = wlan_devapc_debug_dump,
};
#endif

void unregister_plat_connsys_cbs(void)
{
#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	power_throttling_deinit();
#endif

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

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	power_throttling_init();
#endif

#if IS_ENABLED(CONFIG_MTK_DEVAPC)
	if (!g_fgIsRegDevapcCb) {
		register_devapc_vio_callback(&wlan_devapc_vio_handle);
		g_fgIsRegDevapcCb = TRUE;
	}
#endif
}
#endif

#endif /* CFG_SUPPORT_CONNAC3X == 1 */
