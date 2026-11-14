// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

/*! \file   hal_dmashdl_mt7935.c
*    \brief  DMASHDL HAL API for mt7935
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifdef MT7935
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
#include "mt7935.h"
#include "coda/mt7935/wf_ple_top.h"
#include "coda/mt7935/wf_pse_top.h"
#include "coda/mt7935/wf_hif_dmashdl_lite_top.h"
#include "hal_dmashdl_mt7935.h"

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

struct DMASHDL_CFG rMt7935DmashdlCfg = {
	.u2PleTotalPageSize = MT7935_DMASHDL_PLE_TOTAL_PAGE_SIZE,
	.u2PseTotalPageSize = MT7935_DMASHDL_PSE_TOTAL_PAGE_SIZE,
	.u2PktPleMaxPage = MT7935_DMASHDL_PKT_PLE_MAX_PAGE,
	.u2PktPseMaxPage = MT7935_DMASHDL_PKT_PSE_MAX_PAGE,

	/* Use afgRefillEn to represent dmashdl lite group enable/disable */
	.afgRefillEn = {
		MT7935_DMASHDL_GROUP_0_CONTROL_EN,
		MT7935_DMASHDL_GROUP_1_CONTROL_EN,
		MT7935_DMASHDL_GROUP_2_CONTROL_EN,
		MT7935_DMASHDL_GROUP_3_CONTROL_EN,
		MT7935_DMASHDL_GROUP_4_CONTROL_EN,
		MT7935_DMASHDL_GROUP_5_CONTROL_EN,
		MT7935_DMASHDL_GROUP_6_CONTROL_EN,
		MT7935_DMASHDL_GROUP_7_CONTROL_EN,
		MT7935_DMASHDL_GROUP_8_CONTROL_EN,
		MT7935_DMASHDL_GROUP_9_CONTROL_EN,
		MT7935_DMASHDL_GROUP_10_CONTROL_EN,
		MT7935_DMASHDL_GROUP_11_CONTROL_EN,
		MT7935_DMASHDL_GROUP_12_CONTROL_EN,
		MT7935_DMASHDL_GROUP_13_CONTROL_EN,
		MT7935_DMASHDL_GROUP_14_CONTROL_EN,
		MT7935_DMASHDL_GROUP_15_CONTROL_EN,
		/* only use 15 groups */
		/* ignore other group settings */
		[16 ... 63] = MT7935_DMASHDL_GROUP_16_63_CONTROL_EN,
	},

	.au2MaxQuota = {
		MT7935_DMASHDL_GROUP_0_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_1_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_2_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_3_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_4_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_5_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_6_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_7_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_8_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_9_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_10_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_11_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_12_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_13_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_14_MAX_QUOTA,
		MT7935_DMASHDL_GROUP_15_MAX_QUOTA,
		/* only use 15 groups */
		/* ignore other group settings */
		[16 ... 63] = MT7935_DMASHDL_GROUP_16_63_MAX_QUOTA,
	},

	.au2MinQuota = {
		MT7935_DMASHDL_GROUP_0_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_1_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_2_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_3_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_4_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_5_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_6_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_7_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_8_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_9_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_10_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_11_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_12_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_13_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_14_MIN_QUOTA,
		MT7935_DMASHDL_GROUP_15_MIN_QUOTA,
		/* only use 15 groups */
		/* ignore other group settings */
		[16 ... 63] = MT7935_DMASHDL_GROUP_16_63_MIN_QUOTA,
	},

	.aucQueue2Group = {
		MT7935_DMASHDL_QUEUE_0_TO_GROUP,
		MT7935_DMASHDL_QUEUE_1_TO_GROUP,
		MT7935_DMASHDL_QUEUE_2_TO_GROUP,
		MT7935_DMASHDL_QUEUE_3_TO_GROUP,
		MT7935_DMASHDL_QUEUE_4_TO_GROUP,
		MT7935_DMASHDL_QUEUE_5_TO_GROUP,
		MT7935_DMASHDL_QUEUE_6_TO_GROUP,
		MT7935_DMASHDL_QUEUE_7_TO_GROUP,
		MT7935_DMASHDL_QUEUE_8_TO_GROUP,
		MT7935_DMASHDL_QUEUE_9_TO_GROUP,
		MT7935_DMASHDL_QUEUE_10_TO_GROUP,
		MT7935_DMASHDL_QUEUE_11_TO_GROUP,
		MT7935_DMASHDL_QUEUE_12_TO_GROUP,
		MT7935_DMASHDL_QUEUE_13_TO_GROUP,
		MT7935_DMASHDL_QUEUE_14_TO_GROUP,
		MT7935_DMASHDL_QUEUE_15_TO_GROUP,
		MT7935_DMASHDL_QUEUE_16_TO_GROUP,
		MT7935_DMASHDL_QUEUE_17_TO_GROUP,
		MT7935_DMASHDL_QUEUE_18_TO_GROUP,
		MT7935_DMASHDL_QUEUE_19_TO_GROUP,
		MT7935_DMASHDL_QUEUE_20_TO_GROUP,
		MT7935_DMASHDL_QUEUE_21_TO_GROUP,
		MT7935_DMASHDL_QUEUE_22_TO_GROUP,
		MT7935_DMASHDL_QUEUE_23_TO_GROUP,
		MT7935_DMASHDL_QUEUE_24_TO_GROUP,
		MT7935_DMASHDL_QUEUE_25_TO_GROUP,
		MT7935_DMASHDL_QUEUE_26_TO_GROUP,
		MT7935_DMASHDL_QUEUE_27_TO_GROUP,
		MT7935_DMASHDL_QUEUE_28_TO_GROUP,
		MT7935_DMASHDL_QUEUE_29_TO_GROUP,
		MT7935_DMASHDL_QUEUE_30_TO_GROUP,
		MT7935_DMASHDL_QUEUE_31_TO_GROUP,
		/* only setup 32 group mapping */
		/* ignore other queue to group settings */
		[32 ... 35] = MT7935_DMASHDL_QUEUE_32_35_TO_GROUP,
		[36 ... 99] = MT7935_DMASHDL_QUEUE_36_99_TO_GROUP,
	},

	.u4GroupNum = ENUM_DMASHDL_LITE_GROUP_NUM,

	.ucQueueNum = MT7935_DMASHDL_LMAC_QUEUE_NUM,

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
	WF_HIF_DMASHDL_LITE_TOP_TOTAL_PAGE_SIZE_ple_total_page_size_MASK,
	WF_HIF_DMASHDL_LITE_TOP_TOTAL_PAGE_SIZE_ple_total_page_size_SHFT
	},

	.rPseTotalPageSize = {
		WF_HIF_DMASHDL_LITE_TOP_TOTAL_PAGE_SIZE_ADDR,
	WF_HIF_DMASHDL_LITE_TOP_TOTAL_PAGE_SIZE_pse_total_page_size_MASK,
	WF_HIF_DMASHDL_LITE_TOP_TOTAL_PAGE_SIZE_pse_total_page_size_SHFT
	},

	.rPlePacketMaxSize = {
		WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_ADDR,
	WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_ple_packet_max_size_MASK,
	WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_ple_packet_max_size_SHFT,
	},

	.rPsePacketMaxSize = {
		WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_ADDR,
	WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_pse_packet_max_size_MASK,
	WF_HIF_DMASHDL_LITE_TOP_PACKET_MAX_SIZE_pse_packet_max_size_SHFT,
	},

	.rGroup0RefillDisable = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP_DISABLE_0_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_GROUP_DISABLE_0_group0_disable_MASK,
		WF_HIF_DMASHDL_LITE_TOP_GROUP_DISABLE_0_group0_disable_SHFT,
	},

	.rGroup0ControlMaxQuota = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_group0_max_quota_MASK,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_group0_max_quota_SHFT
	},

	.rGroup0ControlMinQuota = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_group0_min_quota_MASK,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_CONTROL_group0_min_quota_SHFT
	},

	.rQueueMapping0Queue0 = {
		WF_HIF_DMASHDL_LITE_TOP_QUEUE_MAPPING0_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_QUEUE_MAPPING0_queue0_mapping_MASK,
		WF_HIF_DMASHDL_LITE_TOP_QUEUE_MAPPING0_queue0_mapping_SHFT
	},

	.rStatusRdGp0SrcCnt = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_SRC_CNT_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_SRC_CNT_group0_src_cnt_MASK,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_SRC_CNT_group0_src_cnt_SHFT,
	},

	.rStatusRdGp0AckCnt = {
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_ACK_CNT_ADDR,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_ACK_CNT_group0_ack_cnt_MASK,
		WF_HIF_DMASHDL_LITE_TOP_GROUP0_ACK_CNT_group0_ack_cnt_SHFT
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

void mt7935DmashdlInit(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;
	struct DMASHDL_CFG *prCfg = &rMt7935DmashdlCfg;
	uint32_t idx, u4Val = 0, u4Addr = 0;

	prBusInfo->prDmashdlCfg = prCfg;

	u4Addr = WF_HIF_DMASHDL_LITE_TOP_MAIN_CONTROL_ADDR;
	u4Val = WF_HIF_DMASHDL_LITE_TOP_MAIN_CONTROL_sw_rst_b_MASK;

	if (prChipInfo->is_support_dmashdl_lite_wlanid_dec)
		u4Val |=
		WF_HIF_DMASHDL_LITE_TOP_MAIN_CONTROL_wlan_id_dec_en_MASK;

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	asicConnac3xDmashdlLiteSetTotalPlePsePageSize(
		prAdapter,
		prCfg->u2PleTotalPageSize,
		prCfg->u2PseTotalPageSize);

	asicConnac3xDmashdlSetPlePsePktMaxPage(
		prAdapter,
		prCfg->u2PktPleMaxPage,
		prCfg->u2PktPseMaxPage);

	u4Addr = WF_HIF_DMASHDL_LITE_TOP_GROUP_SN_CHK_0_ADDR;
	u4Val = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_HIF_DMASHDL_LITE_TOP_GROUP_SN_CHK_1_ADDR;
	u4Val = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_HIF_DMASHDL_LITE_TOP_GROUP_UDF_CHK_0_ADDR;
	u4Val = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_HIF_DMASHDL_LITE_TOP_GROUP_UDF_CHK_1_ADDR;
	u4Val = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	for (idx = 0; idx < 32; idx++) {
		asicConnac3xDmashdlLiteSetQueueMapping(
			prAdapter, idx,
			prCfg->aucQueue2Group[idx]);
	}

	/* group 0~31 */
	u4Addr = WF_HIF_DMASHDL_LITE_TOP_GROUP_DISABLE_0_ADDR;
	u4Val = 0;
	for (idx = 0; idx < (ENUM_DMASHDL_LITE_GROUP_NUM / 2); idx++) {
		if (prCfg->afgRefillEn[idx])
			u4Val &= ~(1 << idx);
		else
			u4Val |= (1 << idx);
	}
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	/* group 32~63 */
	u4Addr = WF_HIF_DMASHDL_LITE_TOP_GROUP_DISABLE_1_ADDR;
	u4Val = 0;
	for (; idx < ENUM_DMASHDL_LITE_GROUP_NUM; idx++) {
		if (prCfg->afgRefillEn[idx])
			u4Val &= ~(1 << (idx - 32));
		else
			u4Val |= (1 << (idx - 32));
	}
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	for (idx = 0; idx < ENUM_DMASHDL_LITE_GROUP_NUM; idx++) {
		asicConnac3xDmashdlSetMinMaxQuota(
			prAdapter, idx,
			prCfg->au2MinQuota[idx],
			prCfg->au2MaxQuota[idx]);
	}
}

#endif /* defined(_HIF_PCIE) || defined(_HIF_AXI) */

#endif /* MT7935 */
