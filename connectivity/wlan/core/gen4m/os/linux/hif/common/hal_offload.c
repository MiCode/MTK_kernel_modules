// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   hal_offload.c
*    \brief  Internal driver stack will export
*    the required procedures here for GLUE Layer.
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
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

#include "hif_pdma.h"

#include <linux/mm.h>
#ifndef CONFIG_X86
#include <asm/memory.h>
#endif

#include "mt66xx_reg.h"
#include "gl_kal.h"

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
#include "conninfra.h"
#endif

#ifdef MT6639
#include "coda/mt6639/mawd_reg.h"
#include "coda/mt6639/wf_rro_top.h"
#include "coda/mt6653/wf_wfdma_host_dma0.h"
#endif

#ifdef MT6653
#include "coda/mt6653/mawd_reg.h"
#include "coda/mt6653/wf_rro_top.h"
#include "coda/mt6653/wf_wfdma_host_dma0.h"
#endif

#ifdef MT6655
#include "coda/mt6655/mawd_reg.h"
#include "coda/mt6655/wf_rro_top.h"
#include "coda/mt6653/wf_wfdma_host_dma0.h"
#endif

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define MAWD_RRO_ADDR_OFFSET	(WF_RRO_TOP_BASE - 0xDA000)
#define MAWD_WFDMA_ADDR_OFFSET	\
	(CONN_INFRA_REMAPPING_OFFSET - 0xD0000 + 0x18020000)

#define MAWD_CR_BACKUP_OFFSET_VER_1_0	88
#define MAWD_CR_BACKUP_OFFSET_VER_1_1	128
#define MAWD_CR_BACKUP_NUM		64

#define MAWD_AMSDU_MAX_CNT		8
#define MAWD_READ_COUNT_BY_EMI		0

#define RRO_STA_EMI_BASE		0x78000000
#define RRO_STA_EMI_OFFSET		0x255B80
#define RRO_DISABLE_EMI_OFFSET		0x255B88

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
/* reset mawd idx to default value
 * 0: md_rx_blk_ring_dma_idx	(default = 0)
 * 1: ap_rx_blk_ring_dma_idx	(default = 0)
 * 2: ind_cmd_q_magic		(default = 0)
 * 3: ind_cmd_q_rdix		(default = 0)
 * 4: ring0_hiftxd_adr_off	(default = 0)
 * 5: hiftxd_q0_ridx		(default = 0)
 * 6: ring1_hiftxd_adr_off	(default = 0)
 * 7: hiftxd_q1_ridx		(default = 0)
 * 8: ring2_hiftxd_adr_off	(default = 0)
 * 9: hiftxd_q2_ridx		(default = 0)
 * 10: err_rpt_dma_idx		(default = 0)
 * 11: dmad_q0_widx		(default = 0)
 * 12: dmad_q1_widx		(default = 0)
 * 13: dmad_q2_widx		(default = 0)
 * 14: dmad_q0_ridx		(default = 0)
 * 15: dmad_q1_ridx		(default = 0)
 * 16: dmad_q2_ridx		(default = 0)
 * 17: md_rx_blk_ing_magic_cnt	(default = 0)
 * 18: ap_rx_blk_ing_magic_cnt	(default = 0)
 */
static uint32_t au4MawdIdxPatchVer1_0[] = {
	0, 0, 0, 0,
	32, 0, 32, 0,
	32, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0
};

static uint32_t au4MawdIdxPatchVer1_1[] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
static uint32_t halGetMawdCsrOffset(void)
{
	struct mt66xx_chip_info *prChipInfo = NULL;

	glGetChipInfo((void **)&prChipInfo);

	return prChipInfo ? prChipInfo->u4HostCsrOffset : 0;
}

#define MAWD_WFDMA_HIGH_ADDR	(halGetMawdCsrOffset() == 0 ? 0x4 : 0)
#define MAWD_WFDMA_LOW_ADDR	(halGetMawdCsrOffset())
#define MAWD_CR_OFFSET		(halGetMawdCsrOffset())

#define HAL_MAWD_MCR_RD(_A, _R, _V) \
	HAL_RMCR_RD(OFFLOAD_HOST, _A, _R + MAWD_CR_OFFSET, _V)

#define HAL_MAWD_MCR_WR(_A, _R, _V) \
	HAL_MCR_WR(_A, _R + MAWD_CR_OFFSET, _V)

#define HAL_GET_MAWD_RING_DIDX(_A, _R, _V)	\
do { \
	HAL_MAWD_MCR_RD(_A, _R->hw_didx_addr, _V); \
	*_V = (*_V & _R->hw_didx_mask) >> _R->hw_didx_shift; \
} while (0)

#define HAL_SET_MAWD_RING_CIDX(_A, _R, _V) \
	HAL_MAWD_MCR_WR(_A, _R->hw_cidx_addr, _V << _R->hw_cidx_shift)

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static void halMawdBackupCr(struct GLUE_INFO *prGlueInfo,
			    uint32_t u4Addr, uint32_t u4Val);
static u_int8_t halRroHashAdd(struct GL_HIF_INFO *prHifInfo, uint64_t u8Key,
			      struct sk_buff *prSkb, struct RX_CTRL_BLK *prRcb);
static u_int8_t halRroHashDel(struct GL_HIF_INFO *prHifInfo, uint64_t u8Key);
static struct RCB_NODE *halRroHashSearch(
	struct GL_HIF_INFO *prHifInfo, uint64_t u8Key);
static void halMawdInitSram(struct GLUE_INFO *prGlueInfo);
static void halMawdReadSram(
	struct GLUE_INFO *prGlueInfo,
	uint32_t u4Offset,
	uint32_t *pu4ValL,
	uint32_t *pu4Val);
static void halMawdUpdateSram(
	struct GLUE_INFO *prGlueInfo,
	uint32_t u4Offset,
	uint32_t u4ValL,
	uint32_t u4ValH);
static void halMawdTxFreeMem(struct GLUE_INFO *prGlueInfo);
static u_int8_t __halMawdWakeup(void);
static void __halMawdSleep(void);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static void halMawdWakeupSleepDebugDump(struct ADAPTER *prAdapter)
{
	uint32_t u4Addr = 0, u4Idx = 0, u4Val = 0;

	for (u4Idx = 0x11 ; u4Idx <= 0x13; u4Idx++) {
		u4Addr = MAWD_DEBUG_SETTING2;
		HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Idx);
		u4Addr = MAWD_DEBUG_SETTING1;
		HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
		DBGLOG(HAL, DEBUG, "WR 100=[%x], RD 104=[0x%08x]\n",
		       u4Idx, u4Val);
	}

	halMawdDumpSram(prAdapter->prGlueInfo);
}

static u_int8_t halMawdWakeUpVer1_0(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct ADAPTER *prAdapter;
	struct RTMP_RX_RING *prRxRing;
	struct WIFI_VAR *prWifiVar;
	uint32_t u4Addr = 0, u4Val = 0, u4Idx = 0;
	u_int8_t fgRet = TRUE;

	prHifInfo = &prGlueInfo->rHifInfo;
	prAdapter = prGlueInfo->prAdapter;
	prWifiVar = &prAdapter->rWifiVar;
	prRxRing = &prHifInfo->RxBlkRing[0];

	u4Addr = MAWD_DUMMY_REG_RW;
	u4Val = BIT(0);
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	/* mawd speed up */
	u4Addr = MAWD_POWER_UP;
	u4Val = BIT(0);
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);
	for (u4Idx = 0; u4Idx < MAWD_POWER_UP_RETRY_CNT; u4Idx++) {
		HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
		if ((u4Val & BIT(2)) == BIT(2))
			break;
		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
	}
	if (u4Idx == MAWD_POWER_UP_RETRY_CNT) {
		fgRet = FALSE;
		DBGLOG(HAL, ERROR, "Mawd wakeup failed\n");
	}

	if (!IS_FEATURE_ENABLED(prWifiVar->fgEnableRro))
		goto done;

	u4Addr = MAWD_IDX_REG_PATCH;
	u4Val = 0x81000000 | (prHifInfo->u4RxBlkDidx & 0xFFFF);
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);
	u4Val = 0x92000000 | (prHifInfo->u4RxBlkMagicCnt & 0xFFFF);
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);
	HAL_SET_MAWD_RING_CIDX(prAdapter, prRxRing, prRxRing->RxCpuIdx);

	/* BKRS index from RRO */
	u4Addr = WF_RRO_TOP_IND_CMD_0_CTRL3_ADDR;
	HAL_RMCR_RD(OFFLOAD_READ, prAdapter, u4Addr, &u4Val);
	u4Addr = MAWD_IND_CMD_SIGNATURE1;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	/* prevent MD disable RRO after AP eable RRO */
	u4Addr = MAWD_BA_PARAM;
	for (u4Idx = 0; u4Idx < MAWD_POWER_UP_RETRY_CNT; u4Idx++) {
		HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
		if ((u4Val & BITS(0, 1)) != 2)
			break;
		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
	}
	if (u4Idx == MAWD_POWER_UP_RETRY_CNT) {
		fgRet = FALSE;
		DBGLOG(HAL, ERROR, "Polling MD in sleep flow failed\n");
	}

	u4Addr = WF_RRO_TOP_IND_CMD_SIGNATURE_BASE_1_ADDR;
	u4Val = WF_RRO_TOP_IND_CMD_SIGNATURE_BASE_1_EN_MASK |
		MAWD_WFDMA_HIGH_ADDR;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = MAWD_SOFTRESET;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);

done:
	return fgRet;
}

static u_int8_t halMawdWakeUpVer1_1(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct ADAPTER *prAdapter;
	struct WIFI_VAR *prWifiVar;
	struct RTMP_RX_RING *prRxRing;
	uint32_t u4Addr = 0, u4Val = 0, u4Idx = 0;
	u_int8_t fgRet = TRUE;

	prHifInfo = &prGlueInfo->rHifInfo;
	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;
	prRxRing = &prHifInfo->RxBlkRing[0];

	/* mawd speed up */
	u4Addr = MAWD_POWER_UP;
	for (u4Idx = 0; u4Idx < MAWD_POWER_UP_RETRY_CNT; u4Idx++) {
		HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
		if ((u4Val & BIT(2)) == BIT(2))
			break;
		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
	}
	if (u4Idx == MAWD_POWER_UP_RETRY_CNT) {
		fgRet = FALSE;
		DBGLOG(HAL, ERROR, "Mawd wakeup polling failed[0x%08x]\n",
		       u4Val);
		halMawdWakeupSleepDebugDump(prAdapter);
		GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_MAWD_WAKEUP_FAIL);
		goto done;
	}

	if (!IS_FEATURE_ENABLED(prWifiVar->fgEnableRro))
		goto done;

	HAL_SET_MAWD_RING_CIDX(prAdapter, prRxRing, prRxRing->RxCpuIdx);

	u4Addr = WF_RRO_TOP_IND_CMD_SIGNATURE_BASE_1_ADDR;
	u4Val = WF_RRO_TOP_IND_CMD_SIGNATURE_BASE_1_EN_MASK |
		MAWD_WFDMA_HIGH_ADDR;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	/* BKRS index from RRO */
	if (emi_mem_read(prChipInfo, RRO_STA_EMI_OFFSET,
			 &u4Val, sizeof(uint32_t))) {
		/* read cr only if read emi fail */
		u4Addr = WF_RRO_TOP_IND_CMD_0_CTRL3_ADDR;
		u4Val = 0;
		HAL_RMCR_RD(OFFLOAD_READ, prAdapter, u4Addr, &u4Val);
	}
	u4Addr = MAWD_IND_CMD_SIGNATURE1;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

done:
	return fgRet;
}

u_int8_t halMawdWakeup(struct GLUE_INFO *prGlueInfo)
{
#if MAWD_ENABLE_WAKEUP_SLEEP
	struct GL_HIF_INFO *prHifInfo;
	struct ADAPTER *prAdapter;
	u_int8_t fgRet = TRUE;

	prHifInfo = &prGlueInfo->rHifInfo;
	prAdapter = prGlueInfo->prAdapter;

#if CFG_MTK_ANDROID_WMT
	if (!is_cal_flow_finished())
		goto exit;
#endif

#if (CFG_MTK_FPGA_PLATFORM == 0)
	fgRet = __halMawdWakeup();
	if (!fgRet)
		goto exit;
#endif

	if (!prAdapter->fgIsFwDownloaded ||
	    !prHifInfo->fgIsMawdSuspend)
		goto exit;

	if (kalGetMawdVer() == MAWD_VER_1_0)
		fgRet = halMawdWakeUpVer1_0(prGlueInfo);
	else
		fgRet = halMawdWakeUpVer1_1(prGlueInfo);
	if (!fgRet)
		goto exit;

	prHifInfo->fgIsMawdSuspend = FALSE;
	DBGLOG(HAL, LOUD, "Mawd wakeup done\n");

exit:
	return fgRet;
#else /* MAWD_ENABLE_WAKEUP_SLEEP == 0 */
	return TRUE;
#endif /* MAWD_ENABLE_WAKEUP_SLEEP */
}

static u_int8_t halMawdSleepVer1_0(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct ADAPTER *prAdapter;
	struct WIFI_VAR *prWifiVar;
	struct RTMP_RX_RING *prRxRing;
	uint32_t u4Addr, u4Val, u4Idx;
	u_int8_t fgRet = TRUE;

	prHifInfo = &prGlueInfo->rHifInfo;
	prAdapter = prGlueInfo->prAdapter;
	prWifiVar = &prAdapter->rWifiVar;
	prRxRing = &prHifInfo->RxBlkRing[0];

	u4Addr = MAWD_DUMMY_REG_RW;
	u4Val = BIT(1);
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	if (!IS_FEATURE_ENABLED(prWifiVar->fgEnableRro))
		goto done;

	u4Addr = MAWD_BA_PARAM;
	HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
	/* MD is wakeup */
	if ((u4Val & BITS(0, 1)) == 0)
		goto mawd_sleep;

	u4Addr = MAWD_POWER_UP;
	HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
	for (u4Idx = 0; u4Idx < MAWD_POWER_UP_RETRY_CNT; u4Idx++) {
		HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
		if ((u4Val & BITS(8, 12)) == BITS(8, 12))
			break;
		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
	}
	if (u4Idx == MAWD_POWER_UP_RETRY_CNT) {
		fgRet = FALSE;
		DBGLOG(HAL, ERROR, "Mawd sleep failed\n");
		goto exit;
	}

	u4Addr = WF_RRO_TOP_IND_CMD_SIGNATURE_BASE_1_ADDR;
	u4Val = MAWD_WFDMA_HIGH_ADDR;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

mawd_sleep:
	u4Addr = MAWD_AP_RX_BLK_CTRL2;
	HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
	prHifInfo->u4RxBlkDidx = (u4Val & BITS(12, 23)) >> 12;
	prHifInfo->u4RxBlkMagicCnt = (u4Val & BITS(30, 31)) >> 30;

done:
	u4Addr = MAWD_DUMMY_REG_RW;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);

exit:
	return fgRet;
}

static u_int8_t halMawdDisableRro(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	uint32_t u4Addr, u4Val = 0, u4Idx = 0;

	prAdapter = prGlueInfo->prAdapter;

	/* notify fw disable rro */
	halTriggerSwInterrupt(prAdapter, MCU_INT_DISABLE_RRO);

	/* dummy CR for disable RRO */
	u4Addr = MAWD_DUMMY_REG_RW;
	for (u4Idx = 0; u4Idx < MAWD_POWER_UP_RETRY_CNT; u4Idx++) {
		HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
		if ((u4Val & BIT(0))) {
			HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);
			return TRUE;
		}

		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
	}

	DBGLOG(HAL, ERROR, "Polling timeout [0x%08x]=[0x%08x]\n",
	       u4Addr, u4Val);
	return FALSE;
}

static u_int8_t halMawdSleepVer1_1(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct ADAPTER *prAdapter;
	struct WIFI_VAR *prWifiVar;
	uint32_t u4Addr, u4Val, u4Idx;
	u_int8_t fgRet = TRUE;

	prHifInfo = &prGlueInfo->rHifInfo;
	prAdapter = prGlueInfo->prAdapter;
	prWifiVar = &prAdapter->rWifiVar;

	if (!IS_FEATURE_ENABLED(prWifiVar->fgEnableRro))
		goto exit;

	u4Addr = MAWD_POWER_UP;
	u4Val = BIT(1);
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);
	for (u4Idx = 0; u4Idx < MAWD_POWER_UP_RETRY_CNT; u4Idx++) {
		HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
		if ((u4Val & BIT(1)) == 0)
			break;
		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
	}
	if (u4Idx == MAWD_POWER_UP_RETRY_CNT) {
		fgRet = FALSE;
		DBGLOG(HAL, ERROR, "Mawd sleep polling failed[0x%08x]\n",
		       u4Val);
		halMawdWakeupSleepDebugDump(prAdapter);
		goto exit;
	}

	fgRet = halMawdDisableRro(prGlueInfo);

exit:
	return fgRet;
}

u_int8_t halMawdSleep(struct GLUE_INFO *prGlueInfo)
{
#if MAWD_ENABLE_WAKEUP_SLEEP
	struct GL_HIF_INFO *prHifInfo;
	struct ADAPTER *prAdapter;
	u_int8_t fgRet = TRUE;

	prHifInfo = &prGlueInfo->rHifInfo;
	prAdapter = prGlueInfo->prAdapter;

#if CFG_MTK_ANDROID_WMT
	if (!is_cal_flow_finished())
		goto exit;
#endif

	if (!prAdapter->fgIsFwDownloaded ||
	    prHifInfo->fgIsMawdSuspend ||
	    prAdapter->ucSerState != SER_IDLE_DONE)
		goto exit;

	if (halMawdGetRxBlkDoneCnt(prGlueInfo, 0)) {
		fgRet = FALSE;
		goto exit;
	}

	if (kalGetMawdVer() == MAWD_VER_1_0)
		fgRet = halMawdSleepVer1_0(prGlueInfo);
	else
		fgRet = halMawdSleepVer1_1(prGlueInfo);
	if (!fgRet)
		goto exit;

#if (CFG_MTK_FPGA_PLATFORM == 0)
	__halMawdSleep();
#endif

	prHifInfo->fgIsMawdSuspend = TRUE;
	DBGLOG(HAL, LOUD, "Mawd sleep done\n");

exit:
	return fgRet;
#else /* MAWD_ENABLE_WAKEUP_SLEEP == 0 */
	return TRUE;
#endif /* MAWD_ENABLE_WAKEUP_SLEEP */
}

void halRroAllocMem(struct GLUE_INFO *prGlueInfo, u_int8_t fgAllocMem)
{
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_DMABUF *prCache, *prAddrArray, *prIndCmd;
	uint32_t u4AddrNum;

	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prCache = &prHifInfo->BaBitmapCache;
	prAddrArray = &prHifInfo->AddrArray;
	prIndCmd = &prHifInfo->IndCmdRing;

	if (!fgAllocMem)
		goto reset;

	prCache->AllocSize =
		RRO_BA_BITMAP_SIZE * RRO_MAX_WINDOW_NUM;
	if (prMemOps->allocExtBuf)
		prMemOps->allocExtBuf(prHifInfo, prCache,
				      WFDMA_MEMORY_ALIGNMENT);

	if (prCache->AllocVa == NULL) {
		DBGLOG(HAL, ERROR, "BaBitmap allocation failed!!\n");
		return;
	}

	prIndCmd->AllocSize =
		RRO_IND_CMD_RING_SIZE * sizeof(struct RRO_IND_CMD);
	if (prMemOps->allocExtBuf)
		prMemOps->allocExtBuf(prHifInfo, prIndCmd,
				      WFDMA_MEMORY_ALIGNMENT);

	if (prIndCmd->AllocVa == NULL) {
		DBGLOG(HAL, ERROR, "IndCmd allocation failed!!\n");
		return;
	}

	/* session num + particular */
	u4AddrNum = (RRO_TOTAL_ADDR_ELEM_NUM + 1) * RRO_MAX_WINDOW_NUM;
	prAddrArray->AllocSize = RRO_ADDR_ELEM_SIZE * u4AddrNum;
	if (prMemOps->allocExtBuf)
		prMemOps->allocExtBuf(prHifInfo, prAddrArray,
				      WFDMA_MEMORY_ALIGNMENT);

	if (prAddrArray->AllocVa == NULL) {
		DBGLOG(HAL, ERROR, "AddrArray allocation failed!!\n");
		return;
	}

reset:
	halRroResetMem(prGlueInfo);
}

void halRroResetMem(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prCache, *prAddrArray, *prIndCmd;
	struct RRO_ADDR_ELEM *prAddrElem;
	struct RRO_IND_CMD *prIndCmdElem;
	uint32_t u4Idx, u4AddrNum;

	prHifInfo = &prGlueInfo->rHifInfo;
	prCache = &prHifInfo->BaBitmapCache;
	prAddrArray = &prHifInfo->AddrArray;
	prIndCmd = &prHifInfo->IndCmdRing;

	if (prCache->AllocVa)
		memset(prCache->AllocVa, 0, prCache->AllocSize);

	if (prIndCmd->AllocVa) {
		memset(prIndCmd->AllocVa, 0, prIndCmd->AllocSize);
		for (u4Idx = 0; u4Idx < RRO_IND_CMD_RING_SIZE; u4Idx++) {
			prIndCmdElem = (struct RRO_IND_CMD *)
				(prIndCmd->AllocVa +
				 u4Idx * sizeof(struct RRO_IND_CMD));
			prIndCmdElem->magic_cnt = 4;
		}
	}

	if (prAddrArray->AllocVa) {
		memset(prAddrArray->AllocVa, 0, prAddrArray->AllocSize);
		u4AddrNum = (RRO_TOTAL_ADDR_ELEM_NUM + 1) * RRO_MAX_WINDOW_NUM;
		for (u4Idx = 0; u4Idx < u4AddrNum; u4Idx++) {
			prAddrElem = (struct RRO_ADDR_ELEM *)
				(prAddrArray->AllocVa +
				 u4Idx * sizeof(struct RRO_ADDR_ELEM));
			prAddrElem->elem0.signature = 0x7;
			prAddrElem->elem1.signature = 0x7;
		}
	}
}

static void halMawdTxFreeMem(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_DMABUF *prTxDesc, *prErrRpt;
	uint32_t u4Num;

	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prErrRpt = &prHifInfo->ErrRptRing;

	if (!prMemOps->freeExtBuf)
		return;

	for (u4Num = 0; u4Num < MAWD_MD_TX_RING_NUM; u4Num++) {
		prTxDesc = &prHifInfo->HifTxDescRing[u4Num];
		prMemOps->freeExtBuf(prHifInfo, prTxDesc);
	}
	prMemOps->freeExtBuf(prHifInfo, prErrRpt);
}

void halRroFreeMem(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_DMABUF *prCache, *prAddrArray, *prIndCmd;

	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prCache = &prHifInfo->BaBitmapCache;
	prAddrArray = &prHifInfo->AddrArray;
	prIndCmd = &prHifInfo->IndCmdRing;

	if (!prMemOps->freeExtBuf)
		return;

	prMemOps->freeExtBuf(prHifInfo, prCache);
	prMemOps->freeExtBuf(prHifInfo, prAddrArray);
	prMemOps->freeExtBuf(prHifInfo, prIndCmd);
}

static void halRroSetupBaBitmap(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prCache;
	uint32_t u4Addr, u4Val;

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	prCache = &prHifInfo->BaBitmapCache;

	u4Addr = WF_RRO_TOP_BA_BITMAP_BASE_0_ADDR;
	u4Val = prCache->AllocPa;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_RRO_TOP_BA_BITMAP_BASE_1_ADDR;
	u4Val = (prCache->AllocPa >> DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

#ifdef WF_RRO_TOP_BA_BITMAP_BASE_EXT0_ADDR
	u4Addr = WF_RRO_TOP_BA_BITMAP_BASE_EXT0_ADDR;
	u4Val = prCache->AllocPa;
	u4Val += RRO_BA_BITMAP_SIZE * RRO_MAX_WINDOW_NUM / 2;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_RRO_TOP_BA_BITMAP_BASE_EXT1_ADDR;
	u4Val = (prCache->AllocPa >> DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
#endif /* WF_RRO_TOP_BA_BITMAP_BASE_EXT0_ADDR */
}

static void halRroSetupAddressElement(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prAddrArray;
	uint32_t u4Addr, u4Val;

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	prAddrArray = &prHifInfo->AddrArray;

	u4Addr = WF_RRO_TOP_ADDR_ARRAY_BASE_0_ADDR;
	u4Val = prAddrArray->AllocPa;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_RRO_TOP_ADDR_ARRAY_BASE_1_ADDR;
	u4Val = ((prAddrArray->AllocPa >> DMA_BITS_OFFSET) &
	    DMA_HIGHER_4BITS_MASK) | BIT(30);
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

#ifdef WF_RRO_TOP_PARTICULAR_CFG_0_ADDR
	u4Addr = WF_RRO_TOP_PARTICULAR_CFG_0_ADDR;
	u4Val = prAddrArray->AllocPa +
		(RRO_ADDR_ELEM_SIZE *
		 RRO_TOTAL_ADDR_ELEM_NUM *
		 RRO_MAX_WINDOW_NUM);
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
#endif /* WF_RRO_TOP_PARTICULAR_CFG_0_ADDR */

#ifdef WF_RRO_TOP_PARTICULAR_CFG_1_ADDR
	u4Addr = WF_RRO_TOP_PARTICULAR_CFG_1_ADDR;
	u4Val = (prAddrArray->AllocPa >> DMA_BITS_OFFSET) |
		(RRO_TOTAL_ADDR_ELEM_NUM << 16) | BIT(31);
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
#endif /* WF_RRO_TOP_PARTICULAR_CFG_1_ADDR */

	u4Addr = WF_RRO_TOP_BA_STA_CFG_ADDR;
	u4Val = (0x5 << 24) |
		(0x20 << 16) |
		((RRO_MAX_TID_NUM - 1) << 12) |
		(RRO_MAX_STA_NUM - 1);
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

static void halRroSetupIndicateCmdRing(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct WIFI_VAR *prWifiVar;
	struct RTMP_DMABUF *prIndCmd;
	uint32_t u4Addr, u4Val;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prWifiVar = &prAdapter->rWifiVar;
	prIndCmd = &prHifInfo->IndCmdRing;

	prHifInfo->u4RroMagicCnt = 0;
	prHifInfo->u4IndCmdDmaIdx = 0;
	prHifInfo->u4RcbErrorCnt = 0;
	prHifInfo->u4RcbSkipCnt = 0;
	prHifInfo->u4RcbFixCnt = 0;
	prHifInfo->u4RcbHeadCnt = 0;
	prHifInfo->u4RxBlkDidx = 0;
	prHifInfo->u4RxBlkMagicCnt = 0;

	u4Addr = WF_RRO_TOP_IND_CMD_0_CTRL0_ADDR;
	u4Val = prIndCmd->AllocPa;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_RRO_TOP_IND_CMD_0_CTRL1_ADDR;
	u4Val = (((prIndCmd->AllocPa >> DMA_BITS_OFFSET) &
		DMA_HIGHER_4BITS_MASK) << 16) |
		RRO_IND_CMD_RING_SIZE;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_RRO_TOP_IND_CMD_0_CTRL2_ADDR;
	u4Val = RRO_IND_CMD_RING_SIZE - 1;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_RRO_TOP_IND_CMD_0_CTRL3_ADDR;
	u4Val = 0;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd)) {
		u4Addr = WF_RRO_TOP_IND_CMD_SIGNATURE_BASE_0_ADDR;
		u4Val = MAWD_IND_CMD_SIGNATURE0 | MAWD_WFDMA_LOW_ADDR;
		HAL_MCR_WR(prAdapter, u4Addr, u4Val);

		u4Addr = WF_RRO_TOP_IND_CMD_SIGNATURE_BASE_1_ADDR;
		u4Val = WF_RRO_TOP_IND_CMD_SIGNATURE_BASE_1_EN_MASK |
			MAWD_WFDMA_HIGH_ADDR;
		HAL_MCR_WR(prAdapter, u4Addr, u4Val);
	}

	/* enable rro host interrupt */
	u4Addr = WF_RRO_TOP_HOST_INT_ENA_ADDR;
	u4Val = WF_RRO_TOP_HOST_INT_ENA_HOST_RRO_DONE_ENA_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

static void halRroSetupTimeoutConfig(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4Addr, u4Val;

	u4Addr = WF_RRO_TOP_TIMEOUT_CONF_0_TO_STEP_ONE_0_ADDR;
	u4Val = 9;
	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4Val);
}

void halRroWfdmaInit(struct GLUE_INFO *prGlueInfo)
{
#if (CFG_MTK_MDDP_SUPPORT == 1)
	uint32_t u4Addr, u4Val;

	/* Pre-pause threshold setting */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_PAUSE_RRO_Q_ADDR;
	u4Val = 0x20;
	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4Val);
#endif
}

void halRroMawdInit(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prAddrArray, *prIndCmd;
	uint32_t u4Addr, u4Val;
	uint32_t u4MawdRroAddrOffset = MAWD_RRO_ADDR_OFFSET;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prAddrArray = &prHifInfo->AddrArray;
	prIndCmd = &prHifInfo->IndCmdRing;
	u4MawdRroAddrOffset -=
		(uint32_t)(prChipInfo->u8CsrOffset & BITS(0, 31));

	prHifInfo->fgIsMawdSuspend = TRUE;

	/* speed up the PLL after power up */
	u4Addr = MAWD_POWER_UP;
	u4Val = BIT(0);
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	/* setup addr array */
	u4Addr = MAWD_ADDR_ARRAY_BASE_L;
	u4Val = prAddrArray->AllocPa;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = MAWD_ADDR_ARRAY_BASE_M;
	u4Val = (prAddrArray->AllocPa >> DMA_BITS_OFFSET) &
		DMA_HIGHER_4BITS_MASK;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	/* setup ind cmd array */
	u4Addr = MAWD_IND_CMD_CTRL0;
	u4Val = prIndCmd->AllocPa;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = MAWD_IND_CMD_CTRL1;
	u4Val = ((prIndCmd->AllocPa >> DMA_BITS_OFFSET) &
		 DMA_HIGHER_4BITS_MASK) |
		(RRO_IND_CMD_RING_SIZE << 4);
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	/* setup ack sn */
	u4Addr = MAWD_RRO_ACK_SN_BASE_L;
	u4Val = WF_RRO_TOP_ACK_SN_CTRL_ADDR - u4MawdRroAddrOffset;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = MAWD_RRO_ACK_SN_BASE_M;
	u4Val = MAWD_WFDMA_HIGH_ADDR;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	if (kalGetMawdVer() == MAWD_VER_1_0) {
		u4Addr = MAWD_SOFTRESET;
		HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);
	} else {
		u4Addr = MAWD_CR_OFFSET + 0x1140;
		HAL_MCR_WR(NULL, u4Addr, BIT(0));
	}

#if CFG_MTK_FPGA_PLATFORM
	/* set remapping CR for MAWD in connsys FPGA */
	HAL_MCR_WR(prAdapter, 0x7C023118, 0x1);
#endif
}

void halMawdAllocRxBlkRing(struct GLUE_INFO *prGlueInfo,
			   u_int8_t fgAllocMem, uint32_t u4Num)
{
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_RX_RING *pRxRing;
	struct RTMP_DMABUF *prRxDesc;
	struct RTMP_DMABUF *pDmaBuf;
	struct RTMP_DMACB *prRxCell;
	phys_addr_t RingBasePa;
	void *RingBaseVa;
	uint32_t u4BufSize, u4DescSize = sizeof(struct RX_BLK_DESC);
	uint32_t u4Idx = 0;

	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prRxDesc = &prHifInfo->RxBlkDescRing[u4Num];

	/* Don't re-alloc memory when second time call alloc ring */
	prRxDesc->AllocSize = prHifInfo->u4RxDataRingSize * u4DescSize;
	if (fgAllocMem && prMemOps->allocExtBuf)
		prMemOps->allocExtBuf(prHifInfo, prRxDesc,
				      WFDMA_MEMORY_ALIGNMENT);

	if (prRxDesc->AllocVa == NULL) {
		DBGLOG(HAL, ERROR, "RxBlkRing allocation failed!!\n");
		return;
	}

	DBGLOG(HAL, TRACE, "RxBlkRing[%p]: total %lu bytes allocated\n",
		prRxDesc->AllocVa, prRxDesc->AllocSize);

	u4BufSize = CFG_RX_MAX_PKT_SIZE;

	/* Initialize Rx Ring and associated buffer memory */
	RingBasePa = prRxDesc->AllocPa;
	RingBaseVa = prRxDesc->AllocVa;

	pRxRing = &prHifInfo->RxBlkRing[u4Num];
	pRxRing->u4BufSize = u4BufSize;
	pRxRing->u4RingSize = prHifInfo->u4RxDataRingSize;
	pRxRing->u4RingIdx = u4Num;
	pRxRing->fgRxSegPkt = FALSE;
#if (CFG_SUPPORT_PDMA_SCATTER == 1)
	pRxRing->pvSegPkt = NULL;
	pRxRing->u4SegPktLen = 0;
#endif

	for (u4Idx = 0; u4Idx < pRxRing->u4RingSize; u4Idx++) {
		/* Init RX Ring Size, Va, Pa variables */
		prRxCell = &pRxRing->Cell[u4Idx];
		prRxCell->AllocSize = u4DescSize;
		prRxCell->AllocVa = RingBaseVa;
		prRxCell->AllocPa = RingBasePa;
		prRxCell->prToken = NULL;

		RingBasePa += u4DescSize;
		RingBaseVa += u4DescSize;

		pDmaBuf = &prRxCell->DmaBuf;
		pDmaBuf->AllocSize = u4BufSize;
	}

	DBGLOG(HAL, TRACE, "RxBlkRing: total %d entry allocated\n", u4Idx);
}

void halMawdFreeRxBlkRing(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_DMABUF *prRxDesc;
	uint32_t u4Idx;

	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;

	for (u4Idx = 0; u4Idx < MAWD_NUM_OF_RX_BLK_RING; u4Idx++) {
		prRxDesc = &prHifInfo->RxBlkDescRing[u4Idx];

		if (prMemOps->freeExtBuf)
			prMemOps->freeExtBuf(prHifInfo, prRxDesc);
	}
}

void halMawdInitRxBlkRing(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct RTMP_RX_RING *prRxRing = NULL;
	struct RTMP_DMACB *prRxCell;
	struct RX_BLK_DESC *prRxBlkD = NULL;
	uint32_t u4PhyAddr = 0, u4PhyAddrExt = 0, u4Idx, u4Num;

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	/* reset all RX Ring register */
	for (u4Num = 0; u4Num < MAWD_NUM_OF_RX_BLK_RING; u4Num++) {
		prRxRing = &prHifInfo->RxBlkRing[u4Num];

		prRxRing->hw_desc_base = prBusInfo->mawd_rx_blk_ctrl0;
		prRxRing->hw_cidx_addr = prBusInfo->mawd_rx_blk_ctrl2;
		prRxRing->hw_didx_addr = prBusInfo->mawd_rx_blk_ctrl2;
		prRxRing->hw_cnt_addr = prBusInfo->mawd_rx_blk_ctrl1;
#if CFG_ENABLE_MAWD_MD_RING
		if (u4Num > 0) {
			prRxRing->hw_desc_base =
				prBusInfo->mawd_md_rx_blk_ctrl0;
			prRxRing->hw_cidx_addr =
				prBusInfo->mawd_md_rx_blk_ctrl2;
			prRxRing->hw_didx_addr =
				prBusInfo->mawd_md_rx_blk_ctrl2;
			prRxRing->hw_cnt_addr =
				prBusInfo->mawd_md_rx_blk_ctrl1;
		}
#endif /* CFG_ENABLE_MAWD_MD_RING */
		prRxRing->hw_cidx_mask = BITS(0, 11);
		prRxRing->hw_cidx_shift = 0;
		prRxRing->hw_didx_mask = BITS(12, 23);
		prRxRing->hw_didx_shift = 12;
		prRxRing->hw_cnt_mask = BITS(16, 27);
		prRxRing->hw_cnt_shift = 16;

		u4PhyAddr = ((uint64_t)prRxRing->Cell[0].AllocPa &
			     DMA_LOWER_32BITS_MASK);
		u4PhyAddrExt = ((uint64_t)prRxRing->Cell[0].AllocPa >>
				DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK;
		prRxRing->RxCpuIdx = 0;
		prRxRing->RxDmaIdx = 0;
		prRxRing->u4MagicCnt = 0;
		HAL_MAWD_MCR_WR(prAdapter, prRxRing->hw_desc_base, u4PhyAddr);
		HAL_MAWD_MCR_WR(prAdapter, prRxRing->hw_cnt_addr,
			u4PhyAddrExt | (prRxRing->u4RingSize << 16));

		for (u4Idx = 0; u4Idx < prRxRing->u4RingSize; u4Idx++) {
			/* Init RX Ring Size, Va, Pa variables */
			prRxCell = &prRxRing->Cell[u4Idx];
			prRxBlkD = (struct RX_BLK_DESC *)prRxCell->AllocVa;
			prRxBlkD->magic_cnt = RX_BLK_MAGIC_CNT_NUM - 1;
		}

		DBGLOG(HAL, TRACE,
		       "-->RX_BLK_RING[0x%x]: Base=0x%x, Cnt=%d\n",
		       prRxRing->hw_desc_base,
		       u4PhyAddr, prRxRing->u4RingSize);
	}
}

static u_int8_t halRroHashAdd(struct GL_HIF_INFO *prHifInfo, uint64_t u8Key,
			      struct sk_buff *prSkb, struct RX_CTRL_BLK *prRcb)
{
	struct RCB_NODE *prNewNode;

	if (halRroHashSearch(prHifInfo, u8Key))
		return FALSE;

	if (hlist_empty(&prHifInfo->rRcbHTblFreeList))
		return FALSE;

	prNewNode = list_entry(prHifInfo->rRcbHTblFreeList.first,
			       struct RCB_NODE, rNode);
	hlist_del(prHifInfo->rRcbHTblFreeList.first);
	prNewNode->u8Key = u8Key;
	prNewNode->prSkb = prSkb;
	prNewNode->prRcb = prRcb;

	hash_add_rcu(prHifInfo->arRcbHTbl, &prNewNode->rNode, u8Key);

	return TRUE;
}

static u_int8_t halRroHashDel(struct GL_HIF_INFO *prHifInfo, uint64_t u8Key)
{
	struct RCB_NODE *prRcbNode;

	prRcbNode = halRroHashSearch(prHifInfo, u8Key);
	if (prRcbNode) {
		hash_del_rcu(&prRcbNode->rNode);
		hlist_add_head(&prRcbNode->rNode, &prHifInfo->rRcbHTblFreeList);
	}

	return prRcbNode != NULL;
}

static struct RCB_NODE *halRroHashSearch(
	struct GL_HIF_INFO *prHifInfo, uint64_t u8Key)
{
	struct RCB_NODE *prRcbNode;

	hash_for_each_possible_rcu(prHifInfo->arRcbHTbl,
				   prRcbNode, rNode, u8Key) {
		if (prRcbNode->u8Key == u8Key)
			return prRcbNode;
	}

	return NULL;
}

static void halRroAddNewRcbBlk(struct GL_HIF_INFO *prHifInfo,
			 struct sk_buff *prSkb, phys_addr_t rAddr)
{
	struct RX_CTRL_BLK *prRcb;

	prRcb = (struct RX_CTRL_BLK *)prSkb->cb;
	prRcb->rPhyAddr = rAddr;
	prRcb->prSkb = prSkb;

	list_add_tail(&prRcb->rNode, &prHifInfo->rRcbFreeList);
	prHifInfo->u4RcbFreeListCnt++;
}

struct RX_CTRL_BLK *halRroGetFreeRcbBlk(
	struct GL_HIF_INFO *prHifInfo,
	struct RTMP_DMABUF *pDmaBuf,
	uint32_t u4Idx)
{
	struct list_head *prNode;
	struct RX_CTRL_BLK *prRcb;

	if (list_empty(&prHifInfo->rRcbFreeList)) {
		DBGLOG(HAL, ERROR, "rcb list is empty\n");
		return NULL;
	}

	prNode = prHifInfo->rRcbFreeList.next;
	list_del(prNode);
	prHifInfo->u4RcbFreeListCnt--;

	list_add_tail(prNode, &prHifInfo->rRcbUsedList[u4Idx]);
	prHifInfo->u4RcbUsedListCnt[u4Idx]++;

	prRcb = list_entry(prNode, struct RX_CTRL_BLK, rNode);
	prRcb->u4Idx = u4Idx;
	pDmaBuf->AllocPa = prRcb->rPhyAddr;
	pDmaBuf->AllocVa = (void *)prRcb->prSkb->data;

	halRroHashAdd(prHifInfo, (uint64_t)prRcb->rPhyAddr,
		      prRcb->prSkb, prRcb);

	return prRcb;
}


static uint32_t halRroSearchPrtSnAddrElem(struct GL_HIF_INFO *prHifInfo,
					  uint64_t u8TargetAddr)
{
	struct RTMP_DMABUF *prAddrArray;
	struct RRO_ADDR_ELEM *prAddrElem;
	uint32_t u4Idx, u4AddrNum;
	uint64_t u8Addr;

	if (u8TargetAddr == 0)
		return RRO_MAX_WINDOW_NUM;

	prAddrArray = &prHifInfo->AddrArray;

	for (u4Idx = 0; u4Idx < RRO_MAX_WINDOW_NUM; u4Idx++) {
		u4AddrNum =
			RRO_TOTAL_ADDR_ELEM_NUM * RRO_MAX_WINDOW_NUM + u4Idx;
		prAddrElem = (struct RRO_ADDR_ELEM *)
			(prAddrArray->AllocVa +
			 u4AddrNum * sizeof(struct RRO_ADDR_ELEM));
		u8Addr = prAddrElem->elem0.addr_h;
		u8Addr = (u8Addr << 32) | prAddrElem->elem0.addr;
		if (u8Addr == u8TargetAddr)
			return u4Idx;
	}

	return RRO_MAX_WINDOW_NUM;
}

void halRroAllocRcbList(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct BUS_INFO *prBusInfo;
	struct WIFI_VAR *prWifiVar;
	struct HIF_MEM_OPS *prMemOps;
	struct sk_buff *prSkb = NULL;
	struct RTMP_DMABUF rDmaBuf;
	struct RCB_NODE *prNewNode;
	struct RTMP_DMABUF *prAddrArray;
	uint32_t u4Cnt, u4Idx, u4RxBufNum = 0;

	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;
	prMemOps = &prHifInfo->rMemOps;
	prAddrArray = &prHifInfo->AddrArray;

	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		INIT_LIST_HEAD(&prHifInfo->rRcbUsedList[u4Idx]);
		prHifInfo->u4RcbUsedListCnt[u4Idx] = 0;
	}
	INIT_LIST_HEAD(&prHifInfo->rRcbFreeList);
	prHifInfo->u4RcbFreeListCnt = 0;
	hash_init(prHifInfo->arRcbHTbl);
	INIT_HLIST_HEAD(&prHifInfo->rRcbHTblFreeList);

	u4RxBufNum = prHifInfo->u4RxDataRingSize * prBusInfo->rx_data_ring_num;
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRroPreFillRxRing))
		u4RxBufNum += prHifInfo->u4RxDataRingSize;

	rDmaBuf.AllocSize = CFG_RX_MAX_PKT_SIZE;
	for (u4Cnt = 0; u4Cnt < u4RxBufNum; u4Cnt++) {
		if (prMemOps->allocRxDataBuf)
			prSkb = prMemOps->allocRxDataBuf(
				prHifInfo, &rDmaBuf, 0, u4Cnt);
		if (!prSkb) {
			DBGLOG(HAL, ERROR,
			       "can't allocate rx %u size packet\n",
			       CFG_RX_MAX_PKT_SIZE);
			break;
		}
		halRroAddNewRcbBlk(prHifInfo, prSkb, rDmaBuf.AllocPa);

		prNewNode = kalMemAlloc(sizeof(struct RCB_NODE), VIR_MEM_TYPE);
		hlist_add_head(&prNewNode->rNode, &prHifInfo->rRcbHTblFreeList);
	}

	DBGLOG(HAL, TRACE, "Alloc Rcb[%d]", u4Cnt);
}

void halRroFreeRcbList(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RX_CTRL_BLK *prRcb;
	struct RCB_NODE *prRcbNode;
	struct hlist_node *prHCur, *prHNext;
	struct list_head *prCur, *prNext;
	uint32_t u4Idx;
	int i4Bkt = 0;

	prHifInfo = &prGlueInfo->rHifInfo;

	prMemOps = &prHifInfo->rMemOps;

	hash_for_each_safe(prHifInfo->arRcbHTbl, i4Bkt,
			   prHCur, prRcbNode, rNode) {
		hash_del_rcu(&prRcbNode->rNode);
		kalMemFree(prRcbNode, VIR_MEM_TYPE, sizeof(struct RCB_NODE));
	}

	hlist_for_each_safe(prHCur, prHNext, &prHifInfo->rRcbHTblFreeList) {
		prRcbNode = list_entry(prHCur, struct RCB_NODE, rNode);
		kalMemFree(prRcbNode, VIR_MEM_TYPE, sizeof(struct RCB_N1ODE));
	}

	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		list_for_each_safe(prCur, prNext,
				   &prHifInfo->rRcbUsedList[u4Idx]) {
			prRcb = list_entry(prCur, struct RX_CTRL_BLK, rNode);
			list_del(prCur);
			if (prMemOps->unmapRxBuf)
				prMemOps->unmapRxBuf(
					prHifInfo, prRcb->rPhyAddr,
					CFG_RX_MAX_PKT_SIZE);
			if (prMemOps->freePacket && prRcb->prSkb)
				prMemOps->freePacket(
					prHifInfo, (void *)prRcb->prSkb, 0);
		}
	}

	list_for_each_safe(prCur, prNext, &prHifInfo->rRcbFreeList) {
		prRcb = list_entry(prCur, struct RX_CTRL_BLK, rNode);
		list_del(prCur);
		if (prMemOps->unmapRxBuf)
			prMemOps->unmapRxBuf(
				prHifInfo, prRcb->rPhyAddr,
				CFG_RX_MAX_PKT_SIZE);
		if (prMemOps->freePacket && prRcb->prSkb)
			prMemOps->freePacket(
				prHifInfo, (void *)prRcb->prSkb, 0);
	}
	prHifInfo->u4RcbFreeListCnt = 0;

	DBGLOG(HAL, TRACE, "Release Rcb List");
}

void halRroResetRcbList(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RX_CTRL_BLK *prRcb;
	struct RCB_NODE *prRcbNode;
	struct hlist_node *prHCur;
	struct list_head *prCur, *prNext;
	uint32_t u4Idx = 0;
	int i4Bkt = 0;

	prHifInfo = &prGlueInfo->rHifInfo;

	prMemOps = &prHifInfo->rMemOps;

	hash_for_each_safe(prHifInfo->arRcbHTbl, i4Bkt,
			   prHCur, prRcbNode, rNode) {
		hash_del_rcu(&prRcbNode->rNode);
		hlist_add_head(&prRcbNode->rNode, &prHifInfo->rRcbHTblFreeList);
	}

	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		list_for_each_safe(prCur, prNext,
				   &prHifInfo->rRcbUsedList[u4Idx]) {
			prRcb = list_entry(prCur, struct RX_CTRL_BLK, rNode);
			list_del(prCur);
			list_add_tail(&prRcb->rNode, &prHifInfo->rRcbFreeList);
			prHifInfo->u4RcbFreeListCnt++;
		}
		prHifInfo->u4RcbUsedListCnt[u4Idx] = 0;
	}

	DBGLOG(HAL, TRACE, "Reset Rcb List");
}

void halRroTurnOff(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4Addr = 0, u4Val = 0;

	u4Addr = WF_RRO_TOP_GLOBAL_CONFG_ADDR;
	u4Val != BITS(0, 1);
	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4Val);
}

static void halRroSetup(struct GLUE_INFO *prGlueInfo)
{
	struct WIFI_VAR *prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

	halRroSetupBaBitmap(prGlueInfo);
	halRroSetupAddressElement(prGlueInfo);
	halRroSetupIndicateCmdRing(prGlueInfo);
	halRroSetupTimeoutConfig(prGlueInfo);

	halRroWfdmaInit(prGlueInfo);
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd))
		halRroMawdInit(prGlueInfo);
}

void halRroInit(struct GLUE_INFO *prGlueInfo)
{
	struct WIFI_VAR *prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawdTx))
		halMawdInitTxRing(prGlueInfo);

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro)) {
		halRroSetup(prGlueInfo);
		if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd))
			halMawdInitRxBlkRing(prGlueInfo);
	} else if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro2Md)) {
		halRroSetup(prGlueInfo);
	}

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd))
		halMawdInitSram(prGlueInfo);
}

void halRroUninit(struct GLUE_INFO *prGlueInfo)
{
}

void halOffloadAllocMem(struct GLUE_INFO *prGlueInfo, u_int8_t fgAllocMem)
{
	struct WIFI_VAR *prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawdTx))
		halMawdAllocTxRing(prGlueInfo, TRUE);

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro)) {
		halRroAllocMem(prGlueInfo, fgAllocMem);
		if (!fgAllocMem)
			halRroFreeRcbList(prGlueInfo);
		halRroAllocRcbList(prGlueInfo);
		if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd)) {
			halMawdAllocRxBlkRing(prGlueInfo, TRUE, 0);
#if CFG_ENABLE_MAWD_MD_RING
			halMawdAllocRxBlkRing(prGlueInfo, TRUE, 1);
#endif
		}
	} else if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro2Md)) {
		halRroAllocMem(prGlueInfo, fgAllocMem);
	}
}

void halOffloadFreeMem(struct GLUE_INFO *prGlueInfo)
{
	struct WIFI_VAR *prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawdTx))
		halMawdTxFreeMem(prGlueInfo);

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro)) {
		halRroFreeMem(prGlueInfo);
		halRroFreeRcbList(prGlueInfo);
		if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd))
			halMawdFreeRxBlkRing(prGlueInfo);
	} else if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro2Md)) {
		halRroFreeMem(prGlueInfo);
	}
}

uint32_t halMawdGetRxBlkDoneCntByMagicCnt(struct GLUE_INFO *prGlueInfo,
					  uint32_t u4Num)
{
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_RX_RING *prRxRing;
	struct RTMP_DMACB *prRxCell;
	struct RX_BLK_DESC *prRxBlkD;
	uint32_t u4Cnt = 0, u4CpuIdx = 0, u4MagicCnt = 0;

	prHifInfo = &prGlueInfo->rHifInfo;
	prRxRing = &prHifInfo->RxBlkRing[u4Num];
	u4CpuIdx = prRxRing->RxCpuIdx;
	u4MagicCnt = prRxRing->u4MagicCnt;

	for (u4Cnt = 0; u4Cnt < prRxRing->u4RingSize; u4Cnt++) {
		prRxCell = &prRxRing->Cell[u4CpuIdx];
		prRxBlkD = (struct RX_BLK_DESC *)prRxCell->AllocVa;
		if (u4MagicCnt != prRxBlkD->magic_cnt)
			break;

		INC_RING_INDEX(u4CpuIdx, prRxRing->u4RingSize);
		if (u4CpuIdx == 0)
			INC_RING_INDEX(u4MagicCnt, RX_BLK_MAGIC_CNT_NUM);
	}

	return u4Cnt;
}

uint32_t halMawdGetRxBlkDoneCnt(struct GLUE_INFO *prGlueInfo,
				uint32_t u4Num)
{
	struct ADAPTER *prAdapter;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct RTMP_RX_RING *prRxRing = NULL;
	uint32_t u4MaxCnt = 0, u4CpuIdx = 0, u4DmaIdx = 0, u4RxPktCnt;

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	prRxRing = &prHifInfo->RxBlkRing[u4Num];
	HAL_GET_MAWD_RING_DIDX(prAdapter, prRxRing, &prRxRing->RxDmaIdx);
	u4MaxCnt = prRxRing->u4RingSize;
	u4CpuIdx = prRxRing->RxCpuIdx;
	u4DmaIdx = prRxRing->RxDmaIdx;

	if (u4CpuIdx > u4DmaIdx)
		u4RxPktCnt = u4MaxCnt + u4DmaIdx - u4CpuIdx;
	else
		u4RxPktCnt = u4DmaIdx - u4CpuIdx;

	return u4RxPktCnt;
}

void halUpdateRFBInfo(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	struct GLUE_INFO *prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct RX_DESC_OPS_T *prRxDescOps;
	void *prRxStatus;

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	prRxDescOps = prChipInfo->prRxDescOps;
	prRxStatus = prSwRfb->prRxStatus;

	if (!prRxStatus) {
		DBGLOG(HAL, ERROR, "prRxStatus is NULL\n");
		return;
	}

	NIC_DUMP_RXD_HEADER(prAdapter, "Dump RXD:\n");
	NIC_DUMP_RXD(prAdapter, (uint8_t *)prRxStatus, prChipInfo->rxd_size);

	if (prRxDescOps->nic_rxd_get_pkt_type) {
		prSwRfb->ucPacketType =
			prRxDescOps->nic_rxd_get_pkt_type(prRxStatus);
	}
#if DBG
	if (prRxDescOps->nic_rxd_get_sec_mode) {
		DBGLOG_LIMITED(RX, LOUD, "ucPacketType = %u, ucSecMode = %u\n",
			       prSwRfb->ucPacketType,
			       prRxDescOps->nic_rxd_get_sec_mode(prRxStatus));
	}
#endif /* DBG */

	GLUE_RX_SET_PKT_INT_TIME(prSwRfb->pvPacket,
				 prGlueInfo->u8HifIntTime);
	GLUE_RX_SET_PKT_RX_TIME(prSwRfb->pvPacket, sched_clock());

	if (prRxDescOps->nic_rxd_get_wlan_idx) {
		prSwRfb->ucStaRecIdx =
			secGetStaIdxByWlanIdx(
				prAdapter,
				prRxDescOps->nic_rxd_get_wlan_idx(prRxStatus));
	}
}

static u_int8_t halRroHandleRxRcb(
	struct ADAPTER *prAdapter, struct RX_CTRL_BLK *prRcb,
	uint32_t u4Reason, uint32_t *au4RingCnt,
	struct QUE *prFreeSwRfbList, struct QUE *prRecvRfbList)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_DMACB rRxCell;
	struct RXD_STRUCT rRxD;
	struct RTMP_DMABUF rDmaBuf;
	struct RX_CTRL *prRxCtrl;
	struct SW_RFB *prSwRfb;
	struct sk_buff *prSkb;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prRxCtrl = &prAdapter->rRxCtrl;

	QUEUE_REMOVE_HEAD(prFreeSwRfbList, prSwRfb, struct SW_RFB *);
	if (!prSwRfb) {
		DBGLOG_LIMITED(RX, WARN, "No More RFB\n");
		return FALSE;
	}

	list_del(&prRcb->rNode);
	prHifInfo->u4RcbUsedListCnt[prRcb->u4Idx]--;
	au4RingCnt[prRcb->u4Idx]++;
	if (!halRroHashDel(prHifInfo, prRcb->rPhyAddr)) {
		DBGLOG(RX, ERROR, "Delete hash failed[0x%llx]!!!\n",
		       prRcb->rPhyAddr);
	}

	rDmaBuf.AllocSize = CFG_RX_MAX_PKT_SIZE;
	rDmaBuf.AllocPa = prRcb->rPhyAddr;
	rDmaBuf.AllocVa = prRcb->prSkb->data;
	rRxCell.pPacket = (void *)prRcb->prSkb;
	rRxCell.AllocVa = (void *)&rRxD;
	rRxD.SDLen0 = CFG_RX_MAX_PKT_SIZE;

	/* For RRO, PTR is updated later inside halRroUpdateWfdmaRxBlk c*/
	if (prMemOps->copyRxData &&
	    !prMemOps->copyRxData(prHifInfo, &rRxCell, &rRxD, prSwRfb, FALSE)) {
		DBGLOG_LIMITED(RX, ERROR, "Read Rxblk fail\n");
		QUEUE_INSERT_TAIL(prFreeSwRfbList, &prSwRfb->rQueEntry);
		return FALSE;
	}

	prSkb = (struct sk_buff *)rRxCell.pPacket;
	halRroAddNewRcbBlk(prHifInfo, prSkb, rDmaBuf.AllocPa);

	prSwRfb->pucRecvBuff = ((struct sk_buff *)prSwRfb->pvPacket)->data;
	prSwRfb->prRxStatus = (void *)prSwRfb->pucRecvBuff;
	prSwRfb->u4IndReason = u4Reason;

	halUpdateRFBInfo(prAdapter, prSwRfb);

#if RRO_DROP_BY_HIF
	if (u4Reason == RRO_REPEAT || u4Reason == RRO_OLDPKT) {
		QUEUE_INSERT_TAIL(prFreeSwRfbList, &prSwRfb->rQueEntry);
		DBGLOG(HAL, DEBUG, "Drop skb by hif[%d]\n", u4Reason);
	} else {
		halRxInsertRecvRfbList(prAdapter, prRecvRfbList, prSwRfb);
	}
#else
	halRxInsertRecvRfbList(prAdapter, prRecvRfbList, prSwRfb);
#endif

	GLUE_INC_REF_CNT(prAdapter->rHifStats.u4DataRxCount);

	RX_INC_CNT(prRxCtrl, RX_MPDU_TOTAL_COUNT);
	DBGLOG(RX, TEMP, "Recv p=%p total:%lu\n",
	       prSwRfb, RX_GET_CNT(prRxCtrl, RX_MPDU_TOTAL_COUNT));
	kalTraceEvent("Recv p=%p total:%lu",
		      prSwRfb, RX_GET_CNT(prRxCtrl, RX_MPDU_TOTAL_COUNT));

	return TRUE;
}

struct RRO_MAC_DESC {
	uint32_t u4DW0;
	uint32_t u4DW1;
	uint32_t u4DW2;
	uint32_t u4DW3;
	uint32_t u4DW4;
	uint32_t u4DW5;
	uint32_t u4DW6;
	uint32_t u4DW7;
	uint32_t u4DW8;
	uint32_t u4DW9;
	uint32_t u4DW10;
};

static uint32_t halRroGetSn(void *prRxD)
{
	struct RRO_MAC_DESC *prRroMacD = (struct RRO_MAC_DESC *)prRxD;

	return (prRroMacD->u4DW10 & 0xFFFF) >> 4;
}

static void halRroSearchIndCmd(struct ADAPTER *prAdapter,
			       uint32_t u4Session, uint32_t u4StartSn)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prIndCmd;
	struct RRO_IND_CMD *aurIndCmd;
	uint32_t u4Idx;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prIndCmd = &prHifInfo->IndCmdRing;
	aurIndCmd = (struct RRO_IND_CMD *)prIndCmd->AllocVa;

	for (u4Idx = 0; u4Idx < RRO_IND_CMD_RING_SIZE; u4Idx++) {
		if (aurIndCmd[u4Idx].session_id == u4Session &&
		    aurIndCmd[u4Idx].start_sn == u4StartSn) {
			DBGLOG(HAL, ERROR, "Dump IndCmd[%u]!\n", u4Idx);
			dumpMemory32((uint32_t *)&aurIndCmd[u4Idx],
				     sizeof(struct RRO_IND_CMD));
		}
	}
}

#define RRO_ADDR_ELEM_DUMP_CNT 3
static void halRroSearchAddrElem(struct ADAPTER *prAdapter,
				 uint64_t u8TargetAddr)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prAddrArray;
	struct RRO_ADDR_ELEM *prAddrElem;
	uint32_t u4Idx, u4Id, u4AddrNum;
	uint64_t u8Addr;

	if (u8TargetAddr == 0)
		return;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prAddrArray = &prHifInfo->AddrArray;

	for (u4Id = 0; u4Id <= RRO_TOTAL_ADDR_ELEM_NUM; u4Id++) {
		for (u4Idx = 0; u4Idx < RRO_MAX_WINDOW_NUM; u4Idx++) {
			u4AddrNum = u4Id * RRO_MAX_WINDOW_NUM +	u4Idx;
			prAddrElem = (struct RRO_ADDR_ELEM *)
				(prAddrArray->AllocVa +
				 u4AddrNum * sizeof(struct RRO_ADDR_ELEM));

#if CFG_SUPPORT_RX_PAGE_POOL
			u8Addr = prAddrElem->elem1.addr_h;
			u8Addr = (u8Addr << 32) | prAddrElem->elem1.addr;
			if ((u8Addr & 0xf0000000) == 0x90000000) {
				void *rAddr = phys_to_virt((phys_addr_t)u8Addr);

				if (rAddr) {
					DBGLOG(HAL, DEBUG, "Dump RXD:\n");
					dumpMemory32(rAddr, 64);
				}
			}
#endif
			u8Addr = prAddrElem->elem0.addr_h;
			u8Addr = (u8Addr << 32) | prAddrElem->elem0.addr;
			if (u8Addr != u8TargetAddr)
				continue;

			DBGLOG(HAL, ERROR,
			       "Dump AddrElem id[%u]sn[%u]!\n",
			       u4Id, u4Idx);
			if (u4Idx < RRO_ADDR_ELEM_DUMP_CNT) {
				uint32_t u4DumpCnt =
					RRO_ADDR_ELEM_DUMP_CNT - u4Idx;

				prAddrElem = (struct RRO_ADDR_ELEM *)
					(prAddrArray->AllocVa +
					 (RRO_MAX_WINDOW_NUM - u4DumpCnt - 1) *
					 sizeof(struct RRO_ADDR_ELEM));
				dumpMemory32((uint32_t *)prAddrElem,
					     sizeof(struct RRO_ADDR_ELEM) *
					     (RRO_ADDR_ELEM_DUMP_CNT * 2 + 1));
				u4AddrNum = 0;
			} else {
				u4AddrNum -= RRO_ADDR_ELEM_DUMP_CNT;
			}
			prAddrElem = (struct RRO_ADDR_ELEM *)
				(prAddrArray->AllocVa +
				 u4AddrNum * sizeof(struct RRO_ADDR_ELEM));
			dumpMemory32((uint32_t *)prAddrElem,
				     sizeof(struct RRO_ADDR_ELEM) *
				     (RRO_ADDR_ELEM_DUMP_CNT * 2 + 1));
			halRroSearchIndCmd(prAdapter, u4Id, u4Idx);
		}
	}
}

static u_int8_t halRroFixAmsduError(
	struct ADAPTER *prAdapter,
	struct RX_CTRL_BLK **pprRcb,
	uint32_t *pu4MsduCnt)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RX_CTRL_BLK *prRcb;
	struct sk_buff *prSkb;
	uint32_t u4Idx, u4Pf;
	uint32_t u4MsduCnt = *pu4MsduCnt;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;

	if (u4MsduCnt <= 1)
		return FALSE;

	prRcb = *pprRcb;
	prSkb = prRcb->prSkb;

	if (prMemOps->unmapRxBuf) {
		prMemOps->unmapRxBuf(
			prHifInfo, prRcb->rPhyAddr,
			CFG_RX_MAX_PKT_SIZE);
	}

	u4Pf = HAL_MAC_CONNAC3X_RX_STATUS_GET_PAYLOAD_FORMAT(
		(struct RRO_MAC_DESC *)prSkb->data);

	if (prMemOps->mapRxBuf) {
		prRcb->rPhyAddr = prMemOps->mapRxBuf(
			prHifInfo, prSkb->data, 0,
			CFG_RX_MAX_PKT_SIZE);
	}

	if (u4Pf != 0)
		return FALSE;

	for (u4Idx = 0; u4Idx < u4MsduCnt - 1; u4Idx++) {
		if (prRcb->rNode.prev ==
		    &prHifInfo->rRcbUsedList[prRcb->u4Idx]) {
			DBGLOG(HAL, DEBUG, "it's link head\n");
			*pu4MsduCnt -= u4MsduCnt - 1 - u4Idx;
			prHifInfo->u4RcbHeadCnt++;
			break;
		}
		prRcb = list_entry(
			prRcb->rNode.prev,
			struct RX_CTRL_BLK, rNode);
	}

	*pprRcb = prRcb;
	prHifInfo->u4RcbFixCnt++;

	return TRUE;
}

static void halRroDebugGetSnAndPf(
	struct ADAPTER *prAdapter,
	struct RX_CTRL_BLK *prRcb,
	uint32_t u4MsduCnt,
	uint32_t *pu4Pos,
	char **paucBuf)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RX_CTRL_BLK *prCurRcb, *prNextRcb;
	struct sk_buff *prSkb;
	uint32_t u4Idx, u4Pf, u4Sn;
	uint32_t u4BufSize = 1024, pos = *pu4Pos;
	char *aucBuf = *paucBuf;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;

	/* get skb SN & PF */
	prCurRcb = prRcb;
	for (u4Idx = 0; u4Idx < u4MsduCnt; u4Idx++) {
		prNextRcb = list_entry(
			prCurRcb->rNode.next,
			struct RX_CTRL_BLK, rNode);
		prSkb = prCurRcb->prSkb;

		if (prMemOps->unmapRxBuf) {
			prMemOps->unmapRxBuf(
				prHifInfo, prCurRcb->rPhyAddr,
				CFG_RX_MAX_PKT_SIZE);
		}

		u4Sn = halRroGetSn(prSkb->data);
		u4Pf = HAL_MAC_CONNAC3X_RX_STATUS_GET_PAYLOAD_FORMAT(
			(struct RRO_MAC_DESC *)prSkb->data);
		if (u4Idx == 0) {
			pos += kalSnprintf(
				aucBuf + pos, u4BufSize - pos,
				"[0x%llx]Sn[%u]Pf[%u]",
				prCurRcb->rPhyAddr, u4Sn, u4Pf);
		} else {
			pos += kalSnprintf(
				aucBuf + pos, u4BufSize - pos,
				" -> [0x%llx]Sn[%u]Pf[%u]",
				prCurRcb->rPhyAddr, u4Sn, u4Pf);
		}

		if (prMemOps->mapRxBuf) {
			prCurRcb->rPhyAddr = prMemOps->mapRxBuf(
				prHifInfo, prSkb->data, 0,
				CFG_RX_MAX_PKT_SIZE);
		}
		prCurRcb = prNextRcb;
	}
	*pu4Pos = pos;
}

static u_int8_t halRroDebugCheckSnAndPf(
	struct ADAPTER *prAdapter,
	struct RX_CTRL_BLK *prRcb,
	uint32_t u4MsduCnt)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RX_CTRL_BLK *prCurRcb, *prNextRcb;
	struct sk_buff *prSkb;
	uint32_t u4Idx, u4Pf, u4Sn;
	uint32_t u4BufSize = 1024, pos = 0;
	char *aucBuf;
	u_int8_t fgRet = FALSE, fgErr = FALSE;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;

	aucBuf = (char *)kalMemAlloc(u4BufSize, VIR_MEM_TYPE);
	if (!aucBuf) {
		DBGLOG(HAL, ERROR, "alloc buf fail\n");
		return fgRet;
	}
	kalMemZero(aucBuf, u4BufSize);

	/* get SN & PF */
	halRroDebugGetSnAndPf(prAdapter, prRcb, u4MsduCnt, &pos, &aucBuf);
	u4Sn = halRroGetSn(prRcb->prSkb->data);

	prCurRcb = prRcb;
	/* check skb SN & PF */
	for (u4Idx = 0; u4Idx < u4MsduCnt; u4Idx++) {
		prNextRcb = list_entry(
			prCurRcb->rNode.next,
			struct RX_CTRL_BLK, rNode);
		prSkb = prCurRcb->prSkb;
		fgErr = FALSE;

		u4Pf = HAL_MAC_CONNAC3X_RX_STATUS_GET_PAYLOAD_FORMAT(
			(struct RRO_MAC_DESC *)prSkb->data);

		if (u4MsduCnt > 1) {
			if (u4Idx == 0) {
				if (u4Pf != 3) {
					pos += kalSnprintf(
						aucBuf + pos, u4BufSize - pos,
						", [%u]u4Pf != 3", u4Idx);
					fgErr = TRUE;
				}
			} else if (u4Idx == (u4MsduCnt - 1)) {
				if (u4Pf != 1) {
					pos += kalSnprintf(
						aucBuf + pos, u4BufSize - pos,
						", [%u]u4Pf != 1", u4Idx);
					fgErr = TRUE;
				}
			} else {
				if (u4Pf != 2) {
					pos += kalSnprintf(
						aucBuf + pos, u4BufSize - pos,
						", [%u]u4Pf != 2", u4Idx);
					fgErr = TRUE;
				}
			}
		} else {
			if (u4Pf != 0) {
				pos += kalSnprintf(
					aucBuf + pos, u4BufSize - pos,
					", [%u]u4Pf != 0", u4Idx);
				fgErr = TRUE;
			}
		}

		if (u4Sn != halRroGetSn(prSkb->data)) {
			pos += kalSnprintf(
				aucBuf + pos, u4BufSize - pos,
				", [%u]Sn not match", u4Idx);
			fgErr = TRUE;
		}

		if (fgErr) {
			halRroSearchAddrElem(
				prAdapter, (uint64_t)prCurRcb->rPhyAddr);
			dumpMemory32((uint32_t *)prSkb->data, 0x8C);
			fgRet = TRUE;
		}

		prCurRcb = prNextRcb;
	}

	if (fgRet)
		DBGLOG(HAL, DEBUG, "%s\n", aucBuf);

	kalMemFree(aucBuf, VIR_MEM_TYPE, u4BufSize);

	return fgRet;
}

static void halRroDebugCheckPrevRcbList(
	struct ADAPTER *prAdapter, struct RX_CTRL_BLK *prRcb)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RX_CTRL_BLK *prCurRcb, *prPreRcb;
	struct sk_buff *prSkb;
	uint32_t u4Idx, u4Pf, u4Sn;
	uint32_t u4BufSize = 1024, pos = 0;
	char *aucBuf;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;

	if (prRcb->rNode.prev ==
	    &prHifInfo->rRcbUsedList[prRcb->u4Idx]) {
		DBGLOG(HAL, DEBUG, "it's link head\n");
		return;
	}

	aucBuf = (char *)kalMemAlloc(u4BufSize, VIR_MEM_TYPE);
	if (!aucBuf) {
		DBGLOG(HAL, ERROR, "alloc buf fail\n");
		return;
	}
	kalMemZero(aucBuf, u4BufSize);

	prCurRcb = prRcb;
	/* check previous rcb list */
	for (u4Idx = 0; u4Idx < 8; u4Idx++) {
		prPreRcb = list_entry(
			prCurRcb->rNode.prev,
			struct RX_CTRL_BLK, rNode);
		prSkb = prPreRcb->prSkb;
		if (!prSkb) {
			DBGLOG(HAL, DEBUG,
			       "prSkb == NULL, it's link head\n");
			break;
		}

		if (prMemOps->unmapRxBuf) {
			prMemOps->unmapRxBuf(
				prHifInfo, prPreRcb->rPhyAddr,
				CFG_RX_MAX_PKT_SIZE);
		}

		u4Sn = halRroGetSn(prSkb->data);
		u4Pf = HAL_MAC_CONNAC3X_RX_STATUS_GET_PAYLOAD_FORMAT(
			(struct RRO_MAC_DESC *)prSkb->data);
		if (u4Idx == 0) {
			pos += kalSnprintf(
				aucBuf + pos, u4BufSize - pos,
				"PrevLink [0x%llx]Sn[%u]Pf[%u]",
				prPreRcb->rPhyAddr, u4Sn, u4Pf);
		} else {
			pos += kalSnprintf(
				aucBuf + pos, u4BufSize - pos,
				" -> [0x%llx]Sn[%u]Pf[%u]",
				prPreRcb->rPhyAddr, u4Sn, u4Pf);
		}
		dumpMemory32((uint32_t *)prSkb->data, 64);

		if (prMemOps->mapRxBuf) {
			prPreRcb->rPhyAddr = prMemOps->mapRxBuf(
				prHifInfo, prSkb->data, 0,
				CFG_RX_MAX_PKT_SIZE);
		}

		if (u4Pf == 0 || u4Pf == 3)
			break;
		prCurRcb = prPreRcb;
	}

	DBGLOG(HAL, DEBUG, "%s\n", aucBuf);
	kalMemFree(aucBuf, VIR_MEM_TYPE, u4BufSize);
}

static u_int8_t halRroDebugPreLookingRxList(
	struct ADAPTER *prAdapter,
	struct RX_CTRL_BLK *prRcb,
	uint32_t *pu4MsduCnt)
{
	u_int8_t fgRet = FALSE;
	uint32_t u4MsduCnt = *pu4MsduCnt;

	fgRet = halRroDebugCheckSnAndPf(prAdapter, prRcb, u4MsduCnt);
	if (fgRet)
		halRroDebugCheckPrevRcbList(prAdapter, prRcb);

	return fgRet;
}

static u_int8_t halRroHandleRxRcbMsdu(
	struct ADAPTER *prAdapter,
	struct RX_CTRL_BLK *prRcb,
	uint32_t u4Reason,
	uint32_t u4MsduCnt,
	uint32_t *au4RingCnt,
	struct QUE *prFreeSwRfbList,
	struct QUE *prRecvRfbList)
{
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_VAR *prWifiVar;
	struct RX_CTRL_BLK *prCurRcb, *prNextRcb;
	uint32_t u4Idx;

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRroDbg))
		halRroDebugPreLookingRxList(prAdapter, prRcb, &u4MsduCnt);

	if (prChipInfo->is_en_fix_rro_amsdu_error)
		halRroFixAmsduError(prAdapter, &prRcb, &u4MsduCnt);

	prCurRcb = prRcb;
	for (u4Idx = 0; u4Idx < u4MsduCnt; u4Idx++) {
		prNextRcb = list_entry(
			prCurRcb->rNode.next,
			struct RX_CTRL_BLK, rNode);
		if (!halRroHandleRxRcb(prAdapter, prCurRcb,
				       u4Reason, au4RingCnt,
				       prFreeSwRfbList, prRecvRfbList))
			return FALSE;
		prCurRcb = prNextRcb;
	}

	return TRUE;
}

void halRroDumpRcb(struct ADAPTER *prAdapter,
		   struct RX_BLK_DESC *prRxBlkD,
		   uint64_t u8Addr)
{
	if (prRxBlkD) {
		DBGLOG(HAL, DEBUG, "Dump RxBlkD:\n");
		dumpMemory32((uint32_t *)prRxBlkD, sizeof(struct RX_BLK_DESC));
	}

	halRroSearchAddrElem(prAdapter, u8Addr);

#if CFG_SUPPORT_RX_PAGE_POOL
	if ((u8Addr & 0xf0000000) == 0x90000000) {
		void *rAddr = phys_to_virt((phys_addr_t)u8Addr);

		if (rAddr) {
			DBGLOG(HAL, DEBUG, "Dump RXD:\n");
			dumpMemory32(rAddr, 64);
		}
	}
#endif
}

void halRroDumpDebugInfo(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct WIFI_VAR *prWifiVar;
	struct RTMP_DMABUF *prRxDesc;
	struct RTMP_DMABUF *prIndCmd, *prAddrArray;
	struct RRO_IND_CMD *aurIndCmd;
	struct RRO_ADDR_ELEM *prAddrElem;
	uint32_t u4Addr, u4Val = 0, u4DmaIdx;
	uint32_t u4Id, u4Sn, u4AddrNum;

	prHifInfo = &prGlueInfo->rHifInfo;
	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;
	prIndCmd = &prHifInfo->IndCmdRing;
	prAddrArray = &prHifInfo->AddrArray;
	prRxDesc = &prHifInfo->RxBlkDescRing[0];
	aurIndCmd = (struct RRO_IND_CMD *)prIndCmd->AllocVa;

	connac3x_show_rro_info(prGlueInfo->prAdapter);

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd)) {
		DBGLOG(HAL, DEBUG, "Mawd RXD\n");
		dumpMemory32((uint32_t *)prRxDesc->AllocVa,
			     sizeof(struct RX_BLK_DESC) * 4095);
	}

	u4Addr = WF_RRO_TOP_IND_CMD_0_CTRL3_ADDR;
	HAL_RMCR_RD(OFFLOAD_DBG, prGlueInfo->prAdapter, u4Addr, &u4Val);
	u4DmaIdx = u4Val & WF_RRO_TOP_IND_CMD_0_CTRL3_DMA_IDX_MASK;

	u4Id = aurIndCmd[u4DmaIdx].session_id;
	u4Sn = aurIndCmd[u4DmaIdx].start_sn;
	DBGLOG(HAL, DEBUG, "IndCmd[0x%x] Id[%u] Sn[%u]\n",
	       u4DmaIdx, u4Id, u4Sn);
	dumpMemory32((uint32_t *)prIndCmd->AllocVa,
		     RRO_IND_CMD_RING_SIZE * sizeof(struct RRO_IND_CMD));

	u4Id = 0x10;
	DBGLOG(HAL, DEBUG, "AddrArray[%u]\n", u4Id);
	u4AddrNum = u4Id * RRO_MAX_WINDOW_NUM;
	prAddrElem = (struct RRO_ADDR_ELEM *)
		(prAddrArray->AllocVa +
		 u4AddrNum * sizeof(struct RRO_ADDR_ELEM));
	dumpMemory32((uint32_t *)prAddrElem,
		     sizeof(struct RRO_ADDR_ELEM) * RRO_MAX_WINDOW_NUM);

	u4Id = RRO_MAX_STA_NUM * RRO_MAX_TID_NUM;
	DBGLOG(HAL, DEBUG, "AddrArray[%u]\n", u4Id);
	u4AddrNum = u4Id * RRO_MAX_WINDOW_NUM;
	prAddrElem = (struct RRO_ADDR_ELEM *)
		(prAddrArray->AllocVa +
		 u4AddrNum * sizeof(struct RRO_ADDR_ELEM));
	dumpMemory32((uint32_t *)prAddrElem,
		     sizeof(struct RRO_ADDR_ELEM) * RRO_MAX_WINDOW_NUM);
}

static u_int8_t halRroHandleReadRxBlk(
	struct GLUE_INFO *prGlueInfo,
	struct RX_BLK_DESC *prRxBlkD,
	uint32_t *au4RingCnt,
	struct QUE *prFreeSwRfbList,
	struct QUE *prRecvRfbList)
{
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct WIFI_VAR *prWifiVar;
	struct RX_CTRL_BLK *prRcb;
	struct RCB_NODE *prRcbHNode;
	uint64_t u8Addr;

	prHifInfo = &prGlueInfo->rHifInfo;
	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;
	prMemOps = &prHifInfo->rMemOps;

	u8Addr = prRxBlkD->addr_h;
	u8Addr = (u8Addr << 32) | prRxBlkD->addr;
	prRcbHNode = halRroHashSearch(prHifInfo, u8Addr);
	if (!prRcbHNode) {
		prHifInfo->u4RcbErrorCnt++;
		DBGLOG(HAL, ERROR,
		       "Cannot find RCB[0x%llx], ignore it\n",
		       u8Addr);
		halRroDumpRcb(prGlueInfo->prAdapter, prRxBlkD, u8Addr);
		if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRroDbg))
			halRroDumpDebugInfo(prGlueInfo);
		return TRUE;
	}
	if (!prRcbHNode->prSkb) {
		DBGLOG(HAL, ERROR, "skb is null\n");
		dumpMemory32((uint32_t *)prRxBlkD, sizeof(struct RX_BLK_DESC));
		return FALSE;
	}
	prRcb = prRcbHNode->prRcb;
	if (!prRcb) {
		DBGLOG(HAL, ERROR, "rcb is null\n");
		dumpMemory32((uint32_t *)prRxBlkD, sizeof(struct RX_BLK_DESC));
		return FALSE;
	}

	if (!halRroHandleRxRcbMsdu(
		    prGlueInfo->prAdapter, prRcb,
		    prRxBlkD->ind_reason, prRxBlkD->msdu_cnt,
		    au4RingCnt, prFreeSwRfbList, prRecvRfbList))
		return FALSE;

	return TRUE;
}

static u_int8_t halMawdWaitMagicCnt(struct ADAPTER *prAdapter,
				    struct RX_BLK_DESC *prRxBlkD,
				    uint32_t u4MagicCnt,
				    uint32_t u4Num)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_RX_RING *prRxRing;
	uint32_t u4Count = 0;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prRxRing = &prHifInfo->RxBlkRing[u4Num];

	for (u4Count = 0; u4MagicCnt != prRxBlkD->magic_cnt; u4Count++) {
		if (u4Count > DMA_DONE_WAITING_COUNT) {
			DBGLOG(HAL, ERROR,
			"Magic cnt mismatch RxBlkD[%u] exp[%u] cidx[0x%x]\n",
			       prRxBlkD->magic_cnt, u4MagicCnt,
			       prRxRing->RxCpuIdx);
			dumpMemory32((uint32_t *)prRxBlkD,
				     sizeof(struct RX_BLK_DESC));
			connac3x_show_rro_info(prGlueInfo->prAdapter);
			return FALSE;
		}

		kalUdelay(DMA_DONE_WAITING_TIME);
	}
	return TRUE;
}

static void halRroGetFreeSwRfbList(struct ADAPTER *prAdapter,
				   struct QUE *prFreeSwRfbList,
				   uint32_t u4RxCnt)
{
	struct RX_CTRL *prRxCtrl = &prAdapter->rRxCtrl;
	uint32_t u4MaxRfbCnt = u4RxCnt * MAWD_AMSDU_MAX_CNT;

#if CFG_DYNAMIC_RFB_ADJUSTMENT
	if (RX_GET_FREE_RFB_CNT(prRxCtrl) < u4MaxRfbCnt)
		nicRxIncRfbCnt(prAdapter);
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */

	NIC_RX_DEQUEUE_FREE_QUE(prAdapter, u4MaxRfbCnt, prFreeSwRfbList,
		RFB_TRACK_HIF);

	if (prFreeSwRfbList->u4NumElem < u4MaxRfbCnt) {
		DBGLOG_LIMITED(
			RX, WARN,
			"No More RFB for MAWD, RxCnt:%u, RfbCnt:%u, Ind:%u\n",
			u4RxCnt,
			prFreeSwRfbList->u4NumElem,
			RX_GET_INDICATED_RFB_CNT(prRxCtrl));
	}
}

static void halMawdReadRxBlkRing(
	struct ADAPTER *prAdapter,
	uint32_t *au4RingCnt,
	struct QUE *prFreeSwRfbList,
	struct QUE *prRecvRfbList,
	uint32_t u4Num)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_RX_RING *prRxRing;
	struct RTMP_DMACB *prRxCell;
	struct RX_BLK_DESC rRxBlkD, *prRxBlkD;
	struct RX_CTRL *prRxCtrl;
	uint32_t u4RxCnt, u4RxDoneCnt = 0;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prRxCtrl = &prAdapter->rRxCtrl;
	prRxRing = &prHifInfo->RxBlkRing[u4Num];

#if MAWD_READ_COUNT_BY_EMI
	u4RxCnt = halMawdGetRxBlkDoneCntByMagicCnt(prGlueInfo, u4Num);
#else
	u4RxCnt = halMawdGetRxBlkDoneCnt(prGlueInfo, u4Num);
#endif
	DBGLOG(RX, TEMP, "halMawdReadRxBlks: u4RxCnt:%d\n", u4RxCnt);
	if (!u4RxCnt)
		goto end;

	halRroGetFreeSwRfbList(prAdapter, prFreeSwRfbList, u4RxCnt);

	for (u4RxDoneCnt = 0; u4RxDoneCnt < u4RxCnt; u4RxDoneCnt++) {
#if CFG_SUPPORT_RX_NAPI
		/* if fifo exhausted, stop deQ and schedule NAPI */
		if (prGlueInfo->prRxDirectNapi &&
			KAL_FIFO_IS_FULL(&prGlueInfo->rRxKfifoQ)) {
			RX_INC_CNT(prRxCtrl, RX_NAPI_FIFO_FULL_COUNT);
			kalNapiSchedule(prAdapter);
			break;
		}
#endif /* CFG_SUPPORT_RX_NAPI */

		prRxCell = &prRxRing->Cell[prRxRing->RxCpuIdx];
		prRxBlkD = (struct RX_BLK_DESC *)prRxCell->AllocVa;
		if (prFreeSwRfbList->u4NumElem < prRxBlkD->msdu_cnt)
			break;

#if (MAWD_READ_COUNT_BY_EMI == 0)
		if (!halMawdWaitMagicCnt(prAdapter, prRxBlkD,
					 prRxRing->u4MagicCnt,
					 u4Num))
			break;
#endif

		/* copy to cache memory */
		kalMemCopyFromIo(&rRxBlkD, prRxBlkD,
				 sizeof(struct RX_BLK_DESC));
		if (!halRroHandleReadRxBlk(prGlueInfo, &rRxBlkD, au4RingCnt,
					   prFreeSwRfbList, prRecvRfbList))
			break;

		INC_RING_INDEX(prRxRing->RxCpuIdx, prRxRing->u4RingSize);
		if (prRxRing->RxCpuIdx == 0)
			INC_RING_INDEX(prRxRing->u4MagicCnt,
				       RX_BLK_MAGIC_CNT_NUM);
	}

#if CFG_SUPPORT_RX_NAPI_THREADED
	if (prGlueInfo->prRxDirectNapi &&
	    !KAL_FIFO_IS_EMPTY(&prGlueInfo->rRxKfifoQ))
		kalNapiSchedule(prAdapter);
#endif
	HAL_SET_MAWD_RING_CIDX(prAdapter, prRxRing, prRxRing->RxCpuIdx);

end:
	prRxRing->u4PendingCnt = u4RxCnt - u4RxDoneCnt;

	if (prRxRing->u4PendingCnt == 0)
		KAL_CLR_BIT(RX_RRO_DATA, prAdapter->ulNoMoreRfb);
	else
		KAL_SET_BIT(RX_RRO_DATA, prAdapter->ulNoMoreRfb);
}

static u_int8_t halRroWaitMagicCnt(struct ADAPTER *prAdapter,
			       struct RRO_IND_CMD *prIndCmd,
			       uint32_t u4MagicCnt)
{
	uint32_t u4Count = 0;

	for (u4Count = 0; u4MagicCnt != prIndCmd->magic_cnt; u4Count++) {
		if (u4Count > DMA_DONE_WAITING_COUNT) {
			DBGLOG(HAL, ERROR,
			       "Magic cnt mismatch IndCmd[%u] exp[%u]\n",
			       prIndCmd->magic_cnt, u4MagicCnt);
			dumpMemory32((uint32_t *)prIndCmd,
				     sizeof(struct RRO_IND_CMD));
			return FALSE;
		}

		kalUdelay(DMA_DONE_WAITING_TIME);
	}
	return TRUE;
}

static void halRroReadIndCmd(
	struct ADAPTER *prAdapter,
	uint32_t *au4RingCnt,
	struct QUE *prFreeSwRfbList,
	struct QUE *prRecvRfbList)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prIndCmd, *prAddrArray;
	struct RRO_IND_CMD *aurIndCmd;
	struct RRO_ADDR_ELEM *prAddrElem;
	struct RX_BLK_DESC rRxBlkD;
	union RRO_ACK_SN_CMD rAckSn;
	uint32_t u4Addr, u4Val = 0, u4DmaIdx, u4IndCmdIdx, u4Idx;
	uint32_t u4Id, u4Sn, u4AddrNum;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prIndCmd = &prHifInfo->IndCmdRing;
	prAddrArray = &prHifInfo->AddrArray;
	aurIndCmd = (struct RRO_IND_CMD *)prIndCmd->AllocVa;

	u4Addr = WF_RRO_TOP_IND_CMD_0_CTRL3_ADDR;
	HAL_RMCR_RD(OFFLOAD_READ, prAdapter, u4Addr, &u4Val);
	u4DmaIdx = prHifInfo->u4IndCmdDmaIdx;
	u4IndCmdIdx = u4Val & WF_RRO_TOP_IND_CMD_0_CTRL3_DMA_IDX_MASK;

	while (u4DmaIdx != u4IndCmdIdx) {
		u4Id = aurIndCmd[u4DmaIdx].session_id;
		u4Sn = aurIndCmd[u4DmaIdx].start_sn;

#if CFG_SUPPORT_RX_NAPI
		/* if fifo exhausted, stop deQ and schedule NAPI */
		if (prGlueInfo->prRxDirectNapi &&
			KAL_FIFO_IS_FULL(&prGlueInfo->rRxKfifoQ)) {
			RX_INC_CNT(&prAdapter->rRxCtrl,
				   RX_NAPI_FIFO_FULL_COUNT);
			kalNapiSchedule(prAdapter);
			break;
		}
#endif /* CFG_SUPPORT_RX_NAPI */

		if (!halRroWaitMagicCnt(prAdapter, &aurIndCmd[u4DmaIdx],
					prHifInfo->u4RroMagicCnt))
			break;

		halRroGetFreeSwRfbList(prAdapter, prFreeSwRfbList,
				       aurIndCmd[u4DmaIdx].ind_cnt);

		for (u4Idx = 0; u4Idx < aurIndCmd[u4DmaIdx].ind_cnt; u4Idx++) {
			u4AddrNum = u4Id * RRO_MAX_WINDOW_NUM +
				(u4Sn + u4Idx) % RRO_MAX_WINDOW_NUM;
			prAddrElem = (struct RRO_ADDR_ELEM *)
				(prAddrArray->AllocVa +
				 u4AddrNum * sizeof(struct RRO_ADDR_ELEM));
			if (prAddrElem->elem0.signature !=
			    ((u4Sn + u4Idx) / RRO_MAX_WINDOW_NUM)) {
				kalMemSet(prAddrElem, 0xff,
					  sizeof(struct RRO_ADDR_ELEM));
				continue;
			}

			rRxBlkD.addr = prAddrElem->elem0.addr;
			rRxBlkD.addr_h = prAddrElem->elem0.addr_h;
			rRxBlkD.msdu_cnt = prAddrElem->elem0.msdu_cnt;
			rRxBlkD.out_of_range = prAddrElem->elem0.out_of_range;
			rRxBlkD.ind_reason = aurIndCmd[u4DmaIdx].ind_reason;
			rRxBlkD.magic_cnt = aurIndCmd[u4DmaIdx].magic_cnt;

			if (!halRroHandleReadRxBlk(
				    prGlueInfo, &rRxBlkD, au4RingCnt,
				    prFreeSwRfbList, prRecvRfbList)) {
				break;
			}
		}
		if (u4Idx != aurIndCmd[u4DmaIdx].ind_cnt) {
			DBGLOG(HAL, ERROR, "Handle RxBlk failed[%u][%u]\n",
			       u4Idx, aurIndCmd[u4DmaIdx].ind_cnt);
			break;
		}

		rAckSn.field.session_id = u4Id;
		rAckSn.field.ack_sn = (u4Sn + u4Idx - 1) % 4096;
		rAckSn.field.is_last = 1;

		u4Addr = WF_RRO_TOP_ACK_SN_CTRL_ADDR;
		HAL_MCR_WR(prAdapter, WF_RRO_TOP_ACK_SN_CTRL_ADDR,
			       rAckSn.word);
		DBGLOG(HAL, TRACE, "update ind cmd[0x%08x]\n", rAckSn.word);

		INC_RING_INDEX(u4DmaIdx, RRO_IND_CMD_RING_SIZE);
		if (u4DmaIdx == 0)
			INC_RING_INDEX(prHifInfo->u4RroMagicCnt,
				       INDCMD_MAGIC_CNT_NUM);
	}

	if (u4DmaIdx == u4IndCmdIdx)
		KAL_CLR_BIT(RX_RRO_DATA, prAdapter->ulNoMoreRfb);
	else
		KAL_SET_BIT(RX_RRO_DATA, prAdapter->ulNoMoreRfb);

	prHifInfo->u4IndCmdDmaIdx = u4DmaIdx;
}

void halRroReadRxData(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct WIFI_VAR *prWifiVar;
	struct RX_CTRL *prRxCtrl;
	struct QUE rFreeSwRfbList, rRecvRfbList;
	struct QUE *prFreeSwRfbList = NULL, *prRecvRfbList = NULL;
	uint32_t au4RingCnt[NUM_OF_RX_RING] = {0};
	uint32_t u4Idx, u4TotalCnt = 0;
	struct RTMP_RX_RING *prRxRing;

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prWifiVar = &prAdapter->rWifiVar;

	prRxCtrl = &prAdapter->rRxCtrl;
	prFreeSwRfbList = &rFreeSwRfbList;
	prRecvRfbList = &rRecvRfbList;
	QUEUE_INITIALIZE(prFreeSwRfbList);
	QUEUE_INITIALIZE(prRecvRfbList);

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd)) {
		halMawdReadRxBlkRing(prAdapter, au4RingCnt,
				     prFreeSwRfbList, prRecvRfbList, 0);
#if CFG_ENABLE_MAWD_MD_RING
		halMawdReadRxBlkRing(prAdapter, au4RingCnt,
				     prFreeSwRfbList, prRecvRfbList, 1);
#endif
	} else {
		halRroReadIndCmd(prAdapter, au4RingCnt,
				 prFreeSwRfbList, prRecvRfbList);
	}

	if (prFreeSwRfbList->u4NumElem)
		NIC_RX_CONCAT_FREE_QUE(prAdapter, prFreeSwRfbList);
	if (prRecvRfbList->u4NumElem)
		NIC_RX_CONCAT_RX_QUE(prAdapter, prRecvRfbList);

	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		prRxRing = &prHifInfo->RxRing[u4Idx];

		prRxRing->u4TotalCnt += au4RingCnt[u4Idx];
		u4TotalCnt += au4RingCnt[u4Idx];
	}

	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		uint32_t u4Res = au4RingCnt[u4Idx];

		if (!halIsDataRing(RX_RING, u4Idx))
			continue;

		if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRroPreFillRxRing) &&
		    (prHifInfo->u4RcbFreeListCnt >= u4TotalCnt) &&
		    (prHifInfo->u4RcbFreeListCnt - u4TotalCnt > u4Res))
			u4Res = prHifInfo->u4RcbFreeListCnt - u4TotalCnt;

		halRroUpdateWfdmaRxBlk(prGlueInfo, u4Idx, u4Res);
		u4TotalCnt -= au4RingCnt[u4Idx];
	}
}

void halRroUpdateWfdmaRxBlk(struct GLUE_INFO *prGlueInfo,
			    uint16_t u2Port, uint32_t u4ResCnt)
{
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_RX_RING *prRxRing;
	struct RTMP_DMABUF *pDmaBuf;
	struct RTMP_DMACB *prRxCell;
	struct RXD_STRUCT rRxD, *pRxD;
	struct RX_CTRL_BLK *prRcb;
	uint32_t u4CpuIdx, u4Cnt, u4Idx;

	if (u4ResCnt == 0)
		return;

	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prRxRing = &prHifInfo->RxRing[u2Port];
	u4Cnt = halWpdmaGetRxDmaDoneCnt(prGlueInfo, u2Port);
	if (u4Cnt == 0)
		return;

	DBGLOG(HAL, TRACE, "update rx blk[%d] cnt[%d] res[%d]\n",
	       u2Port, u4Cnt, u4ResCnt);

	if (u4ResCnt < u4Cnt)
		u4Cnt = u4ResCnt;

	pRxD = &rRxD;
	kalMemZero(pRxD, sizeof(struct RXD_STRUCT));

	u4CpuIdx = prRxRing->RxCpuIdx;
	for (u4Idx = 0; u4Idx < u4Cnt; u4Idx++) {
		INC_RING_INDEX(u4CpuIdx, prRxRing->u4RingSize);
		if (u4CpuIdx == 0) {
			INC_RING_INDEX(prRxRing->u4MagicCnt,
				       WFDMA_MAGIC_CNT_NUM);
		}

		prRxCell = &prRxRing->Cell[u4CpuIdx];
		pDmaBuf = &prRxCell->DmaBuf;
		prRcb = halRroGetFreeRcbBlk(prHifInfo, pDmaBuf, u2Port);
		if (!prRcb) {
			DBGLOG(HAL, ERROR, "no rcb resource\n");
			break;
		}

		prRxCell->pPacket = prRcb->prSkb;

		halUpdateRxDmadSdp(pRxD, pDmaBuf);

		pRxD->SDLen0 = pDmaBuf->AllocSize;
		pRxD->DMADONE = 0;
		pRxD->MagicCnt = prRxRing->u4MagicCnt;

		kalMemCopyToIo(prRxCell->AllocVa, pRxD,
			       sizeof(struct RXD_STRUCT));
		INC_RING_INDEX(prRxRing->RxCpuIdx, prRxRing->u4RingSize);
	}

	HAL_SET_RING_CIDX(prGlueInfo->prAdapter,
			  prRxRing, prRxRing->RxCpuIdx);
}

static void halMawdReadSram(
	struct GLUE_INFO *prGlueInfo,
	uint32_t u4Offset,
	uint32_t *pu4ValL,
	uint32_t *pu4ValH)
{
	struct ADAPTER *prAdapter;
	struct BUS_INFO *prBusInfo;
	uint32_t u4Val = 0, u4Idx;

	prAdapter = prGlueInfo->prAdapter;
	prBusInfo = prAdapter->chip_info->bus_info;

	HAL_MAWD_MCR_RD(prAdapter, prBusInfo->mawd_settings2, &u4Val);
	u4Val = (u4Val & BITS(16, 25)) | (u4Offset & BITS(0, 7));
	HAL_MAWD_MCR_WR(prAdapter, prBusInfo->mawd_settings2, u4Val);

	HAL_MAWD_MCR_WR(prAdapter, prBusInfo->mawd_settings4, BIT(1));
	for (u4Idx = 0; u4Idx < MAWD_POWER_UP_RETRY_CNT; u4Idx++) {
		HAL_MAWD_MCR_RD(prAdapter, prBusInfo->mawd_settings4, &u4Val);
		if ((u4Val & BIT(8)) == 0)
			break;
		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
	}
	HAL_MAWD_MCR_RD(prAdapter, prBusInfo->mawd_settings5, pu4ValL);
	HAL_MAWD_MCR_RD(prAdapter, prBusInfo->mawd_settings6, pu4ValH);
}

static void halMawdInitSram(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4Val = 0, u4Addr = 0, u4Idx = 0;

	for (u4Idx = 0; u4Idx < MAWD_CR_BACKUP_NUM; u4Idx++) {
		u4Addr = MAWD_REG_BASE + u4Idx * 4;
		HAL_MAWD_MCR_RD(prGlueInfo->prAdapter, u4Addr, &u4Val);
		halMawdBackupCr(prGlueInfo, u4Addr, u4Val);
	}
}

static void halMawdUpdateSram(
	struct GLUE_INFO *prGlueInfo,
	uint32_t u4Offset,
	uint32_t u4ValL,
	uint32_t u4ValH)
{
	struct ADAPTER *prAdapter;
	struct BUS_INFO *prBusInfo;
	uint32_t u4Val = 0, u4Idx;

	prAdapter = prGlueInfo->prAdapter;
	prBusInfo = prAdapter->chip_info->bus_info;

	HAL_MAWD_MCR_WR(prAdapter, prBusInfo->mawd_settings0, u4ValL);
	HAL_MAWD_MCR_WR(prAdapter, prBusInfo->mawd_settings1, u4ValH);
	HAL_MAWD_MCR_RD(prAdapter, prBusInfo->mawd_settings2, &u4Val);
	u4Val = (u4Val & BITS(16, 25)) | (u4Offset & BITS(0, 7));
	HAL_MAWD_MCR_WR(prAdapter, prBusInfo->mawd_settings2, u4Val);

	HAL_MAWD_MCR_WR(prAdapter, prBusInfo->mawd_settings4, BIT(0));
	for (u4Idx = 0; u4Idx < MAWD_POWER_UP_RETRY_CNT; u4Idx++) {
		HAL_MAWD_MCR_RD(prAdapter, prBusInfo->mawd_settings4, &u4Val);
		if ((u4Val & BIT(8)) == 0)
			break;
		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
	}

	DBGLOG(HAL, TRACE, "Update SRAM[%d] H[0x%08x] L[0x%08x]",
	       u4Offset, u4ValH, u4ValL);
}

void halMawdDumpSram(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4BackupOffset = MAWD_CR_BACKUP_OFFSET_VER_1_1;
	uint32_t u4ValL, u4ValH, u4Idx, u4Num;

	if (kalGetMawdVer() == MAWD_VER_1_0)
		u4BackupOffset = MAWD_CR_BACKUP_OFFSET_VER_1_0;

	u4Num = u4BackupOffset + MAWD_CR_BACKUP_NUM / 2 + 1;
	for (u4Idx = 0; u4Idx < u4Num; u4Idx++) {
		halMawdReadSram(prGlueInfo, u4Idx, &u4ValL, &u4ValH);
		DBGLOG(HAL, DEBUG, "Read SRAM[%d] H[0x%08x] L[0x%08x]",
		       u4Idx, u4ValH, u4ValL);
	}
}

static void halMawdBackupCr(struct GLUE_INFO *prGlueInfo,
			    uint32_t u4Addr, uint32_t u4Val)
{
	uint32_t u4CrNum, u4Offset;
	uint32_t u4ValL, u4ValH;
	uint32_t u4BackupOffset = MAWD_CR_BACKUP_OFFSET_VER_1_1;

	if (kalGetMawdVer() == MAWD_VER_1_0)
		u4BackupOffset = MAWD_CR_BACKUP_OFFSET_VER_1_0;

	u4CrNum = (u4Addr - MAWD_REG_BASE) >> 2;
	u4Offset = (u4BackupOffset + 1) + (u4CrNum / 2);

	halMawdReadSram(prGlueInfo, u4Offset, &u4ValL, &u4ValH);
	if (u4CrNum % 2)
		u4ValH = u4Val;
	else
		u4ValL = u4Val;
	halMawdUpdateSram(prGlueInfo, u4Offset, u4ValL, u4ValH);

	halMawdReadSram(prGlueInfo, u4BackupOffset, &u4ValL, &u4ValH);
	if (u4CrNum >= 32)
		u4ValH |= BIT(u4CrNum - 32);
	else
		u4ValL |= BIT(u4CrNum);
	halMawdUpdateSram(prGlueInfo, u4BackupOffset, u4ValL, u4ValH);
}

static void halMawdSetupTxRing(struct GLUE_INFO *prGlueInfo,
			      struct RTMP_TX_RING *prWfdmaTxRing,
			      struct RTMP_TX_RING *prTxRing,
			      uint32_t u4Idx,
			      uint32_t u4PhyAddr,
			      uint32_t u4PhyAddrExt)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct RTMP_DMACB *prTxCell = &prTxRing->Cell[0];
	uint32_t u4Base = u4Idx, u4Val = 0, u4Addr, u4DWCnt;
	uint32_t u4HifTxdOffset = u4Base * 0xC;
	uint32_t u4WfdmaOffset = u4Base * 0x14;
	uint32_t u4MawdWfdmaAddrOffset = MAWD_WFDMA_ADDR_OFFSET;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	u4MawdWfdmaAddrOffset -=
		(uint32_t)(prChipInfo->u8CsrOffset & BITS(0, 31));

	u4Addr = prBusInfo->mawd_ring_ctrl0 + u4WfdmaOffset;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4PhyAddr);
	u4Addr = prBusInfo->mawd_ring_ctrl1 + u4WfdmaOffset;
	u4Val = prWfdmaTxRing->hw_cidx_addr - u4MawdWfdmaAddrOffset;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);
	u4Addr = prBusInfo->mawd_ring_ctrl2 + u4WfdmaOffset;
	u4Val = prWfdmaTxRing->hw_didx_addr - u4MawdWfdmaAddrOffset;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);
	u4Addr = prBusInfo->mawd_ring_ctrl3 + u4WfdmaOffset;
	u4Val = (TX_RING_DATA_SIZE << 19) | (TXD_SIZE << 12) |
		(u4PhyAddrExt << 8) | (MAWD_WFDMA_HIGH_ADDR << 4) |
		MAWD_WFDMA_HIGH_ADDR;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	prTxRing->hw_desc_base =
		prBusInfo->mawd_hif_txd_ctrl0 + u4HifTxdOffset;

	prTxRing->hw_cidx_addr =
		prBusInfo->mawd_hif_txd_ctrl2 + u4HifTxdOffset;
	prTxRing->hw_cidx_mask = BITS(0, 12);
	prTxRing->hw_cidx_shift = 0;

	prTxRing->hw_didx_addr =
		prBusInfo->mawd_hif_txd_ctrl2 + u4HifTxdOffset;
	prTxRing->hw_didx_mask = BITS(16, 28);
	prTxRing->hw_didx_shift = 16;

	prTxRing->hw_cnt_addr =
		prBusInfo->mawd_hif_txd_ctrl1 + u4HifTxdOffset;
	prTxRing->hw_cnt_mask = BITS(16, 28);
	prTxRing->hw_cnt_shift = 16;

	HAL_MAWD_MCR_WR(prAdapter, prTxRing->hw_desc_base,
		       prTxCell->AllocPa);
	/* setup tx ring/hif txd size */
	u4DWCnt = BYTE_TO_DWORD(NIC_TX_DESC_LONG_FORMAT_LENGTH +
				prChipInfo->hif_txd_append_size);
	u4Val = (prTxCell->AllocPa >> DMA_BITS_OFFSET) &
		DMA_HIGHER_4BITS_MASK |
		(prTxRing->u4RingSize << 16) | (u4DWCnt << 8);
	HAL_MAWD_MCR_WR(prAdapter, prTxRing->hw_cnt_addr, u4Val);
}

static void halMawdInitErrRptRing(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct GL_HIF_INFO *prHifInfo;
	struct BUS_INFO *prBusInfo;
	struct RTMP_DMABUF *prErrRpt;
	uint32_t u4Val;

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prErrRpt = &prHifInfo->ErrRptRing;

	HAL_MAWD_MCR_WR(prAdapter,
			prBusInfo->mawd_err_rpt_ctrl0,
			prErrRpt->AllocPa);
	u4Val = (prErrRpt->AllocPa >> DMA_BITS_OFFSET) &
		DMA_HIGHER_4BITS_MASK |
		(prHifInfo->u4RxEvtRingSize << 16) | (32 << 8);
	HAL_MAWD_MCR_WR(prAdapter,
			prBusInfo->mawd_err_rpt_ctrl1,
			u4Val);
}

void halMawdUpdateL2Tbl(struct GLUE_INFO *prGlueInfo,
		    union mawd_l2tbl rL2Tbl, uint32_t u4Set)
{
	struct ADAPTER *prAdapter;
	struct GL_HIF_INFO *prHifInfo;
	struct BUS_INFO *prBusInfo;
	uint32_t u4Idx = 0, u4Val = 0;

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	for (u4Idx = 0; u4Idx < 4; u4Idx++) {
		halMawdUpdateSram(prGlueInfo,
				  u4Set * 4 + u4Idx,
				  rL2Tbl.data[u4Idx * 2],
				  rL2Tbl.data[u4Idx * 2 + 1]);
	}

	HAL_MAWD_MCR_RD(prAdapter, prBusInfo->mawd_settings2, &u4Val);
	u4Val = (prHifInfo->u4MawdL2TblCnt << 21) |
		(30 << 16) |
		(u4Val & BITS(0, 20));
	HAL_MAWD_MCR_WR(prAdapter, prBusInfo->mawd_settings2, u4Val);

	HAL_MAWD_MCR_RD(prAdapter, prBusInfo->mawd_settings3, &u4Val);
	u4Val |= BIT(u4Set);
	HAL_MAWD_MCR_WR(prAdapter, prBusInfo->mawd_settings3, u4Val);
}

static u_int8_t halMawdAllocHifTxRing(struct GLUE_INFO *prGlueInfo,
				  uint32_t u4Num,
				  uint32_t u4Size,
				  uint32_t u4DescSize,
				  u_int8_t fgAllocMem)
{
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_TX_RING *pTxRing;
	struct RTMP_DMABUF *prTxDesc;
	struct RTMP_DMACB *prTxCell;
	phys_addr_t RingBasePa;
	void *RingBaseVa;
	uint32_t u4Idx;

	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prTxDesc = &prHifInfo->HifTxDescRing[u4Num];

	prTxDesc->AllocSize = u4Size * u4DescSize;
	if (fgAllocMem && prMemOps->allocExtBuf)
		prMemOps->allocExtBuf(prHifInfo, prTxDesc,
				      WFDMA_WB_MEMORY_ALIGNMENT);

	if (prTxDesc->AllocVa == NULL) {
		DBGLOG(HAL, ERROR,
		       "MawdTxDescRing[%d] allocation failed\n", u4Num);
		return FALSE;
	}

	/* Save PA & VA for further operation */
	RingBasePa = prTxDesc->AllocPa;
	RingBaseVa = prTxDesc->AllocVa;

	/*
	 * Initialize Tx Ring Descriptor and associated buffer memory
	 */
	pTxRing = &prHifInfo->MawdTxRing[u4Num];
	pTxRing->u4RingSize = u4Size;
	for (u4Idx = 0; u4Idx < u4Size; u4Idx++) {
		/* Init Tx Ring Size, Va, Pa variables */
		prTxCell = &pTxRing->Cell[u4Idx];
		prTxCell->pPacket = NULL;
		prTxCell->pBuffer = NULL;

		prTxCell->AllocSize = u4DescSize;
		prTxCell->AllocVa = RingBaseVa;
		prTxCell->AllocPa = RingBasePa;

		RingBasePa += u4DescSize;
		RingBaseVa += u4DescSize;
	}

	return TRUE;
}

u_int8_t halMawdAllocTxRing(struct GLUE_INFO *prGlueInfo, u_int8_t fgAllocMem)
{
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct RTMP_DMABUF *prErrRpt;
	int32_t u4Num, u4TxDSize;

	prHifInfo = &prGlueInfo->rHifInfo;
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prMemOps = &prHifInfo->rMemOps;
	prErrRpt = &prHifInfo->ErrRptRing;

	u4TxDSize = NIC_TX_DESC_LONG_FORMAT_LENGTH +
		     prChipInfo->hif_txd_append_size;

	for (u4Num = 0; u4Num < MAWD_MD_TX_RING_NUM; u4Num++) {
		if (!halMawdAllocHifTxRing(prGlueInfo, u4Num, TX_RING_DATA_SIZE,
					   u4TxDSize, fgAllocMem)) {
			DBGLOG(HAL, ERROR, "AllocMawdTxRing[%d] fail\n",
			       u4Num);
			return FALSE;
		}
		prHifInfo->MawdTxRing[u4Num].TxSwUsedIdx = 0;
		prHifInfo->MawdTxRing[u4Num].TxCpuIdx = 0;
	}

	prErrRpt->AllocSize = prHifInfo->u4RxEvtRingSize * 4;
	if (fgAllocMem && prMemOps->allocExtBuf)
		prMemOps->allocExtBuf(prHifInfo, prErrRpt,
				      WFDMA_MEMORY_ALIGNMENT);

	if (prErrRpt->AllocVa == NULL) {
		DBGLOG(HAL, ERROR, "ErrRpt allocation failed!!\n");
		return FALSE;
	}

	return TRUE;
}

void halMawdInitTxRing(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct RTMP_TX_RING *prTxRing = NULL;
	struct RTMP_TX_RING *prMawdTxRing = NULL;
	struct RTMP_DMACB *prTxCell = NULL;
	uint32_t u4Idx = 0, u4PhyAddr = 0, u4PhyAddrExt = 0, u4Val = 0;

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	for (u4Idx = 0; u4Idx < MAWD_MD_TX_RING_NUM; u4Idx++) {
#if CFG_ENABLE_MAWD_MD_RING
		prTxRing = &prHifInfo->TxRing[u4Idx + MAWD_TX_RING_OFFSET];
#else
		prTxRing = &prHifInfo->TxRing[u4Idx];
#endif
		prMawdTxRing = &prHifInfo->MawdTxRing[u4Idx];
		prTxCell = &prTxRing->Cell[0];
		u4PhyAddr = ((uint64_t)prTxCell->AllocPa) &
			DMA_LOWER_32BITS_MASK;
		u4PhyAddrExt = ((uint64_t)prTxCell->AllocPa >>
				DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK;

		halMawdSetupTxRing(prGlueInfo, prTxRing,
			prMawdTxRing, u4Idx, u4PhyAddr, u4PhyAddrExt);

		DBGLOG(HAL, TRACE,
		       "-->MAWD TX_RING_%d[0x%x]: Base=[0x%x][0x%x], Cnt=%d!\n",
		       u4Idx, prTxRing->hw_desc_base,
		       u4PhyAddrExt, u4PhyAddr, prMawdTxRing->u4RingSize);
	}

	HAL_MAWD_MCR_RD(prAdapter, MAWD_MISC_SETTING2, &u4Val);
	u4Val |= BIT(11);
	HAL_MAWD_MCR_WR(prAdapter, MAWD_MISC_SETTING2, u4Val);

	halMawdInitErrRptRing(prGlueInfo);
}

static uint8_t halMawdTxRingDataSelect(struct ADAPTER *prAdapter,
				       struct MSDU_INFO *prMsduInfo)
{
	return prMsduInfo->ucWmmQueSet;
}

u_int8_t halMawdFillTxRing(struct GLUE_INFO *prGlueInfo,
			   struct MSDU_TOKEN_ENTRY *prToken)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct mt66xx_chip_info *prChipInfo;
	struct RTMP_TX_RING *prTxRing, *prWfdmaTxRing;
	struct RTMP_DMACB *pTxCell;
	uint16_t u2Port = TX_RING_DATA0;

	prHifInfo = &prGlueInfo->rHifInfo;
	prChipInfo = prGlueInfo->prAdapter->chip_info;

	u2Port = halTxRingDataSelect(
		prGlueInfo->prAdapter, prToken->prMsduInfo);
	prTxRing = &prHifInfo->MawdTxRing[u2Port];
#if CFG_ENABLE_MAWD_MD_RING
	prWfdmaTxRing = &prHifInfo->TxRing[u2Port + MAWD_TX_RING_OFFSET];
#else
	prWfdmaTxRing = &prHifInfo->TxRing[u2Port];
#endif

	pTxCell = &prTxRing->Cell[prTxRing->TxCpuIdx];
	prToken->u4CpuIdx = prTxRing->TxCpuIdx;
	prToken->u2Port = u2Port;
	pTxCell->prToken = prToken;

	/* Increase TX_CTX_IDX, but write to register later. */
	INC_RING_INDEX(prTxRing->TxCpuIdx, prTxRing->u4RingSize);

	/* Update HW Tx DMA ring */
	prTxRing->u4UsedCnt++;
	prTxRing->u4TotalCnt++;
	prWfdmaTxRing->u4UsedCnt += 2;
	HAL_SET_MAWD_RING_CIDX(prGlueInfo->prAdapter,
			       prTxRing, prTxRing->TxCpuIdx);

	DBGLOG_LIMITED(HAL, TRACE,
		"MAWD Tx Data:Ring%d CPU idx[0x%x] Used[%u]\n",
		u2Port, prTxRing->TxCpuIdx, prTxRing->u4UsedCnt);

	GLUE_INC_REF_CNT(prGlueInfo->prAdapter->rHifStats.u4DataTxCount);

	return TRUE;
}

void halMawdReset(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	uint32_t u4Addr = 0, u4Val = 0, u4Idx, u4Cnt;
	uint32_t *au4MawdIdxPatch = au4MawdIdxPatchVer1_1;

	prAdapter = prGlueInfo->prAdapter;

#if CFG_MTK_ANDROID_WMT
	if (!is_cal_flow_finished())
		return;
#endif

	if (kalGetMawdVer() == MAWD_VER_1_0)
		au4MawdIdxPatch = au4MawdIdxPatchVer1_0;

	/* set mask */
	u4Addr = MAWD_MD_INTERRUPT_SETTING1;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0xFFFFFFFF);
	u4Addr = MAWD_AP_INTERRUPT_SETTING1;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0xFFFFFFFF);

	/* slp_prot enable */
	u4Addr = MAWD_AXI_SLEEP_PROT_SETTING;
	HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
	u4Val |= BIT(0);
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	/* clear cpu_idx */
	/* md_hiftxd_ring0/1/2_cpu_idx */
	u4Addr = MAWD_HIF_TXD_MD_CTRL2;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);
	u4Addr = MAWD_HIF_TXD_MD_CTRL5;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);
	u4Addr = MAWD_HIF_TXD_MD_CTRL8;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);
	/* err_rpt_cpu_idx */
	u4Addr = MAWD_ERR_RPT_CTRL2;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);
	/* ind_cmd_dma_idx_rro/mcu */
	u4Addr = MAWD_IND_CMD_SIGNATURE0;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);
	u4Addr = MAWD_IND_CMD_SIGNATURE1;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);
	/* md/ap_rx_blk_ring_cpu_idx */
	u4Addr = MAWD_AP_RX_BLK_CTRL2;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);
	u4Addr = MAWD_MD_RX_BLK_CTRL2;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);

	/* reset index */
	u4Addr = MAWD_IDX_REG_PATCH;
	for (u4Idx = 0; u4Idx < MAWD_MAX_PATCH_NUM; u4Idx++) {
		u4Val = BIT(31) | (u4Idx << 24) |
			au4MawdIdxPatch[u4Idx];
		HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);
	}

	/* set SW reset */
	u4Addr = MAWD_SOFTRESET;
	u4Val = BITS(0, 2);
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);

	/* polling idle */
	u4Addr = MAWD_POWER_UP;
	HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
	for (u4Cnt = 0; (u4Val & BITS(8, 12)) != BITS(8, 12); u4Cnt++) {
		if (u4Cnt > DMA_DONE_WAITING_COUNT) {
			DBGLOG(HAL, ERROR, "Wait MAWD ready timeout\n");
			break;
		}
		HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
	}

	/* release mask */
	u4Addr = MAWD_MD_INTERRUPT_SETTING1;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);
	u4Addr = MAWD_AP_INTERRUPT_SETTING1;
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, 0);

	/* release slp_prot */
	u4Addr = MAWD_AXI_SLEEP_PROT_SETTING;
	HAL_MAWD_MCR_RD(prAdapter, u4Addr, &u4Val);
	u4Val &= ~BIT(0);
	HAL_MAWD_MCR_WR(prAdapter, u4Addr, u4Val);
}

static u_int8_t __halMawdWakeup(void)
{
	uint32_t u4Addr = 0, u4Val = 0, u4Idx = 0;
	u_int8_t fgRet = TRUE;
	uint32_t u4ConnInfraId = kalGetConnInfraId();

	/* sequence 1 */
	u4Addr = MAWD_CR_OFFSET + 0x601A4;
	HAL_MCR_WR(NULL, u4Addr, 1);
	kalUdelay(200);
	u4Addr = MAWD_CR_OFFSET + 0x11000;
	HAL_RMCR_RD(OFFLOAD_HOST, NULL, u4Addr, &u4Val);
	for (u4Idx = 0; u4Idx < MAWD_POWER_UP_RETRY_CNT; u4Idx++) {
		if (u4Val == u4ConnInfraId)
			break;
		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
		HAL_RMCR_RD(OFFLOAD_HOST, NULL, u4Addr, &u4Val);
	}
	if (u4Idx == MAWD_POWER_UP_RETRY_CNT) {
		DBGLOG(HAL, ERROR, "polling ID fail[0x%08x != 0x%08x]\n",
		       u4ConnInfraId, u4Val);
		fgRet = FALSE;
		goto exit;
	}

	u4Addr = MAWD_CR_OFFSET + 0x1210;
	HAL_RMCR_RD(OFFLOAD_HOST, NULL, u4Addr, &u4Val);
	for (u4Idx = 0; u4Idx < MAWD_POWER_UP_RETRY_CNT; u4Idx++) {
		if (u4Val & BIT(16))
			break;
		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
		HAL_RMCR_RD(OFFLOAD_HOST, NULL, u4Addr, &u4Val);
	}
	if (u4Idx == MAWD_POWER_UP_RETRY_CNT) {
		DBGLOG(HAL, ERROR, "polling conn_infra_cfg_on ready fail\n");
		fgRet = FALSE;
		goto exit;
	}

	/* sequence 2 */
	u4Addr = MAWD_CR_OFFSET + 0x120A4;
	HAL_MCR_WR(NULL, u4Addr, BIT(0));
	HAL_RMCR_RD(OFFLOAD_HOST, NULL, u4Addr, &u4Val);

	/* sequence 3 */
	u4Addr = MAWD_CR_OFFSET + 0x120B4;
	HAL_MCR_WR(NULL, u4Addr, BIT(0));
	u4Addr = MAWD_CR_OFFSET + 0x11030;
	HAL_RMCR_RD(OFFLOAD_HOST, NULL, u4Addr, &u4Val);
	for (u4Idx = 0; u4Idx < MAWD_POWER_UP_RETRY_CNT; u4Idx++) {
		if (u4Val)
			break;
		kalUdelay(MAWD_POWER_UP_WAIT_TIME);
		HAL_RMCR_RD(OFFLOAD_HOST, NULL, u4Addr, &u4Val);
	}
	if (u4Idx == MAWD_POWER_UP_RETRY_CNT) {
		DBGLOG(HAL, ERROR, "polling conn_infra_cfg ready fail\n");
		fgRet = FALSE;
		goto exit;
	}

	u4Addr = MAWD_REG_PLL_CTRL_0;
	HAL_MAWD_MCR_WR(NULL, u4Addr, BIT(0));

exit:
	return fgRet;
}

static void __halMawdSleep(void)
{
	uint32_t u4Addr;

	/* sequence 3 */
	u4Addr = MAWD_REG_PLL_CTRL_0;
	HAL_MAWD_MCR_WR(NULL, u4Addr, 0);

	/* sequence 2 */

	/* sequence 1 */
	u4Addr = MAWD_CR_OFFSET + 0x601A4;
	HAL_MCR_WR(NULL, u4Addr, 0);
}

u_int8_t halMawdCheckInfra(struct ADAPTER *prAdapter)
{
	u_int8_t fgRet = TRUE;
#if (CFG_MTK_FPGA_PLATFORM == 0)
	struct GL_HIF_INFO *prHifInfo =
		&prAdapter->prGlueInfo->rHifInfo;
	uint32_t u4Addr, u4Val = 0;

	if (prHifInfo->fgIsMawdSuspend)
		return FALSE;

	u4Addr = MAWD_CR_OFFSET + 0x11000;
	HAL_RMCR_RD(OFFLOAD_HOST, prAdapter, u4Addr, &u4Val);
	if (u4Val != kalGetConnInfraId())
		fgRet = FALSE;
	DBGLOG(HAL, DEBUG, "CR [0x%08x]=[0x%08x]", u4Addr, u4Val);
#endif /* CFG_MTK_FPGA_PLATFORM == 0 */

	return fgRet;
}

int halMawdPwrOn(void)
{
	int ret = 0;

	if (!kalIsSupportMawd())
		goto exit;

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	ret = conninfra_pwr_on(CONNDRV_TYPE_MAWD);
	if (ret != 0) {
		DBGLOG(HAL, ERROR,
		       "conninfra pwr on failed, ret=%d\n", ret);
		goto exit;
	}
#endif
#if (MAWD_ENABLE_WAKEUP_SLEEP == 0) && (CFG_MTK_FPGA_PLATFORM == 0)
	if (!__halMawdWakeup()) {
		DBGLOG(HAL, ERROR, "Mawd power on fail\n");
		ret = -1;
	}
#endif
exit:
	return ret;
}

void halMawdPwrOff(void)
{
	if (!kalIsSupportMawd())
		return;

#if (MAWD_ENABLE_WAKEUP_SLEEP == 0) && (CFG_MTK_FPGA_PLATFORM == 0)
	__halMawdSleep();
#endif

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	conninfra_pwr_off(CONNDRV_TYPE_MAWD);
#endif
}

#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
