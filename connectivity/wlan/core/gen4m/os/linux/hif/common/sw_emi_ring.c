// SPDX-License-Identifier: BSD-2-Clause
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

#if CFG_MTK_WIFI_SW_EMI_RING
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

	if (prMemOps->getWifiMiscRsvEmi) {
		prMem = prMemOps->getWifiMiscRsvEmi(prChipInfo,
			WIFI_MISC_MEM_BLOCK_NON_MMIO);
	}

	if (!prMem || !prMem->va) {
		prSwEmiRingInfo->fgIsEnable = FALSE;
		DBGLOG(HAL, ERROR, "sw emi ring init fail\n");
		return;
	}

	prEmi = (struct SW_EMI_CTX *)prMem->va;
	prSwEmiRingInfo->prEmi = prEmi;

	kalMemSet(prEmi, 0, sizeof(struct SW_EMI_CTX));
	prEmi->u4RingSize = SW_EMI_RING_SIZE;
	prSwEmiRingInfo->u4ReadBlockCnt = 0;
	prSwEmiRingInfo->fgIsEnable = TRUE;

	DBGLOG(HAL, DEBUG, "base: 0x%llx\n", prMem->pa);
}

u_int8_t halSwEmiRead(struct GLUE_INFO *prGlueInfo, uint32_t u4Addr,
		      uint32_t *pu4Val)
{
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct CHIP_DBG_OPS *prDbgOps;
	struct WIFI_VAR *prWifiVar;
	struct SW_EMI_RING_INFO *prSwEmiRingInfo;
	struct SW_EMI_CTX *prEmi;
	uint32_t u4DrvIdx = 0, u4Cnt = 0, u4ReadBlockCnt = 0;
	u_int8_t fgRet = TRUE, fgDbg = FALSE;

	KAL_BOOTTIME_INTERVAL_DECLARATION();

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prDbgOps = prChipInfo->prDebugOps;
	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;
	prSwEmiRingInfo = &prBusInfo->rSwEmiRingInfo;
	prEmi = prSwEmiRingInfo->prEmi;

	if (!prSwEmiRingInfo->fgIsSupport ||
	    !prSwEmiRingInfo->fgIsEnable ||
	    !prGlueInfo->prAdapter->fgIsFwDownloaded) {
		fgRet = FALSE;
		goto exit;
	}

	if (!prSwEmiRingInfo->rOps.triggerInt) {
		DBGLOG(HAL, ERROR, "triggerInt callback is null!\n");
		fgRet = FALSE;
		goto exit;
	}

	if ((prEmi->u4DrvIdx >= SW_EMI_RING_SIZE) ||
	    (prEmi->u4FwIdx >= SW_EMI_RING_SIZE) ||
	    (prEmi->u4RingSize > SW_EMI_RING_SIZE) ||
	    (prEmi->u4RingSize == 0)) {
		fgDbg = TRUE;
		fgRet = FALSE;
		goto debug;
	}

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnSwEmiDbg))
		KAL_BOOT_TIME_START();

	GLUE_INC_REF_CNT(prSwEmiRingInfo->u4ReadBlockCnt);
	u4ReadBlockCnt = GLUE_GET_REF_CNT(prSwEmiRingInfo->u4ReadBlockCnt);
	if (u4ReadBlockCnt > 1) {
		DBGLOG(INIT, WARN, "ReadBlockCnt = %u\n", u4ReadBlockCnt);
		fgRet = FALSE;
		goto end;
	}

	if (prEmi->u4DrvIdx != prEmi->u4FwIdx) {
		DBGLOG(HAL, ERROR, "DrvIdx[%u] & FwIdx[%u] mismatch!\n",
		       prEmi->u4DrvIdx, prEmi->u4FwIdx);
		halSwEmiDebug(prGlueInfo);
		prEmi->u4DrvIdx = prEmi->u4FwIdx;
	}

	u4DrvIdx = prEmi->u4DrvIdx;
	prEmi->au4Addr[u4DrvIdx] = u4Addr;
	INC_RING_INDEX(u4DrvIdx, prEmi->u4RingSize);

	prSwEmiRingInfo->rOps.triggerInt(prGlueInfo);

	for (u4Cnt = 0; u4DrvIdx != prEmi->u4FwIdx; u4Cnt++) {
		if (u4Cnt > SW_EMI_WAITING_FW_READY_CNT) {
			DBGLOG(HAL, ERROR,
			       "Read[0x%08x] timeout DrvIdx[%u] & FwIdx[%u] ",
			       u4Addr, prEmi->u4DrvIdx, prEmi->u4FwIdx);
			fgDbg = TRUE;
			fgRet = FALSE;
			goto end;
		}
		kalUdelay(10);
	}

	*pu4Val = prEmi->au4Val[prEmi->u4DrvIdx];
	prEmi->u4DrvIdx = u4DrvIdx;

end:
	GLUE_DEC_REF_CNT(prSwEmiRingInfo->u4ReadBlockCnt);

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnSwEmiDbg)) {
		KAL_BOOT_TIME_END();
		fgDbg = TRUE;
		DBGLOG(HAL, DEBUG,
		       "read [0x%08x]=[0x%08x] time[%llu us]\n",
		       u4Addr, *pu4Val, KAL_GET_BOOTTIME_INTERVAL());
	}
debug:
	if (fgDbg) {
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

	DBGLOG(HAL, DEBUG,
	       "En[%d] CCIF[0x%08x %u] DrvIdx[%u] FwIdx[%u] Size[%u]\n",
	       prSwEmiRingInfo->fgIsEnable,
	       prSwEmiRingInfo->u4CcifTchnumAddr,
	       prSwEmiRingInfo->u4CcifChlNum,
	       prEmi->u4DrvIdx,
	       prEmi->u4FwIdx,
	       prEmi->u4RingSize);
	DBGLOG(HAL, DEBUG, "Dump EMI:\n");
	DBGLOG_MEM32(HAL, DEBUG, prEmi, sizeof(struct SW_EMI_CTX));
}

#endif /* CFG_MTK_WIFI_SW_EMI_RING */
