// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   hal_dmashdl_mt7999.c
*    \brief  DMASHDL HAL API for mt7999
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifdef MT7999
#if defined(_HIF_PCIE) || defined(_HIF_AXI)

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#include "precomp.h"
#include "mt7999.h"
#include "coda/mt7999/wf_ple_top.h"
#include "coda/mt7999/wf_pse_top.h"
#include "coda/mt7999/wf_hif_dmashdl_lite_top.h"
#include "hal_dmashdl_mt7999.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/


/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct DMASHDL_CFG rMt7999DmashdlCfg = {
	.u2PleTotalPageSize = MT7999_DMASHDL_PLE_TOTAL_PAGE_SIZE,
	.u2PseTotalPageSize = MT7999_DMASHDL_PSE_TOTAL_PAGE_SIZE,
	.u2PktPleMaxPage = MT7999_DMASHDL_PKT_PLE_MAX_PAGE,
	.u2PktPseMaxPage = MT7999_DMASHDL_PKT_PSE_MAX_PAGE,

	.afgRefillEn = {
		MT7999_DMASHDL_GROUP_0_REFILL_EN,
		MT7999_DMASHDL_GROUP_1_REFILL_EN,
		MT7999_DMASHDL_GROUP_2_REFILL_EN,
		MT7999_DMASHDL_GROUP_3_REFILL_EN,
		MT7999_DMASHDL_GROUP_4_REFILL_EN,
		MT7999_DMASHDL_GROUP_5_REFILL_EN,
		MT7999_DMASHDL_GROUP_6_REFILL_EN,
		MT7999_DMASHDL_GROUP_7_REFILL_EN,
		MT7999_DMASHDL_GROUP_8_REFILL_EN,
		MT7999_DMASHDL_GROUP_9_REFILL_EN,
		MT7999_DMASHDL_GROUP_10_REFILL_EN,
		MT7999_DMASHDL_GROUP_11_REFILL_EN,
		MT7999_DMASHDL_GROUP_12_REFILL_EN,
		MT7999_DMASHDL_GROUP_13_REFILL_EN,
		MT7999_DMASHDL_GROUP_14_REFILL_EN,
		MT7999_DMASHDL_GROUP_15_REFILL_EN,
	},

	.au2MaxQuota = {
		MT7999_DMASHDL_GROUP_0_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_1_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_2_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_3_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_4_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_5_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_6_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_7_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_8_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_9_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_10_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_11_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_12_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_13_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_14_MAX_QUOTA,
		MT7999_DMASHDL_GROUP_15_MAX_QUOTA,
	},

	.au2MinQuota = {
		MT7999_DMASHDL_GROUP_0_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_1_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_2_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_3_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_4_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_5_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_6_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_7_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_8_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_9_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_10_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_11_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_12_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_13_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_14_MIN_QUOTA,
		MT7999_DMASHDL_GROUP_15_MIN_QUOTA,
	},

	.aucQueue2Group = {
		MT7999_DMASHDL_QUEUE_0_TO_GROUP,
		MT7999_DMASHDL_QUEUE_1_TO_GROUP,
		MT7999_DMASHDL_QUEUE_2_TO_GROUP,
		MT7999_DMASHDL_QUEUE_3_TO_GROUP,
		MT7999_DMASHDL_QUEUE_4_TO_GROUP,
		MT7999_DMASHDL_QUEUE_5_TO_GROUP,
		MT7999_DMASHDL_QUEUE_6_TO_GROUP,
		MT7999_DMASHDL_QUEUE_7_TO_GROUP,
		MT7999_DMASHDL_QUEUE_8_TO_GROUP,
		MT7999_DMASHDL_QUEUE_9_TO_GROUP,
		MT7999_DMASHDL_QUEUE_10_TO_GROUP,
		MT7999_DMASHDL_QUEUE_11_TO_GROUP,
		MT7999_DMASHDL_QUEUE_12_TO_GROUP,
		MT7999_DMASHDL_QUEUE_13_TO_GROUP,
		MT7999_DMASHDL_QUEUE_14_TO_GROUP,
		MT7999_DMASHDL_QUEUE_15_TO_GROUP,
		MT7999_DMASHDL_QUEUE_16_TO_GROUP,
		MT7999_DMASHDL_QUEUE_17_TO_GROUP,
		MT7999_DMASHDL_QUEUE_18_TO_GROUP,
		MT7999_DMASHDL_QUEUE_19_TO_GROUP,
		MT7999_DMASHDL_QUEUE_20_TO_GROUP,
		MT7999_DMASHDL_QUEUE_21_TO_GROUP,
		MT7999_DMASHDL_QUEUE_22_TO_GROUP,
		MT7999_DMASHDL_QUEUE_23_TO_GROUP,
		MT7999_DMASHDL_QUEUE_24_TO_GROUP,
		MT7999_DMASHDL_QUEUE_25_TO_GROUP,
		MT7999_DMASHDL_QUEUE_26_TO_GROUP,
		MT7999_DMASHDL_QUEUE_27_TO_GROUP,
		MT7999_DMASHDL_QUEUE_28_TO_GROUP,
		MT7999_DMASHDL_QUEUE_29_TO_GROUP,
		MT7999_DMASHDL_QUEUE_30_TO_GROUP,
		MT7999_DMASHDL_QUEUE_31_TO_GROUP,
	},

	.u4GroupNum = ENUM_DMASHDL_GROUP_NUM,

	.ucQueueNum = MT7999_DMASHDL_LMAC_QUEUE_NUM,

	.rMainControl = {
		WF_HIF_DMASHDL_LITE_TOP_MAIN_CONTROL_ADDR,
		0,
		0,
	},

	.rGroupSnChk = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP_SN_CHK_0_ADDR,
		0,
		0,
	},

	.rGroupUdfChk = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP_UDF_CHK_0_ADDR,
		0,
		0,
	},

	.rPleTotalPageSize = {
		WF_HIF_DMASHDL_LITE_TOP_TOTAL_PAGE_SIZE_ADDR,
	WF_HIF_DMASHDL_LITE_TOP_TOTAL_PAGE_SIZE_PLE_TOTAL_PAGE_SIZE_MASK,
	WF_HIF_DMASHDL_LITE_TOP_TOTAL_PAGE_SIZE_PLE_TOTAL_PAGE_SIZE_SHFT
	},

	.rPseTotalPageSize = {
		WF_HIF_DMASHDL_LITE_TOP_TOTAL_PAGE_SIZE_ADDR,
	WF_HIF_DMASHDL_LITE_TOP_TOTAL_PAGE_SIZE_PSE_TOTAL_PAGE_SIZE_MASK,
	WF_HIF_DMASHDL_LITE_TOP_TOTAL_PAGE_SIZE_PSE_TOTAL_PAGE_SIZE_SHFT
	},

	.rPlePacketMaxSize = {
		WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_ADDR,
	WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_PLE_PACKET_MAX_SIZE_MASK,
	WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_PLE_PACKET_MAX_SIZE_SHFT,
	},

	.rPsePacketMaxSize = {
		WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_ADDR,
	WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_PSE_PACKET_MAX_SIZE_MASK,
	WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_PSE_PACKET_MAX_SIZE_SHFT,
	},

	.rGroup0RefillDisable = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP_DISABLE_0_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_GROUP_DISABLE_0_GROUP0_DISABLE_MASK,
		WF_HIF_DMASHDL_LITE_TOP_GROUP_DISABLE_0_GROUP0_DISABLE_SHFT,
	},

	.rGroup0ControlMaxQuota = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_GROUP0_MAX_QUOTA_MASK,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_GROUP0_MAX_QUOTA_SHFT
	},

	.rGroup0ControlMinQuota = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_GROUP0_MIN_QUOTA_MASK,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_GROUP0_MIN_QUOTA_SHFT
	},

	.rQueueMapping0Queue0 = {
		WF_HIF_DMASHDL_LITE_TOP_QUEUE_MAPPING0_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_QUEUE_MAPPING0_QUEUE0_MAPPING_MASK,
		WF_HIF_DMASHDL_LITE_TOP_QUEUE_MAPPING0_QUEUE0_MAPPING_SHFT
	},

	.rStatusRdGp0SrcCnt = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_SRC_CNT_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_SRC_CNT_GROUP0_SRC_CNT_MASK,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_SRC_CNT_GROUP0_SRC_CNT_SHFT,
	},

	.rStatusRdGp0AckCnt = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_ACK_CNT_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_ACK_CNT_GROUP0_ACK_CNT_MASK,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_ACK_CNT_GROUP0_ACK_CNT_SHFT
	},

	.rHifPgInfoHifRsvCnt = {
		WF_PLE_TOP_HIF_PG_INFO_ADDR,
		WF_PLE_TOP_HIF_PG_INFO_HIF_RSV_CNT_MASK,
		WF_PLE_TOP_HIF_PG_INFO_HIF_RSV_CNT_SHFT
	},

	.rHifPgInfoHifSrcCnt = {
		WF_PLE_TOP_HIF_PG_INFO_ADDR,
		WF_PLE_TOP_HIF_PG_INFO_HIF_SRC_CNT_MASK,
		WF_PLE_TOP_HIF_PG_INFO_HIF_SRC_CNT_SHFT
	},
};

void mt7999DmashdlInit(struct ADAPTER *prAdapter)
{
#if (CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1)
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
#endif
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;
	struct DMASHDL_CFG *prCfg = &rMt7999DmashdlCfg;
	uint32_t idx, u4Val = 0, u4Addr = 0;
	uint32_t u4MinQuota = 0, u4MaxQuota = 0;
	u_int8_t fgSetQuota = TRUE;

	prBusInfo->prDmashdlCfg = prCfg;

	u4Addr = WF_HIF_DMASHDL_LITE_TOP_MAIN_CONTROL_ADDR;
	u4Val = WF_HIF_DMASHDL_LITE_TOP_MAIN_CONTROL_SW_RST_B_MASK |
		WF_HIF_DMASHDL_LITE_TOP_MAIN_CONTROL_WLAN_ID_DEC_EN_MASK;
	/* pse_page_size(bit[21:20]) should set to 256B(0x1) */
	u4Val |= (1 << 20);
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	asicConnac5xDmashdlLiteSetTotalPlePsePageSize(
		prAdapter,
		prCfg->u2PleTotalPageSize,
		prCfg->u2PseTotalPageSize);

	asicConnac5xDmashdlSetPlePsePktMaxPage(
		prAdapter,
		prCfg->u2PktPleMaxPage,
		prCfg->u2PktPseMaxPage);

	u4Addr = WF_HIF_DMASHDL_LITE_TOP_GROUP_SN_CHK_0_ADDR;
	u4Val = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_HIF_DMASHDL_LITE_TOP_GROUP_UDF_CHK_0_ADDR;
	u4Val = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	for (idx = 0; idx < 32; idx++) {
		asicConnac5xDmashdlLiteSetQueueMapping(
			prAdapter, idx,
			prCfg->aucQueue2Group[idx]);
	}

	/* group 0~31 */
	u4Addr = WF_HIF_DMASHDL_LITE_TOP_GROUP_DISABLE_0_ADDR;
	u4Val = 0xFFFF0000;
	for (idx = 0; idx < prCfg->u4GroupNum; idx++) {
		if (!prCfg->afgRefillEn[idx])
			u4Val |= (1 << idx);
	}
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	for (idx = 0; idx < prCfg->u4GroupNum; idx++) {
#if (CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1)
		u4MinQuota = prAdapter->chip_info->u4DefaultMinQuota;
		u4MaxQuota = asicConnac5xDynamicDmashdlGetInUsedMaxQuota(
			prAdapter, idx, rMt7999DmashdlCfg.au2MaxQuota[idx]);
		/* don't set quota on SER */
		if (prHifInfo->rErrRecoveryCtl.eErrRecovState !=
		    ERR_RECOV_STOP_IDLE)
			fgSetQuota = FALSE;
#else
		u4MinQuota = rMt7999DmashdlCfg.au2MinQuota[idx];
		u4MaxQuota = rMt7999DmashdlCfg.au2MaxQuota[idx];
#endif
		if (fgSetQuota) {
			asicConnac5xDmashdlSetMinMaxQuota(
				prAdapter, idx, u4MinQuota, u4MaxQuota);
		}
	}
}

#endif /* defined(_HIF_PCIE) || defined(_HIF_AXI) */

#endif /* MT7999 */
