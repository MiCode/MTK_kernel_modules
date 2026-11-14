// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   hal_dmashdl_mt7961.c
*    \brief  DMASHDL HAL API for MT7961
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifdef MT7961
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
#include "mt7961.h"
#include "coda/mt7961/wf_hif_dmashdl_top.h"
#include "hal_dmashdl_mt7961.h"

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

struct DMASHDL_CFG rMT7961DmashdlCfg = {
	.fgSlotArbiterEn = MT7961_DMASHDL_SLOT_ARBITER_EN,

	.u2PktPleMaxPage = MT7961_DMASHDL_PKT_PLE_MAX_PAGE,

	.u2PktPseMaxPage = MT7961_DMASHDL_PKT_PSE_MAX_PAGE,

	.afgRefillEn = {
		MT7961_DMASHDL_GROUP_0_REFILL_EN,
		MT7961_DMASHDL_GROUP_1_REFILL_EN,
		MT7961_DMASHDL_GROUP_2_REFILL_EN,
		MT7961_DMASHDL_GROUP_3_REFILL_EN,
		MT7961_DMASHDL_GROUP_4_REFILL_EN,
		MT7961_DMASHDL_GROUP_5_REFILL_EN,
		MT7961_DMASHDL_GROUP_6_REFILL_EN,
		MT7961_DMASHDL_GROUP_7_REFILL_EN,
		MT7961_DMASHDL_GROUP_8_REFILL_EN,
		MT7961_DMASHDL_GROUP_9_REFILL_EN,
		MT7961_DMASHDL_GROUP_10_REFILL_EN,
		MT7961_DMASHDL_GROUP_11_REFILL_EN,
		MT7961_DMASHDL_GROUP_12_REFILL_EN,
		MT7961_DMASHDL_GROUP_13_REFILL_EN,
		MT7961_DMASHDL_GROUP_14_REFILL_EN,
		MT7961_DMASHDL_GROUP_15_REFILL_EN,
	},

	.au2MaxQuota = {
		MT7961_DMASHDL_GROUP_0_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_1_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_2_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_3_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_4_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_5_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_6_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_7_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_8_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_9_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_10_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_11_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_12_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_13_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_14_MAX_QUOTA,
		MT7961_DMASHDL_GROUP_15_MAX_QUOTA,
	},

	.au2MinQuota = {
		MT7961_DMASHDL_GROUP_0_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_1_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_2_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_3_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_4_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_5_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_6_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_7_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_8_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_9_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_10_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_11_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_12_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_13_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_14_MIN_QUOTA,
		MT7961_DMASHDL_GROUP_15_MIN_QUOTA,
	},

	.aucQueue2Group = {
		MT7961_DMASHDL_QUEUE_0_TO_GROUP,
		MT7961_DMASHDL_QUEUE_1_TO_GROUP,
		MT7961_DMASHDL_QUEUE_2_TO_GROUP,
		MT7961_DMASHDL_QUEUE_3_TO_GROUP,
		MT7961_DMASHDL_QUEUE_4_TO_GROUP,
		MT7961_DMASHDL_QUEUE_5_TO_GROUP,
		MT7961_DMASHDL_QUEUE_6_TO_GROUP,
		MT7961_DMASHDL_QUEUE_7_TO_GROUP,
		MT7961_DMASHDL_QUEUE_8_TO_GROUP,
		MT7961_DMASHDL_QUEUE_9_TO_GROUP,
		MT7961_DMASHDL_QUEUE_10_TO_GROUP,
		MT7961_DMASHDL_QUEUE_11_TO_GROUP,
		MT7961_DMASHDL_QUEUE_12_TO_GROUP,
		MT7961_DMASHDL_QUEUE_13_TO_GROUP,
		MT7961_DMASHDL_QUEUE_14_TO_GROUP,
		MT7961_DMASHDL_QUEUE_15_TO_GROUP,
		MT7961_DMASHDL_QUEUE_16_TO_GROUP,
		MT7961_DMASHDL_QUEUE_17_TO_GROUP,
		MT7961_DMASHDL_QUEUE_18_TO_GROUP,
		MT7961_DMASHDL_QUEUE_19_TO_GROUP,
		MT7961_DMASHDL_QUEUE_20_TO_GROUP,
		MT7961_DMASHDL_QUEUE_21_TO_GROUP,
		MT7961_DMASHDL_QUEUE_22_TO_GROUP,
		MT7961_DMASHDL_QUEUE_23_TO_GROUP,
		MT7961_DMASHDL_QUEUE_24_TO_GROUP,
		MT7961_DMASHDL_QUEUE_25_TO_GROUP,
		MT7961_DMASHDL_QUEUE_26_TO_GROUP,
		MT7961_DMASHDL_QUEUE_27_TO_GROUP,
		MT7961_DMASHDL_QUEUE_28_TO_GROUP,
		MT7961_DMASHDL_QUEUE_29_TO_GROUP,
		MT7961_DMASHDL_QUEUE_30_TO_GROUP,
		MT7961_DMASHDL_QUEUE_31_TO_GROUP,
	},

	.aucPriority2Group = {
		MT7961_DMASHDL_PRIORITY0_GROUP,
		MT7961_DMASHDL_PRIORITY1_GROUP,
		MT7961_DMASHDL_PRIORITY2_GROUP,
		MT7961_DMASHDL_PRIORITY3_GROUP,
		MT7961_DMASHDL_PRIORITY4_GROUP,
		MT7961_DMASHDL_PRIORITY5_GROUP,
		MT7961_DMASHDL_PRIORITY6_GROUP,
		MT7961_DMASHDL_PRIORITY7_GROUP,
		MT7961_DMASHDL_PRIORITY8_GROUP,
		MT7961_DMASHDL_PRIORITY9_GROUP,
		MT7961_DMASHDL_PRIORITY10_GROUP,
		MT7961_DMASHDL_PRIORITY11_GROUP,
		MT7961_DMASHDL_PRIORITY12_GROUP,
		MT7961_DMASHDL_PRIORITY13_GROUP,
		MT7961_DMASHDL_PRIORITY14_GROUP,
		MT7961_DMASHDL_PRIORITY15_GROUP,
	},

	.u4GroupNum = ENUM_MT7961_DMASHDL_GROUP_NUM,

	.ucQueueNum = MT7961_DMASHDL_QUEUE_NUM,

	.ucPriorityNum = MT7961_DMASHDL_PRIORITY_NUM,

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
};

/*----------------------------------------------------------------------------*/
/*!
 * \brief Do DMASDHL init when WIFISYS is initialized at probe, L0.5 reset, etc.
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void mt7961DmashdlInit(struct ADAPTER *prAdapter)
{
	asicConnac2xDmashdlSetPktMaxPage(prAdapter,
				      rMT7961DmashdlCfg.u2PktPleMaxPage,
				      rMT7961DmashdlCfg.u2PktPseMaxPage);

	asicConnac2xDmashdlSetAllRefill(prAdapter,
					rMT7961DmashdlCfg.afgRefillEn);

	asicConnac2xDmashdlSetAllQuota(prAdapter, rMT7961DmashdlCfg.au2MaxQuota,
				    rMT7961DmashdlCfg.au2MinQuota);

	asicConnac2xDmashdlSetAllQueueMapping(prAdapter,
					   rMT7961DmashdlCfg.aucQueue2Group);

	asicConnac2xDmashdlSetAllUserDefinedPriority(prAdapter,
					   rMT7961DmashdlCfg.aucPriority2Group);

	asicConnac2xDmashdlSetSlotArbiter(prAdapter,
				       rMT7961DmashdlCfg.fgSlotArbiterEn);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Although DMASHDL was init, we need to reinit it again due to falcon
 *        L1 reset, etc. The difference between mt7961DmashdlInit and
 *        mt7961DmashdlReInit is that we don't init CRs such as refill,
 *        min_quota, max_quota in mt7961DmashdlReInit, which are backup and
 *        restored in fw. The reason why some DMASHDL CRs are reinit by driver
 *        and some by fw is
 *        1. Some DMASHDL CRs shall be inited before fw releases UMAC reset
 *           in L1 procedure. Then, these CRs are backup and restored by fw.
 *        2. However, the backup and restore of each DMASHDL CR in fw needs
 *           wm DLM space. So, we save DLM space by reinit the remaining
 *           DMASHDL CRs in driver.
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void mt7961DmashdlReInit(struct ADAPTER *prAdapter)
{
	asicConnac2xDmashdlSetPktMaxPage(prAdapter,
				      rMT7961DmashdlCfg.u2PktPleMaxPage,
				      rMT7961DmashdlCfg.u2PktPseMaxPage);

	asicConnac2xDmashdlSetAllQueueMapping(prAdapter,
					   rMT7961DmashdlCfg.aucQueue2Group);

	asicConnac2xDmashdlSetAllUserDefinedPriority(prAdapter,
					   rMT7961DmashdlCfg.aucPriority2Group);

	asicConnac2xDmashdlSetSlotArbiter(prAdapter,
				       rMT7961DmashdlCfg.fgSlotArbiterEn);
}

uint32_t mt7961UpdateDmashdlQuota(struct ADAPTER *prAdapter,
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
		ucGroupIdx = rMT7961DmashdlCfg.aucQueue2Group[ucAcIdx];
		u2MaxQuotaFinal = u4MaxQuota;
		if (fgIsMaxQuotaInvalid) {
			/* Set quota to default */
			u2MaxQuotaFinal =
				rMT7961DmashdlCfg.au2MaxQuota[ucGroupIdx];
		}

		if (u2MaxQuotaFinal) {
			DBGLOG(HAL, DEBUG,
				"ucWmmIndex,%u,ucGroupIdx,%u,u2MaxQuotaFinal,0x%x\n",
				ucWmmIndex, ucGroupIdx, u2MaxQuotaFinal);
			asicConnac2xDmashdlSetMaxQuota(prAdapter,
				ucGroupIdx,
				u2MaxQuotaFinal);
		}
	}
	return WLAN_STATUS_SUCCESS;
}

uint32_t mt7961dmashdlQuotaDecision(struct ADAPTER *prAdapter,
			uint8_t ucWmmIndex)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex;
	uint16_t u2MaxQuota = 0;
#if (CFG_SUPPORT_WIFI_6G == 1)
	u_int8_t fgIs5g = FALSE, fgIs6g = FALSE;
#endif
	enum ENUM_BAND eTargetBand = BAND_NULL;

	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucSwBssIdNum; ucBssIndex++) {

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

#if (CFG_SUPPORT_WIFI_6G == 1)
		if (prBssInfo->eBand == BAND_6G)
			fgIs6g = TRUE;
		else if (prBssInfo->eBand == BAND_5G)
			fgIs5g = TRUE;
#endif
		if (prBssInfo->ucWmmQueSet == ucWmmIndex)
			eTargetBand = prBssInfo->eBand;

	}

	if (eTargetBand != BAND_NULL) {
		if (eTargetBand == BAND_2G4) /* for 2G in case 2+6 or 2+5 */
			u2MaxQuota = MT7961_DMASHDL_DBDC_2G_MAX_QUOTA;
		else /* for 5G and 6G in case 2+6 or 2+5 */
			u2MaxQuota = MT7961_DMASHDL_DBDC_5G_MAX_QUOTA;
#if (CFG_SUPPORT_WIFI_6G == 1)
		if (fgIs6g && fgIs5g) /* for 5+6 case */
			u2MaxQuota = MT7961_DMASHDL_DBDC_5G_6G_MAX_QUOTA;
#endif
	}

	return u2MaxQuota;
}


#endif /* defined(_HIF_PCIE) || defined(_HIF_AXI) || defined(_HIF_USB) */
#endif /* MT7961 */
