// SPDX-License-Identifier: BSD-2-Clause
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
#include "mt_dmac.h"
#if CFG_SUPPORT_MBRAIN
#include "gl_mbrain.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct wfdma_ring_info {
	char name[20];
	uint32_t ring_idx;
	bool dump_ring_content;

	/* query from register */
	uint32_t base;
	uint32_t base_ext;
	uint32_t cnt;
	uint32_t cidx;
	uint32_t didx;
};

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
static u_int8_t halDumpHifDebugLog(struct ADAPTER *prAdapter);
static bool halIsTxTimeout(struct ADAPTER *prAdapter, uint32_t *u4Token);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

void halPrintHifDbgInfo(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *chip_info = prAdapter->chip_info;
	struct CHIP_DBG_OPS *debug_ops = chip_info->prDebugOps;
	struct GL_HIF_INFO *prHifInfo = NULL;

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	if (!kalIsResetting() &&
	    prHifInfo->rErrRecoveryCtl.eErrRecovState != ERR_RECOV_STOP_IDLE) {
		DBGLOG(HAL, ERROR,
			"SER on-going. ser state: %d reset: %d\n",
			prHifInfo->rErrRecoveryCtl.eErrRecovState,
			kalIsResetting());
		return;
	}

#if (CFG_PCIE_GEN_SWITCH == 1)
	if (prAdapter->ucStopMMIO) {
		DBGLOG(HAL, ERROR, "Skip due to stop mmio.\n");
		return;
	}
#endif

	if (prAdapter->fgIsFwOwn) {
		DBGLOG(HAL, ERROR, "Skip due to FW own.\n");
		return;
	}

	halCheckHifState(prAdapter);
	if (!halDumpHifDebugLog(prAdapter))
		return;

	if (debug_ops && debug_ops->dumpwfsyscpupcr)
		debug_ops->dumpwfsyscpupcr(prAdapter);
}

static bool halIsFwReadyDump(struct ADAPTER *prAdapter)
{
	struct CHIP_DBG_OPS *prDbgOps;
	uint32_t u4Val = 0;

	prDbgOps = prAdapter->chip_info->prDebugOps;

	if (prDbgOps && prDbgOps->getFwDebug)
		u4Val = prDbgOps->getFwDebug(prAdapter);

	u4Val &= DBG_PLE_INT_FW_READY_MASK;

	return (u4Val == DBG_PLE_INT_FW_READY) || (u4Val == 0);
}

static void halTriggerTxHangFwDebugSop(
	struct ADAPTER *prAdapter, uint32_t u4Module,
	uint32_t u4BssIndex, uint32_t u4Reason)
{
	struct CHIP_DBG_OPS *prDbgOps = prAdapter->chip_info->prDebugOps;

	if (prDbgOps && prDbgOps->setFwDebug) {
		/* trigger tx debug sop */
		prDbgOps->setFwDebug(
			prAdapter,
			true,
			0xffff,
			(u4Module << DBG_PLE_INT_MODULE_SHIFT) |
			(u4BssIndex << DBG_PLE_INT_BAND_BSS_SHIFT) |
			(1 << DBG_PLE_INT_VER_SHIFT) |
			(u4Reason)
			);
		DBGLOG(HAL, INFO, "Trigger Fw Debug SOP[%d][%d]\n",
		       u4Module, u4BssIndex);
	}
}

static void halDumpTxHangLog(struct ADAPTER *prAdapter, uint32_t u4TokenId)
{
	struct CHIP_DBG_OPS *prDbgOps;
	struct MSDU_TOKEN_INFO *prTokenInfo;
	struct MSDU_TOKEN_ENTRY *prToken;
	uint32_t u4DebugLevel = 0, u4Val = 0;
	bool MMIORead = FALSE;
	uint8_t ucBssIndex;

	prDbgOps = prAdapter->chip_info->prDebugOps;
	prTokenInfo = &prAdapter->prGlueInfo->rHifInfo.rTokenInfo;
	prToken = &prTokenInfo->arToken[u4TokenId];
	wlanGetDriverDbgLevel(DBG_TX_IDX, &u4DebugLevel);

	/* check fw is dumping log */
	if (prDbgOps && prDbgOps->getFwDebug) {
		MMIORead = TRUE;
		u4Val = prDbgOps->getFwDebug(prAdapter);
	}

	if (u4Val & DBG_PLE_INT_FW_SYNC_MASK) {
		DBGLOG(HAL, ERROR, "Fw is dumping log. Skip to dump mac log\n");
		return;
	}

	/* If no MMIO read, skip read FW status and skip MAC/PHY dump */
	if (MMIORead && halIsFwReadyDump(prAdapter)) {
		if (prDbgOps && prDbgOps->setFwDebug) {
			/* set drv print log sync flag */
			prDbgOps->setFwDebug(
				prAdapter, false, 0, DBG_PLE_INT_DRV_SYNC_MASK);
		}

		if (prDbgOps && prDbgOps->dumpMacInfo)
			prDbgOps->dumpMacInfo(prAdapter);

		if (prDbgOps && prDbgOps->setFwDebug) {
			/* clr drv print log sync flag */
			prDbgOps->setFwDebug(
				prAdapter, false, DBG_PLE_INT_DRV_SYNC_MASK, 0);
		}
	} else {
		if (MMIORead)
			DBGLOG(HAL, ERROR, "Fw not ready to dump log\n");
	}

#if CFG_MTK_MDDP_SUPPORT
	if (prAdapter->u4HifChkFlag & HIF_CHK_MD_TX_TIMEOUT)
		ucBssIndex = prAdapter->ucMddpBssIndex;
	else
#endif
	{
		ucBssIndex = prToken->ucBssIndex;
	}

	DBGLOG(HAL, INFO, "BssIndex: %d\n", ucBssIndex);
	halTriggerTxHangFwDebugSop(
		prAdapter, DBG_PLE_INT_MOD_TX,
		ucBssIndex, DBG_PLE_INT_REASON_MANUAL);
}

bool halCheckFullDump(struct ADAPTER *prAdapter)
{
	bool ret = FALSE;
	uint32_t n = prAdapter->u4HifTxHangDumpBitmap;
	uint32_t idx = prAdapter->u4HifTxHangDumpIdx;
	uint32_t u4HifTxHangDumpNum = prAdapter->u4HifTxHangDumpNum;

	if (prAdapter->rWifiVar.u4TxHangFullDumpMode) {
		ret = TRUE;
		goto end;
	}

#if CFG_MTK_MDDP_SUPPORT
	if (prAdapter->u4HifChkFlag & HIF_CHK_MD_TX_TIMEOUT) {
		ret = TRUE;
		DBGLOG(HAL, INFO, "MD Tx timeout dump\n");
		goto end;
	}
#endif

	if (n & BIT(idx))
		u4HifTxHangDumpNum--;

	if (u4HifTxHangDumpNum < LOG_DUMP_FULL_DUMP_TIMES)
		ret = TRUE;

end:
	DBGLOG(HAL, DEBUG,
		"ret: %d, Bitmap: 0x%08x, dumpMode: %d, idx: %d, count: %d\n",
		ret, prAdapter->u4HifTxHangDumpBitmap,
		prAdapter->rWifiVar.u4TxHangFullDumpMode,
		prAdapter->u4HifTxHangDumpIdx,
		u4HifTxHangDumpNum);

	return ret;
}

static void halCheckHifState(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	struct CHIP_DBG_OPS *prDbgOps;
	uint32_t u4TokenId = 0;
	bool fgHifTxHangFullDump = FALSE;
#if (CFG_SUPPORT_CONNAC2X == 1)
	uint32_t ret = 0;
#endif /* CFG_SUPPORT_CONNAC2X */

	prChipInfo = prAdapter->chip_info;
	prDbgOps = prChipInfo->prDebugOps;

	if (prAdapter->u4HifChkFlag & HIF_CHK_TX_TIMEOUT) {
		if (halIsTxTimeout(prAdapter, &u4TokenId)) {
			DBGLOG(HAL, ERROR,
			       "Tx timeout, set hif debug info flag\n");

			fgHifTxHangFullDump = halCheckFullDump(prAdapter);

			if (fgHifTxHangFullDump) {
				if ((prAdapter->u4HifTxHangDumpNum <
					LOG_DUMP_COUNT_PERIOD) &&
					!(BIT(prAdapter->u4HifTxHangDumpIdx) &
					prAdapter->u4HifTxHangDumpBitmap))
					prAdapter->u4HifTxHangDumpNum++;

				prAdapter->u4HifTxHangDumpBitmap |=
					BIT(prAdapter->u4HifTxHangDumpIdx);

#if (CFG_SUPPORT_CONNAC2X == 1)
				/* for debug purpose */
				if (prChipInfo->checkbusNoAck) {
					ret = prChipInfo->checkbusNoAck(
						(void *) prAdapter, TRUE);
					if (ret != 0)
						goto end_dump;
				}
#endif /* CFG_SUPPORT_CONNAC2X */

				if (prDbgOps && prDbgOps->dumpWfBusSectionA)
					prDbgOps->dumpWfBusSectionA(prAdapter);

				if (prDbgOps && prDbgOps->showPleInfo)
					prDbgOps->showPleInfo(prAdapter, FALSE);

				if (prDbgOps && prDbgOps->showPseInfo)
					prDbgOps->showPseInfo(prAdapter);

				if (prDbgOps && prDbgOps->showPdmaInfo)
					prDbgOps->showPdmaInfo(prAdapter);

				if (prDbgOps && prDbgOps->showDmaschInfo)
					prDbgOps->showDmaschInfo(prAdapter);
			}

			halDumpTxHangLog(prAdapter, u4TokenId);
		}
	}

	if (prAdapter->u4HifChkFlag & HIF_DRV_SER)
		halSetDrvSer(prAdapter);

	if (prAdapter->u4HifChkFlag & HIF_TRIGGER_FW_DUMP)
		halTriggerTxHangFwDebugSop(prAdapter, prAdapter->u4HifDbgMod,
					   prAdapter->u4HifDbgBss,
					   prAdapter->u4HifDbgReason);

#if CFG_MTK_MDDP_SUPPORT
	if (prAdapter->u4HifChkFlag & HIF_CHK_MD_RX_STALL) {
		if (prChipInfo->checkMdRxStall)
			prChipInfo->checkMdRxStall(prAdapter);
	}
#endif

#if (CFG_SUPPORT_CONNAC2X == 1)
end_dump:
#endif /* CFG_SUPPORT_CONNAC2X */

	prAdapter->u4HifChkFlag = 0;
	prAdapter->u4HifDbgMod = 0;
	prAdapter->u4HifDbgBss = 0;
	prAdapter->u4HifDbgReason = 0;

	if (!fgHifTxHangFullDump) {
		if (BIT(prAdapter->u4HifTxHangDumpIdx) &
			prAdapter->u4HifTxHangDumpBitmap)
			prAdapter->u4HifTxHangDumpNum--;
		prAdapter->u4HifTxHangDumpBitmap &=
			~(BIT(prAdapter->u4HifTxHangDumpIdx));
	}

	prAdapter->u4HifTxHangDumpIdx =
		(prAdapter->u4HifTxHangDumpIdx + 1) % LOG_DUMP_COUNT_PERIOD;
}

static u_int8_t halDumpHifDebugLog(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	struct GLUE_INFO *prGlueInfo;
	struct BUS_INFO *prBusInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct CHIP_DBG_OPS *prDbgOps;
	u_int8_t fgIsDrvOwn = FALSE;

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prDbgOps = prChipInfo->prDebugOps;

	if (!prAdapter->u4HifDbgFlag)
		return TRUE;

	/* Avoid register checking */
	prHifInfo->fgIsDumpLog = true;

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_HOST_CSR)) {
		if (prDbgOps && prDbgOps->showCsrInfo) {
			bool fgIsClkEn = prDbgOps->showCsrInfo(prAdapter);

			if (!fgIsClkEn)
				return FALSE;
		}
	}

#if (CFG_SUPPORT_CONNAC2X == 1)
	/* need to check Bus readable */
	if (prChipInfo->checkbusNoAck) {
		uint32_t ret = 0;

		ret = prChipInfo->checkbusNoAck((void *)prAdapter, TRUE);
		if (ret != 0) {
			DBGLOG(HAL, ERROR,
				"return due to checkbusNoAck fail %d\n", ret);
			return FALSE;
		}
	}
#endif

	/* Check Driver own HW CR */
	if (prBusInfo->lowPowerOwnRead)
		prBusInfo->lowPowerOwnRead(prAdapter, &fgIsDrvOwn);
	else {
		DBGLOG(HAL, ERROR, "return due to null API\n");
		return FALSE;
	}

	if (fgIsDrvOwn == 0) {
		DBGLOG(HAL, ERROR, "return, not driver-own[%u]\n",
		       fgIsDrvOwn);
		return FALSE;
	}

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_PLE)) {
		if (prDbgOps && prDbgOps->showPleInfo)
			prDbgOps->showPleInfo(prAdapter, FALSE);
	}

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_PSE)) {
		if (prDbgOps && prDbgOps->showPseInfo)
			prDbgOps->showPseInfo(prAdapter);
	}

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_PDMA)) {
		if (prDbgOps && prDbgOps->showPdmaInfo)
			prDbgOps->showPdmaInfo(prAdapter);
	}

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_DMASCH)) {
		if (prDbgOps && prDbgOps->showDmaschInfo)
			prDbgOps->showDmaschInfo(prAdapter);
	}

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_MAC)) {
		if (prDbgOps && prDbgOps->dumpMacInfo)
			prDbgOps->dumpMacInfo(prAdapter);
	}

#if (CFG_SUPPORT_DEBUG_SOP == 1)
	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_PLATFORM_DBG)) {
		if (prDbgOps && prDbgOps->show_debug_sop_info)
			prDbgOps->show_debug_sop_info(prAdapter,
				SLAVENORESP);
	}
#endif

	prHifInfo->fgIsDumpLog = false;
	prAdapter->u4HifDbgFlag = 0;

	return TRUE;
}

static void halDumpTxRing(struct GLUE_INFO *prGlueInfo,
			  uint16_t u2Port, uint32_t u4Idx)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct RTMP_TX_RING *prTxRing;
	struct TXD_STRUCT *pTxD;

	if (u2Port >= NUM_OF_TX_RING) {
		DBGLOG(HAL, INFO, "Dump fail u2Port[%u]\n",
		       u2Port);
		return;
	}

	prTxRing = &prHifInfo->TxRing[u2Port];
	if (u4Idx >= prTxRing->u4RingSize) {
		DBGLOG(HAL, INFO, "Dump fail u2Port[%u] u4Idx[%u]\n",
		       u2Port, u4Idx);
		return;
	}

	pTxD = (struct TXD_STRUCT *) prTxRing->Cell[u4Idx].AllocVa;

	log_dbg(SW4, INFO, "TX Ring[%u] Idx[%04u] SDP0[0x%08x] SDL0[%u] LS[%u] B[%u] DDONE[%u] SDP0_EXT[%u]\n",
		u2Port, u4Idx, pTxD->SDPtr0, pTxD->SDLen0, pTxD->LastSec0,
		pTxD->Burst, pTxD->DMADONE, pTxD->SDPtr0Ext);
}

uint32_t halDumpHifStatus(struct ADAPTER *prAdapter,
	uint8_t *pucBuf, uint32_t u4Max)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Idx, u4DmaIdx = 0;
	uint32_t u4CpuIdx = 0, u4MaxCnt = 0;
	uint32_t u4Len = 0;
	struct RTMP_TX_RING *prTxRing;
	struct RTMP_RX_RING *prRxRing;

	LOGBUF(pucBuf, u4Max, u4Len, "\n------<Dump HIF Status>------\n");

	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
		prTxRing = &prHifInfo->TxRing[u4Idx];
		HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
			       prTxRing->hw_cnt_addr, &u4MaxCnt);
		HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
			       prTxRing->hw_cidx_addr, &u4CpuIdx);
		HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
			       prTxRing->hw_didx_addr, &u4DmaIdx);

		u4MaxCnt &= MT_RING_CNT_MASK;
		u4CpuIdx &= MT_RING_CIDX_MASK;
		u4DmaIdx &= MT_RING_DIDX_MASK;

		LOGBUF(pucBuf, u4Max, u4Len,
			"TX[%u] SZ[%04u] CPU[%04u/%04u] DMA[%04u/%04u] SW_UD[%04u] Used[%u]\n",
			u4Idx, u4MaxCnt, prTxRing->TxCpuIdx,
			u4CpuIdx, prTxRing->TxDmaIdx,
			u4DmaIdx, prTxRing->TxSwUsedIdx, prTxRing->u4UsedCnt);

		if (u4Idx == TX_RING_DATA0) {
			halDumpTxRing(prGlueInfo, u4Idx, prTxRing->TxCpuIdx);
			halDumpTxRing(prGlueInfo, u4Idx, u4CpuIdx);
			halDumpTxRing(prGlueInfo, u4Idx, u4DmaIdx);
			halDumpTxRing(prGlueInfo, u4Idx, prTxRing->TxSwUsedIdx);
		}

		if (u4Idx == TX_RING_DATA1) {
			halDumpTxRing(prGlueInfo, u4Idx, prTxRing->TxCpuIdx);
			halDumpTxRing(prGlueInfo, u4Idx, u4CpuIdx);
			halDumpTxRing(prGlueInfo, u4Idx, u4DmaIdx);
			halDumpTxRing(prGlueInfo, u4Idx, prTxRing->TxSwUsedIdx);
		}
	}

	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		prRxRing = &prHifInfo->RxRing[u4Idx];

		HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
			       prRxRing->hw_cnt_addr, &u4MaxCnt);
		HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
			       prRxRing->hw_cidx_addr, &u4CpuIdx);
		HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
			       prRxRing->hw_didx_addr, &u4DmaIdx);

		u4MaxCnt &= MT_RING_CNT_MASK;
		u4CpuIdx &= MT_RING_CIDX_MASK;
		u4DmaIdx &= MT_RING_DIDX_MASK;

		LOGBUF(pucBuf, u4Max, u4Len,
		       "RX[%u] SZ[%04u] CPU[%04u/%04u] DMA[%04u/%04u]\n",
		       u4Idx, u4MaxCnt, prRxRing->RxCpuIdx, u4CpuIdx,
		       prRxRing->RxDmaIdx, u4DmaIdx);
	}

	LOGBUF(pucBuf, u4Max, u4Len, "MSDU Tok: Free[%u] Used[%u]\n",
		halGetMsduTokenFreeCnt(prGlueInfo->prAdapter),
		prGlueInfo->rHifInfo.rTokenInfo.u4UsedCnt);
	LOGBUF(pucBuf, u4Max, u4Len, "Pending QLen Normal[%u]\n",
		prGlueInfo->i4TxPendingFrameNum);

	LOGBUF(pucBuf, u4Max, u4Len, "---------------------------------\n\n");

	return u4Len;
}

static void halNotifyTxHangEvent(struct ADAPTER *prAdapter,
				 struct MSDU_TOKEN_HISTORY_INFO *prHistory)
{
	struct TOKEN_HISTORY *prCur, *prNext;
	uint32_t u4Idx, u4CurIdx, u4NextIdx;

	ASSERT(prAdapter);
	ASSERT(prHistory);

	u4CurIdx = prHistory->u4CurIdx;

	for (u4Idx = 0; u4Idx < MSDU_TOKEN_HISTORY_NUM - 1; u4Idx++) {
		u4NextIdx = (u4CurIdx + 1) % MSDU_TOKEN_HISTORY_NUM;
		prCur = &prHistory->au4List[u4CurIdx];
		prNext = &prHistory->au4List[u4NextIdx];

		if (prCur->u4LongestId != prNext->u4LongestId)
			return;

		if (prCur->u4UsedCnt == 0)
			return;

		if (prCur->u4UsedCnt > prNext->u4UsedCnt)
			return;

		u4CurIdx = u4NextIdx;
	}

	kalSendUevent(prAdapter, "abnormaltrx=DIR:TX,Event:Hang");
}

static void halCalcTxTimeoutParams(struct ADAPTER *prAdapter,
	uint32_t u4TokenId)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo;
	uint32_t u4TmpIdleSlotDiff = 0;

	if (IS_FEATURE_DISABLED(prWifiVar->fgWarningTxTimeout))
		return;

	 /* Update SameTokenDuration and Idle slot parameter
	  * Assume the TokenId is same before 1st time TX timeout coming
	  */
	if (prAdapter->u4LastTokenId == HIF_TX_MSDU_TOKEN_NUM ||
		prAdapter->u4LastTokenId == u4TokenId) {
		prAdapter->u4SameTokenCnt++;
	} else {
		prAdapter->u4SameTokenCnt = 0;
	}
	prAdapter->u4LastTokenId = u4TokenId;

	prLinkQualityInfo = &(prAdapter->rLinkQualityInfo);

	/* Idle slot diff is less than max of 32bit uint */
	u4TmpIdleSlotDiff = (uint32_t)prLinkQualityInfo->u8DiffIdleSlotCount;
	if (checkAddOverflow(prAdapter->u4SumIdleSlot, u4TmpIdleSlotDiff)) {
		DBGLOG(HAL, ERROR, "idle slot sum is overflow\n");
		return;
	}

	prAdapter->u4SumIdleSlot += u4TmpIdleSlotDiff;
	prAdapter->u4TxTimeoutCnt++;
}

static void halResetTxTimeoutParams(struct ADAPTER *prAdapter)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (IS_FEATURE_DISABLED(prWifiVar->fgWarningTxTimeout))
		return;

	/* Reset SameTokenDuration and Idle slot parameter */
	prAdapter->u4SumIdleSlot = 0;
	prAdapter->u4SameTokenCnt = 0;
	prAdapter->u4TxTimeoutCnt = 0;
	prAdapter->u4LastTokenId = HIF_TX_MSDU_TOKEN_NUM;
}

static void halWarningTxTimeout(struct ADAPTER *prAdapter,
	uint32_t u4LongestPending, uint8_t ucBssIndex)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct BSS_INFO *prBssInfo;
	uint32_t u4AvgIdleSlot = 0;
	char aee_str[64] = {0};

	if (IS_FEATURE_DISABLED(prWifiVar->fgWarningTxTimeout))
		return;

	if (prAdapter->u4TxTimeoutCnt > 0)
		u4AvgIdleSlot = prAdapter->u4SumIdleSlot /
			prAdapter->u4TxTimeoutCnt;

	/* Ignore low idle slow < thr if setting in wifi.cfg */
	if (IS_FEATURE_ENABLED(prWifiVar->fgIgnoreLowIdleSlot)) {
		if (u4AvgIdleSlot < prWifiVar->u4LowIdleSlotThr)
			return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return;

	kalSnprintf(aee_str, sizeof(aee_str),
		"Tx Timeout BSS_Idx[%d](%s)",
		ucBssIndex,
		bssGetRoleTypeString(prAdapter, prBssInfo));
	/* always show if SameToken > thr */
	if (prAdapter->u4SameTokenCnt > prWifiVar->u4SameTokenThr) {
		/* only trigger SER when enable in wifi.cfg */
		prAdapter->u4HifChkFlag |= HIF_DRV_SER;
		kalSendAeeWarning(aee_str,
			"Bss_Idx[%u] Tx timeout same token > %d , idle slot %d SER!\n",
			ucBssIndex, prWifiVar->u4SameTokenThr,
			u4AvgIdleSlot);
	} else if (u4LongestPending >= prWifiVar->u4TxTimeoutWarningThr) {
		kalSendAeeWarning(aee_str,
			"Bss_Idx[%u] Tx timeout > %ds, Warning, idle slot %d\n",
			ucBssIndex, prWifiVar->u4TxTimeoutWarningThr,
			u4AvgIdleSlot);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Checking tx timeout
 *
 * @param prAdapter      a pointer to adapter private data structure.
 *
 * @retval true          tx is timeout because msdu report too long
 */
/*----------------------------------------------------------------------------*/
static bool halIsTxTimeout(struct ADAPTER *prAdapter, uint32_t *u4Token)
{
	struct MSDU_TOKEN_INFO *prTokenInfo;
	struct MSDU_TOKEN_ENTRY *prToken;
	struct MSDU_TOKEN_HISTORY_INFO *prHistory;
	struct BSS_INFO *prBssInfo;
	uint64_t u8Now, u8Longest, u8Timeout, u8DeltaTime;
	uint32_t u4Idx = 0, u4TokenId = 0;
	u_int8_t fgIsTimeout = FALSE;
	struct WIFI_VAR *prWifiVar;
	uint8_t ucStaIdx = 0;
	struct HIF_STATS *prHifStats;
	enum ENUM_OP_MODE eOPMode = OP_MODE_NUM;
	uint32_t u4TimeoutSerTime;
	uint64_t u8LastMsduRptChangedTime = 0;
	uint32_t u4CurrentMsduRptCnt;

	ASSERT(prAdapter);
	ASSERT(prAdapter->prGlueInfo);

#if CFG_MTK_MDDP_SUPPORT
	if (prAdapter->u4HifChkFlag & HIF_CHK_MD_TX_TIMEOUT) {
		DBGLOG(HAL, INFO, "MD Tx timeout dump\n");
		return TRUE;
	}
#endif

	prTokenInfo = &prAdapter->prGlueInfo->rHifInfo.rTokenInfo;
	prHistory = &prTokenInfo->rHistory;
	prWifiVar = &prAdapter->rWifiVar;
	u4TimeoutSerTime = prWifiVar->u4MsduReportTimeoutSerTime;
	prHifStats = &prAdapter->rHifStats;

	u8Longest = 0;
	u8Now = kalGetBootTime();

	for (u4Idx = 0; u4Idx < HIF_TX_MSDU_TOKEN_NUM; u4Idx++) {
		prToken = &prTokenInfo->arToken[u4Idx];

		if (!prToken->prPacket)
			continue;

		u8Timeout = SEC_TO_USEC(prWifiVar->u4MsduReportTimeout);

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prToken->ucBssIndex);
		if (IS_BSS_APGO(prBssInfo)) {
			if (!prWifiVar->fgApGoTxTimeoutEn)
				continue;
			u8Timeout = SEC_TO_USEC(
				prWifiVar->u4ApGoMsduReportTimeout);
		}

		if (prToken->fgInUsed &&
		    CHECK_FOR_TIMEOUT64(u8Now, prToken->u8Tm, u8Timeout)) {
			fgIsTimeout = TRUE;

			/* u8DeltaTime > u8Longest */
			u8DeltaTime = TIME_ABS_DIFF64(u8Now, prToken->u8Tm);
			if (u8DeltaTime > u8Longest) {
				u8Longest = u8DeltaTime;
				u4TokenId = u4Idx;
			}
		}
	}

	/* Save longest to be compared for pending MSDU */
	prAdapter->u4LongestPending = USEC_TO_SEC(u8Longest);

	if (fgIsTimeout) {
		prToken = &prTokenInfo->arToken[u4TokenId];

		if (wlanGetStaIdxByWlanIdx(prAdapter, prToken->ucWlanIndex,
			&ucStaIdx) == WLAN_STATUS_SUCCESS) {

			/* Save tx timeout StaIdx */
			prAdapter->ucTxTimeoutStaIdx = ucStaIdx;
			/* Set bit to dump in main thread  */
			kalSetTxTimeoutDump(prAdapter->prGlueInfo);

		}
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prToken->ucBssIndex);
		if (prBssInfo)
			eOPMode = prBssInfo->eCurrentOPMode;

		DBGLOG(HAL, INFO,
			"TokenId[%u] Wlan_Idx[%u] Bss_Idx[%u] timeout[%lld.%06lld] OpMode[%u]\n",
			u4TokenId, prToken->ucWlanIndex,
			prToken->ucBssIndex,
			USEC_TO_SEC(u8Longest),
			USEC_REM_TO_SEC(u8Longest), eOPMode);

		if (prToken->prPacket)
			DBGLOG_MEM32(HAL, INFO, prToken->prPacket, 64);

		halCalcTxTimeoutParams(prAdapter, u4TokenId);

		prHistory->au4List[prHistory->u4CurIdx].u4LongestId = u4TokenId;
		prHistory->au4List[prHistory->u4CurIdx].u4UsedCnt =
			prTokenInfo->u4UsedCnt;
		prHistory->u4CurIdx =
			(prHistory->u4CurIdx + 1) % MSDU_TOKEN_HISTORY_NUM;
		halNotifyTxHangEvent(prAdapter, prHistory);

		u4CurrentMsduRptCnt = GLUE_GET_REF_CNT(
				prHifStats->u4DataMsduRptCount);
		if (GLUE_GET_REF_CNT(prHifStats->u4LastDataMsduRptCount) !=
			    u4CurrentMsduRptCnt)
			u8LastMsduRptChangedTime = u8Now;

		GLUE_SET_REF_CNT(u4CurrentMsduRptCnt,
				 prHifStats->u4LastDataMsduRptCount);
	} else {
		kalMemZero(prHistory, sizeof(struct MSDU_TOKEN_HISTORY_INFO));
		halResetTxTimeoutParams(prAdapter);
	}

	halWarningTxTimeout(prAdapter, USEC_TO_SEC(u8Longest),
		prToken->ucBssIndex);
#if CFG_SUPPORT_MBRAIN
	mbrIsTxTimeout(prAdapter, u4TokenId, USEC_TO_SEC(u8Longest));
#endif

	/* Trigger SER */
	if (u4TimeoutSerTime == NIC_MSDU_REPORT_DISABLE_SER_TIME) {
		DBGLOG(HAL, TRACE, "Do not trigger SER");
	} else if (USEC_TO_SEC(u8Longest) >= u4TimeoutSerTime) {
		if (CHECK_FOR_TIMEOUT64(u8Now,
			u8LastMsduRptChangedTime,
			SEC_TO_USEC(u4TimeoutSerTime))) {
			prAdapter->u4HifChkFlag |= HIF_DRV_SER;
			DBGLOG(HAL, ERROR, "Timeout > %ds, trigger SER\n",
				u4TimeoutSerTime);
		} else {
			DBGLOG(HAL, ERROR,
				"MSDU reports are returning, do not trigger SER. lastMsduRpt @ %lld, MsduRptCnt[%u] timeout[sec:%lld]",
				USEC_TO_SEC(u8LastMsduRptChangedTime),
				GLUE_GET_REF_CNT(
					prHifStats->u4LastDataMsduRptCount),
				USEC_TO_SEC(u8Longest));
		}
	}

	*u4Token = u4TokenId;

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

	if (u4Num >= prTxRing->u4RingSize)
		return;

	pTxCell = &prTxRing->Cell[u4Num];
	pTxD = (struct TXD_STRUCT *) pTxCell->AllocVa;

	if (!pTxD)
		return;

	DBGLOG(HAL, INFO, "Tx Dese Num[%u]\n", u4Num);
	DBGLOG_MEM32(HAL, INFO, pTxD, sizeof(struct TXD_STRUCT));

	if (!fgDumpContent)
		return;

	if (prMemOps->dumpTx)
		prMemOps->dumpTx(prHifInfo, prTxRing, u4Num, u4DumpLen);
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

	if (prMemOps->dumpRx)
		prMemOps->dumpRx(prHifInfo, prRxRing, u4Num, u4DumpLen);
}

u_int8_t halIsWfdmaRxCidxChanged(struct ADAPTER *prAdapter, uint32_t u4Idx)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	struct RTMP_RX_RING *prRxRing;

	if (u4Idx >= NUM_OF_RX_RING)
		return FALSE;

	prRxRing = &prHifInfo->RxRing[u4Idx];
	if ((GLUE_GET_REF_CNT(prRxRing->u4TotalCntRec) ==
	     prRxRing->u4TotalCnt) &&
	    (GLUE_GET_REF_CNT(prRxRing->u4CidxRec) == prRxRing->RxCpuIdx))
		return FALSE;

	GLUE_SET_REF_CNT(prRxRing->u4TotalCnt, prRxRing->u4TotalCntRec);
	GLUE_SET_REF_CNT(prRxRing->RxCpuIdx, prRxRing->u4CidxRec);

	return TRUE;
}

#if CFG_MTK_WIFI_WFDMA_WB
void halCheckWfdmaStallForWB(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
#if defined(_HIF_PCIE)
	struct BUS_INFO *prBusInfo;
#endif
	struct GL_HIF_INFO *prHifInfo;
	struct WIFI_VAR *prWifiVar;
	struct RTMP_RX_RING *prRxRing;
	uint32_t i, u4RxCnt = 0;

	prGlueInfo = prAdapter->prGlueInfo;
#if defined(_HIF_PCIE)
	prBusInfo = prAdapter->chip_info->bus_info;
#endif
	prHifInfo = &prGlueInfo->rHifInfo;
	prWifiVar = &prAdapter->rWifiVar;

	/* skip SER */
	if (prHifInfo->rErrRecoveryCtl.eErrRecovState != ERR_RECOV_STOP_IDLE)
		return;

	/* skip rx work is ready */
	if (GLUE_GET_REF_CNT(prGlueInfo->u4RxTaskScheduleCnt) > 0)
		return;

	for (i = 0; i < NUM_OF_RX_RING; i++) {
		prRxRing = &prHifInfo->RxRing[i];
		if (!prRxRing->fgEnEmiDidx)
			continue;

		u4RxCnt = halWpdmaGetRxDmaDoneCnt(prGlueInfo, i);
		if (u4RxCnt > 0 && !halIsWfdmaRxCidxChanged(prAdapter, i)) {
			prRxRing->u4CidxErrCnt++;
			DBGLOG(HAL, WARN,
			       "Ring[%u] RxCnt[%u] rec[%u] cidx[%u] err[%u]\n",
			       i, u4RxCnt, prRxRing->u4CidxRec,
			       prRxRing->RxCpuIdx, prRxRing->u4CidxErrCnt);
		} else {
			prRxRing->u4CidxErrCnt = 0;
		}

#if defined(_HIF_PCIE)
		if (prBusInfo->recoverSerStatus)
			prBusInfo->recoverSerStatus(prAdapter);

		if (prRxRing->u4CidxErrCnt >=
		    prWifiVar->u4WfdmaRxTimeoutRecoveryCnt &&
		    prBusInfo->recoveryMsiStatus)
			prBusInfo->recoveryMsiStatus(prAdapter, TRUE);
#endif

		if (prRxRing->u4CidxErrCnt >= prWifiVar->u4WfdmaRxTimeoutCnt) {
			prHifInfo->fgIsTriggerRxTimeout = TRUE;
			DBGLOG(HAL, ERROR, "CidxErrCnt > WfdmaRxTimeoutCnt\n");
			break;
		}
	}
}
#endif /* CFG_MTK_WIFI_WFDMA_WB */

#if (CFG_MTK_WIFI_FORCE_RECV_RX == 1)
void halCheckWfdmaStallForceRecvRx(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_RX_RING *prRxRing;
	uint32_t i, u4RxCnt = 0;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;

	/* skip SER */
	if (prHifInfo->rErrRecoveryCtl.eErrRecovState != ERR_RECOV_STOP_IDLE)
		return;

	for (i = 0; i < NUM_OF_RX_RING; i++) {
		prRxRing = &prHifInfo->RxRing[i];

		u4RxCnt = halWpdmaGetRxDmaDoneCnt(prGlueInfo, i);

		if (prRxRing->u4RingSize - 1 == u4RxCnt) {
			KAL_SET_BIT(i, prAdapter->ulNoMoreRfb);
			DBGLOG(HAL, WARN,
			       "Ring[%u] RxCnt[%u] cidx[%u]\n",
			       i, u4RxCnt, prRxRing->RxCpuIdx);
			kalSetDrvIntEvent(prGlueInfo);
			break;
		}
	}
}
#endif /* CFG_MTK_WIFI_FORCE_RECV_RX */

void halCheckWfdmaStall(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

#if CFG_MTK_WIFI_WFDMA_WB
	halCheckWfdmaStallForWB(prAdapter);
#elif (CFG_MTK_WIFI_FORCE_RECV_RX == 1)
	halCheckWfdmaStallForceRecvRx(prAdapter);
#endif
	if (prHifInfo->fgIsTriggerRxTimeout) {
		prHifInfo->fgIsTriggerRxTimeout = FALSE;
		GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_WFDMA_RX_TIMEOUT);
	}
}


void halCheckTxTimeout(struct ADAPTER *prAdapter)
{
	struct HIF_STATS *prHifStats;

	prHifStats = &prAdapter->rHifStats;

	if (time_before(jiffies, prHifStats->ulTxTimeoutDetectPeriod))
		return;

	prHifStats->ulTxTimeoutDetectPeriod = jiffies +
		prAdapter->rWifiVar.u4HifDetectTxTimeoutPeriod * HZ / 1000;
	prAdapter->u4HifChkFlag |= HIF_CHK_TX_TIMEOUT;
	kalSetHifDbgEvent(prAdapter->prGlueInfo);
}

void halDetectHifStall(struct ADAPTER *prAdapter)
{
	halCheckWfdmaStall(prAdapter);
	halCheckTxTimeout(prAdapter);
}

void halShowPdmaInfo(struct ADAPTER *prAdapter)
{
#define BUF_SIZE 1024

	uint32_t i = 0, u4Value = 0, pos = 0;
	uint32_t offset, offset_ext, SwIdx, SwIdx1, SwIdx2;
	char *buf;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct BUS_INFO *prBus_info = prAdapter->chip_info->bus_info;
	struct RTMP_TX_RING *prTxRing;
	struct RTMP_RX_RING *prRxRing;
	struct wfdma_ring_info wfmda_tx_group[] = {
		{"AP DATA0", prBus_info->tx_ring0_data_idx, true},
		{"AP DATA1", prBus_info->tx_ring1_data_idx, true},
		{"AP PRIO", prBus_info->tx_prio_data_idx, true},
		{"AP CMD", prBus_info->tx_ring_cmd_idx, true},
		{"FWDL", prBus_info->tx_ring_fwdl_idx, true},
		{"MD DATA0", 8, false},
		{"MD DATA1", 9, false},
		{"MD DATA2", 10, false},
		{"MD CMD", 14, false},
	};
	struct wfdma_ring_info wfmda_rx_group[] = {
		{"AP DATA", 0, true},
		{"AP EVENT", 1, true},
		{"MD DATA", 2, false},
		{"MD EVENT", 3, false},
	};

	buf = (char *) kalMemAlloc(BUF_SIZE, VIR_MEM_TYPE);

	/* PDMA HOST_INT */
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, WPDMA_INT_STA, &u4Value);
	DBGLOG(HAL, INFO, "WPDMA HOST_INT:0x%08x = 0x%08x\n",
		WPDMA_INT_STA, u4Value);

	/* PDMA GLOBAL_CFG  */
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, WPDMA_GLO_CFG, &u4Value);
	DBGLOG(HAL, INFO, "WPDMA GLOBAL_CFG:0x%08x = 0x%08x\n",
		WPDMA_GLO_CFG, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, CONN_HIF_RST, &u4Value);
	DBGLOG(HAL, INFO, "WPDMA CONN_HIF_RST:0x%08x = 0x%08x\n",
		CONN_HIF_RST, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, MCU2HOST_SW_INT_STA, &u4Value);
	DBGLOG(HAL, INFO, "WPDMA MCU2HOST_SW_INT_STA:0x%08x = 0x%08x\n",
		MCU2HOST_SW_INT_STA, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, MD_INT_STA, &u4Value);
	DBGLOG(HAL, INFO, "MD_INT_STA:0x%08x = 0x%08x\n",
	       MD_INT_STA, u4Value);
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, MD_WPDMA_GLO_CFG, &u4Value);
	DBGLOG(HAL, INFO, "MD_WPDMA_GLO_CFG:0x%08x = 0x%08x\n",
	       MD_WPDMA_GLO_CFG, u4Value);
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, MD_INT_ENA, &u4Value);
	DBGLOG(HAL, INFO, "MD_INT_ENA:0x%08x = 0x%08x\n",
	       MD_INT_ENA, u4Value);
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       MD_WPDMA_DLY_INIT_CFG, &u4Value);
	DBGLOG(HAL, INFO, "MD_WPDMA_DLY_INIT_CFG:0x%08x = 0x%08x\n",
	       MD_WPDMA_DLY_INIT_CFG, u4Value);
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, MD_WPDMA_MISC, &u4Value);
	DBGLOG(HAL, INFO, "MD_WPDMA_MISC:0x%08x = 0x%08x\n",
	       MD_WPDMA_MISC, u4Value);

	/* PDMA Tx/Rx Ring  Info */
	DBGLOG(HAL, INFO, "Tx Ring configuration\n");
	DBGLOG(HAL, INFO, "%10s%10s%12s%20s%10s%10s%10s\n",
		"Tx Ring", "Idx", "Reg", "Base", "Cnt", "CIDX", "DIDX");

	if (buf) {
		kalMemZero(buf, BUF_SIZE);
		for (i = 0; i < ARRAY_SIZE(wfmda_tx_group); i++) {
			int ret;

			offset = wfmda_tx_group[i].ring_idx *
				MT_RINGREG_DIFF;
			offset_ext = wfmda_tx_group[i].ring_idx *
				MT_RINGREG_EXT_DIFF;

			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       WPDMA_TX_RING0_CTRL0 + offset,
					&wfmda_tx_group[i].base);
			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       WPDMA_TX_RING0_BASE_PTR_EXT +
					offset_ext,
					&wfmda_tx_group[i].base_ext);
			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       WPDMA_TX_RING0_CTRL1 + offset,
					&wfmda_tx_group[i].cnt);
			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       WPDMA_TX_RING0_CTRL2 + offset,
					&wfmda_tx_group[i].cidx);
			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       WPDMA_TX_RING0_CTRL3 + offset,
					&wfmda_tx_group[i].didx);

			ret = kalSnprintf(buf, BUF_SIZE,
				"%10s%10d  0x%08x  0x%016llx%10d%10d%10d",
				wfmda_tx_group[i].name,
				wfmda_tx_group[i].ring_idx,
				WPDMA_TX_RING0_CTRL0 + offset,
				(wfmda_tx_group[i].base + ((uint64_t)
					wfmda_tx_group[i].base_ext << 32)),
				wfmda_tx_group[i].cnt,
				wfmda_tx_group[i].cidx,
				wfmda_tx_group[i].didx);
			if (ret >= 0 || ret < BUF_SIZE)
				DBGLOG(HAL, INFO, "%s\n", buf);
			else
				DBGLOG(INIT, ERROR,
					"[%u] kalSnprintf failed, ret: %d\n",
						__LINE__, ret);
		}

		DBGLOG(HAL, INFO, "Rx Ring configuration\n");
		DBGLOG(HAL, INFO, "%10s%10s%12s%20s%10s%10s%10s\n",
			"Rx Ring", "Idx", "Reg", "Base", "Cnt", "CIDX", "DIDX");

		kalMemZero(buf, BUF_SIZE);
		for (i = 0; i < ARRAY_SIZE(wfmda_rx_group); i++) {
			int ret;

			offset = wfmda_rx_group[i].ring_idx * MT_RINGREG_DIFF;
			offset_ext = wfmda_rx_group[i].ring_idx *
				MT_RINGREG_EXT_DIFF;

			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       WPDMA_RX_RING0_CTRL0 + offset,
				       &wfmda_rx_group[i].base);
			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       WPDMA_RX_RING0_BASE_PTR_EXT +
				       offset_ext,
				       &wfmda_rx_group[i].base_ext);
			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       WPDMA_RX_RING0_CTRL1 + offset,
				       &wfmda_rx_group[i].cnt);
			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       WPDMA_RX_RING0_CTRL2 + offset,
				       &wfmda_rx_group[i].cidx);
			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       WPDMA_RX_RING0_CTRL3 + offset,
				       &wfmda_rx_group[i].didx);

			ret = kalSnprintf(buf, BUF_SIZE,
				"%10s%10d  0x%08x  0x%016llx%10d%10d%10d",
				wfmda_rx_group[i].name,
				wfmda_rx_group[i].ring_idx,
				WPDMA_RX_RING0_CTRL0 + offset,
				(wfmda_rx_group[i].base + ((uint64_t)
					wfmda_rx_group[i].base_ext << 32)),
				wfmda_rx_group[i].cnt,
				wfmda_rx_group[i].cidx,
				wfmda_rx_group[i].didx);
			if (ret >= 0 || ret < BUF_SIZE)
				DBGLOG(HAL, INFO, "%s\n", buf);
			else
				DBGLOG(INIT, ERROR,
					"[%u] kalSnprintf failed, ret: %d\n",
						__LINE__, ret);
		}
	}

	/* PDMA Tx/Rx descriptor & packet content */
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	for (i = 0; i < ARRAY_SIZE(wfmda_tx_group) &&
		 i < NUM_OF_TX_RING; i++) {
		if (!wfmda_tx_group[i].dump_ring_content)
			continue;
		DBGLOG(HAL, INFO, "Dump PDMA Tx Ring[%u]\n",
				wfmda_tx_group[i].ring_idx);
		prTxRing = &prHifInfo->TxRing[i];
		SwIdx = wfmda_tx_group[i].didx;
		kalDumpTxRing(prAdapter->prGlueInfo, prTxRing,
			      SwIdx, true);
		SwIdx = wfmda_tx_group[i].didx == 0 ?
				wfmda_tx_group[i].cnt - 1 :
				wfmda_tx_group[i].didx - 1;
		kalDumpTxRing(prAdapter->prGlueInfo, prTxRing,
			      SwIdx, true);
	}

	for (i = 0; i < ARRAY_SIZE(wfmda_rx_group); i++) {
		if (!wfmda_rx_group[i].dump_ring_content)
			continue;
		DBGLOG(HAL, INFO, "Dump PDMA Rx Ring[%u]\n",
				wfmda_rx_group[i].ring_idx);
		prRxRing = &prHifInfo->RxRing[i];
		SwIdx1 = wfmda_rx_group[i].didx;
		kalDumpRxRing(prAdapter->prGlueInfo, prRxRing,
			      SwIdx1, true);
		SwIdx2 = wfmda_rx_group[i].didx == 0 ?
				wfmda_rx_group[i].cnt - 1 :
				wfmda_rx_group[i].didx - 1;
		kalDumpRxRing(prAdapter->prGlueInfo, prRxRing,
			      SwIdx2, true);
		SwIdx = wfmda_rx_group[i].cidx ==
				wfmda_rx_group[i].cnt - 1 ?
				0 :
				wfmda_rx_group[i].cidx + 1;
		if (SwIdx != SwIdx1 && SwIdx != SwIdx2) {
			kalDumpRxRing(prAdapter->prGlueInfo, prRxRing,
				      SwIdx, true);
		}
	}

	/* PDMA Busy Status */
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       PDMA_DEBUG_BUSY_STATUS, &u4Value);
	DBGLOG(HAL, INFO, "PDMA busy status:0x%08x = 0x%08x\n",
		PDMA_DEBUG_STATUS, u4Value);
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       PDMA_DEBUG_HIF_BUSY_STATUS, &u4Value);
	DBGLOG(HAL, INFO, "CONN_HIF busy status:0x%08x = 0x%08x\n\n",
		PDMA_DEBUG_HIF_BUSY_STATUS, u4Value);

	/* PDMA Debug Flag Info */
	DBGLOG(HAL, INFO, "PDMA core dbg");
	if (buf) {
		kalMemZero(buf, BUF_SIZE);
		pos = 0;
		for (i = 0; i < 24; i++) {
			u4Value = 256 + i;
			HAL_MCR_WR(prAdapter, PDMA_DEBUG_EN, u4Value);
			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       PDMA_DEBUG_STATUS, &u4Value);
			pos += kalSnprintf(buf + pos, 40,
				"Set:0x%02x, result=0x%08x%s",
				i, u4Value, i == 23 ? "\n" : "; ");
			mdelay(1);
		}
		DBGLOG(HAL, INFO, "%s", buf);
	}

	/* AXI Debug Flag */
	HAL_MCR_WR(prAdapter, AXI_DEBUG_DEBUG_EN, PDMA_AXI_DEBUG_FLAG);
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       CONN_HIF_DEBUG_STATUS, &u4Value);
	DBGLOG(HAL, INFO, "Set:0x%04x, pdma axi dbg:0x%08x",
	       PDMA_AXI_DEBUG_FLAG, u4Value);

	HAL_MCR_WR(prAdapter, AXI_DEBUG_DEBUG_EN, GALS_AXI_DEBUG_FLAG);
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       CONN_HIF_DEBUG_STATUS, &u4Value);
	DBGLOG(HAL, INFO, "Set:0x%04x, gals axi dbg:0x%08x",
	       GALS_AXI_DEBUG_FLAG, u4Value);

	HAL_MCR_WR(prAdapter, AXI_DEBUG_DEBUG_EN, MCU_AXI_DEBUG_FLAG);
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       CONN_HIF_DEBUG_STATUS, &u4Value);
	DBGLOG(HAL, INFO, "Set:0x%04x, mcu axi dbg:0x%08x",
	       MCU_AXI_DEBUG_FLAG, u4Value);

	/* Rbus Bridge Debug Flag */
	DBGLOG(HAL, INFO, "rbus dbg");
	HAL_MCR_WR(prAdapter, PDMA_DEBUG_EN, RBUS_DEBUG_FLAG);
	if (buf) {
		kalMemZero(buf, BUF_SIZE);
		pos = 0;
		for (i = 0; i < 9; i++) {
			u4Value = i << 16;
			HAL_MCR_WR(prAdapter, AXI_DEBUG_DEBUG_EN, u4Value);
			HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
				       PDMA_DEBUG_STATUS, &u4Value);
			pos += kalSnprintf(buf + pos, 40,
				"Set[19:16]:0x%02x, result = 0x%08x%s",
				i, u4Value, i == 8 ? "\n" : "; ");
		}
		DBGLOG(HAL, INFO, "%s", buf);
	}
	if (prAdapter->chip_info->prDebugOps->showHifInfo)
		prAdapter->chip_info->prDebugOps->showHifInfo(prAdapter);
	if (buf)
		kalMemFree(buf, VIR_MEM_TYPE, BUF_SIZE);

#undef BUF_SIZE
}

bool halShowHostCsrInfo(struct ADAPTER *prAdapter)
{
	uint32_t i = 0, u4Value = 0;
	bool fgIsDriverOwn = false;
	bool fgEnClock = false;

	DBGLOG(HAL, INFO, "Host CSR Configuration Info:\n\n");

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, HOST_CSR_BASE, &u4Value);
	DBGLOG(HAL, INFO, "Get 0x87654321: 0x%08x = 0x%08x\n",
		HOST_CSR_BASE, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_DRIVER_OWN_INFO, &u4Value);
	DBGLOG(HAL, INFO, "Driver own info: 0x%08x = 0x%08x\n",
		HOST_CSR_DRIVER_OWN_INFO, u4Value);
	fgIsDriverOwn = (u4Value & PCIE_LPCR_HOST_SET_OWN) == 0;

	for (i = 0; i < 5; i++) {
		HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
			       HOST_CSR_MCU_PORG_COUNT, &u4Value);
		DBGLOG(HAL, INFO,
			"MCU programming Counter info (no sync): 0x%08x = 0x%08x\n",
			HOST_CSR_MCU_PORG_COUNT, u4Value);
	}

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, HOST_CSR_RGU, &u4Value);
	DBGLOG(HAL, INFO, "RGU Info: 0x%08x = 0x%08x\n", HOST_CSR_RGU, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_HIF_BUSY_CORQ_WFSYS_ON, &u4Value);
	DBGLOG(HAL, INFO, "HIF_BUSY / CIRQ / WFSYS_ON info: 0x%08x = 0x%08x\n",
		HOST_CSR_HIF_BUSY_CORQ_WFSYS_ON, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_PINMUX_MON_FLAG, &u4Value);
	DBGLOG(HAL, INFO, "Pinmux/mon_flag info: 0x%08x = 0x%08x\n",
		HOST_CSR_PINMUX_MON_FLAG, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_MCU_PWR_STAT, &u4Value);
	DBGLOG(HAL, INFO, "Bit[5] mcu_pwr_stat: 0x%08x = 0x%08x\n",
		HOST_CSR_MCU_PWR_STAT, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter, HOST_CSR_FW_OWN_SET, &u4Value);
	DBGLOG(HAL, INFO, "Bit[15] fw_own_stat: 0x%08x = 0x%08x\n",
		HOST_CSR_FW_OWN_SET, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_MCU_SW_MAILBOX_0, &u4Value);
	DBGLOG(HAL, INFO, "WF Mailbox[0]: 0x%08x = 0x%08x\n",
		HOST_CSR_MCU_SW_MAILBOX_0, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_MCU_SW_MAILBOX_1, &u4Value);
	DBGLOG(HAL, INFO, "MCU Mailbox[1]: 0x%08x = 0x%08x\n",
		HOST_CSR_MCU_SW_MAILBOX_1, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_MCU_SW_MAILBOX_2, &u4Value);
	DBGLOG(HAL, INFO, "BT Mailbox[2]: 0x%08x = 0x%08x\n",
		HOST_CSR_MCU_SW_MAILBOX_2, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_MCU_SW_MAILBOX_3, &u4Value);
	DBGLOG(HAL, INFO, "GPS Mailbox[3]: 0x%08x = 0x%08x\n",
		HOST_CSR_MCU_SW_MAILBOX_3, u4Value);

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_CONN_CFG_ON, &u4Value);
	DBGLOG(HAL, INFO, "Conn_cfg_on info: 0x%08x = 0x%08x\n",
		HOST_CSR_CONN_CFG_ON, u4Value);

#if (CFG_ENABLE_HOST_BUS_TIMEOUT == 1)
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_AP2CONN_AHB_HADDR, &u4Value);
	DBGLOG(HAL, INFO, "HOST_CSR_AP2CONN_AHB_HADDR: 0x%08x = 0x%08x\n",
		HOST_CSR_AP2CONN_AHB_HADDR, u4Value);
#endif

	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_CONN_HIF_ON_MD_LPCTL_ADDR,
		       &u4Value);
	DBGLOG(HAL, INFO,
	       "CONN_HIF_ON_MD_LPCTL_ADDR: 0x%08x = 0x%08x\n",
	       HOST_CSR_CONN_HIF_ON_MD_LPCTL_ADDR, u4Value);
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_CONN_HIF_ON_MD_IRQ_STAT_ADDR,
		       &u4Value);
	DBGLOG(HAL, INFO,
	       "CONN_HIF_ON_MD_IRQ_STAT_ADDR: 0x%08x = 0x%08x\n",
	       HOST_CSR_CONN_HIF_ON_MD_IRQ_STAT_ADDR, u4Value);
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_CONN_HIF_ON_MD_IRQ_ENA_ADDR,
		       &u4Value);
	DBGLOG(HAL, INFO,
	       "CONN_HIF_ON_MD_IRQ_ENA_ADDR: 0x%08x = 0x%08x\n",
	       HOST_CSR_CONN_HIF_ON_MD_IRQ_ENA_ADDR, u4Value);

	HAL_MCR_WR(prAdapter, HOST_CSR_DRIVER_OWN_INFO, 0x00030000);
	kalUdelay(1);
	HAL_RMCR_RD(HIF_CONNAC1_2, prAdapter,
		       HOST_CSR_DRIVER_OWN_INFO, &u4Value);
	DBGLOG(HAL, INFO, "Bit[17]/[16], Get HCLK info: 0x%08x = 0x%08x\n",
		HOST_CSR_DRIVER_OWN_INFO, u4Value);

	/* check clock is enabled */
	fgEnClock = ((u4Value & BIT(17)) != 0) && ((u4Value & BIT(16)) != 0);

	return fgIsDriverOwn && fgEnClock;
}

