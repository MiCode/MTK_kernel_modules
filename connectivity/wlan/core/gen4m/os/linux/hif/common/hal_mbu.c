// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
/*
 * Id: @(#) hal_mbu.c@@
 */
/*  \file   hal_mbu.c
 *  \brief  The program provides HW MBU read HIF APIs
 *
 *  This file contains the support hw mbu read register
 */

#if CFG_MTK_WIFI_MBU
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

#ifdef MT6653
#include "coda/mt6653/cb_dma_top.h"
#include "coda/mt6653/cb_infra_mbu.h"
#include "coda/mt6653/cb_infra_rgu.h"
#endif

#if defined(CB_INFRA_MBU_BASE) && defined(CB_DMA_TOP_BASE) && \
	defined(CB_INFRA_RGU_BASE)

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define MAX_MBU_RB_WAITING_CNT	(1000) /* 1000 * 1us = 1ms timeout */
#define MAX_MBU_EMI_WAITING_CNT	(2000) /* 2000 * 5us = 10ms timeout */
#define MBU_MSI_MIRROR_IDX	7
#define MBU_TIMEOUT_VALUE	0xffffdead
#define MBU_REG_MASK		0xffff0000
#define MBU_TIMEOUT_DBG_CNT	1
#define MBU_DUMP_DBG_MAX_CNT	2

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

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

void halMbuMcrWr(struct ADAPTER *prAdapter, uint32_t u4Addr, uint32_t u4Val)
{
	struct SW_EMI_RING_INFO *prMbuInfo =
		&prAdapter->chip_info->bus_info->rSwEmiRingInfo;

	if ((u4Addr & MBU_REG_MASK) == prMbuInfo->u4RemapRegAddr)
		u4Addr = prMbuInfo->u4RemapBusAddr | (u4Addr & ~MBU_REG_MASK);

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

void halMbuInit(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct SW_EMI_RING_INFO *prMbuInfo;
	struct MBU_EMI_CTX *prEmi;
	struct HIF_MEM *prMem = NULL;
	uint64_t u8MsiMirrorAddr;
	uint32_t u4Addr = 0, u4Val = 0, i = 0;

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prMbuInfo = &prBusInfo->rSwEmiRingInfo;

	if (!prMbuInfo->fgIsSupport)
		return;

	if (prMemOps->getWifiMiscRsvEmi) {
		prMem = prMemOps->getWifiMiscRsvEmi(
			prChipInfo, WIFI_MISC_MEM_BLOCK_NON_MMIO);
	}

	if (!prMem || !prMem->va) {
		prMbuInfo->fgIsEnable = FALSE;
		DBGLOG(HAL, ERROR, "mbu init fail\n");
		return;
	}

	prEmi = (struct MBU_EMI_CTX *)prMem->va;
	prMbuInfo->prMbuEmiData = prEmi;
	u8MsiMirrorAddr = (uint64_t)prMem->pa +
		offsetof(struct MBU_EMI_CTX, arMsiMirror);

	kalMemSet(prEmi, 0, sizeof(struct MBU_EMI_CTX));
	prMbuInfo->fgIsEnable = TRUE;
	prMbuInfo->u4DumpDbgCrCnt = 0;

	/* set remap */
	if (prMbuInfo->u4RemapAddr) {
		HAL_MCR_WR(prAdapter, prMbuInfo->u4RemapAddr,
			   prMbuInfo->u4RemapVal);
	}

	DBGLOG(HAL, DEBUG, "base: pa:0x%llx, va:0x%llx\n",
	       prMem->pa, prMem->va);

	/* 1. [ALL] Program to unmask vector event for cb_infra_mbu */
	/* move to top pos */

	/* 2. TOP POS */
	/* Interrupt enable for IMR #0 (DMA#0) */
	u4Addr = CB_DMA_TOP_CB_DMA_0_G_DMA_0_INT_EN_INTEN_ADDR;
	u4Val = 0x1;
	halMbuMcrWr(prAdapter, u4Addr, u4Val);
	u4Addr = CB_DMA_TOP_CB_DMA_0_G_DMA_1_INT_EN_INTEN_ADDR;
	u4Val = 0x1;
	halMbuMcrWr(prAdapter, u4Addr, u4Val);
	/* Set device address for dummy read #0 */
	u4Addr = CB_DMA_TOP_CB_DMA_0_G_DMA_1_DST_ADDR_DST_ADDR_ADDR;
	u4Val = 0x7413B000;
	halMbuMcrWr(prAdapter, u4Addr, u4Val);

	/* 3. Driver Sequence */
	/* Set EMI destination address for DMA#0 */
	u4Addr = CB_DMA_TOP_CB_DMA_0_G_DMA_0_DST_ADDR_DST_ADDR_ADDR;
	u4Val = ((uint64_t)prMem->pa) & DMA_LOWER_32BITS_MASK;
	halMbuMcrWr(prAdapter, u4Addr, u4Val);
	u4Addr = CB_DMA_TOP_CB_DMA_0_G_DMA_0_DST_ADDR2_DST_ADDR2_ADDR;
	u4Val = ((uint64_t)prMem->pa >> DMA_BITS_OFFSET) | BIT(4);
	halMbuMcrWr(prAdapter, u4Addr, u4Val);

	/* Set EMI_addr as source address for dummy read #0 */
	u4Addr = CB_DMA_TOP_CB_DMA_0_G_DMA_1_SRC_ADDR_SRC_ADDR_ADDR;
	u4Val = ((uint64_t)prMem->pa) & DMA_LOWER_32BITS_MASK;
	halMbuMcrWr(prAdapter, u4Addr, u4Val);

	u4Addr = CB_DMA_TOP_CB_DMA_0_G_DMA_1_SRC_ADDR2_SRC_ADDR2_ADDR;
	u4Val = ((uint64_t)prMem->pa >> DMA_BITS_OFFSET) | BIT(4);
	halMbuMcrWr(prAdapter, u4Addr, u4Val);

	/* Set fetch address for cb_infra_mbu (i = 0~31) */
	for (i = 0; i < 32; i++) {
		u4Addr = CB_DMA_TOP_CB_INFRA_MBU_FHADDR_0_ADDR + i * 0x04;
		u4Val = i * 0x04 << 4;
		halMbuMcrWr(prAdapter, u4Addr, u4Val);
	}

	/* Set MSI mirror enable (i = 0~15): 0x0 or 0x1 */
	u4Addr = CB_DMA_TOP_CB_INFRA_MSI_MIRROR_EN_7_ADDR;
	u4Val = 0x1;
	halMbuMcrWr(prAdapter, u4Addr, u4Val);

	/* Set MSI mirror mode (i = 0~15): 0x0 or 0x1 */
	u4Addr = CB_DMA_TOP_CB_INFRA_MSI_MIRROR_MODE_7_ADDR;
	u4Val = 0x1;
	halMbuMcrWr(prAdapter, u4Addr, u4Val);

	/* [MSI] Set EMI address for MSI mirror */
	u4Addr = CB_DMA_TOP_CB_INFRA_MBU_RMP_CB_INFRA_MBU_RMP_ADDR;
	u4Val = ((u8MsiMirrorAddr >> 28) & BITS(0, 7)) | BIT(8);
	halMbuMcrWr(prAdapter, u4Addr, u4Val);
	u4Addr = CB_DMA_TOP_CB_INFRA_MBU_INTREG_2_CB_INFRA_MBU_INTREG_2_ADDR;
	u4Val = ((u8MsiMirrorAddr >> 28) & BITS(0, 7)) | BIT(8);
	halMbuMcrWr(prAdapter, u4Addr, u4Val);
	u4Addr = CB_DMA_TOP_CB_INFRA_MBU_INTREG_3_CB_INFRA_MBU_INTREG_3_ADDR;
	u4Val = (u8MsiMirrorAddr & BITS(0, 27)) | BITS(28, 31);
	halMbuMcrWr(prAdapter, u4Addr, u4Val);

	/* Program to unmask vector event for cb_infra_mbu */
	HAL_MCR_WR(prAdapter, 0x74130204, 0xFF7FFBF3);

	/* restore remap */
	if (prMbuInfo->u4RemapAddr) {
		HAL_MCR_WR(prAdapter, prMbuInfo->u4RemapAddr,
			   prMbuInfo->u4RemapDefVal);
	}
}

void halMbuUninit(struct GLUE_INFO *prGlueInfo)
{
	struct BUS_INFO *prBusInfo;
	struct SW_EMI_RING_INFO *prMbuInfo;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prMbuInfo = &prBusInfo->rSwEmiRingInfo;

	prMbuInfo->fgIsEnable = FALSE;
	prMbuInfo->prMbuEmiData = NULL;
}

u_int8_t halMbuRead4(struct GLUE_INFO *prGlueInfo, uint32_t u4ReadAddr,
		     uint32_t *pu4Val)
{
	uint32_t u4TmpVal = 0;

	return halMbuRead8(prGlueInfo, u4ReadAddr, pu4Val, &u4TmpVal);
}

u_int8_t halMbuRead8(struct GLUE_INFO *prGlueInfo, uint32_t u4ReadAddr,
		     uint32_t *pu4LowVal, uint32_t *pu4HighVal)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_VAR *prWifiVar;
	const struct PCIE_CHIP_CR_REMAPPING *prRemap;
	const struct ap2wf_remap *prAp2wf;
	struct SW_EMI_RING_INFO *prMbuInfo;
	struct MBU_EMI_CTX *prEmi;
	struct MBU_MSI_MIRROR *prMsiMirror = NULL;
	uint32_t u4Addr = 0, u4Val = 0, u4Cnt = 0, u4ReadBlockCnt = 0;
	u_int8_t fgRet = TRUE, fgDbg = FALSE;

	KAL_BOOTTIME_INTERVAL_DECLARATION();

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;
	prRemap = prChipInfo->bus_info->bus2chip_remap;
	prMbuInfo = &prChipInfo->bus_info->rSwEmiRingInfo;
	prEmi = prMbuInfo->prMbuEmiData;

	if (!prMbuInfo->fgIsSupport || !prMbuInfo->fgIsEnable || !prEmi ||
	    !prAdapter->fgIsFwDownloaded)
		return FALSE;

	if (!prRemap) {
		DBGLOG(INIT, ERROR, "Remapping table NOT supported\n");
		return FALSE;
	}

	prAp2wf = prRemap->ap2wf;
	if (!prAp2wf) {
		DBGLOG(INIT, ERROR, "ap2wf remap NOT supported\n");
		return FALSE;
	}

	GLUE_INC_REF_CNT(prMbuInfo->u4ReadBlockCnt);
	for (u4Cnt = 0; u4Cnt < MAX_MBU_RB_WAITING_CNT; u4Cnt++) {
		u4ReadBlockCnt = GLUE_GET_REF_CNT(prMbuInfo->u4ReadBlockCnt);
		if (u4ReadBlockCnt == 1)
			break;
		kalUdelay(1);
	}
	if (u4Cnt == MAX_MBU_RB_WAITING_CNT) {
		DBGLOG(INIT, WARN, "ReadBlockCnt = %u\n", u4ReadBlockCnt);
		fgRet = FALSE;
		goto exit;
	}

	/* set remap */
	if (prMbuInfo->u4RemapAddr) {
		HAL_MCR_WR(prAdapter, prMbuInfo->u4RemapAddr,
			   prMbuInfo->u4RemapVal);
		/* dummy write */
		HAL_MCR_WR(prAdapter, prMbuInfo->u4RemapAddr,
			   prMbuInfo->u4RemapVal);
	}

	prMsiMirror = &prEmi->arMsiMirror[MBU_MSI_MIRROR_IDX];

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnSwEmiDbg))
		KAL_BOOT_TIME_START();

	/* 4. Clear emi int sta */
	prMsiMirror = &prEmi->arMsiMirror[MBU_MSI_MIRROR_IDX];
	prMsiMirror->u4IntSta = 0;

	/* 5. Modify byte length for DMA#0 (8bytes) */
	HAL_MCR_WR(prAdapter, 0x74040A08, 0x16100008);
	HAL_MCR_WR(prAdapter, 0x74040A0C, 0x16110000);
	HAL_MCR_WR(prAdapter, 0x74040214, 0x16700008);
	HAL_MCR_WR(prAdapter, 0x74040218, 0x16710000);

	/* 6. Trigger events */
	u4Addr = CB_DMA_TOP_CB_INFRA_MBU_MAILBOX_0_CMD_H_ADDR;
	if (IS_CONN_INFRA_MCU_ADDR(u4ReadAddr)) {
		u4Val = u4ReadAddr - CONN_INFRA_REMAPPING_OFFSET;
	} else if (IS_CONN_INFRA_ON_ADDR(u4ReadAddr)) {
		u4Val = u4ReadAddr - CONN_INFRA_ON_REMAPPING_OFFSET;
	} else if (IS_CBTOP_PHY_ADDR(u4ReadAddr)) {
		u4Val = u4ReadAddr;
	} else {
		/* set cb top remap */
		halMbuMcrWr(prAdapter, prAp2wf->reg_base, u4ReadAddr);
		u4Val = prAp2wf->base_addr - CONN_INFRA_MCU_TO_PHY_ADDR_OFFSET;
	}
	halMbuMcrWr(prAdapter, u4Addr, u4Val);

	/* restore remap */
	if (prMbuInfo->u4RemapAddr) {
		HAL_MCR_WR(prAdapter, prMbuInfo->u4RemapAddr,
			   prMbuInfo->u4RemapDefVal);
	}

	/* 6. Host driver receive MSI interrupt */
	for (u4Cnt = 0; !prMsiMirror->u4IntSta; u4Cnt++) {
		if (u4Cnt > MAX_MBU_EMI_WAITING_CNT) {
			DBGLOG(HAL, ERROR,
			       "Read[0x%08x] timeout Sta[0x%08x]\n",
			       u4ReadAddr, prMsiMirror->u4IntSta);
			fgRet = FALSE;
			*pu4LowVal = MBU_TIMEOUT_VALUE;
			*pu4HighVal = MBU_TIMEOUT_VALUE;
			prMbuInfo->u4TimeoutCnt++;
			goto exit;
		}
		kalUdelay(5);
	}

	/* Clear interrupt status SW IRQ #0~#8 */
	u4Addr = CB_INFRA_RGU_PCIE_RSV_SW_0_IRQ_CLR_PCIE_RSV_SW_0_IRQ_CLR_ADDR;
	u4Val = 0x1;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	/* 7. Host driver check rdata on EMI */
	*pu4LowVal = prEmi->u4LowVal;
	*pu4HighVal = prEmi->u4HighVal;

exit:
	GLUE_DEC_REF_CNT(prMbuInfo->u4ReadBlockCnt);

	if (prMbuInfo->rOps.dumpDebugCr) {
		if ((prMbuInfo->u4TimeoutCnt >= MBU_TIMEOUT_DBG_CNT &&
		     prMbuInfo->u4DumpDbgCrCnt < MBU_DUMP_DBG_MAX_CNT) ||
		    IS_FEATURE_ENABLED(prWifiVar->fgEnSwEmiDbg)) {
			prMbuInfo->rOps.dumpDebugCr(prGlueInfo);
			prMbuInfo->u4DumpDbgCrCnt++;
			fgDbg = TRUE;
		}
	}

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnSwEmiDbg) && prMsiMirror) {
		KAL_BOOT_TIME_END();
		fgDbg = TRUE;
		DBGLOG(HAL, DEBUG,
		       "read [0x%08x]=[0x%08x][0x%08x] sta[0x%08x] time[%llu us]\n",
		       u4Addr, *pu4LowVal, *pu4HighVal, prMsiMirror->u4IntSta,
		       KAL_GET_BOOTTIME_INTERVAL());
	}
	if (fgDbg)
		halMbuDebug(prGlueInfo);

	return fgRet;
}

void halMbuEnableDebug(struct GLUE_INFO *prGlueInfo)
{
	struct SW_EMI_RING_INFO *prMbuInfo =
		&prGlueInfo->prAdapter->chip_info->bus_info->rSwEmiRingInfo;

	prMbuInfo->u4DumpDbgCrCnt = 0;
}

void halMbuDebug(struct GLUE_INFO *prGlueInfo)
{
	struct BUS_INFO *prBusInfo;
	struct SW_EMI_RING_INFO *prMbuInfo;
	struct MBU_EMI_CTX *prEmi;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prMbuInfo = &prBusInfo->rSwEmiRingInfo;
	prEmi = prMbuInfo->prMbuEmiData;

	if (!prMbuInfo->fgIsSupport || !prMbuInfo->fgIsEnable || !prEmi)
		return;

	DBGLOG(HAL, DEBUG, "Dump MBU EMI:\n");
	DBGLOG_MEM32(HAL, DEBUG, prEmi, sizeof(struct MBU_EMI_CTX));
}
#endif /* CB_INFRA_MBU_BASE */
#endif /* CFG_MTK_WIFI_MBU */
