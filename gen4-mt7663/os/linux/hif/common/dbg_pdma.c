/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/******************************************************************************
 *[File]             dbg_pdma.c
 *[Version]          v1.0
 *[Revision Date]    2015-09-08
 *[Author]
 *[Description]
 *    The program provides PDMA HIF APIs
 *[Copyright]
 *    Copyright (C) 2015 MediaTek Incorporation. All Rights Reserved.
 ******************************************************************************/

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

#include "pse.h"
#include "wf_ple.h"
#include "host_csr.h"
#include "dma_sch.h"

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
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static void halCheckHifState(struct ADAPTER *prAdapter);
static void halDumpHifDebugLog(struct ADAPTER *prAdapter);
static bool halIsTxHang(struct ADAPTER *prAdapter);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

void halPrintHifDbgInfo(IN struct ADAPTER *prAdapter)
{
	halCheckHifState(prAdapter);
	halDumpHifDebugLog(prAdapter);
}

static void halCheckHifState(struct ADAPTER *prAdapter)
{
	if (prAdapter->u4HifChkFlag & HIF_CHK_TX_HANG) {
		if (halIsTxHang(prAdapter)) {
			DBGLOG(HAL, ERROR,
			       "Tx timeout, set hif debug info flag\n");
			prAdapter->u4HifDbgFlag |= DEG_HIF_ALL;
		}
	}

	if (prAdapter->u4HifChkFlag & HIF_DRV_SER)
		halSetDrvSer(prAdapter);

	prAdapter->u4HifChkFlag = 0;
}

static void halDumpHifDebugLog(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	uint32_t u4Value = 0;
	bool fgIsClkEn = false;

	ASSERT(prAdapter);
	prGlueInfo = prAdapter->prGlueInfo;
	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;

	/* Only dump all hif log once */
	if (prAdapter->u4HifDbgFlag & DEG_HIF_ALL) {
		if (!prAdapter->fgEnHifDbgInfo) {
			prAdapter->u4HifDbgFlag = 0;
			return;
		}
		prAdapter->fgEnHifDbgInfo = false;
	}

	/* Avoid register checking */
	prHifInfo->fgIsDumpLog = true;

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_HOST_CSR))
		fgIsClkEn = halShowHostCsrInfo(prAdapter);
	else {
		HAL_MCR_WR(prAdapter, HOST_CSR_DRIVER_OWN_INFO, 0x00030000);
		kalUdelay(1);
		HAL_MCR_RD(prAdapter, HOST_CSR_DRIVER_OWN_INFO, &u4Value);

		/* check clock is enabled */
		fgIsClkEn = ((u4Value & BIT(17)) != 0) &&
			((u4Value & BIT(16)) != 0);
	}

	if (!fgIsClkEn)
		return;

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_PDMA))
		halShowPdmaInfo(prAdapter);

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_DMASCH))
		halShowDmaschInfo(prAdapter);

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_MAC))
		haldumpMacInfo(prAdapter);

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_PHY))
		haldumpPhyInfo(prAdapter);

	prHifInfo->fgIsDumpLog = false;
	prAdapter->u4HifDbgFlag = 0;
}

static void halDumpTxRing(IN struct GLUE_INFO *prGlueInfo,
			  IN uint16_t u2Port, IN uint32_t u4Idx)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct RTMP_TX_RING *prTxRing;
	struct TXD_STRUCT *pTxD;

	if (u2Port >= NUM_OF_TX_RING || u4Idx >= TX_RING_SIZE) {
		DBGLOG(HAL, INFO, "Dump fail u2Port[%u] u4Idx[%u]\n",
		       u2Port, u4Idx);
		return;
	}

	prTxRing = &prHifInfo->TxRing[u2Port];

	pTxD = (struct TXD_STRUCT *) prTxRing->Cell[u4Idx].AllocVa;

	log_dbg(SW4, INFO, "TX Ring[%u] Idx[%04u] SDP0[0x%08x] SDL0[%u] LS[%u] B[%u] DDONE[%u] SDP0_EXT[%u]\n",
		u2Port, u4Idx, pTxD->SDPtr0, pTxD->SDLen0, pTxD->LastSec0,
		pTxD->Burst, pTxD->DMADONE, pTxD->SDPtr0Ext);
}

uint32_t halDumpHifStatus(IN struct ADAPTER *prAdapter,
	IN uint8_t *pucBuf, IN uint32_t u4Max)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Idx, u4DmaIdx, u4CpuIdx, u4MaxCnt;
	uint32_t u4Len = 0;
	struct RTMP_TX_RING *prTxRing;
	struct RTMP_RX_RING *prRxRing;

	LOGBUF(pucBuf, u4Max, u4Len, "\n------<Dump HIF Status>------\n");

	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
		prTxRing = &prHifInfo->TxRing[u4Idx];
		kalDevRegRead(prGlueInfo, prTxRing->hw_cnt_addr, &u4MaxCnt);
		kalDevRegRead(prGlueInfo, prTxRing->hw_cidx_addr, &u4CpuIdx);
		kalDevRegRead(prGlueInfo, prTxRing->hw_didx_addr, &u4DmaIdx);

		LOGBUF(pucBuf, u4Max, u4Len,
			"TX[%u] SZ[%04u] CPU[%04u/%04u] DMA[%04u/%04u] SW_UD[%04u] Used[%u]\n",
			u4Idx, u4MaxCnt, prTxRing->TxCpuIdx,
			u4CpuIdx, prTxRing->TxDmaIdx,
			u4DmaIdx, prTxRing->TxSwUsedIdx, prTxRing->u4UsedCnt);

		if (u4Idx == TX_RING_DATA0_IDX_0) {
			halDumpTxRing(prGlueInfo, u4Idx, prTxRing->TxCpuIdx);
			halDumpTxRing(prGlueInfo, u4Idx, u4CpuIdx);
			halDumpTxRing(prGlueInfo, u4Idx, u4DmaIdx);
			halDumpTxRing(prGlueInfo, u4Idx, prTxRing->TxSwUsedIdx);
		}
	}

	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		prRxRing = &prHifInfo->RxRing[u4Idx];

		kalDevRegRead(prGlueInfo, prRxRing->hw_cnt_addr, &u4MaxCnt);
		kalDevRegRead(prGlueInfo, prRxRing->hw_cidx_addr, &u4CpuIdx);
		kalDevRegRead(prGlueInfo, prRxRing->hw_didx_addr, &u4DmaIdx);

		LOGBUF(pucBuf, u4Max, u4Len,
		       "RX[%u] SZ[%04u] CPU[%04u/%04u] DMA[%04u/%04u]\n",
		       u4Idx, u4MaxCnt, prRxRing->RxCpuIdx, u4CpuIdx,
		       prRxRing->RxDmaIdx, u4DmaIdx);
	}

	LOGBUF(pucBuf, u4Max, u4Len, "MSDU Tok: Free[%u] Used[%u]\n",
		halGetMsduTokenFreeCnt(prGlueInfo->prAdapter),
		prGlueInfo->rHifInfo.rTokenInfo.i4UsedCnt);
	LOGBUF(pucBuf, u4Max, u4Len, "Pending QLen Normal[%u] Sec[%u]\n",
		prGlueInfo->i4TxPendingFrameNum,
		prGlueInfo->i4TxPendingSecurityFrameNum);

	LOGBUF(pucBuf, u4Max, u4Len, "---------------------------------\n\n");

	return u4Len;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Compare two struct timeval
 *
 * @param prTs1          a pointer to timeval
 * @param prTs2          a pointer to timeval
 *
 *
 * @retval 0             two time value is equal
 * @retval 1             prTs1 value > prTs2 value
 * @retval -1            prTs1 value < prTs2 value
 */
/*----------------------------------------------------------------------------*/
int halTimeCompare(struct timeval *prTs1, struct timeval *prTs2)
{
	if (prTs1->tv_sec > prTs2->tv_sec)
		return 1;
	else if (prTs1->tv_sec < prTs2->tv_sec)
		return -1;
	/* sec part is equal */
	else if (prTs1->tv_usec > prTs2->tv_usec)
		return 1;
	else if (prTs1->tv_usec < prTs2->tv_usec)
		return -1;
	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Checking tx hang
 *
 * @param prAdapter      a pointer to adapter private data structure.
 *
 * @retval true          tx is hang because msdu report too long
 */
/*----------------------------------------------------------------------------*/
static bool halIsTxHang(struct ADAPTER *prAdapter)
{
	struct MSDU_TOKEN_INFO *prTokenInfo;
	struct MSDU_TOKEN_ENTRY *prToken;
	struct timeval rNowTs, rTime, rLongest, rTimeout;
	uint32_t u4Idx = 0, u4TokenId = 0;
	bool fgIsTimeout = false;

	ASSERT(prAdapter);
	ASSERT(prAdapter->prGlueInfo);

	prTokenInfo = &prAdapter->prGlueInfo->rHifInfo.rTokenInfo;

	rTimeout.tv_sec = HIF_MSDU_REPORT_DUMP_TIMEOUT;
	rTimeout.tv_usec = 0;
	rLongest.tv_sec = 0;
	rLongest.tv_usec = 0;
	do_gettimeofday(&rNowTs);

	for (u4Idx = 0; u4Idx < HIF_TX_MSDU_TOKEN_NUM; u4Idx++) {
		prToken = &prTokenInfo->arToken[u4Idx];
		if (!prToken->fgInUsed)
			continue;

		/* Ignore now time < token time */
		if (halTimeCompare(&rNowTs, &prToken->rTs) < 0)
			continue;

		rTime.tv_sec = rNowTs.tv_sec - prToken->rTs.tv_sec;
		rTime.tv_usec = rNowTs.tv_usec;
		if (prToken->rTs.tv_usec > rNowTs.tv_usec) {
			rTime.tv_sec -= 1;
			rTime.tv_usec += SEC_TO_USEC(1);
		}
		rTime.tv_usec -= prToken->rTs.tv_usec;

		if (halTimeCompare(&rTime, &rTimeout) >= 0)
			fgIsTimeout = true;

		/* rTime > rLongest */
		if (halTimeCompare(&rTime, &rLongest) > 0) {
			rLongest.tv_sec = rTime.tv_sec;
			rLongest.tv_usec = rTime.tv_usec;
			u4TokenId = u4Idx;
		}
	}

	if (fgIsTimeout) {
		DBGLOG(HAL, INFO, "TokenId[%u] timeout[sec:%u, usec:%u]\n",
		       u4TokenId, rLongest.tv_sec, rLongest.tv_usec);
		prToken = &prTokenInfo->arToken[u4TokenId];
		if (prToken->prPacket)
			DBGLOG_MEM32(HAL, INFO, prToken->prPacket, 64);
	}

	/* Return token to free stack */
	rTimeout.tv_sec = HIF_MSDU_REPORT_RETURN_TIMEOUT;
	rTimeout.tv_usec = 0;
	if (halTimeCompare(&rLongest, &rTimeout) >= 0)
		halReturnTimeoutMsduToken(prAdapter);

	return fgIsTimeout;
}

void kalDumpTxRing(struct GLUE_INFO *prGlueInfo,
		   struct RTMP_TX_RING *prTxRing,
		   uint32_t u4Num, bool fgDumpContent)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_DMACB *pTxCell;
	struct TXD_STRUCT *pTxD;
	uint32_t u4DumpLen = 64;

	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;

	if (u4Num >= TX_RING_SIZE)
		return;

	pTxCell = &prTxRing->Cell[u4Num];
	pTxD = (struct TXD_STRUCT *) pTxCell->AllocVa;

	if (!pTxD)
		return;

	DBGLOG(HAL, INFO, "Tx Dese Num[%u]\n", u4Num);
	DBGLOG_MEM32(HAL, INFO, pTxD, sizeof(struct TXD_STRUCT));

	if (!fgDumpContent)
		return;

	DBGLOG(HAL, INFO, "Tx Contents\n");
	if (prMemOps->dumpTx)
		prMemOps->dumpTx(prHifInfo, prTxRing, u4Num, u4DumpLen);
	DBGLOG(HAL, INFO, "\n\n");
}

void kalDumpRxRing(struct GLUE_INFO *prGlueInfo,
		   struct RTMP_RX_RING *prRxRing,
		   uint32_t u4Num, bool fgDumpContent)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_DMACB *pRxCell;
	struct RXD_STRUCT *pRxD;
	uint32_t u4DumpLen = 64;

	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;

	if (u4Num >= prRxRing->u4RingSize)
		return;

	pRxCell = &prRxRing->Cell[u4Num];
	pRxD = (struct RXD_STRUCT *) pRxCell->AllocVa;

	if (!pRxD)
		return;

	DBGLOG(HAL, INFO, "Rx Dese Num[%u]\n", u4Num);
	DBGLOG_MEM32(HAL, INFO, pRxD, sizeof(struct RXD_STRUCT));

	if (!fgDumpContent)
		return;

	if (u4DumpLen > pRxD->SDLen0)
		u4DumpLen = pRxD->SDLen0;

	DBGLOG(HAL, INFO, "Rx Contents\n");
	if (prMemOps->dumpRx)
		prMemOps->dumpRx(prHifInfo, prRxRing, u4Num, u4DumpLen);
	DBGLOG(HAL, INFO, "\n\n");
}

void halShowPdmaInfo(IN struct ADAPTER *prAdapter)
{
	uint32_t i = 0, u4Value = 0;
	uint32_t Base[6], Base_Ext[6], Cnt[6], Cidx[6], Didx[6];
	uint32_t offset, offset_ext, SwIdx;
	char buf[100] = {0};
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct BUS_INFO *prBus_info;
	struct RTMP_TX_RING *prTxRing;
	struct RTMP_RX_RING *prRxRing;

	/* PDMA HOST_INT */
	HAL_MCR_RD(prAdapter, WPDMA_INT_STA, &u4Value);
	DBGLOG(HAL, INFO, "WPDMA HOST_INT:0x%08x = 0x%08x\n",
		WPDMA_INT_STA, u4Value);

	/* PDMA GLOBAL_CFG  */
	HAL_MCR_RD(prAdapter, WPDMA_GLO_CFG, &u4Value);
	DBGLOG(HAL, INFO, "WPDMA GLOBAL_CFG:0x%08x = 0x%08x\n",
		WPDMA_GLO_CFG, u4Value);

	HAL_MCR_RD(prAdapter, CONN_HIF_RST, &u4Value);
	DBGLOG(HAL, INFO, "WPDMA CONN_HIF_RST:0x%08x = 0x%08x\n",
		CONN_HIF_RST, u4Value);

	HAL_MCR_RD(prAdapter, MCU2HOST_SW_INT_STA, &u4Value);
	DBGLOG(HAL, INFO, "WPDMA MCU2HOST_SW_INT_STA:0x%08x = 0x%08x\n",
		MCU2HOST_SW_INT_STA, u4Value);

	/* PDMA Tx/Rx Ring  Info */
	prBus_info = prAdapter->chip_info->bus_info;
	DBGLOG(HAL, INFO, "Tx Ring configuration\n");
	DBGLOG(HAL, INFO, "%10s%12s%18s%10s%10s%10s\n",
		"Tx Ring", "Reg", "Base", "Cnt", "CIDX", "DIDX");

	for (i = 0; i < 3; i++) {
		if (i == 0) {
			offset = prBus_info->tx_ring_data_idx * MT_RINGREG_DIFF;
			offset_ext = prBus_info->tx_ring_data_idx *
					MT_RINGREG_EXT_DIFF;
		} else if (i == 1) {
			offset = prBus_info->tx_ring_fwdl_idx * MT_RINGREG_DIFF;
			offset_ext = prBus_info->tx_ring_fwdl_idx *
					MT_RINGREG_EXT_DIFF;
		} else {
			offset = prBus_info->tx_ring_cmd_idx * MT_RINGREG_DIFF;
			offset_ext = prBus_info->tx_ring_cmd_idx *
					MT_RINGREG_EXT_DIFF;
		}

		HAL_MCR_RD(prAdapter, WPDMA_TX_RING0_CTRL0 + offset, &Base[i]);
		HAL_MCR_RD(prAdapter, WPDMA_TX_RING0_BASE_PTR_EXT + offset_ext,
				&Base_Ext[i]);
		HAL_MCR_RD(prAdapter, WPDMA_TX_RING0_CTRL1 + offset, &Cnt[i]);
		HAL_MCR_RD(prAdapter, WPDMA_TX_RING0_CTRL2 + offset, &Cidx[i]);
		HAL_MCR_RD(prAdapter, WPDMA_TX_RING0_CTRL3 + offset, &Didx[i]);

		kalSprintf(buf, "%10d  0x%08x  0x%016llx%10d%10d%10d",
			offset/MT_RINGREG_DIFF, WPDMA_TX_RING0_CTRL0 + offset,
			(Base[i] + ((uint64_t) Base_Ext[i] << 32)),
			Cnt[i], Cidx[i], Didx[i]);
		DBGLOG(HAL, INFO, "%s\n", buf);
	}

	DBGLOG(HAL, INFO, "Rx Ring configuration\n");
	DBGLOG(HAL, INFO, "%10s%12s%18s%10s%10s%10s\n",
		"Rx Ring", "Reg", "Base", "Cnt", "CIDX", "DIDX");
	HAL_MCR_RD(prAdapter, WPDMA_RX_RING0_CTRL0, &Base[3]);
	HAL_MCR_RD(prAdapter, WPDMA_RX_RING0_BASE_PTR_EXT, &Base_Ext[3]);
	HAL_MCR_RD(prAdapter, WPDMA_RX_RING0_CTRL1, &Cnt[3]);
	HAL_MCR_RD(prAdapter, WPDMA_RX_RING0_CTRL2, &Cidx[3]);
	HAL_MCR_RD(prAdapter, WPDMA_RX_RING0_CTRL3, &Didx[3]);
	kalSprintf(buf, "%10d  0x%08x  0x%016llx%10d%10d%10d",
		0, WPDMA_RX_RING0_CTRL0,
		(Base[3] + ((uint64_t)Base_Ext[3] << 32)),
		Cnt[3], Cidx[3], Didx[3]);
	DBGLOG(HAL, INFO, "%s\n", buf);

	HAL_MCR_RD(prAdapter, WPDMA_RX_RING0_CTRL0 + MT_RINGREG_DIFF, &Base[4]);
	HAL_MCR_RD(prAdapter, WPDMA_RX_RING0_BASE_PTR_EXT + MT_RINGREG_EXT_DIFF,
			&Base_Ext[4]);
	HAL_MCR_RD(prAdapter, WPDMA_RX_RING0_CTRL1 + MT_RINGREG_DIFF, &Cnt[4]);
	HAL_MCR_RD(prAdapter, WPDMA_RX_RING0_CTRL2 + MT_RINGREG_DIFF, &Cidx[4]);
	HAL_MCR_RD(prAdapter, WPDMA_RX_RING0_CTRL3 + MT_RINGREG_DIFF, &Didx[4]);
	kalSprintf(buf, "%10d  0x%08x  0x%016llx%10d%10d%10d",
		1, WPDMA_RX_RING0_CTRL0 + MT_RINGREG_DIFF,
		(Base[4] + ((uint64_t)Base_Ext[4] << 32)),
		Cnt[4], Cidx[4], Didx[4]);
	DBGLOG(HAL, INFO, "%s\n", buf);

	/* PDMA Tx/Rx descriptor & packet content */
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	for (i = 0; i < 3; i++) {
		DBGLOG(HAL, INFO, "Dump PDMA Tx Ring[%u]\n", i);
		prTxRing = &prHifInfo->TxRing[i];
		SwIdx = Didx[i];
		kalDumpTxRing(prAdapter->prGlueInfo, prTxRing,
			      SwIdx, true);
		SwIdx = Didx[i] == 0 ? Cnt[i] - 1 : Didx[i] - 1;
		kalDumpTxRing(prAdapter->prGlueInfo, prTxRing,
			      SwIdx, true);
	}

	for (i = 0; i < 2; i++) {
		DBGLOG(HAL, INFO, "Dump PDMA Rx Ring[%u]\n", i);
		prRxRing = &prHifInfo->RxRing[i];
		SwIdx = Didx[i+3];
		kalDumpRxRing(prAdapter->prGlueInfo, prRxRing,
			      SwIdx, true);
		SwIdx = Didx[i+3] == 0 ? Cnt[i+3] - 1 : Didx[i+3] - 1;
		kalDumpRxRing(prAdapter->prGlueInfo, prRxRing,
			      SwIdx, true);
	}

	/* PDMA Busy Status */
	HAL_MCR_RD(prAdapter, PDMA_DEBUG_BUSY_STATUS, &u4Value);
	DBGLOG(HAL, INFO, "PDMA busy status:0x%08x = 0x%08x\n",
		PDMA_DEBUG_STATUS, u4Value);
	HAL_MCR_RD(prAdapter, PDMA_DEBUG_HIF_BUSY_STATUS, &u4Value);
	DBGLOG(HAL, INFO, "CONN_HIF busy status:0x%08x = 0x%08x\n\n",
		PDMA_DEBUG_HIF_BUSY_STATUS, u4Value);

	/* PDMA Debug Flag Info */
	DBGLOG(HAL, INFO, "PDMA core dbg");
	for (i = 0; i < 24; i++) {
		u4Value = 256 + i;
		HAL_MCR_WR(prAdapter, PDMA_DEBUG_EN, u4Value);
		HAL_MCR_RD(prAdapter, PDMA_DEBUG_STATUS, &u4Value);
		DBGLOG(HAL, INFO, "Set:0x%02x, result = 0x%08x\n", i, u4Value);
		mdelay(1);
	}

	/* AXI Debug Flag */
	HAL_MCR_WR(prAdapter, AXI_DEBUG_DEBUG_EN, PDMA_AXI_DEBUG_FLAG);
	HAL_MCR_RD(prAdapter, CONN_HIF_DEBUG_STATUS, &u4Value);
	DBGLOG(HAL, INFO, "Set:0x%04x, pdma axi dbg:0x%08x",
	       PDMA_AXI_DEBUG_FLAG, u4Value);

	HAL_MCR_WR(prAdapter, AXI_DEBUG_DEBUG_EN, GALS_AXI_DEBUG_FLAG);
	HAL_MCR_RD(prAdapter, CONN_HIF_DEBUG_STATUS, &u4Value);
	DBGLOG(HAL, INFO, "Set:0x%04x, gals axi dbg:0x%08x",
	       GALS_AXI_DEBUG_FLAG, u4Value);

	HAL_MCR_WR(prAdapter, AXI_DEBUG_DEBUG_EN, MCU_AXI_DEBUG_FLAG);
	HAL_MCR_RD(prAdapter, CONN_HIF_DEBUG_STATUS, &u4Value);
	DBGLOG(HAL, INFO, "Set:0x%04x, mcu axi dbg:0x%08x",
	       MCU_AXI_DEBUG_FLAG, u4Value);

	/* Rbus Bridge Debug Flag */
	DBGLOG(HAL, INFO, "rbus dbg");
	HAL_MCR_WR(prAdapter, PDMA_DEBUG_EN, RBUS_DEBUG_FLAG);
	for (i = 0; i < 9; i++) {
		u4Value = i << 16;
		HAL_MCR_WR(prAdapter, AXI_DEBUG_DEBUG_EN, u4Value);
		HAL_MCR_RD(prAdapter, PDMA_DEBUG_STATUS, &u4Value);
		DBGLOG(HAL, INFO, "Set[19:16]:0x%02x, result = 0x%08x\n",
		       i, u4Value);
	}
}


bool halShowHostCsrInfo(IN struct ADAPTER *prAdapter)
{
	uint32_t i = 0, u4Value = 0;
	bool fgIsDriverOwn = false;
	bool fgEnClock = false;

	DBGLOG(HAL, INFO, "Host CSR Configuration Info:\n\n");

	HAL_MCR_RD(prAdapter, HOST_CSR_BASE, &u4Value);
	DBGLOG(HAL, INFO, "Get 0x87654321: 0x%08x = 0x%08x\n",
		HOST_CSR_BASE, u4Value);

	HAL_MCR_RD(prAdapter, HOST_CSR_DRIVER_OWN_INFO, &u4Value);
	DBGLOG(HAL, INFO, "Driver own info: 0x%08x = 0x%08x\n",
		HOST_CSR_BASE, u4Value);
	fgIsDriverOwn = (u4Value & PCIE_LPCR_HOST_SET_OWN) == 0;

	for (i = 0; i < 5; i++) {
		HAL_MCR_RD(prAdapter, HOST_CSR_MCU_PORG_COUNT, &u4Value);
		DBGLOG(HAL, INFO,
			"MCU programming Counter info (no sync): 0x%08x = 0x%08x\n",
			HOST_CSR_MCU_PORG_COUNT, u4Value);
	}

	HAL_MCR_RD(prAdapter, HOST_CSR_RGU, &u4Value);
	DBGLOG(HAL, INFO, "RGU Info: 0x%08x = 0x%08x\n", HOST_CSR_RGU, u4Value);

	HAL_MCR_RD(prAdapter, HOST_CSR_HIF_BUSY_CORQ_WFSYS_ON, &u4Value);
	DBGLOG(HAL, INFO, "HIF_BUSY / CIRQ / WFSYS_ON info: 0x%08x = 0x%08x\n",
		HOST_CSR_HIF_BUSY_CORQ_WFSYS_ON, u4Value);

	HAL_MCR_RD(prAdapter, HOST_CSR_PINMUX_MON_FLAG, &u4Value);
	DBGLOG(HAL, INFO, "Pinmux/mon_flag info: 0x%08x = 0x%08x\n",
		HOST_CSR_PINMUX_MON_FLAG, u4Value);

	HAL_MCR_RD(prAdapter, HOST_CSR_MCU_PWR_STAT, &u4Value);
	DBGLOG(HAL, INFO, "Bit[5] mcu_pwr_stat: 0x%08x = 0x%08x\n",
		HOST_CSR_MCU_PWR_STAT, u4Value);

	HAL_MCR_RD(prAdapter, HOST_CSR_FW_OWN_SET, &u4Value);
	DBGLOG(HAL, INFO, "Bit[15] fw_own_stat: 0x%08x = 0x%08x\n",
		HOST_CSR_FW_OWN_SET, u4Value);

	HAL_MCR_RD(prAdapter, HOST_CSR_MCU_SW_MAILBOX_0, &u4Value);
	DBGLOG(HAL, INFO, "WF Mailbox[0]: 0x%08x = 0x%08x\n",
		HOST_CSR_MCU_SW_MAILBOX_0, u4Value);

	HAL_MCR_RD(prAdapter, HOST_CSR_MCU_SW_MAILBOX_1, &u4Value);
	DBGLOG(HAL, INFO, "MCU Mailbox[1]: 0x%08x = 0x%08x\n",
		HOST_CSR_MCU_SW_MAILBOX_1, u4Value);

	HAL_MCR_RD(prAdapter, HOST_CSR_MCU_SW_MAILBOX_2, &u4Value);
	DBGLOG(HAL, INFO, "BT Mailbox[2]: 0x%08x = 0x%08x\n",
		HOST_CSR_MCU_SW_MAILBOX_2, u4Value);

	HAL_MCR_RD(prAdapter, HOST_CSR_MCU_SW_MAILBOX_3, &u4Value);
	DBGLOG(HAL, INFO, "GPS Mailbox[3]: 0x%08x = 0x%08x\n",
		HOST_CSR_MCU_SW_MAILBOX_3, u4Value);

	HAL_MCR_RD(prAdapter, HOST_CSR_CONN_CFG_ON, &u4Value);
	DBGLOG(HAL, INFO, "Conn_cfg_on info: 0x%08x = 0x%08x\n",
		HOST_CSR_CONN_CFG_ON, u4Value);

	HAL_MCR_WR(prAdapter, HOST_CSR_DRIVER_OWN_INFO, 0x00030000);
	kalUdelay(1);
	HAL_MCR_RD(prAdapter, HOST_CSR_DRIVER_OWN_INFO, &u4Value);
	DBGLOG(HAL, INFO, "Bit[17]/[16], Get HCLK info: 0x%08x = 0x%08x\n",
		HOST_CSR_DRIVER_OWN_INFO, u4Value);

	/* check clock is enabled */
	fgEnClock = ((u4Value & BIT(17)) != 0) && ((u4Value & BIT(16)) != 0);

	return fgIsDriverOwn && fgEnClock;
}

void halShowDmaschInfo(IN struct ADAPTER *prAdapter)
{
	uint32_t value;
	uint32_t ple_pkt_max_sz;
	uint32_t pse_pkt_max_sz;
	uint32_t max_quota;
	uint32_t min_quota;
	uint32_t rsv_cnt;
	uint32_t src_cnt;
	uint32_t pse_rsv_cnt = 0;
	uint32_t pse_src_cnt = 0;
	uint32_t odd_group_pktin_cnt = 0;
	uint32_t odd_group_ask_cnt = 0;
	uint32_t pktin_cnt;
	uint32_t ask_cnt;
	uint32_t total_src_cnt = 0;
	uint32_t total_rsv_cnt = 0;
	uint32_t ffa_cnt;
	uint32_t free_pg_cnt;
	uint32_t Group_Mapping_Q[16] = {0};
	uint32_t qmapping_addr = MT_HIF_DMASHDL_Q_MAP0;
	uint32_t status_addr = MT_HIF_DMASHDL_STATUS_RD_GP0;
	uint32_t quota_addr = MT_HIF_DMASHDL_GROUP0_CTRL;
	uint32_t pkt_cnt_addr = MT_HIF_DMASHDLRD_GP_PKT_CNT_0;
	uint32_t mapping_mask = 0xf;
	uint32_t mapping_offset = 0;
	uint32_t mapping_qidx;
	uint32_t groupidx = 0;
	uint8_t idx = 0;
	u_int8_t pktin_int_refill_ena;
	u_int8_t pdma_add_int_refill_ena;
	u_int8_t ple_add_int_refill_ena;
	u_int8_t ple_sub_ena;
	u_int8_t hif_ask_sub_ena;
	u_int8_t ple_txd_gt_max_size_flag_clr;
	uint32_t ple_rpg_hif;
	uint32_t ple_upg_hif;
	uint32_t pse_rpg_hif = 0;
	uint32_t pse_upg_hif = 0;
	uint8_t is_mismatch = FALSE;

	for (mapping_qidx = 0; mapping_qidx < 32; mapping_qidx++) {
		uint32_t mapping_group;

		idx = 0;

		if (mapping_qidx == 0) {
			qmapping_addr = MT_HIF_DMASHDL_Q_MAP0;
			mapping_mask = 0xf;
			mapping_offset = 0;
		} else if ((mapping_qidx % 8) == 0) {
			qmapping_addr += 0x4;
			mapping_mask = 0xf;
			mapping_offset = 0;
		} else {
			mapping_offset += 4;
			mapping_mask = 0xf << mapping_offset;
		}

		HAL_MCR_RD(prAdapter, qmapping_addr, &value);
		mapping_group = (value & mapping_mask) >> mapping_offset;
		Group_Mapping_Q[mapping_group] |= 1 << mapping_qidx;
	}

	DBGLOG(HAL, INFO, "Dma scheduler info:\n");
	HAL_MCR_RD(prAdapter, MT_HIF_DMASHDL_CTRL_SIGNAL, &value);
	pktin_int_refill_ena =
		(value & DMASHDL_PKTIN_INT_REFILL_ENA) ? TRUE : FALSE;
	pdma_add_int_refill_ena =
		(value & DMASHDL_PDMA_ADD_INT_REFILL_ENA) ? TRUE : FALSE;
	ple_add_int_refill_ena =
		(value & DMASHDL_PLE_ADD_INT_REFILL_ENA) ? TRUE : FALSE;
	ple_sub_ena = (value & DMASHDL_PLE_SUB_ENA) ? TRUE : FALSE;
	hif_ask_sub_ena = (value & DMASHDL_HIF_ASK_SUB_ENA) ? TRUE : FALSE;
	ple_txd_gt_max_size_flag_clr =
		(value & DMASHDL_PLE_TXD_GT_MAX_SIZE_FLAG_CLR) ? TRUE : FALSE;
	DBGLOG(HAL, INFO, "DMASHDL Ctrl Signal(0x5000A018): 0x%08x\n", value);
	DBGLOG(HAL, INFO, "\tple_txd_gt_max_size_flag_clr(BIT0) = %d\n",
		ple_txd_gt_max_size_flag_clr);
	DBGLOG(HAL, INFO, "\thif_ask_sub_ena(BIT16) = %d\n", hif_ask_sub_ena);
	DBGLOG(HAL, INFO, "\tple_sub_ena(BIT17) = %d\n", ple_sub_ena);
	DBGLOG(HAL, INFO, "\tple_add_int_refill_ena(BIT29) = %d\n",
		ple_add_int_refill_ena);
	DBGLOG(HAL, INFO, "\tpdma_add_int_refill_ena(BIT30) = %d\n",
		pdma_add_int_refill_ena);
	DBGLOG(HAL, INFO, "\tpktin_int_refill(BIT31)_ena = %d\n",
		pktin_int_refill_ena);
	HAL_MCR_RD(prAdapter, MT_HIF_DMASHDL_PKT_MAX_SIZE, &value);
	ple_pkt_max_sz = GET_PLE_PKT_MAX_SIZE_NUM(value);
	pse_pkt_max_sz = GET_PSE_PKT_MAX_SIZE_NUM(value);
	DBGLOG(HAL, INFO,
		"DMASHDL Packet_max_size(0x5000A01c): 0x%08x\n", value);
	DBGLOG(HAL, INFO,
		"PLE/PSE packet max size=0x%03x/0x%03x\n",
		 ple_pkt_max_sz, pse_pkt_max_sz);
	HAL_MCR_RD(prAdapter, MT_HIF_DMASHDL_ERROR_FLAG_CTRL, &value);
	DBGLOG(HAL, INFO, "DMASHDL ERR FLAG CTRL(0x5000A09c): 0x%08x\n", value);
	HAL_MCR_RD(prAdapter, MT_HIF_DMASHDL_STATUS_RD, &value);
	ffa_cnt = (value & DMASHDL_FFA_CNT_MASK) >> DMASHDL_FFA_CNT_OFFSET;
	free_pg_cnt = (value & DMASHDL_FREE_PG_CNT_MASK) >>
		DMASHDL_FREE_PG_CNT_OFFSET;
	DBGLOG(HAL, INFO, "DMASHDL Status_RD(0x5000A100): 0x%08x\n", value);
	DBGLOG(HAL, INFO,
		"free page cnt = 0x%03x, ffa cnt = 0x%03x\n",
		free_pg_cnt, ffa_cnt);
	HAL_MCR_RD(prAdapter, PDMA_DEBUG_REFill, &value);
	DBGLOG(HAL, INFO,
		"DMASHDL ReFill Control(0x5000A010): 0x%08x\n", value);

	for (groupidx = 0; groupidx < 16; groupidx++) {
		DBGLOG(HAL, INFO, "Group %d info:", groupidx);
		HAL_MCR_RD(prAdapter, status_addr, &value);
		rsv_cnt = (value & DMASHDL_RSV_CNT_MASK) >>
			DMASHDL_RSV_CNT_OFFSET;
		src_cnt = (value & DMASHDL_SRC_CNT_MASK) >>
			DMASHDL_SRC_CNT_OFFSET;
		DBGLOG(HAL, INFO,
			"\tDMASHDL Status_RD_GP%d(0x%08x): 0x%08x\n",
			groupidx, status_addr, value);
		HAL_MCR_RD(prAdapter, quota_addr, &value);
		max_quota = (value & DMASHDL_MAX_QUOTA_MASK) >>
			DMASHDL_MAX_QUOTA_OFFSET;
		min_quota = (value & DMASHDL_MIN_QUOTA_MASK) >>
			DMASHDL_MIN_QUOTA_OFFSET;
		DBGLOG(HAL, INFO,
			"\tDMASHDL Group%d control(0x%08x): 0x%08x\n",
			groupidx, quota_addr, value);

		if ((groupidx & 0x1) == 0) {
			HAL_MCR_RD(prAdapter, pkt_cnt_addr, &value);
			DBGLOG(HAL, INFO,
				"\tDMASHDL RD_group_pkt_cnt_%d(0x%08x): 0x%08x\n",
				groupidx / 2, pkt_cnt_addr, value);
			odd_group_pktin_cnt = GET_ODD_GROUP_PKT_IN_CNT(value);
			odd_group_ask_cnt = GET_ODD_GROUP_ASK_CNT(value);
			pktin_cnt = GET_EVEN_GROUP_PKT_IN_CNT(value);
			ask_cnt = GET_EVEN_GROUP_ASK_CNT(value);
		} else {
			pktin_cnt = odd_group_pktin_cnt;
			ask_cnt = odd_group_ask_cnt;
		}

		DBGLOG(HAL, INFO,
			"\trsv_cnt = 0x%03x, src_cnt = 0x%03x\n",
			rsv_cnt, src_cnt);
		DBGLOG(HAL, INFO,
			"\tmax/min quota = 0x%03x/ 0x%03x\n",
			max_quota, min_quota);
		DBGLOG(HAL, INFO,
			"\tpktin_cnt = 0x%02x, ask_cnt = 0x%02x",
			pktin_cnt, ask_cnt);

		if (hif_ask_sub_ena && pktin_cnt != ask_cnt) {
			DBGLOG(HAL, INFO, ", mismatch!");
			is_mismatch = TRUE;
		}

		/* Group15 is for PSE */
		if (groupidx == 15 && Group_Mapping_Q[groupidx] == 0) {
			pse_src_cnt = src_cnt;
			pse_rsv_cnt = rsv_cnt;
			break;
		}

		DBGLOG(HAL, INFO, "\tMapping Qidx: 0x%x",
		       Group_Mapping_Q[groupidx]);

		total_src_cnt += src_cnt;
		total_rsv_cnt += rsv_cnt;
		status_addr = status_addr + 4;
		quota_addr = quota_addr + 4;

		if (groupidx & 0x1)
			pkt_cnt_addr = pkt_cnt_addr + 4;
	}

	DBGLOG(HAL, INFO, "\nCounter Check:\n");
	HAL_MCR_RD(prAdapter, PLE_HIF_PG_INFO, &value);
	ple_rpg_hif = value & 0xfff;
	ple_upg_hif = (value & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"PLE:\n\tThe used/reserved pages of PLE HIF group=0x%03x/0x%03x\n",
		 ple_upg_hif, ple_rpg_hif);
	HAL_MCR_RD(prAdapter, PSE_HIF1_PG_INFO, &value);
	pse_rpg_hif = value & 0xfff;
	pse_upg_hif = (value & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"PSE:\n\tThe used/reserved pages of PSE HIF group=0x%03x/0x%03x\n",
		 pse_upg_hif, pse_rpg_hif);
	DBGLOG(HAL, INFO,
		"DMASHDL:\n\tThe total used pages of group0~14=0x%03x",
		 total_src_cnt);

	if (ple_upg_hif != total_src_cnt) {
		DBGLOG(HAL, INFO, ", mismatch!");
		is_mismatch = TRUE;
	}

	DBGLOG(HAL, INFO, "\n");
	DBGLOG(HAL, INFO,
		"\tThe total reserved pages of group0~14=0x%03x\n",
		total_rsv_cnt);
	DBGLOG(HAL, INFO,
		"\tThe total ffa pages of group0~14=0x%03x\n",
		ffa_cnt);
	DBGLOG(HAL, INFO,
		"\tThe total free pages of group0~14=0x%03x", free_pg_cnt);

	if (free_pg_cnt != total_rsv_cnt + ffa_cnt) {
		DBGLOG(HAL, INFO,
			", mismatch(total_rsv_cnt + ffa_cnt in DMASHDL)");
		is_mismatch = TRUE;
	}

	if (free_pg_cnt != ple_rpg_hif) {
		DBGLOG(HAL, INFO, ", mismatch(reserved pages in PLE)");
		is_mismatch = TRUE;
	}

	DBGLOG(HAL, INFO, "\n");
	DBGLOG(HAL, INFO, "\tThe used pages of group15=0x%03x", pse_src_cnt);

	if (pse_upg_hif != pse_src_cnt) {
		DBGLOG(HAL, INFO, ", mismatch!");
		is_mismatch = TRUE;
	}

	DBGLOG(HAL, INFO, "\n");
	DBGLOG(HAL, INFO,
		"\tThe reserved pages of group15=0x%03x", pse_rsv_cnt);

	if (pse_rpg_hif != pse_rsv_cnt) {
		DBGLOG(HAL, INFO, ", mismatch!");
		is_mismatch = TRUE;
	}

	DBGLOG(HAL, INFO, "\n");

	if (!is_mismatch)
		DBGLOG(HAL, INFO, "DMASHDL: no counter mismatch\n");
}

void haldumpPhyInfo(struct ADAPTER *prAdapter)
{
	uint32_t i = 0, value = 0;

	for (i = 0; i < 20; i++) {
		HAL_MCR_RD(prAdapter, 0x82072644, &value);
		DBGLOG(HAL, INFO, "0x82072644: 0x%08x\n", value);
		kalMdelay(1);
	}
}

void haldumpMacInfo(struct ADAPTER *prAdapter)
{
	uint32_t i = 0, j = 0;
	uint32_t value = 0, index = 0, flag = 0;

	DBGLOG(HAL, INFO, "Print 0x820F3190 5*20 times\n");
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 20; j++) {
			HAL_MCR_RD(prAdapter, 0x820F3190, &value);
			DBGLOG(HAL, INFO, "0x820F3190: 0x%08x\n", value);
		}
		kalMdelay(1);
	}

	for (j = 0; j < 20; j++) {
		HAL_MCR_RD(prAdapter, 0x820FD020, &value);
		DBGLOG(HAL, INFO, "0x820FD020: 0x%08x\n", value);
		kalMdelay(1);
	}

	HAL_MCR_RD(prAdapter, 0x820F4124, &value);
	DBGLOG(HAL, INFO, "Dump CR: 0x820F4124 = %08x\n", value);

	for (j = 0x820F4130; j < 0x820F4148; j += 4) {
		HAL_MCR_RD(prAdapter, j, &value);
		DBGLOG(HAL, INFO, "0x%08x: 0x%08x\n", j, value);
		kalMdelay(1);
	}

	HAL_MCR_RD(prAdapter, 0x820F409C, &value);
	DBGLOG(HAL, INFO, "Dump CR: 0x820F409C = %08x\n", value);

	HAL_MCR_RD(prAdapter, 0x820F409C, &value);
	DBGLOG(HAL, INFO, "Dump CR: 0x820F409C = %08x\n", value);

	HAL_MCR_RD(prAdapter, 0x820F3080, &value);
	DBGLOG(HAL, INFO, "Dump CR: 0x820F3080	= %08x\n", value);

	DBGLOG(HAL, INFO, "Dump ARB CR: 820F3000~820F33FF\n");
	for (index = 0x820f3000; index < 0x820f33ff; index += 4) {
		HAL_MCR_RD(prAdapter, index, &value);
		DBGLOG(HAL, INFO, "0x%08x: 0x%08x\n", index, value);
	}

	DBGLOG(HAL, INFO, "Dump AGG CR: 820F000~820F21FF\n");
	for (index = 0x820f2000; index < 0x820f21ff; index += 4) {
		HAL_MCR_RD(prAdapter, index, &value);
		DBGLOG(HAL, INFO, "0x%08x: 0x%08x\n", index, value);
	}

	DBGLOG(HAL, INFO, "Dump TRB\n");
	HAL_MCR_WR(prAdapter, 0x80025104, 0x02020202);
	flag = 0x01010000;
	for (i = 0; i < 64; i++) {
		HAL_MCR_WR(prAdapter, 0x80025104, flag);
		HAL_MCR_RD(prAdapter, 0x820f0024, &value);
		DBGLOG(HAL, INFO, "write flag = 0x%08x, 0x820f0024: 0x%08x\n",
		       flag, value);
		flag += 0x02020202;
	}

	DBGLOG(HAL, INFO, "Dump ARB\n");
	HAL_MCR_WR(prAdapter, 0x80025104, 0x04040404);
	flag = 0x01010000;
	for (i = 0; i < 64; i++) {
		HAL_MCR_WR(prAdapter, 0x80025104, flag);
		HAL_MCR_RD(prAdapter, 0x820f0024, &value);
		DBGLOG(HAL, INFO, "write flag = 0x%08x, 0x820f0024: 0x%08x\n",
		       flag, value);
		flag += 0x02020202;
	}

	DBGLOG(HAL, INFO, "Dump AGG\n");
	HAL_MCR_WR(prAdapter, 0x80025104, 0x05050505);
	flag = 0x01010000;
	for (i = 0; i < 64; i++) {
		HAL_MCR_WR(prAdapter, 0x80025104, flag);
		HAL_MCR_RD(prAdapter, 0x820f0024, &value);
		DBGLOG(HAL, INFO, "write flag = 0x%08x, 0x820f0024: 0x%08x\n",
		       flag, value);
		flag += 0x02020202;
	}

	DBGLOG(HAL, INFO, "Dump DMA\n");
	HAL_MCR_WR(prAdapter, 0x80025104, 0x06060606);
	flag = 0x01010000;
	for (i = 0; i < 64; i++) {
		HAL_MCR_WR(prAdapter, 0x80025104, flag);
		HAL_MCR_RD(prAdapter, 0x820f0024, &value);
		DBGLOG(HAL, INFO, "write flag = 0x%08x, 0x820f0024: 0x%08x\n",
		       flag, value);
		flag += 0x02020202;
	}

	DBGLOG(HAL, INFO, "Dump TMAC\n");
	HAL_MCR_WR(prAdapter, 0x80025104, 0x07070707);
	flag = 0x01010000;
	for (i = 0; i < 33; i++) {
		HAL_MCR_WR(prAdapter, 0x80025104, flag);
		HAL_MCR_RD(prAdapter, 0x820f0024, &value);
		DBGLOG(HAL, INFO, "write flag = 0x%08x, 0x820f0024: 0x%08x\n",
		       flag, value);
		flag += 0x02020202;
	}

	DBGLOG(HAL, INFO, "Read TXV\n");
	for (i = 0x820F4120; i <= 0x820F412C; i += 4) {
		HAL_MCR_RD(prAdapter, i, &value);
		DBGLOG(HAL, INFO, "0x%08x: 0x%08x\n", i, value);
	}
	for (i = 0x820F4130; i <= 0x820F4148; i += 4) {
		HAL_MCR_RD(prAdapter, i, &value);
		DBGLOG(HAL, INFO, "0x%08x: 0x%08x\n", i, value);
	}
}
