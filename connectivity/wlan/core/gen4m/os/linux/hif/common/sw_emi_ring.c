/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
/*
 * Id: @(#) sw_emi_ring.c@@
 */
/*  \file   sw_emi_ring.c
 *  \brief  The program provides SW EMI read HIF APIs
 *
 *  This file contains the support emi read register
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

#include "mt66xx_reg.h"
#include "gl_kal.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define SW_EMI_WAITING_FW_READY_CNT	(100 *  100) /* 100ms timeout */
#define SW_EMI_RING_DEBUG		0

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
void halSwEmiInit(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct SW_EMI_RING_INFO *prSwEmiRingInfo;
	struct SW_EMI_CTX *prEmi;
	struct HIF_MEM *prMem = NULL;

	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prSwEmiRingInfo = &prBusInfo->rSwEmiRingInfo;

	if (!prSwEmiRingInfo->fgIsSupport)
		return;

	if (prMemOps->getRsvEmi)
		prMem = prMemOps->getRsvEmi(prHifInfo);

	if (!prMem || !prMem->va) {
		prSwEmiRingInfo->fgIsEnable = FALSE;
		DBGLOG(HAL, ERROR, "sw emi ring init fail\n");
		return;
	}

	prEmi = (struct SW_EMI_CTX *)prMem->va;
	prSwEmiRingInfo->prEmi = prEmi;

	kalMemSet(prEmi, 0, sizeof(struct SW_EMI_CTX));
	prEmi->u4RingSize = SW_EMI_RING_SIZE;
	spin_lock_init(&prSwEmiRingInfo->rRingLock);
	prSwEmiRingInfo->fgIsEnable = TRUE;
}

u_int8_t halSwEmiRead(struct GLUE_INFO *prGlueInfo, uint32_t u4Addr,
		      uint32_t *pu4Val)
{
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct CHIP_DBG_OPS *prDbgOps;
	struct SW_EMI_RING_INFO *prSwEmiRingInfo;
	struct SW_EMI_CTX *prEmi;
	uint32_t u4DrvIdx = 0, u4Cnt = 0;
	u_int8_t fgRet = TRUE, fgDbg = FALSE;
	unsigned long ulFlags = 0;
#if SW_EMI_RING_DEBUG
	KAL_TIME_INTERVAL_DECLARATION();
#endif

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prDbgOps = prChipInfo->prDebugOps;
	prSwEmiRingInfo = &prBusInfo->rSwEmiRingInfo;
	prEmi = prSwEmiRingInfo->prEmi;

	if (!prSwEmiRingInfo->fgIsSupport ||
	    !prSwEmiRingInfo->fgIsEnable ||
	    !prGlueInfo->prAdapter->fgIsFwDownloaded) {
		fgRet = FALSE;
		goto exit;
	}

#if SW_EMI_RING_DEBUG
	KAL_REC_TIME_START();
#endif
	kalAcquireSpinLock(prGlueInfo, SPIN_LOCK_SW_EMI_RING, &ulFlags);

	if (prEmi->u4DrvIdx != prEmi->u4FwIdx) {
		DBGLOG(HAL, ERROR, "DrvIdx[%u] & FwIdx[%u] mismatch!\n",
		       prEmi->u4DrvIdx, prEmi->u4FwIdx);
		halSwEmiDebug(prGlueInfo);
		prEmi->u4DrvIdx = prEmi->u4FwIdx;
	}

	u4DrvIdx = prEmi->u4DrvIdx;
	prEmi->au4Addr[u4DrvIdx] = u4Addr;
	INC_RING_INDEX(u4DrvIdx, prEmi->u4RingSize);

	if (prSwEmiRingInfo->rOps.triggerInt) {
		prSwEmiRingInfo->rOps.triggerInt(prGlueInfo);
	} else {
		DBGLOG(HAL, ERROR, "triggerInt callback is null!\n");
		fgRet = FALSE;
		goto unlock;
	}

	for (u4Cnt = 0; u4DrvIdx != prEmi->u4FwIdx; u4Cnt++) {
		if (u4Cnt > SW_EMI_WAITING_FW_READY_CNT) {
			fgDbg = TRUE;
			fgRet = FALSE;
			goto unlock;
		}
		kalUdelay(10);
	}

	*pu4Val = prEmi->au4Val[prEmi->u4DrvIdx];
	prEmi->u4DrvIdx = u4DrvIdx;

unlock:
	kalReleaseSpinLock(prGlueInfo, SPIN_LOCK_SW_EMI_RING, ulFlags);
#if SW_EMI_RING_DEBUG
	KAL_REC_TIME_END();
	DBGLOG(HAL, INFO,
	       "read [0x%08x]=[0x%08x] time[%lu us]\n",
	       u4Addr, *pu4Val, KAL_GET_TIME_INTERVAL());
#endif
	if (fgDbg) {
		DBGLOG(HAL, ERROR,
		       "Read[0x%08x] timeout DrvIdx[%u] & FwIdx[%u] ",
		       u4Addr, prEmi->u4DrvIdx, prEmi->u4FwIdx);
		halSwEmiDebug(prGlueInfo);
		if (prDbgOps && prDbgOps->dumpwfsyscpupcr)
			prDbgOps->dumpwfsyscpupcr(prGlueInfo->prAdapter);
	}
exit:
	return fgRet;
}

void halSwEmiDebug(struct GLUE_INFO *prGlueInfo)
{
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct SW_EMI_RING_INFO *prSwEmiRingInfo;
	struct SW_EMI_CTX *prEmi;

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prSwEmiRingInfo = &prBusInfo->rSwEmiRingInfo;
	prEmi = prSwEmiRingInfo->prEmi;

	if (!prSwEmiRingInfo->fgIsSupport ||
	    !prSwEmiRingInfo->fgIsEnable ||
	    !prEmi)
		return;

	DBGLOG(HAL, INFO,
	       "En[%d] CCIF[0x%08x %u] DrvIdx[%u] FwIdx[%u] Size[%u]\n",
	       prSwEmiRingInfo->fgIsEnable,
	       prSwEmiRingInfo->u4CcifTchnumAddr,
	       prSwEmiRingInfo->u4CcifChlNum,
	       prEmi->u4DrvIdx,
	       prEmi->u4FwIdx,
	       prEmi->u4RingSize);
	DBGLOG(HAL, INFO, "Dump EMI:\n");
	DBGLOG_MEM32(HAL, INFO, prEmi, sizeof(struct SW_EMI_CTX));
}
