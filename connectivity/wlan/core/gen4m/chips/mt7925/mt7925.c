// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   mt7925.c
*    \brief  Internal driver stack will export
*    the required procedures here for GLUE Layer.
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifdef MT7925

#include "precomp.h"
#include "mt7925.h"
#include "hal_dmashdl_mt7925.h"
#include "coda/mt7925/pcie_mac_ireg.h"
#include "coda/mt7925/cb_infra_rgu.h"
#include "coda/mt7925/conn_cfg.h"
#include "coda/mt7925/conn_bus_cr_von.h"
#include "coda/mt7925/conn_mcu_bus_cr.h"
#include "coda/mt7925/conn_host_csr_top.h"
#include "coda/mt7925/wf_cr_sw_def.h"
#include "coda/mt7925/wf_wfdma_ext_wrap_csr.h"
#include "coda/mt7925/wf_wfdma_host_dma0.h"
#include "coda/mt7925/wf_wfdma_mcu_dma0.h"
#include "coda/mt7925/wf_hif_dmashdl_top.h"
#include "coda/mt7925/wf_pse_top.h"
#include "coda/mt7925/wf_top_cfg_on.h"
#include "coda/mt7925/wf_top_cfg_on.h"
#include "coda/mt7925/wf_wtblon_top.h"
#include "coda/mt7925/wf_uwtbl_top.h"
#include "hal_wfsys_reset_mt7925.h"
#if (CFG_SUPPORT_DEBUG_SOP == 1)
#include "dbg_mt7925.h"
#endif
#define CFG_SUPPORT_VCODE_VDFS 0

#if (CFG_SUPPORT_VCODE_VDFS == 1)
#include <linux/pm_qos.h>

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#endif /*#ifndef CFG_SUPPORT_VCODE_VDFS*/

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define CONCAT(NAME1, NAME2) (NAME1##NAME2)

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static uint32_t mt7925GetFlavorVer(struct GLUE_INFO *prGlueInfo,
	uint8_t *flavor);

static void mt7925_ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx);

static void mt7925_ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
static void mt7925_ConstructIdxLogBinName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName);
#endif

#if defined(_HIF_PCIE)
static uint8_t mt7925SetRxRingHwAddr(struct RTMP_RX_RING *prRxRing,
	struct BUS_INFO *prBusInfo, uint32_t u4SwRingIdx);

static bool mt7925WfdmaAllocRxRing(struct GLUE_INFO *prGlueInfo,
	bool fgAllocMem);

static void mt7925ProcessTxInterrupt(
	struct ADAPTER *prAdapter);

static void mt7925ProcessRxInterrupt(
	struct ADAPTER *prAdapter);

static void mt7925WfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo);

static void mt7925ReadIntStatus(struct ADAPTER *prAdapter,
	uint32_t *pu4IntStatus);

static void mt7925EnableInterrupt(struct ADAPTER *prAdapter);
static void mt7925DisableInterrupt(struct ADAPTER *prAdapter);

static void mt7925ConfigIntMask(struct GLUE_INFO *prGlueInfo,
	u_int8_t enable);

static void mt7925WpdmaConfig(struct GLUE_INFO *prGlueInfo,
	u_int8_t enable, bool fgResetHif);

static void mt7925WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	uint32_t index);

static void mt7925InitPcieInt(struct GLUE_INFO *prGlueInfo);

#if CFG_SUPPORT_PCIE_ASPM
static void mt7925ConfigPcieAspm(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn);
#endif

static void mt7925ShowPcieDebugInfo(struct GLUE_INFO *prGlueInfo);

#if CFG_ASSERTDUMP_BYPASS_CHIP_RESET
static void mt7925bypassWfWdt(struct ADAPTER *prAdapter, bool fgBypass);
#endif

#if CFG_PCIE_LTR_UPDATE
static void mt7925PcieLTRValue(struct ADAPTER *prAdapter, uint8_t ucState);
#endif
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
static uint8_t mt7925_apsLinkPlanDecision(struct ADAPTER *prAdapter,
	struct AP_COLLECTION *prAp, enum ENUM_MLO_LINK_PLAN eLinkPlan,
	uint8_t ucBssIndex);
static void mt7925_apsFillBssDescSet(struct ADAPTER *prAdapter,
		struct BSS_DESC_SET *set, uint8_t ucBssIndex);
static void mt7925_apsUpdateTotalScore(struct ADAPTER *prAdapter,
	struct BSS_DESC *arLinks[], uint8_t ucLinkNum,
	struct AP_SCORE_INFO *prScoreInfo, uint8_t ucBssidx);
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct ECO_INFO mt7925_eco_table[] = {
	/* HW version,  ROM version,    Factory version */
	{0x00, 0x00, 0xA, 0x1},	/* E1 */
	{0x00, 0x00, 0x0, 0x0}	/* End of table */
};

uint8_t *apucmt7925FwName[] = {
	(uint8_t *) CFG_FW_FILENAME "_7925",
	NULL
};

#if defined(_HIF_PCIE)
struct PCIE_CHIP_CR_MAPPING mt7925_bus2chip_cr_mapping[] = {
	/* chip addr, bus addr, range */
	{0x830c0000, 0x00000, 0x1000}, /* WF_MCU_BUS_CR_REMAP */
	{0x54000000, 0x02000, 0x1000},  /* WFDMA PCIE0 MCU DMA0 */
	{0x55000000, 0x03000, 0x1000},  /* WFDMA PCIE0 MCU DMA1 */
	{0x56000000, 0x04000, 0x1000},  /* WFDMA reserved */
	{0x57000000, 0x05000, 0x1000},  /* WFDMA MCU wrap CR */
	{0x58000000, 0x06000, 0x1000},  /* WFDMA PCIE1 MCU DMA0 (MEM_DMA) */
	{0x59000000, 0x07000, 0x1000},  /* WFDMA PCIE1 MCU DMA1 */
	{0x820c0000, 0x08000, 0x4000},  /* WF_UMAC_TOP (PLE) */
	{0x820c8000, 0x0c000, 0x2000},  /* WF_UMAC_TOP (PSE) */
	{0x820cc000, 0x0e000, 0x2000},  /* WF_UMAC_TOP (PP) */
	{0x74030000, 0x10000, 0x1000},  /* PCIe MAC */
	{0x820e0000, 0x20000, 0x0400},  /* WF_LMAC_TOP BN0 (WF_CFG) */
	{0x820e1000, 0x20400, 0x0200},  /* WF_LMAC_TOP BN0 (WF_TRB) */
	{0x820e2000, 0x20800, 0x0400},  /* WF_LMAC_TOP BN0 (WF_AGG) */
	{0x820e3000, 0x20c00, 0x0400},  /* WF_LMAC_TOP BN0 (WF_ARB) */
	{0x820e4000, 0x21000, 0x0400},  /* WF_LMAC_TOP BN0 (WF_TMAC) */
	{0x820e5000, 0x21400, 0x0800},  /* WF_LMAC_TOP BN0 (WF_RMAC) */
	{0x820ce000, 0x21c00, 0x0200},  /* WF_LMAC_TOP (WF_SEC) */
	{0x820e7000, 0x21e00, 0x0200},  /* WF_LMAC_TOP BN0 (WF_DMA) */
	{0x820cf000, 0x22000, 0x1000},  /* WF_LMAC_TOP (WF_PF) */
	{0x820e9000, 0x23400, 0x0200},  /* WF_LMAC_TOP BN0 (WF_WTBLOFF) */
	{0x820ea000, 0x24000, 0x0200},  /* WF_LMAC_TOP BN0 (WF_ETBF) */
	{0x820eb000, 0x24200, 0x0400},  /* WF_LMAC_TOP BN0 (WF_LPON) */
	{0x820ec000, 0x24600, 0x0200},  /* WF_LMAC_TOP BN0 (WF_INT) */
	{0x820ed000, 0x24800, 0x0800},  /* WF_LMAC_TOP BN0 (WF_MIB) */
	{0x820ca000, 0x26000, 0x2000},  /* WF_LMAC_TOP BN0 (WF_MUCOP) */
	{0x820d0000, 0x30000, 0x10000}, /* WF_LMAC_TOP (WF_WTBLON) */
	{0x40000000, 0x70000, 0x10000}, /* WF_UMAC_SYSRAM */
	{0x00400000, 0x80000, 0x10000}, /* WF_MCU_SYSRAM */
	{0x00410000, 0x90000, 0x10000}, /* WF_MCU_SYSRAM (configure register) */
	{0x820f0000, 0xa0000, 0x0400},  /* WF_LMAC_TOP BN1 (WF_CFG) */
	{0x820f1000, 0xa0600, 0x0200},  /* WF_LMAC_TOP BN1 (WF_TRB) */
	{0x820f2000, 0xa0800, 0x0400},  /* WF_LMAC_TOP BN1 (WF_AGG) */
	{0x820f3000, 0xa0c00, 0x0400},  /* WF_LMAC_TOP BN1 (WF_ARB) */
	{0x820f4000, 0xa1000, 0x0400},  /* WF_LMAC_TOP BN1 (WF_TMAC) */
	{0x820f5000, 0xa1400, 0x0800},  /* WF_LMAC_TOP BN1 (WF_RMAC) */
	{0x820f7000, 0xa1e00, 0x0200},  /* WF_LMAC_TOP BN1 (WF_DMA) */
	{0x820f9000, 0xa3400, 0x0200},  /* WF_LMAC_TOP BN1 (WF_WTBLOFF) */
	{0x820fa000, 0xa4000, 0x0200},  /* WF_LMAC_TOP BN1 (WF_ETBF) */
	{0x820fb000, 0xa4200, 0x0400},  /* WF_LMAC_TOP BN1 (WF_LPON) */
	{0x820fc000, 0xa4600, 0x0200},  /* WF_LMAC_TOP BN1 (WF_INT) */
	{0x820fd000, 0xa4800, 0x0800},  /* WF_LMAC_TOP BN1 (WF_MIB) */
	{0x820c4000, 0xa8000, 0x4000},  /* WF_LMAC_TOP BN1 (WF_MUCOP) */
	{0x820b0000, 0xae000, 0x1000},  /* [APB2] WFSYS_ON */
	{0x80020000, 0xb0000, 0x10000}, /* WF_TOP_MISC_OFF */
	{0x81020000, 0xc0000, 0x10000}, /* WF_TOP_MISC_ON */
	{0x7c020000, 0xd0000, 0x10000}, /* CONN_INFRA, wfdma */
	{0x7c060000, 0xe0000, 0x10000}, /* CONN_INFRA, conn_host_csr_top */
	{0x7c000000, 0xf0000, 0x10000}, /* CONN_INFRA */
	{0x70020000, 0x1f0000, 0x10000}, /* Reserved for CBTOP, can't switch */
	{0x7c500000, MT7925_PCIE2AP_REMAP_BASE_ADDR, 0x2000000}, /* remap */
	{0x70000000, 0x1e0000, 0x9000},
	{0x7c090000, 0x150000, 0x10000}, /* Remap change on owl */
	{0x0, 0x0, 0x0} /* End */
};
#endif

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
struct pcie2ap_remap mt7925_pcie2ap_remap = {
	.reg_base = CONCAT(CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP,
		_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_ADDR),
	.reg_mask = CONCAT(CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP,
		_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_MASK),
	.reg_shift = CONCAT(CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP,
		_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_SHFT),
	.base_addr = MT7925_PCIE2AP_REMAP_BASE_ADDR
};

struct ap2wf_remap mt7925_ap2wf_remap = {
	.reg_base = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_ADDR,
	.reg_mask = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_MASK,
	.reg_shift = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_SHFT,
	.base_addr = MT7925_REMAP_BASE_ADDR
};

struct PCIE_CHIP_CR_REMAPPING mt7925_bus2chip_cr_remapping = {
	.pcie2ap = &mt7925_pcie2ap_remap,
	.ap2wf = &mt7925_ap2wf_remap,
};

struct wfdma_group_info mt7925_wfmda_host_tx_group[] = {
	{"P0T0:AP DATA0", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL0_ADDR},
	{"P0T1:AP DATA1", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING1_CTRL0_ADDR},
	{"P0T2:AP DATA2", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING2_CTRL0_ADDR},
	{"P0T3:AP DATA3", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING3_CTRL0_ADDR},
	{"P0T8:MD DATA0", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING8_CTRL0_ADDR},
	{"P0T9:MD DATA1", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING9_CTRL0_ADDR},
	{"P0T10:MD DATA2", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING10_CTRL0_ADDR},
	{"P0T11:MD DATA3", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING11_CTRL0_ADDR},
	{"P0T14:MD CMD", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING14_CTRL0_ADDR},
	{"P0T15:AP CMD", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING15_CTRL0_ADDR},
	{"P0T16:FWDL", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING16_CTRL0_ADDR},
};

struct wfdma_group_info mt7925_wfmda_host_rx_group[] = {
	{"P0R2:AP DATA0", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_CTRL0_ADDR},
	{"P0R0:AP EVENT", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL0_ADDR},
	{"P0R3:AP DATA1", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING3_CTRL0_ADDR},
	{"P0R1:AP TDONE", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING1_CTRL0_ADDR},
};

struct wfdma_group_info mt7925_wfmda_wm_tx_group[] = {
	{"P0T6:LMAC TXD", WF_WFDMA_MCU_DMA0_WPDMA_TX_RING6_CTRL0_ADDR},
};

struct wfdma_group_info mt7925_wfmda_wm_rx_group[] = {
	{"P0R0:FWDL", WF_WFDMA_MCU_DMA0_WPDMA_RX_RING0_CTRL0_ADDR},
	{"P0R2:TXD0", WF_WFDMA_MCU_DMA0_WPDMA_RX_RING2_CTRL0_ADDR},
	{"P0R3:TXD1", WF_WFDMA_MCU_DMA0_WPDMA_RX_RING3_CTRL0_ADDR},
};

struct pse_group_info mt7925_pse_group[] = {
	{"HIF0(TX data)", WF_PSE_TOP_PG_HIF0_GROUP_ADDR,
		WF_PSE_TOP_HIF0_PG_INFO_ADDR},
	{"HIF1(Talos CMD)", WF_PSE_TOP_PG_HIF1_GROUP_ADDR,
		WF_PSE_TOP_HIF1_PG_INFO_ADDR},
	{"CPU(I/O r/w)",  WF_PSE_TOP_PG_CPU_GROUP_ADDR,
		WF_PSE_TOP_CPU_PG_INFO_ADDR},
	{"PLE(host report)",  WF_PSE_TOP_PG_PLE_GROUP_ADDR,
		WF_PSE_TOP_PLE_PG_INFO_ADDR},
	{"PLE1(SPL report)", WF_PSE_TOP_PG_PLE1_GROUP_ADDR,
		WF_PSE_TOP_PLE1_PG_INFO_ADDR},
	{"LMAC0(RX data)", WF_PSE_TOP_PG_LMAC0_GROUP_ADDR,
			WF_PSE_TOP_LMAC0_PG_INFO_ADDR},
	{"LMAC1(RX_VEC)", WF_PSE_TOP_PG_LMAC1_GROUP_ADDR,
			WF_PSE_TOP_LMAC1_PG_INFO_ADDR},
	{"LMAC2(TXS)", WF_PSE_TOP_PG_LMAC2_GROUP_ADDR,
			WF_PSE_TOP_LMAC2_PG_INFO_ADDR},
	{"LMAC3(TXCMD/RXRPT)", WF_PSE_TOP_PG_LMAC3_GROUP_ADDR,
			WF_PSE_TOP_LMAC3_PG_INFO_ADDR},
	{"MDP",  WF_PSE_TOP_PG_MDP_GROUP_ADDR,
			WF_PSE_TOP_MDP_PG_INFO_ADDR},
};
#endif /*_HIF_PCIE || _HIF_AXI */

#if defined(_HIF_PCIE)
struct pcie_msi_layout mt7925_pcie_msi_layout[] = {
	{"conn_hif_host_int", mtk_pci_isr, mtk_pci_isr_thread, AP_INT, 0},
	{"conn_hif_host_int", mtk_pci_isr, mtk_pci_isr_thread, AP_INT, 0},
	{"conn_hif_host_int", mtk_pci_isr, mtk_pci_isr_thread, AP_INT, 0},
	{"conn_hif_host_int", mtk_pci_isr, mtk_pci_isr_thread, AP_INT, 0},
	{"conn_hif_host_int", mtk_pci_isr, mtk_pci_isr_thread, AP_INT, 0},
	{"conn_hif_host_int", mtk_pci_isr, mtk_pci_isr_thread, AP_INT, 0},
	{"conn_hif_host_int", mtk_pci_isr, mtk_pci_isr_thread, AP_INT, 0},
	{"conn_hif_host_int", mtk_pci_isr, mtk_pci_isr_thread, AP_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"wm_conn2ap_wdt_irq", NULL, NULL, NONE_INT, 0},
	{"wf_mcu_jtag_det_eint", NULL, NULL, NONE_INT, 0},
	{"pmic_eint", NULL, NULL, NONE_INT, 0},
	{"ccif_bgf2ap_sw_irq", NULL, NULL, NONE_INT, 0},
	{"ccif_wf2ap_sw_irq", pcie_sw_int_top_handler,
	 pcie_sw_int_thread_handler, AP_MISC_INT, 0},
	{"ccif_bgf2ap_irq_0", NULL, NULL, NONE_INT, 0},
	{"ccif_bgf2ap_irq_1", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
};
#endif

struct BUS_INFO mt7925_bus_info = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.top_cfg_base = MT7925_TOP_CFG_BASE,

	/* host_dma0 for TXP */
	.host_dma0_base = WF_WFDMA_HOST_DMA0_BASE,
	.host_int_status_addr = WF_WFDMA_HOST_DMA0_HOST_INT_STA_ADDR,

	.host_int_txdone_bits =
	(
#if (CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0)
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_0_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_1_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_2_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_3_MASK |
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0 */
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_15_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_16_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_mcu2host_sw_int_sts_MASK),
	.host_int_rxdone_bits =
	(WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_0_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_1_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_2_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_3_MASK),

	.host_tx_ring_base = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL0_ADDR,
	.host_tx_ring_ext_ctrl_base =
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_EXT_CTRL_ADDR,
	.host_tx_ring_cidx_addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL2_ADDR,
	.host_tx_ring_didx_addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL3_ADDR,
	.host_tx_ring_cnt_addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL1_ADDR,

	.host_rx_ring_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL0_ADDR,
	.host_rx_ring_ext_ctrl_base =
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_EXT_CTRL_ADDR,
	.host_rx_ring_cidx_addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL2_ADDR,
	.host_rx_ring_didx_addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL3_ADDR,
	.host_rx_ring_cnt_addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL1_ADDR,

	.bus2chip = mt7925_bus2chip_cr_mapping,
	.bus2chip_remap = &mt7925_bus2chip_cr_remapping,
	.max_static_map_addr = 0x00200000,

	.tx_ring_fwdl_idx = CONNAC3X_FWDL_TX_RING_IDX,
	.tx_ring_cmd_idx = 15,
	.tx_ring0_data_idx = 0,
	.tx_ring1_data_idx = 1,
	.tx_prio_data_idx = 2,
	.tx_altx_data_idx = 3,
	.rx_data_ring_num = 2,
	.rx_evt_ring_num = 2,
	.rx_data_ring_size = 3072,
	.rx_evt_ring_size = 128,
	.fw_own_clear_addr = CONNAC3X_BN0_IRQ_STAT_ADDR,
	.fw_own_clear_bit = PCIE_LPCR_FW_CLR_OWN,
	.fgCheckDriverOwnInt = FALSE,
	.u4DmaMask = 32,
	.wfmda_host_tx_group = mt7925_wfmda_host_tx_group,
	.wfmda_host_tx_group_len = ARRAY_SIZE(mt7925_wfmda_host_tx_group),
	.wfmda_host_rx_group = mt7925_wfmda_host_rx_group,
	.wfmda_host_rx_group_len = ARRAY_SIZE(mt7925_wfmda_host_rx_group),
	.wfmda_wm_tx_group = mt7925_wfmda_wm_tx_group,
	.wfmda_wm_tx_group_len = ARRAY_SIZE(mt7925_wfmda_wm_tx_group),
	.wfmda_wm_rx_group = mt7925_wfmda_wm_rx_group,
	.wfmda_wm_rx_group_len = ARRAY_SIZE(mt7925_wfmda_wm_rx_group),
	.prDmashdlCfg = &rMt7925DmashdlCfg,
#if (DBG_DISABLE_ALL_INFO == 0)
	.prPleTopCr = &rMt7925PleTopCr,
	.prPseTopCr = &rMt7925PseTopCr,
	.prPpTopCr = &rMt7925PpTopCr,
#endif
	.prPseGroup = mt7925_pse_group,
	.u4PseGroupLen = ARRAY_SIZE(mt7925_pse_group),
	.pdmaSetup = mt7925WpdmaConfig,
	.enableInterrupt = mt7925EnableInterrupt,
	.disableInterrupt = mt7925DisableInterrupt,
#if defined(_HIF_PCIE)
	.initPcieInt = mt7925InitPcieInt,
#if CFG_SUPPORT_PCIE_ASPM
	.configPcieAspm = mt7925ConfigPcieAspm,
#endif
	.pdmaStop = asicConnac3xWfdmaStop,
	.pdmaPollingIdle = asicConnac3xWfdmaPollingAllIdle,
	.pcie_msi_info = {
		.prMsiLayout = mt7925_pcie_msi_layout,
		.u4MaxMsiNum = ARRAY_SIZE(mt7925_pcie_msi_layout),
	},
	.showDebugInfo = mt7925ShowPcieDebugInfo,
#if CFG_ASSERTDUMP_BYPASS_CHIP_RESET
	.bypassWfWdt = mt7925bypassWfWdt,
#endif
#if CFG_PCIE_LTR_UPDATE
	.pcieLTRValue = mt7925PcieLTRValue,
#endif
#endif /* _HIF_PCIE */
	.processTxInterrupt = mt7925ProcessTxInterrupt,
	.processRxInterrupt = mt7925ProcessRxInterrupt,
	.tx_ring_ext_ctrl = asicConnac3xWfdmaTxRingExtCtrl,
	.rx_ring_ext_ctrl = mt7925WfdmaRxRingExtCtrl,
	/* null wfdmaManualPrefetch if want to disable manual mode */
	.wfdmaManualPrefetch = mt7925WfdmaManualPrefetch,
	.lowPowerOwnRead = asicConnac3xLowPowerOwnRead,
	.lowPowerOwnSet = asicConnac3xLowPowerOwnSet,
	.lowPowerOwnClear = asicConnac3xLowPowerOwnClear,
	.wakeUpWiFi = asicWakeUpWiFi,
	.processSoftwareInterrupt = asicConnac3xProcessSoftwareInterrupt,
	.softwareInterruptMcu = asicConnac3xSoftwareInterruptMcu,
	.hifRst = asicConnac3xHifRst,
	.devReadIntStatus = mt7925ReadIntStatus,
	.setRxRingHwAddr = mt7925SetRxRingHwAddr,
	.wfdmaAllocRxRing = mt7925WfdmaAllocRxRing,
#endif /*_HIF_PCIE || _HIF_AXI */
#if defined(_HIF_PCIE) || defined(_HIF_AXI) || defined(_HIF_USB)
	.DmaShdlInit = mt7925DmashdlInit,
	.updateTxRingMaxQuota = mt7925UpdateDmashdlQuota,
#endif
#if defined(_HIF_USB)
	.prDmashdlCfg = &rMt7925DmashdlCfg,
	.u4UdmaWlCfg_0_Addr = CONNAC3X_UDMA_WLCFG_0,
	.u4UdmaWlCfg_1_Addr = CONNAC3X_UDMA_WLCFG_1,
	.u4UdmaWlCfg_0 =
	    (CONNAC3X_UDMA_WLCFG_0_WL_TX_EN(1) |
	     CONNAC3X_UDMA_WLCFG_0_WL_RX_EN(1) |
	     CONNAC3X_UDMA_WLCFG_0_WL_RX_MPSZ_PAD0(1) |
	     CONNAC3X_UDMA_WLCFG_0_TICK_1US_EN(1)),
	.u4UdmaTxQsel = CONNAC3X_UDMA_TX_QSEL,
	.u4device_vender_request_in = DEVICE_VENDOR_REQUEST_IN_CONNAC2,
	.u4device_vender_request_out = DEVICE_VENDOR_REQUEST_OUT_CONNAC2,
	.u4SuspendVer = SUSPEND_V2,
	.fgIsSupportWdtEp = TRUE,
	.asicUsbResume = asicConnac3xUsbResume,
	.asicUsbEventEpDetected = asicConnac3xUsbEventEpDetected,
	.asicUsbRxByteCount = asicConnac3xUsbRxByteCount,
	.asicUdmaRxFlush = asicConnac3xUdmaRxFlush,
#if CFG_CHIP_RESET_SUPPORT
	.asicUsbEpctlRstOpt = mt7925HalUsbEpctlRstOpt,
#endif
#endif
#if defined(_HIF_NONE)
	/* for compiler need one entry */
	.DmaShdlInit = NULL
#endif
};

#if CFG_ENABLE_FW_DOWNLOAD
struct FWDL_OPS_T mt7925_fw_dl_ops = {
	.constructFirmwarePrio = mt7925_ConstructFirmwarePrio,
	.constructPatchName = mt7925_ConstructPatchName,
#if CFG_SUPPORT_SINGLE_FW_BINARY
	.parseSingleBinaryFile = wlanParseSingleBinaryFile,
#endif
#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
	.constrcutIdxLogBin = mt7925_ConstructIdxLogBinName,
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */
	.downloadPatch = wlanDownloadPatch,
	.downloadFirmware = wlanConnacFormatDownload,
	.downloadByDynMemMap = NULL,
	.getFwInfo = wlanGetConnacFwInfo,
	.getFwDlInfo = asicGetFwDlInfo,
	.downloadEMI = wlanDownloadEMISectionViaDma,
#if (CFG_SUPPORT_PRE_ON_PHY_ACTION == 1)
	.phyAction = wlanPhyAction,
#else
	.phyAction = NULL,
#endif
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
	.constructBtPatchName = asicConnac3xConstructBtPatchName,
	.downloadBtPatch = asicConnac3xDownloadBtPatch,
#if (CFG_SUPPORT_CONNAC3X == 1)
	.configBtImageSection = asicConnac3xConfigBtImageSection,
#endif
#endif
	.getFwVerInfo = wlanParseRamCodeReleaseManifest,
	.getFlavorVer = mt7925GetFlavorVer,
};
#endif /* CFG_ENABLE_FW_DOWNLOAD */

struct TX_DESC_OPS_T mt7925_TxDescOps = {
	.fillNicAppend = fillNicTxDescAppend,
	.fillHifAppend = fillTxDescAppendByHostV2,
	.fillTxByteCount = fillConnac3xTxDescTxByteCount,
};

struct RX_DESC_OPS_T mt7925_RxDescOps = {0};

#if (DBG_DISABLE_ALL_INFO == 0)
struct CHIP_DBG_OPS mt7925_DebugOps = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.showPdmaInfo = connac3x_show_wfdma_info,
#endif
	.showPseInfo = connac3x_show_pse_info,
	.showPleInfo = connac3x_show_ple_info,
	.showTxdInfo = connac3x_show_txd_Info,
	.showWtblInfo = connac3x_show_wtbl_info,
	.showUmacWtblInfo = connac3x_show_umac_wtbl_info,
	.showCsrInfo = NULL,
#if defined(_HIF_PCIE) || defined(_HIF_AXI) || defined(_HIF_USB)
	.showDmaschInfo = connac3x_show_dmashdl_info,
#endif
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.getFwDebug = connac3x_get_ple_int,
	.setFwDebug = connac3x_set_ple_int,
#endif
	.showHifInfo = NULL,
	.printHifDbgInfo = NULL,
	.show_rx_rate_info = connac3x_show_rx_rate_info,
	.show_rx_rssi_info = connac3x_show_rx_rssi_info,
	.show_stat_info = connac3x_show_stat_info,
	.get_tx_info_from_txv = connac3x_get_tx_info_from_txv,
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	.show_mld_info = connac3x_show_mld_info,
#endif
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.show_wfdma_dbg_probe_info = mt7925_show_wfdma_dbg_probe_info,
	.show_wfdma_wrapper_info = mt7925_show_wfdma_wrapper_info,
	.dumpwfsyscpupcr = mt7925_dumpWfsyscpupcr,
	.dumpBusStatus = mt7925_DumpBusStatus,
#endif
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	.get_rx_rate_info = mt7925_get_rx_rate_info,
#endif
#if CFG_SUPPORT_LLS
	.get_rx_link_stats = mt7925_get_rx_link_stats,
#endif
	.dumpTxdInfo = connac3x_dump_tmac_info,
#if (CFG_SUPPORT_DEBUG_SOP == 1)
	.show_debug_sop_info = mt7925_show_debug_sop_info,
#endif
};
#endif /* DBG_DISABLE_ALL_INFO */

#if CFG_SUPPORT_QA_TOOL
#if (CONFIG_WLAN_SERVICE == 1)
struct test_capability mt7925_toolCapability = {
	/* u_int32 version; */
	8,
	/* u_int32 tag_num; */
	2,
	/* struct test_capability_ph_cap ph_cap; */
	{
		/* GET_CAPABILITY_TAG_PHY */
		1,	/* u_int32 tag; */

		/* GET_CAPABILITY_TAG_PHY_LEN */
		16,	/* u_int32 tag_len; */

		/* BIT0: 11 a/b/g, BIT1: 11n, BIT2: 11ac, BIT3: 11ax */
		0x1F,	/* u_int32 protocol; */

		/* 1:1x1, 2:2x2, ... */
		2,	/* u_int32 max_ant_num; */

		/* BIT0: DBDC support */
		1,	/* u_int32 dbdc; */

		/* BIT0: TxLDPC, BTI1: RxLDPC, BIT2: TxSTBC, BIT3: RxSTBC */
		0xF,	/* u_int32 coding; */

		/* BIT0: 2.4G, BIT1: 5G, BIT2: 6G */
		0x7,	/* u_int32 channel_band; */

		/* BIT0: BW20, BIT1: BW40, BIT2: BW80 */
		/* BIT3: BW160C, BIT4: BW80+80(BW160NC) */
		/* BIT5: BW320*/
		0xF,	/* u_int32 bandwidth; */

		/* BIT[15:0]: Band0 2.4G, 5G, 6G, 0x7 */
		/* BIT[31:16]: Band1 5G, 6G, 0x6 */
		0x00060007,	/* u_int32 channel_band_dbdc; */

		/* BIT[15:0]: Band2 N/A, 0 */
		/* BIT[31:16]: Band3 N/A, 0*/
		0x00000000,	/* u_int32 channel_band_dbdc_ext; */

		/* BIT[7:0]: Support phy 0x1 (bitwise) */
		/* BIT[15:8]: Support Adie 0x1 (bitwise) */
		0x0101,	/* u_int32 phy_adie_index; CFG_SUPPORT_CONNAC3X */

		/* BIT[7:0]: Band0 TX path 2 */
		/* BIT[15:8]: Band0 RX path 2 */
		/* BIT[23:16]: Band1 TX path 2 */
		/* BIT[31:24]: Band1 RX path 2 */
		0x02020202,	/* u_int32 band_0_1_wf_path_num; */

		/* BIT[7:0]: Band2 TX path 0 */
		/* BIT[15:8]: Band2 RX path 0*/
		/* BIT[23:16]: Band3 TX path 0 */
		/* BIT[31:24]: Band3 RX path 1 */
		0x00000000,	/* u_int32 band_2_3_wf_path_num; */

		/* BIT[7:0]: Band0 BW160, 0xF */
		/* BIT[15:8]: Band1 BW160, 0xF */
		/* BIT[23:16]: Band2 N/A, 0 */
		/* BIT[31:24]: Band3 N/A, 0 */
		0x00000F0F,	/* u_int32 band_bandwidth; */

		{ 0, 0, 0, 0 }	/* u_int32 reserved[4]; */
	},

	/* struct test_capability_ext_cap ext_cap; */
	{
		/* GET_CAPABILITY_TAG_PHY_EXT */
		2,	/* u_int32 tag; */
		/* GET_CAPABILITY_TAG_PHY_EXT_LEN */
		16,	/* u_int32 tag_len; */

		/* BIT0: AntSwap 0 */
		/* BIT1: HW TX support 0*/
		/* BIT2: Little core support 0 */
		/* BIT3: XTAL trim support 1 */
		/* BIT4: DBDC/MIMO switch support 1 */
		/* BIT5: eMLSR support 1 */
		/* BIT6: MLR+, ALR support 1 */
		/* BIT7: Bandwidth duplcate debug support 0 */
		/* BIT8: dRU support */
		0x78,	/*u_int32 feature1; */

		/* u_int32 reserved[15]; */
		{ 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0 }
	}
};
#endif

struct ATE_OPS_T mt7925_AteOps = {
	/* ICapStart phase out , wlan_service instead */
	.setICapStart = connacSetICapStart,
	/* ICapStatus phase out , wlan_service instead */
	.getICapStatus = connacGetICapStatus,
	/* CapIQData phase out , wlan_service instead */
	.getICapIQData = connacGetICapIQData,
	.getRbistDataDumpEvent = nicExtEventICapIQData,
#if (CFG_SUPPORT_ICAP_SOLICITED_EVENT == 1)
	.getICapDataDumpCmdEvent = nicExtCmdEventSolicitICapIQData,
#endif
	.icapRiseVcoreClockRate = mt7925_icapRiseVcoreClockRate,
	.icapDownVcoreClockRate = mt7925_icapDownVcoreClockRate,
#if (CONFIG_WLAN_SERVICE == 1)
	.tool_capability = &mt7925_toolCapability,
#endif
};
#endif /* CFG_SUPPORT_QA_TOOL */

#if CFG_SUPPORT_THERMAL_QUERY
struct thermal_sensor_info mt7925_thermal_sensor_info[] = {
	{"wifi_adie_0", THERMAL_TEMP_TYPE_ADIE, 0},
	{"wifi_ddie_0", THERMAL_TEMP_TYPE_DDIE, 0},
	{"wifi_ddie_1", THERMAL_TEMP_TYPE_DDIE, 1},
	{"wifi_ddie_2", THERMAL_TEMP_TYPE_DDIE, 2},
	{"wifi_ddie_3", THERMAL_TEMP_TYPE_DDIE, 3},
};
#endif

struct mt66xx_chip_info mt66xx_chip_info_mt7925 = {
	.bus_info = &mt7925_bus_info,
#if CFG_ENABLE_FW_DOWNLOAD
	.fw_dl_ops = &mt7925_fw_dl_ops,
#endif /* CFG_ENABLE_FW_DOWNLOAD */
#if CFG_SUPPORT_QA_TOOL
	.prAteOps = &mt7925_AteOps,
#endif /* CFG_SUPPORT_QA_TOOL */
	.prTxDescOps = &mt7925_TxDescOps,
	.prRxDescOps = &mt7925_RxDescOps,
#if (DBG_DISABLE_ALL_INFO == 0)
	.prDebugOps = &mt7925_DebugOps,
#endif
	.chip_id = MT7925_CHIP_ID,
	.should_verify_chip_id = FALSE,
	.sw_sync0 = CONNAC3X_CONN_CFG_ON_CONN_ON_MISC_ADDR,
	.sw_ready_bits = WIFI_FUNC_NO_CR4_READY_BITS,
	.sw_ready_bit_offset =
		Connac3x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT,
#if defined(_HIF_USB)
	.vdr_pwr_on = USB_VND_PWR_ON_ADDR,
	.vdr_pwr_on_chk_bit = USB_VND_PWR_ON_ACK_BIT,
	.is_need_check_vdr_pwr_on = FALSE,
#endif
	.patch_addr = MT7925_PATCH_START_ADDR,
	.is_support_cr4 = FALSE,
	.is_support_wacpu = FALSE,
	.sw_sync_emi_info = NULL,
	.txd_append_size = MT7925_TX_DESC_APPEND_LENGTH,
	.hif_txd_append_size = MT7925_HIF_TX_DESC_APPEND_LENGTH,
	.rxd_size = MT7925_RX_DESC_LENGTH,
	.init_evt_rxd_size = MT7925_RX_INIT_DESC_LENGTH,
	.pse_header_length = CONNAC3X_NIC_TX_PSE_HEADER_LENGTH,
	.init_event_size = CONNAC3X_RX_INIT_EVENT_LENGTH,
	.eco_info = mt7925_eco_table,
	.isNicCapV1 = FALSE,
	.is_support_efuse = TRUE,
	.top_hcr = CONNAC3X_TOP_HCR,
	.top_hvr = CONNAC3X_TOP_HVR,
	.top_fvr = CONNAC3X_TOP_FVR,
#if (CFG_SUPPORT_802_11AX == 1)
	.arb_ac_mode_addr = MT7925_ARB_AC_MODE_ADDR,
#endif
	.asicCapInit = asicConnac3xCapInit,
#if CFG_ENABLE_FW_DOWNLOAD
	.asicEnableFWDownload = NULL,
#endif /* CFG_ENABLE_FW_DOWNLOAD */
	.downloadBufferBin = wlanConnac3XDownloadBufferBin,
	.is_support_hw_amsdu = TRUE,
	.is_support_nvram_fragment = TRUE,
	.is_support_asic_lp = TRUE,
	.asicWfdmaReInit = asicConnac3xWfdmaReInit,
	.asicWfdmaReInit_handshakeInit = asicConnac3xWfdmaDummyCrWrite,
	.group5_size = sizeof(struct HW_MAC_RX_STS_GROUP_5),
	.u4LmacWtblDUAddr = CONNAC3X_WIFI_LWTBL_BASE,
	.u4UmacWtblDUAddr = CONNAC3X_WIFI_UWTBL_BASE,
	.isSupportMddpAOR = false,
	.isSupportMddpSHM = false,
	.u4HostWfdmaBaseAddr = WF_WFDMA_HOST_DMA0_BASE,
	.u4HostWfdmaWrapBaseAddr = WF_WFDMA_EXT_WRAP_CSR_BASE,
	.u4McuWfdmaBaseAddr = WF_WFDMA_MCU_DMA0_BASE,
	.u4DmaShdlBaseAddr = WF_HIF_DMASHDL_TOP_BASE,
	.cmd_max_pkt_size = CFG_TX_MAX_PKT_SIZE, /* size 1600 */

	.prTxPwrLimitFile = "TxPwrLimit_MT7925.dat",
#if (CFG_SUPPORT_POWER_SKU_ENHANCE == 1)
	.prTxPwrLimit1ss1tFile = "TxPwrLimit_MT7925_1ss1t.dat",
#endif
#if (CFG_SUPPORT_SINGLE_SKU_6G == 1)
	.prTxPwrLimit6GFile = "TxPwrLimit6G_MT7925.dat",
#if (CFG_SUPPORT_SINGLE_SKU_6G_1SS1T == 1)
	.prTxPwrLimit6G1ss1tFile = "TxPwrLimit6G_MT7925_1ss1t.dat",
#endif
#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
	.prTxPwrLimit6GVlpFile = "TxPwrLimit6G_MT7925_VLP.dat",
	.prTxPwrLimit6GSpFile = "TxPwrLimit6G_MT7925_SP.dat",
#endif  /*CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1*/
#endif
	.ucTxPwrLimitBatchSize = 3,

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	.apsLinkPlanDecision = mt7925_apsLinkPlanDecision,
	.apsFillBssDescSet = mt7925_apsFillBssDescSet,
	.apsUpdateTotalScore = mt7925_apsUpdateTotalScore,
#endif

#if defined(_HIF_USB)
	.asicUsbInit = asicConnac3xWfdmaInitForUSB,
	.asicUsbInit_ic_specific = NULL,
	.u4SerUsbMcuEventAddr = WF_SW_DEF_CR_USB_MCU_EVENT_ADDR,
	.u4SerUsbHostAckAddr = WF_SW_DEF_CR_USB_HOST_ACK_ADDR,
#endif
	.chip_capability = BIT(CHIP_CAPA_FW_LOG_TIME_SYNC) |
		BIT(CHIP_CAPA_XTAL_TRIM),
	.custom_oid_interface_version = MTK_CUSTOM_OID_INTERFACE_VERSION,
	.em_interface_version = MTK_EM_INTERFACE_VERSION,
#if CFG_CHIP_RESET_SUPPORT
	.asicWfsysRst = mt7925HalCbInfraRguWfRst,
	.asicPollWfsysSwInitDone = mt7925HalPollWfsysSwInitDone,
#endif
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	/* owner set true when feature is ready. */
	.fgIsSupportL0p5Reset = TRUE,
#elif defined(_HIF_USB)
	.fgIsSupportL0p5Reset = TRUE,
	.dmashdlQuotaDecision = mt7925dmashdlQuotaDecision,
#elif defined(_HIF_SDIO)
	/* owner set true when feature is ready. */
	.fgIsSupportL0p5Reset = FALSE,
#endif
	.u4MinTxLen = 2,
};

struct mt66xx_hif_driver_data mt66xx_driver_data_mt7925 = {
	.chip_info = &mt66xx_chip_info_mt7925,
};

void mt7925_icapRiseVcoreClockRate(void)
{

#if (CFG_SUPPORT_VCODE_VDFS == 1)
	int value = 0;

#if (KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE)
	/* Implementation for kernel-5.4 */
	struct mt66xx_hif_driver_data *prDriverData =
		get_platform_driver_data();
	struct mt66xx_chip_info *prChipInfo;
	void *pdev;

	prChipInfo = ((struct mt66xx_hif_driver_data *)prDriverData)
		->chip_info;
	pdev = (void *)prChipInfo->pdev;

	dvfsrc_vcore_power = regulator_get(
		&((struct platform_device *)pdev)->dev, "dvfsrc-vcore");

	/* Enable VCore to 0.725 */
	regulator_set_voltage(dvfsrc_vcore_power, 725000, INT_MAX);
#else
	/* init */
	if (!pm_qos_request_active(&wifi_req))
		pm_qos_add_request(&wifi_req, PM_QOS_VCORE_OPP,
						PM_QOS_VCORE_OPP_DEFAULT_VALUE);

	/* update Vcore */
	pm_qos_update_request(&wifi_req, 0);
#endif

	DBGLOG(HAL, STATE, "icapRiseVcoreClockRate done\n");

	/* Seq2: update clock rate sel bus clock to 213MHz */

	/* 0x1801_2050[6:4]=3'b111 */
	wf_ioremap_read(WF_CONN_INFA_BUS_CLOCK_RATE, &value);
	value |= 0x00000070;
	wf_ioremap_write(WF_CONN_INFA_BUS_CLOCK_RATE, value);

	/* Seq3: enable clock select sw mode */

	/* 0x1801_2050[0]=1'b1 */
	wf_ioremap_read(WF_CONN_INFA_BUS_CLOCK_RATE, &value);
	value |= 0x1;
	wf_ioremap_write(WF_CONN_INFA_BUS_CLOCK_RATE, value);

#else
	DBGLOG(HAL, STATE, "icapRiseVcoreClockRate skip\n");
#endif  /*#ifndef CFG_BUILD_X86_PLATFORM*/
}

void mt7925_icapDownVcoreClockRate(void)
{

#if (CFG_SUPPORT_VCODE_VDFS == 1)
	int value = 0;

#if (KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE)
	/* Implementation for kernel-5.4 */
	struct mt66xx_chip_info *prChipInfo;
	struct mt66xx_hif_driver_data *prDriverData =
		get_platform_driver_data();
	void *pdev;

	prChipInfo = ((struct mt66xx_hif_driver_data *)prDriverData)
		->chip_info;
	pdev = (void *)prChipInfo->pdev;

	dvfsrc_vcore_power = regulator_get(
		&((struct platform_device *)pdev)->dev, "dvfsrc-vcore");

	/* resume to default Vcore value */
	regulator_set_voltage(dvfsrc_vcore_power, 575000, INT_MAX);
#else
	/*init*/
	if (!pm_qos_request_active(&wifi_req))
		pm_qos_add_request(&wifi_req, PM_QOS_VCORE_OPP,
						PM_QOS_VCORE_OPP_DEFAULT_VALUE);

	/*restore to default Vcore*/
	pm_qos_update_request(&wifi_req,
		PM_QOS_VCORE_OPP_DEFAULT_VALUE);
#endif

	/*disable VCore to normal setting*/
	DBGLOG(HAL, STATE, "icapDownVcoreClockRate done!\n");

	/* Seq2: update clock rate sel bus clock to default value */

	/* 0x1801_2050[6:4]=3'b000 */
	wf_ioremap_read(WF_CONN_INFA_BUS_CLOCK_RATE, &value);
	value &= ~(0x00000070);
	wf_ioremap_write(WF_CONN_INFA_BUS_CLOCK_RATE, value);

	/* Seq3: disble clock select sw mode */

	/* 0x1801_2050[0]=1'b0 */
	wf_ioremap_read(WF_CONN_INFA_BUS_CLOCK_RATE, &value);
	value &= ~(0x1);
	wf_ioremap_write(WF_CONN_INFA_BUS_CLOCK_RATE, value);

#else
	DBGLOG(HAL, STATE, "icapDownVcoreClockRate skip\n");
#endif  /*#ifndef CFG_BUILD_X86_PLATFORM*/
}

static void mt7925_ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx)
{
	int ret = 0;
	uint8_t ucIdx = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7925GetFlavorVer(prGlueInfo, &aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt7925_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7925_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt7925_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7925_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2 */
	/* WIFI_RAM_CODE_MT7925_1_1.bin */
	/* WIFI_RAM_CODE_MT7925_1t_1.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"WIFI_RAM_CODE_MT%x_%s_%u.bin",
			MT7925_CHIP_ID,
			aucFlavor,
			MT7925_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	for (ucIdx = 0; apucmt7925FwName[ucIdx]; ucIdx++) {
		if ((*pucNameIdx + 3) >= ucMaxNameIdx) {
			/* the table is not large enough */
			DBGLOG(INIT, ERROR,
				"kalFirmwareImageMapping >> file name array is not enough.\n");
			return;
		}

		/* Type 3. WIFI_RAM_CODE_7925.bin */
		ret = kalSnprintf(*(apucName + (*pucNameIdx)),
				CFG_FW_NAME_MAX_LEN, "%s.bin",
				apucmt7925FwName[ucIdx]);
		if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
			(*pucNameIdx) += 1;
		else
			DBGLOG(INIT, ERROR,
				"[%u] kalSnprintf failed, ret: %d\n",
				__LINE__, ret);
	}
}

static void mt7925_ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7925GetFlavorVer(prGlueInfo, &aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt7925_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7925_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt7925_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7925_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2 */
	/* WIFI_MT7925_PATCH_MCU_1_1_hdr.bin */
	/* WIFI_MT7925_PATCH_MCU_1t_1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_MT%x_PATCH_MCU_%s_%u_hdr.bin",
			  MT7925_CHIP_ID,
			  aucFlavor,
			  MT7925_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 3. mt7925_patch_e1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "mt7925_patch_e1_hdr.bin");
	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
}

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
static void mt7925_ConstructIdxLogBinName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	mt7925GetFlavorVer(prGlueInfo, &aucFlavor[0]);

	/* Type 1 */
	/* WIFI_RAM_CODE_MT7925_1_1_idxlog.bin */
	/* WIFI_RAM_CODE_MT7925_1t_1_idxlog.bin */
	ret = kalSnprintf(apucName[0],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_RAM_CODE_MT%x_%s_%u_idxlog.bin",
			  MT7925_CHIP_ID,
			  aucFlavor,
			  MT7925_ROM_VERSION);

	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
}
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */

#if defined(_HIF_PCIE)
static uint8_t mt7925SetRxRingHwAddr(struct RTMP_RX_RING *prRxRing,
	struct BUS_INFO *prBusInfo, uint32_t u4SwRingIdx)
{
	uint32_t offset = 0;

	/*
	 * RX_RING_DATA0   (RX_Ring0) - Band0 Rx Data
	 * RX_RING_DATA1 (RX_Ring1) - Band1 Rx Data
	 * RX_RING_EVT    (RX_Ring2) - Band0 Tx Free Done Event / Rx Event
	 * RX_RING_TXDONE0 (RX_Ring3) - Band1 Tx Free Done Event
	*/
	switch (u4SwRingIdx) {
	case RX_RING_EVT:
		offset = 0;
		break;
	case RX_RING_DATA0:
		offset = 2;
		break;
	case RX_RING_DATA1:
		offset = 3;
		break;
	case RX_RING_TXDONE0:
		offset = 1;
		break;
	default:
		return FALSE;
	}

	halSetRxRingHwAddr(prRxRing, prBusInfo, offset);

	return TRUE;
}

static bool mt7925WfdmaAllocRxRing(struct GLUE_INFO *prGlueInfo,
	bool fgAllocMem)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;

	/* Band1 Data Rx path */
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_DATA1, prHifInfo->u4RxDataRingSize,
			RXD_SIZE, CFG_RX_MAX_PKT_SIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[2] fail\n");
		return false;
	}
	/* Band0 Tx Free Done Event */
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_TXDONE0, prHifInfo->u4RxEvtRingSize,
			RXD_SIZE, RX_BUFFER_AGGRESIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[3] fail\n");
		return false;
	}
	return true;
}

static void mt7925ProcessTxInterrupt(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if (u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_16_MASK)
		halWpdmaProcessCmdDmaDone(
			prAdapter->prGlueInfo, TX_RING_FWDL);

	if (u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_15_MASK)
		halWpdmaProcessCmdDmaDone(
			prAdapter->prGlueInfo, TX_RING_CMD);

#if (CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0)
	if (u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_0_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA0);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}

	if (u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_1_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA1);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}

	if (u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_2_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA_PRIO);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}

	if (u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_3_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA_ALTX);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0 */
}

static void mt7925ProcessRxDataInterrupt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if ((u4Sta &
	     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_2_MASK) ||
	    (KAL_TEST_BIT(RX_RING_DATA0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_DATA0, TRUE);

	if ((u4Sta &
	     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_3_MASK) ||
	    (KAL_TEST_BIT(RX_RING_DATA1, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_DATA1, TRUE);
}

static void mt7925ProcessRxInterrupt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	mt7925ProcessRxDataInterrupt(prAdapter);

	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_0_MASK) ||
	    (KAL_TEST_BIT(RX_RING_EVT, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_EVT, FALSE);

	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_1_MASK) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE0, FALSE);
}

static void mt7925SetMDTRXRingPriorityInterrupt(struct ADAPTER *prAdapter)
{
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_INT_RX_PRI_SEL_ADDR, 0xF00);
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_INT_TX_PRI_SEL_ADDR, 0x7F00);
}

static void mt7925WfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint32_t val = 0;
	uint32_t u4WrVal = 0, u4Addr = 0;
	uint32_t u4PrefetchCnt = 0x4, u4TxDataPrefetchCnt = 0x10;

	HAL_MCR_RD(prAdapter, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR, &val);
	/* disable prefetch offset calculation auto-mode */
	val &=
	~WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN_MASK;
	HAL_MCR_WR(prAdapter, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR, val);

	/* Rx ring */
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_RX_RING3_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += 0x00400000;
	}

	/* Tx ring */
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_TX_RING3_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4TxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += 0x01000000;
	}

	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING15_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_TX_RING16_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += 0x00400000;
	}

	mt7925SetMDTRXRingPriorityInterrupt(prAdapter);

	/* reset dma TRX idx */
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_RST_DTX_PTR_ADDR, 0xFFFFFFFF);
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_RST_DRX_PTR_ADDR, 0xFFFFFFFF);
}

static void mt7925ReadIntStatus(struct ADAPTER *prAdapter,
	uint32_t *pu4IntStatus)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct BUS_INFO *prBusInfo = prChipInfo->bus_info;
	uint32_t u4RegValue = 0, u4WrValue = 0, u4Addr;
#if CFG_CHIP_RESET_SUPPORT
	uint32_t u4IntSta = 0;
#endif
	*pu4IntStatus = 0;

	u4Addr = WF_WFDMA_HOST_DMA0_HOST_INT_STA_ADDR;
	HAL_MCR_RD(prAdapter, u4Addr, &u4RegValue);

#if defined(_HIF_PCIE) || defined(_HIF_AXI)

	if (HAL_IS_CONNAC3X_EXT_RX_DONE_INTR(
		    u4RegValue, prBusInfo->host_int_rxdone_bits)) {
		*pu4IntStatus |= WHISR_RX0_DONE_INT;
		u4WrValue |= (u4RegValue & prBusInfo->host_int_rxdone_bits);
	}

	if (HAL_IS_CONNAC3X_EXT_TX_DONE_INTR(
		    u4RegValue, prBusInfo->host_int_txdone_bits)) {
		*pu4IntStatus |= WHISR_TX_DONE_INT;
		u4WrValue |= (u4RegValue & prBusInfo->host_int_txdone_bits);
	}
#endif
	if (u4RegValue & CONNAC_MCU_SW_INT) {
		*pu4IntStatus |= WHISR_D2H_SW_INT;
		u4WrValue |= (u4RegValue & CONNAC_MCU_SW_INT);
	}

	if (u4RegValue & CONNAC_SUBSYS_INT) {
		*pu4IntStatus |= WHISR_RX0_DONE_INT;
		u4WrValue |= (u4RegValue & CONNAC_SUBSYS_INT);
	}

	prHifInfo->u4IntStatus = u4RegValue;

	/* clear interrupt */
	HAL_MCR_WR(prAdapter, u4Addr, u4WrValue);
#if CFG_CHIP_RESET_SUPPORT
	/* WF WDT interrupt to host via PCIE */
	HAL_MCR_RD(prAdapter, PCIE_MAC_IREG_ISTATUS_HOST_ADDR,
		&u4IntSta);
	if (prAdapter->eWfsysResetState == WFSYS_RESET_STATE_IDLE &&
			prAdapter->chip_info->fgIsSupportL0p5Reset == TRUE
#if CFG_ASSERTDUMP_BYPASS_CHIP_RESET
			&& prAdapter->fgN9AssertDumpOngoing == FALSE
#endif
		) {
		if (u4IntSta & PCIE_MAC_IREG_ISTATUS_HOST_WDT_INT_MASK)
			*pu4IntStatus |= WHISR_WDT_INT;
	}
	HAL_MCR_WR(prAdapter, PCIE_MAC_IREG_ISTATUS_HOST_ADDR,
		u4IntSta);
#endif
}

static void mt7925ConfigIntMask(struct GLUE_INFO *prGlueInfo,
	u_int8_t enable)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_VAR *prWifiVar;
	uint32_t u4Addr = 0, u4WrVal = 0, u4Val = 0;

	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;

	u4Addr = enable ? WF_WFDMA_HOST_DMA0_HOST_INT_ENA_SET_ADDR :
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_CLR_ADDR;
	u4WrVal =
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA0_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA1_MASK |
#if (CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0)
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA0_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA1_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA2_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA3_MASK
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0 */
		/* Owl use tx ring 15 0x7c024204[25] to tx CMD,
		*  but the enable setting should use tx ring 17 0x7c024204[27]
		*/
		/*WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA15_MASK*/
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA16_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA17_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_mcu2host_sw_int_ena_MASK;

	u4WrVal |=
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA2_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA3_MASK;

	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4WrVal);

	HAL_MCR_RD(prGlueInfo->prAdapter,
		   WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR, &u4Val);

	DBGLOG(HAL, TRACE,
	       "HOST_INT_ENA(0x%08x):0x%08x, En:%u, WrVal:0x%08x\n",
	       WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR,
	       u4Val,
	       enable,
	       u4WrVal);
}

static void mt7925EnableInterrupt(struct ADAPTER *prAdapter)
{
	asicConnac3xEnablePlatformIRQ(prAdapter);
	mt7925ConfigIntMask(prAdapter->prGlueInfo, TRUE);
}

static void mt7925DisableInterrupt(struct ADAPTER *prAdapter)
{
	mt7925ConfigIntMask(prAdapter->prGlueInfo, FALSE);
	asicConnac3xDisablePlatformIRQ(prAdapter);
}

static void mt7925WpdmaMsiConfig(struct ADAPTER *prAdapter)
{
/*
 * ilog2(WFDMA_AP_MSI 1) = WFDMA_AP_MSI_NUM for CR shitf
 * please do NOT use linux API, which is finally implemented in assembly
 */
#define WFDMA_AP_MSI_NUM		0

	struct mt66xx_chip_info *prChipInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct pcie_msi_info *prMsiInfo = NULL;
	uint32_t u4Value = 0;

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	if (!prMsiInfo->fgMsiEnabled)
		return;

	/* configure MSI number */
	HAL_MCR_RD(prAdapter, WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_ADDR,
		&u4Value);
	u4Value |= ((WFDMA_AP_MSI_NUM <<
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_pcie0_msi_num_SHFT) &
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_pcie0_msi_num_MASK);
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_ADDR,
		u4Value);

	/* Set WFDMA MSI_Ring Mapping */
	u4Value = 0x00660077;
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_EXT_WRAP_CSR_MSI_INT_CFG0_ADDR,
		u4Value);

	u4Value = 0x00001100;
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_EXT_WRAP_CSR_MSI_INT_CFG1_ADDR,
		u4Value);

	u4Value = 0x0030004F;
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_EXT_WRAP_CSR_MSI_INT_CFG2_ADDR,
		u4Value);

	u4Value = 0x00542200;
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_EXT_WRAP_CSR_MSI_INT_CFG3_ADDR,
		u4Value);
}

static void mt7925WpdmaConfig(struct GLUE_INFO *prGlueInfo,
	u_int8_t enable, bool fgResetHif)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	union WPDMA_GLO_CFG_STRUCT GloCfg;
	uint32_t u4DmaCfgCr = 0;
	uint32_t idx = 0, u4Val = 0;

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	asicConnac3xWfdmaControl(prGlueInfo, 0, enable);
	u4DmaCfgCr = asicConnac3xWfdmaCfgAddrGet(prGlueInfo, 0);
#endif
	HAL_MCR_RD(prAdapter, u4DmaCfgCr, &GloCfg.word);

	mt7925ConfigIntMask(prGlueInfo, enable);

	if (enable) {
#if defined(_HIF_PCIE)
		mt7925WpdmaMsiConfig(prGlueInfo->prAdapter);
#endif
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
		u4DmaCfgCr = asicConnac3xWfdmaCfgAddrGet(prGlueInfo, idx);
#endif
		GloCfg.field_conn3x.tx_dma_en = 1;
		GloCfg.field_conn3x.rx_dma_en = 1;
		HAL_MCR_WR(prAdapter, u4DmaCfgCr, GloCfg.word);
	}

	/* packet based TX flow control */
	HAL_MCR_RD(prAdapter,
		   WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT1_ADDR,
		   &u4Val);
	u4Val |= WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT1_CSR_TX_FCTRL_MODE_MASK;
	kalDevRegWrite(prGlueInfo,
		       WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT1_ADDR,
		       u4Val);
}

static void mt7925WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *prRxRing,
	uint32_t index)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	uint32_t ext_offset;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	switch (index) {
	case RX_RING_EVT:
		ext_offset = 0 * 4;
		break;
	case RX_RING_DATA0:
		ext_offset = 2 * 4;
		break;
	case RX_RING_DATA1:
		ext_offset = 3 * 4;
		break;
	case RX_RING_TXDONE0:
		ext_offset = 1 * 4;
		break;
	default:
		DBGLOG(RX, ERROR, "Error index=%d\n", index);
		return;
	}

	prRxRing->hw_desc_base_ext =
		prBusInfo->host_rx_ring_ext_ctrl_base + ext_offset;

	HAL_MCR_WR(prAdapter, prRxRing->hw_desc_base_ext,
		   CONNAC3X_RX_RING_DISP_MAX_CNT);
}

static void mt7925InitPcieInt(struct GLUE_INFO *prGlueInfo)
{
	uint32_t value = 0;

	HAL_MCR_RD(prGlueInfo->prAdapter,
		PCIE_MAC_IREG_IMASK_HOST_ADDR,
		&value);
	value |= PCIE_MAC_IREG_IMASK_HOST_INT_REQUEST_EN_MASK |
		PCIE_MAC_IREG_IMASK_HOST_P_ATR_EVT_EN_MASK |
		PCIE_MAC_IREG_IMASK_HOST_A_ATR_EVT_EN_MASK |
		PCIE_MAC_IREG_IMASK_HOST_DMA_ERROR_EN_MASK |
		PCIE_MAC_IREG_IMASK_HOST_DMA_END_EN_MASK;
	HAL_MCR_WR(prGlueInfo->prAdapter,
		PCIE_MAC_IREG_IMASK_HOST_ADDR,
		value);
	/* Enable PCIe MSI on init*/
	HAL_MCR_WR(prGlueInfo->prAdapter,
		CB_INFRA_SLP_CTRL_CB_INFRA_PCIE_SLP_CFG1_ADDR, 0xFFFFFFFF);
}

#if CFG_SUPPORT_PCIE_ASPM
static void mt7925ConfigPcieAspm(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Val = 0;

	if (fgEn) {
		/* Restore original setting*/
		HAL_MCR_WR(prGlueInfo->prAdapter,
			   PCIE_MAC_IREG_PCIE_LTR_VALUES_ADDR,
			   prHifInfo->u4PcieLTR);
		HAL_MCR_WR(prGlueInfo->prAdapter,
			   PCIE_MAC_IREG_PCIE_LOW_POWER_CTRL_ADDR,
			   prHifInfo->u4PcieASPM);
		DBGLOG(HAL, DEBUG, "Enable aspm L1.1/L1.2 0x%08x\n",
			prHifInfo->u4PcieASPM);
	} else {
		/*
		 *	Backup original setting then
		 *	disable L1.1, L1.2 and set LTR to 0
		 */
		HAL_MCR_RD(prGlueInfo->prAdapter,
			   PCIE_MAC_IREG_PCIE_LTR_VALUES_ADDR,
			   &prHifInfo->u4PcieLTR);
		HAL_MCR_RD(prGlueInfo->prAdapter,
			   PCIE_MAC_IREG_PCIE_LOW_POWER_CTRL_ADDR,
			   &prHifInfo->u4PcieASPM);
		HAL_MCR_WR(prGlueInfo->prAdapter,
			PCIE_MAC_IREG_PCIE_LTR_VALUES_ADDR, 0);

		u4Val = prHifInfo->u4PcieASPM &
			~PCIE_LOW_POWER_CTRL_DIS_L1 |
			PCIE_LOW_POWER_CTRL_DIS_L1_1 |
			PCIE_LOW_POWER_CTRL_DIS_L1_2;
		HAL_MCR_WR(prGlueInfo->prAdapter,
			   PCIE_MAC_IREG_PCIE_LOW_POWER_CTRL_ADDR,
			   u4Val);
		DBGLOG(HAL, DEBUG, "Disable aspm L1.1/L1.2 0x%08x\n", u4Val);
	}
}
#endif

static void mt7925ShowPcieDebugInfo(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4Addr, u4Val = 0;

	if (!in_interrupt()) {
		u4Addr = 0x112F0184;
		wf_ioremap_read(u4Addr, &u4Val);
		DBGLOG(HAL, DEBUG, "PCIE CR [0x%08x]=[0x%08x]", u4Addr, u4Val);
		for (u4Addr = 0x112F0C04; u4Addr <= 0x112F0C1C; u4Addr += 4) {
			wf_ioremap_read(u4Addr, &u4Val);
			DBGLOG(HAL, DEBUG, "PCIE CR [0x%08x]=[0x%08x]",
			       u4Addr, u4Val);
		}
	}
}

#if CFG_ASSERTDUMP_BYPASS_CHIP_RESET
static void mt7925bypassWfWdt(struct ADAPTER *prAdapter, bool fgBypass)
{
	uint32_t u4IntMask = 0;

	HAL_MCR_RD(prAdapter, PCIE_MAC_IREG_IMASK_HOST_ADDR,
		&u4IntMask);
	if (fgBypass)
		u4IntMask &= ~PCIE_MAC_IREG_ISTATUS_HOST_WDT_INT_MASK;
	else
		u4IntMask |= PCIE_MAC_IREG_ISTATUS_HOST_WDT_INT_MASK;

	HAL_MCR_WR(prAdapter, PCIE_MAC_IREG_IMASK_HOST_ADDR,
		u4IntMask);
}
#endif

#if CFG_PCIE_LTR_UPDATE
uint8_t g_ucLTRStat;

static void mt7925PcieLTRValue(struct ADAPTER *prAdapter, uint8_t ucState)
{
	if (ucState == PCIE_LTR_STATE_TX_START) {
		if (g_ucLTRStat == PCIE_LTR_STATE_TX_END) {
			HAL_MCR_WR(prAdapter,
				PCIE_MAC_IREG_PCIE_LTR_VALUES_ADDR,
				PCIE_LOW_LATENCY_LTR_VALUE);
			g_ucLTRStat = PCIE_LTR_STATE_TX_START;
			DBGLOG(HAL, LOUD, "LTR val = 0x%x\n",
				PCIE_LOW_LATENCY_LTR_VALUE);
		}
	} else if (ucState == PCIE_LTR_STATE_TX_END) {
		if (g_ucLTRStat == PCIE_LTR_STATE_TX_START) {
			HAL_MCR_WR(prAdapter,
				PCIE_MAC_IREG_PCIE_LTR_VALUES_ADDR,
				PCIE_HIGH_LATENCY_LTR_VALUE);
			g_ucLTRStat = PCIE_LTR_STATE_TX_END;
			DBGLOG(HAL, LOUD, "LTR val = 0x%x\n",
				PCIE_HIGH_LATENCY_LTR_VALUE);
		}
	} else
		DBGLOG(HAL, LOUD, "input LTR value wrong\n");
}
#endif
#endif /* _HIF_PCIE */

static uint32_t mt7925GetFlavorVer(struct GLUE_INFO *prGlueInfo,
	uint8_t *flavor)
{
	int ret;
	u_int8_t fgTestFW = FALSE;

#if CFG_WIFI_TESTMODE_FW_REDOWNLOAD
	if (prGlueInfo)
		fgTestFW = prGlueInfo->fgTestFwDl;
#endif

	ret = kalScnprintf(flavor, CFG_FW_FLAVOR_MAX_LEN,
		fgTestFW ? "1t" : "1");
	return ret;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint8_t mt7925_apsLinkPlanDecision(struct ADAPTER *prAdapter,
	struct AP_COLLECTION *prAp, enum ENUM_MLO_LINK_PLAN eLinkPlan,
	uint8_t ucBssIndex)
{
	uint8_t ucCanSupportDBDCAA = 0;
	uint8_t ucTmpBssIndex;
	uint8_t ucHasActiveBss = FALSE;
	struct BSS_INFO *prBssInfo;
	uint32_t u4TmpLinkPlanBmap;
	uint32_t u4LinkPlanBmap =
		BIT(MLO_LINK_PLAN_2_5)
#if (CFG_SUPPORT_WIFI_6G == 1)
		| BIT(MLO_LINK_PLAN_2_6)
#endif
	;
	uint32_t u4LinkPlanAABmap =
		BIT(MLO_LINK_PLAN_2_5)
#if (CFG_SUPPORT_EMLSR_SAME_A_BAND == 1)
		| BIT(MLO_LINK_PLAN_5_5)
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
		| BIT(MLO_LINK_PLAN_2_6)
		| BIT(MLO_LINK_PLAN_5_6)
#endif
	;
#if (CFG_SUPPORT_MLO_HYBRID == 1)
	uint32_t u4LinkPlan3Bmap =
		BIT(MLO_LINK_PLAN_2_5)
#if (CFG_SUPPORT_WIFI_6G == 1)
		| BIT(MLO_LINK_PLAN_2_5_6)
#endif
#if (CFG_SUPPORT_EMLSR_SAME_A_BAND == 1)
		| BIT(MLO_LINK_PLAN_2_5_5)
#endif
	;
#endif
	uint32_t u4LinkPlanNoneMLDBmap =
		BIT(MLO_LINK_PLAN_2)
		| BIT(MLO_LINK_PLAN_5)
#if (CFG_SUPPORT_WIFI_6G == 1)
		| BIT(MLO_LINK_PLAN_6)
#endif
	;

	DBGLOG_LIMITED(HAL, DEBUG, "WifiDBDCAwithA: %d, MaxSimuLinks: %d\n",
		prAdapter->rWifiFemCfg.u2WifiDBDCAwithA,
		prAdapter->rWifiVar.ucMaxSimuLinks);

	for (ucTmpBssIndex = 0;
		ucTmpBssIndex < prAdapter->ucSwBssIdNum; ucTmpBssIndex++) {
		prBssInfo = prAdapter->aprBssInfo[ucTmpBssIndex];
		if (IS_BSS_ALIVE(prAdapter, prBssInfo) &&
			(IS_BSS_P2P(prBssInfo) ||
			 IS_BSS_NAN(prBssInfo)))
			ucHasActiveBss = TRUE;
	}

	if (prAdapter->rWifiFemCfg.u2WifiDBDCAwithA == TRUE)
		ucCanSupportDBDCAA = 1;
	else {
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		/*if HW not support A+A
		 *STR mode can not support DBDC A+A
		 *EMLSR/Hybird mode can support DBDC A+A always
		 */
		if (prAdapter->rWifiVar.ucEnableMlo >= 1 &&
			prAdapter->rWifiVar.ucMaxSimuLinks >= 1)
			ucCanSupportDBDCAA = 0;
		else
			ucCanSupportDBDCAA = 1;
#else
		ucCanSupportDBDCAA = 0;
#endif
	}

	if (ucHasActiveBss &&
		prAdapter->rWifiVar.ucMaxSimuLinks == 0) {
	/*has active Bss, block MLSR connection */
		u4TmpLinkPlanBmap = u4LinkPlanNoneMLDBmap;
		DBGLOG_LIMITED(HAL, DEBUG, "use aeLinkPlanNoneMLD\n");
	}
#if (CFG_SUPPORT_MLO_HYBRID == 1)
	else if (ucCanSupportDBDCAA && IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.ucNonApHyMloSupport) &&
		IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.ucNonApHyMloSupportCap)) {
		u4TmpLinkPlanBmap = u4LinkPlan3Bmap;
		DBGLOG_LIMITED(HAL, DEBUG, "use aeTriLinkPlan\n");
	}
#endif
	else if (ucCanSupportDBDCAA) {
		u4TmpLinkPlanBmap = u4LinkPlanAABmap;
		DBGLOG_LIMITED(HAL, DEBUG, "use aeLinkPlanAwithA\n");
	} else {
		u4TmpLinkPlanBmap = u4LinkPlanBmap;
		DBGLOG_LIMITED(HAL, DEBUG, "use aeLinkPlan\n");
	}

	return !!(u4TmpLinkPlanBmap & BIT(eLinkPlan));
}

static void mt7925_apsFillBssDescSet(struct ADAPTER *prAdapter,
		struct BSS_DESC_SET *set, uint8_t ucBssIndex)
{
#if (CFG_SUPPORT_MLO_HYBRID == 1)
	uint8_t i;
	uint8_t ucL3BnlimitBmap = prAdapter->rWifiVar.ucLink3BandLimitBitmap;
	struct CONNECTION_SETTINGS *conn =
		aisGetConnSettings(prAdapter, ucBssIndex);
	enum ENUM_PARAM_CONNECTION_POLICY policy = conn->eConnectionPolicy;


	/* swap link 3 to link 2 depend on fw capbility
	 *(2g or 5g can't be the 3rd link)
	 */
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucNonApHyMloSupport) &&
	    IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucNonApHyMloSupportCap) &&
		set && set->ucLinkNum == MLD_HYBRID_MLO_LINK_NUM) {
		struct BSS_DESC_W *w;

		if ((ucL3BnlimitBmap & BAND_5G) &&
		     set->aprBssDescW[MLD_HYBRID_MLO_LINK_NUM - 1]->eBand ==
				BAND_5G) {
			w = set->aprBssDescW[MLD_HYBRID_MLO_LINK_NUM - 1];
			set->aprBssDescW[MLD_HYBRID_MLO_LINK_NUM - 1] =
				set->aprBssDescW[MLD_HYBRID_MLO_LINK_NUM - 2];
			set->aprBssDescW[MLD_HYBRID_MLO_LINK_NUM - 2] = w;
		} else if ((ucL3BnlimitBmap & BAND_2G4) &&
		     set->aprBssDescW[MLD_HYBRID_MLO_LINK_NUM - 1]->eBand ==
				BAND_2G4) {
			w = set->aprBssDescW[MLD_HYBRID_MLO_LINK_NUM - 1];
			set->aprBssDescW[MLD_HYBRID_MLO_LINK_NUM - 1] =
				set->aprBssDescW[MLD_HYBRID_MLO_LINK_NUM - 2];
			set->aprBssDescW[MLD_HYBRID_MLO_LINK_NUM - 2] = w;
		}
	}

	if (set->eMloMode == MLO_MODE_HYMLO ||
	    set->eMloMode == MLO_MODE_HYEMLSR) {
		for (i = 0; i < set->ucLinkNum; i++)
			set->aprBssDescW[i]->fgUnSyncOm = TRUE;
	}

	if (policy == CONNECT_BY_BSSID)
		return;

#if (CFG_SUPPORT_FORCE_LINK_SORT == 1)
	if (prAdapter->ucForceLinkSort) {
		struct BSS_DESC_W *sorted[MLD_LINK_MAX] = {0};
		enum ENUM_BAND order[3] = {0};
		uint8_t j;

		switch (prAdapter->ucForceLinkSortType) {
		case 0x00:
			order[0] = BAND_2G4;
			order[1] = BAND_5G;
			order[2] = BAND_6G;
			break;
		case 0x01:
			order[0] = BAND_2G4;
			order[1] = BAND_6G;
			order[2] = BAND_5G;
			break;
		case 0x10:
			order[0] = BAND_5G;
			order[1] = BAND_2G4;
			order[2] = BAND_6G;
			break;
		case 0x11:
			order[0] = BAND_5G;
			order[1] = BAND_6G;
			order[2] = BAND_2G4;
			break;
		case 0x20:
			order[0] = BAND_6G;
			order[1] = BAND_2G4;
			order[2] = BAND_5G;
			break;
		case 0x21:
			order[0] = BAND_6G;
			order[1] = BAND_5G;
			order[2] = BAND_2G4;
			break;
		}

		for (i = 0; i < MLD_LINK_MAX; i++) {
			for (j = 0; j < MLD_LINK_MAX; j++) {
				if (set->aprBssDescW[j] &&
				    set->aprBssDescW[j]->eBand == order[i]) {
					sorted[i] = set->aprBssDescW[j];
					break;
				}
			}
		}

		for (i = 0; i < MLD_LINK_MAX; i++)
			set->aprBssDescW[i] = sorted[i];
	}
#endif
#endif
}

static void mt7925_apsUpdateTotalScore(struct ADAPTER *prAdapter,
	struct BSS_DESC *arLinks[], uint8_t ucLinkNum,
	struct AP_SCORE_INFO *prScoreInfo, uint8_t ucBssidx)
{
	uint32_t u4TotalScore = 0;
	uint32_t u4TotalTput = 0;
	struct BSS_DESC *best_bss = arLinks[0]; /* links is sorted by score */
	uint8_t i;
	enum ENUM_MLO_MODE eMloMode = MLO_MODE_SLSR;
	uint8_t ucMaxSimuLinks = 0;
	uint8_t tmpIsEmlsrPermittedAP = FALSE;
	uint8_t fgNeedCheckEmlsrAllowlist = FALSE;

	for (i = 0; i < ucLinkNum; i++) {
		u4TotalScore += arLinks[i]->u2Score;
		u4TotalTput += arLinks[i]->u4Tput;
		if (arLinks[i]->rMlInfo.fgIsEmlsrPermittedAP)
			tmpIsEmlsrPermittedAP = TRUE;
	}

	if (IS_FEATURE_DISABLED(
			prAdapter->rWifiVar.ucDisEmlsrAllowlist) &&
			prAdapter->rWifiVar.u4SwTestMode ==
			ENUM_SW_TEST_MODE_NONE)
		fgNeedCheckEmlsrAllowlist = TRUE;

	if (ucLinkNum > 1) {
		//ucMaxSimuLinks = prAdapter->rWifiVar.ucMaxSimuLinksCap;
		ucMaxSimuLinks = 0;
		eMloMode = MLO_MODE_MLSR;
	}

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
	if (IS_FEATURE_ENABLED(
			prAdapter->rWifiVar.ucNonApMldEMLSupport) &&
			BE_IS_EML_CAP_SUPPORT_EMLSR(
				best_bss->rMlInfo.u2EmlCap)) {
		/* The AP is in allow list, the connection
		 * select EMLSR, otherwise it need select MLSR.
		 */
		if (tmpIsEmlsrPermittedAP == FALSE &&
			fgNeedCheckEmlsrAllowlist == TRUE)
			eMloMode = MLO_MODE_MLSR;
		else
			eMloMode = MLO_MODE_EMLSR;

		ucMaxSimuLinks = 0;
	}
#endif

#if (CFG_SUPPORT_MLO_HYBRID == 1)
	if (IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.ucNonApHyMloSupport) &&
		IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.ucNonApHyMloSupportCap)) {
		if (eMloMode == MLO_MODE_EMLSR)
			eMloMode = MLO_MODE_HYEMLSR;
		else
			eMloMode = MLO_MODE_HYMLO;
		ucMaxSimuLinks = 0;
	}
#endif
	DBGLOG(ML, INFO, "eMloMode: %d\n", eMloMode);
	kalMemCopy(prScoreInfo->aprTarget, arLinks,
		sizeof(prScoreInfo->aprTarget));
	prScoreInfo->ucLinkNum = ucLinkNum;
	prScoreInfo->u4TotalScore = u4TotalScore;
	prScoreInfo->u4TotalTput = u4TotalTput;
	prScoreInfo->eMloMode = eMloMode;
	prScoreInfo->ucMaxSimuLinks = ucMaxSimuLinks;
}

#endif /* CFG_SUPPORT_802_11BE_MLO */

#endif  /* MT7925 */
