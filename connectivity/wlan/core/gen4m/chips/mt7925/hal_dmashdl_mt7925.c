// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   hal_dmashdl_mt7925.c
*    \brief  DMASHDL HAL API for mt7925
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifdef MT7925
#if defined(_HIF_PCIE) || defined(_HIF_AXI) || defined(_HIF_USB)

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#include "precomp.h"
#include "mt7925.h"
#include "coda/mt7925/wf_ple_top.h"
#include "coda/mt7925/wf_pse_top.h"
#include "coda/mt7925/wf_hif_dmashdl_top.h"
#include "hal_dmashdl_mt7925.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

#define SW_WORKAROUND_FOR_DMASHDL_ISSUE_HWITS00058160 1

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

struct DMASHDL_CFG rMt7925DmashdlCfg = {
	.fgSlotArbiterEn = MT7925_DMASHDL_SLOT_ARBITER_EN,

	.u2PktPleMaxPage = MT7925_DMASHDL_PKT_PLE_MAX_PAGE,

	.u2PktPseMaxPage = MT7925_DMASHDL_PKT_PSE_MAX_PAGE,

	.afgRefillEn = {
		MT7925_DMASHDL_GROUP_0_REFILL_EN,
		MT7925_DMASHDL_GROUP_1_REFILL_EN,
		MT7925_DMASHDL_GROUP_2_REFILL_EN,
		MT7925_DMASHDL_GROUP_3_REFILL_EN,
		MT7925_DMASHDL_GROUP_4_REFILL_EN,
		MT7925_DMASHDL_GROUP_5_REFILL_EN,
		MT7925_DMASHDL_GROUP_6_REFILL_EN,
		MT7925_DMASHDL_GROUP_7_REFILL_EN,
		MT7925_DMASHDL_GROUP_8_REFILL_EN,
		MT7925_DMASHDL_GROUP_9_REFILL_EN,
		MT7925_DMASHDL_GROUP_10_REFILL_EN,
		MT7925_DMASHDL_GROUP_11_REFILL_EN,
		MT7925_DMASHDL_GROUP_12_REFILL_EN,
		MT7925_DMASHDL_GROUP_13_REFILL_EN,
		MT7925_DMASHDL_GROUP_14_REFILL_EN,
		MT7925_DMASHDL_GROUP_15_REFILL_EN,
	},

	.au2MaxQuota = {
		MT7925_DMASHDL_GROUP_0_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_1_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_2_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_3_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_4_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_5_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_6_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_7_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_8_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_9_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_10_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_11_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_12_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_13_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_14_MAX_QUOTA,
		MT7925_DMASHDL_GROUP_15_MAX_QUOTA,
	},

	.au2MinQuota = {
		MT7925_DMASHDL_GROUP_0_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_1_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_2_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_3_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_4_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_5_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_6_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_7_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_8_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_9_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_10_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_11_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_12_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_13_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_14_MIN_QUOTA,
		MT7925_DMASHDL_GROUP_15_MIN_QUOTA,
	},

	.aucQueue2Group = {
		MT7925_DMASHDL_QUEUE_0_TO_GROUP,
		MT7925_DMASHDL_QUEUE_1_TO_GROUP,
		MT7925_DMASHDL_QUEUE_2_TO_GROUP,
		MT7925_DMASHDL_QUEUE_3_TO_GROUP,
		MT7925_DMASHDL_QUEUE_4_TO_GROUP,
		MT7925_DMASHDL_QUEUE_5_TO_GROUP,
		MT7925_DMASHDL_QUEUE_6_TO_GROUP,
		MT7925_DMASHDL_QUEUE_7_TO_GROUP,
		MT7925_DMASHDL_QUEUE_8_TO_GROUP,
		MT7925_DMASHDL_QUEUE_9_TO_GROUP,
		MT7925_DMASHDL_QUEUE_10_TO_GROUP,
		MT7925_DMASHDL_QUEUE_11_TO_GROUP,
		MT7925_DMASHDL_QUEUE_12_TO_GROUP,
		MT7925_DMASHDL_QUEUE_13_TO_GROUP,
		MT7925_DMASHDL_QUEUE_14_TO_GROUP,
		MT7925_DMASHDL_QUEUE_15_TO_GROUP,
		MT7925_DMASHDL_QUEUE_16_TO_GROUP,
		MT7925_DMASHDL_QUEUE_17_TO_GROUP,
		MT7925_DMASHDL_QUEUE_18_TO_GROUP,
		MT7925_DMASHDL_QUEUE_19_TO_GROUP,
		MT7925_DMASHDL_QUEUE_20_TO_GROUP,
		MT7925_DMASHDL_QUEUE_21_TO_GROUP,
		MT7925_DMASHDL_QUEUE_22_TO_GROUP,
		MT7925_DMASHDL_QUEUE_23_TO_GROUP,
		MT7925_DMASHDL_QUEUE_24_TO_GROUP,
		MT7925_DMASHDL_QUEUE_25_TO_GROUP,
		MT7925_DMASHDL_QUEUE_26_TO_GROUP,
		MT7925_DMASHDL_QUEUE_27_TO_GROUP,
		MT7925_DMASHDL_QUEUE_28_TO_GROUP,
		MT7925_DMASHDL_QUEUE_29_TO_GROUP,
		MT7925_DMASHDL_QUEUE_30_TO_GROUP,
		MT7925_DMASHDL_QUEUE_31_TO_GROUP,
	},

	.aucPriority2Group = {
		MT7925_DMASHDL_PRIORITY0_GROUP,
		MT7925_DMASHDL_PRIORITY1_GROUP,
		MT7925_DMASHDL_PRIORITY2_GROUP,
		MT7925_DMASHDL_PRIORITY3_GROUP,
		MT7925_DMASHDL_PRIORITY4_GROUP,
		MT7925_DMASHDL_PRIORITY5_GROUP,
		MT7925_DMASHDL_PRIORITY6_GROUP,
		MT7925_DMASHDL_PRIORITY7_GROUP,
		MT7925_DMASHDL_PRIORITY8_GROUP,
		MT7925_DMASHDL_PRIORITY9_GROUP,
		MT7925_DMASHDL_PRIORITY10_GROUP,
		MT7925_DMASHDL_PRIORITY11_GROUP,
		MT7925_DMASHDL_PRIORITY12_GROUP,
		MT7925_DMASHDL_PRIORITY13_GROUP,
		MT7925_DMASHDL_PRIORITY14_GROUP,
		MT7925_DMASHDL_PRIORITY15_GROUP,
	},

	.u2HifAckCntTh = MT7925_DMASHDL_HIF_ACK_CNT_TH,
	.u2HifGupActMap = MT7925_DMASHDL_HIF_GUP_ACT_MAP,
	.u4GroupNum = ENUM_DMASHDL_GROUP_NUM,

	.rPlePacketMaxSize = {
		WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_ADDR,
		WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_PLE_PACKET_MAX_SIZE_MASK,
		WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_PLE_PACKET_MAX_SIZE_SHFT
	},

	.rPsePacketMaxSize = {
		WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_ADDR,
		WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_PSE_PACKET_MAX_SIZE_MASK,
		WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_PSE_PACKET_MAX_SIZE_SHFT
	},

	.rGroup0RefillDisable = {
		WF_HIF_DMASHDL_TOP_REFILL_CONTROL_ADDR,
		WF_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP0_REFILL_DISABLE_MASK,
		0
	},

	.rGroup0ControlMaxQuota = {
		WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_ADDR,
		WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_GROUP0_MAX_QUOTA_MASK,
		WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_GROUP0_MAX_QUOTA_SHFT
	},

	.rGroup0ControlMinQuota = {
		WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_ADDR,
		WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_GROUP0_MIN_QUOTA_MASK,
		WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_GROUP0_MIN_QUOTA_SHFT
	},

	.rQueueMapping0Queue0 = {
		WF_HIF_DMASHDL_TOP_QUEUE_MAPPING0_ADDR,
		WF_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE0_MAPPING_MASK,
		WF_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE0_MAPPING_SHFT
	},

	.rPageSettingGroupSeqOrderType = {
		WF_HIF_DMASHDL_TOP_PAGE_SETTING_ADDR,
		WF_HIF_DMASHDL_TOP_PAGE_SETTING_GROUP_SEQUENCE_ORDER_TYPE_MASK,
		WF_HIF_DMASHDL_TOP_PAGE_SETTING_GROUP_SEQUENCE_ORDER_TYPE_SHFT,
	},

	.rSchdulerSetting0Priority0Group = {
		WF_HIF_DMASHDL_TOP_HIF_SCHEDULER_SETTING0_ADDR,
		WF_HIF_DMASHDL_TOP_HIF_SCHEDULER_SETTING0_PRIORITY0_GROUP_MASK,
		WF_HIF_DMASHDL_TOP_HIF_SCHEDULER_SETTING0_PRIORITY0_GROUP_SHFT
	},

	.rStatusRdGp0RsvCnt = {
		WF_HIF_DMASHDL_TOP_STATUS_RD_GP0_ADDR,
		WF_HIF_DMASHDL_TOP_STATUS_RD_GP0_G0_RSV_CNT_MASK,
		WF_HIF_DMASHDL_TOP_STATUS_RD_GP0_G0_RSV_CNT_SHFT
	},

	.rStatusRdGp0SrcCnt = {
		WF_HIF_DMASHDL_TOP_STATUS_RD_GP0_ADDR,
		WF_HIF_DMASHDL_TOP_STATUS_RD_GP0_G0_SRC_CNT_MASK,
		WF_HIF_DMASHDL_TOP_STATUS_RD_GP0_G0_SRC_CNT_SHFT
	},

	.rRdGroupPktCnt0 = {
		WF_HIF_DMASHDL_TOP_RD_GROUP_PKT_CNT0_ADDR,
		0,
		0
	},

	.rOptionalControlCrHifAckCntTh = {
		WF_HIF_DMASHDL_TOP_OPTIONAL_CONTROL_ADDR,
		WF_HIF_DMASHDL_TOP_OPTIONAL_CONTROL_CR_HIF_ACK_CNT_TH_MASK,
		WF_HIF_DMASHDL_TOP_OPTIONAL_CONTROL_CR_HIF_ACK_CNT_TH_SHFT
	},

	.rOptionalControlCrHifGupActMap = {
		WF_HIF_DMASHDL_TOP_OPTIONAL_CONTROL_ADDR,
		WF_HIF_DMASHDL_TOP_OPTIONAL_CONTROL_CR_HIF_GUP_ACT_MAP_MASK,
		WF_HIF_DMASHDL_TOP_OPTIONAL_CONTROL_CR_HIF_GUP_ACT_MAP_SHFT,
	},

	.rErrorFlagCtrl = {
		WF_HIF_DMASHDL_TOP_ERROR_FLAG_CTRL_ADDR,
		0,
		0
	},

	.rStatusRdFfaCnt = {
		WF_HIF_DMASHDL_TOP_STATUS_RD_ADDR,
		WF_HIF_DMASHDL_TOP_STATUS_RD_FFA_CNT_MASK,
		WF_HIF_DMASHDL_TOP_STATUS_RD_FFA_CNT_SHFT
	},

	.rStatusRdFreePageCnt = {
		WF_HIF_DMASHDL_TOP_STATUS_RD_ADDR,
		WF_HIF_DMASHDL_TOP_STATUS_RD_FREE_PAGE_CNT_MASK,
		WF_HIF_DMASHDL_TOP_STATUS_RD_FREE_PAGE_CNT_SHFT
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

void mt7925DmashdlInit(struct ADAPTER *prAdapter)
{
	uint32_t idx, u4DefVal;

	asicConnac3xDmashdlSetPlePsePktMaxPage(
		prAdapter,
		rMt7925DmashdlCfg.u2PktPleMaxPage,
		rMt7925DmashdlCfg.u2PktPseMaxPage);

	for (idx = 0; idx < ENUM_DMASHDL_GROUP_NUM; idx++) {
		asicConnac3xDmashdlSetRefill(
			prAdapter, idx,
			rMt7925DmashdlCfg.afgRefillEn[idx]);

		asicConnac3xDmashdlSetMinMaxQuota(
			prAdapter, idx,
			rMt7925DmashdlCfg.au2MinQuota[idx],
			rMt7925DmashdlCfg.au2MaxQuota[idx]);
	}

	for (idx = 0; idx < 32; idx++)
		asicConnac3xDmashdlSetQueueMapping(
			prAdapter, idx,
			rMt7925DmashdlCfg.aucQueue2Group[idx]);

	for (idx = 0; idx < 16; idx++)
		asicConnac3xDmashdlSetUserDefinedPriority(
			prAdapter, idx,
			rMt7925DmashdlCfg.aucPriority2Group[idx]);

	u4DefVal = WF_HIF_DMASHDL_TOP_PAGE_SETTING_QUP_ACL_SLOT_CG_EN_MASK |
		WF_HIF_DMASHDL_TOP_PAGE_SETTING_SRC_CNT_PRI_EN_MASK |
		WF_HIF_DMASHDL_TOP_PAGE_SETTING_DUMMY_01_MASK |
		WF_HIF_DMASHDL_TOP_PAGE_SETTING_DUMMY_00_MASK |
#if (SW_WORKAROUND_FOR_DMASHDL_ISSUE_HWITS00058160 == 1)
		WF_HIF_DMASHDL_TOP_PAGE_SETTING_SLOT_TYPE_ARBITER_CONTROL_MASK;
#else
		WF_HIF_DMASHDL_TOP_PAGE_SETTING_SLOT_TYPE_ARBITER_CONTROL_MASK |
		WF_HIF_DMASHDL_TOP_PAGE_SETTING_PP_OFFSET_ADD_ENA_MASK;
#endif
	asicConnac3xDmashdlSetSlotArbiter(
		prAdapter, rMt7925DmashdlCfg.fgSlotArbiterEn,
		u4DefVal);

	u4DefVal =
WF_HIF_DMASHDL_TOP_OPTIONAL_CONTROL_CR_PSEBF_BL_TH2_NOBMIN_RASIGN_ENA_MASK |
		WF_HIF_DMASHDL_TOP_OPTIONAL_CONTROL_CR_HIF_ASK_MIN_RR_ENA_MASK |
		WF_HIF_DMASHDL_TOP_OPTIONAL_CONTROL_CR_HIF_ASK_RR_ENA_MASK;
	asicConnac3xDmashdlSetOptionalControl(prAdapter,
		rMt7925DmashdlCfg.u2HifAckCntTh,
		rMt7925DmashdlCfg.u2HifGupActMap,
		u4DefVal);
}

uint32_t mt7925UpdateDmashdlQuota(struct ADAPTER *prAdapter,
				uint8_t ucWmmIndex, uint32_t u4MaxQuota)
{
	uint8_t ucGroupIdx, ucAcIdx;
	uint32_t idx;
	uint16_t u2MaxQuotaFinal;
	bool fgIsMaxQuotaInvalid = FALSE;

	ASSERT(prAdapter);
	if (u4MaxQuota > (DMASHDL_MAX_QUOTA_MASK >> DMASHDL_MAX_QUOTA_OFFSET))
		fgIsMaxQuotaInvalid = TRUE;

	for (idx = 0; idx < WMM_AC_INDEX_NUM; idx++) {
		ucAcIdx = idx + (ucWmmIndex * WMM_AC_INDEX_NUM);
		ucGroupIdx = rMt7925DmashdlCfg.aucQueue2Group[ucAcIdx];
		u2MaxQuotaFinal = u4MaxQuota;
		if (fgIsMaxQuotaInvalid) {
			/* Set quota to default */
			u2MaxQuotaFinal =
				rMt7925DmashdlCfg.au2MaxQuota[ucGroupIdx];
		}

		if (u2MaxQuotaFinal) {
			DBGLOG(HAL, DEBUG,
				"ucWmmIndex,%u,ucGroupIdx,%u,u2MaxQuotaFinal,0x%x\n",
				ucWmmIndex, ucGroupIdx, u2MaxQuotaFinal);
			asicConnac3xDmashdlSetMinMaxQuota(prAdapter, ucGroupIdx,
							3, u2MaxQuotaFinal);

		}
	}
	return WLAN_STATUS_SUCCESS;
}

uint32_t mt7925dmashdlQuotaDecision(struct ADAPTER *prAdapter,
			uint8_t ucWmmIndex)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex;
	uint8_t ucABandWmmIdx = HW_WMM_NUM;
	bool fgAAWmmConcurrent = false;
	uint16_t u2MaxQuota = 0;
	enum ENUM_BAND eTargetBand = BAND_NULL;
	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucHwBssIdNum; ucBssIndex++) {

		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

		if (IS_BSS_NOT_ALIVE(prAdapter, prBssInfo))
			continue;

		if (prBssInfo->eBand != BAND_2G4
		    && prBssInfo->eBand != BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
		    && prBssInfo->eBand != BAND_6G
#endif
		    )
			continue;

		if (prBssInfo->eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
			|| prBssInfo->eBand == BAND_6G
#endif
		) {
			/* MLO STR A+G is regarded as A band to concurrent */
			/* If there are many WMM set with A band, */
			/* that means there is A+A Wmm concurent */
			if (prBssInfo->ucWmmQueSet != ucABandWmmIdx) {
				if (ucABandWmmIdx == HW_WMM_NUM)
					ucABandWmmIdx = prBssInfo->ucWmmQueSet;
				else
					fgAAWmmConcurrent = true;
			}
		}

		/* MLO STR A+G is regarded as A band to concurrent */
		/* So using A band as target band in the same WMM set */
		if (prBssInfo->ucWmmQueSet == ucWmmIndex)
			if (prBssInfo->eBand > eTargetBand)
				eTargetBand = prBssInfo->eBand;
	}

	if (eTargetBand != BAND_NULL) {
		if (eTargetBand == BAND_2G4) /* for G band in case A+G */
			u2MaxQuota = MT7925_DMASHDL_DBDC_2G_MAX_QUOTA;
		else {
			if (fgAAWmmConcurrent) /* for A+A case */
				u2MaxQuota =
					MT7925_DMASHDL_DBDC_5G_6G_MAX_QUOTA;
			else /* for A band in case A+G */
				u2MaxQuota = MT7925_DMASHDL_DBDC_5G_MAX_QUOTA;
		}
	}

	return u2MaxQuota;
}
#endif /* defined(_HIF_PCIE) || defined(_HIF_AXI) || defined(_HIF_USB) */
#endif /* MT7925 */
