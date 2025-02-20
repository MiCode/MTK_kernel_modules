// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

/*! \file   mt7935.c
*    \brief  Internal driver stack will export
*    the required procedures here for GLUE Layer.
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifdef MT7935

#include "precomp.h"
#include "mt7935.h"
#include "coda/mt7935/cb_ckgen_top.h"
#include "coda/mt7935/cb_infra_misc0.h"
#include "coda/mt7935/cb_infra_rgu.h"
#include "coda/mt7935/cb_infra_slp_ctrl.h"
#include "coda/mt7935/gpio.h"
#include "coda/mt7935/conn_bus_cr.h"
#include "coda/mt7935/conn_cfg.h"
#include "coda/mt7935/conn_dbg_ctl.h"
#include "coda/mt7935/conn_host_csr_top.h"
#include "coda/mt7935/conn_semaphore.h"
#include "coda/mt7935/wf_cr_sw_def.h"
#include "coda/mt7935/wf_top_cfg.h"
#include "coda/mt7935/wf_wfdma_ext_wrap_csr.h"
#include "coda/mt7935/wf_wfdma_host_dma0.h"
#include "coda/mt7935/wf_wfdma_mcu_dma0.h"
#include "coda/mt7935/wf_hif_dmashdl_top.h"
#include "coda/mt7935/wf_pse_top.h"
#include "coda/mt7935/pcie_mac_ireg.h"
#include "coda/mt7935/conn_mcu_bus_cr.h"
#include "coda/mt7935/conn_bus_cr_von.h"
#include "coda/mt7935/vlp_uds_ctrl.h"
#include "coda/mt7935/wf_rro_top.h"
#include "coda/mt7935/wf_top_cfg_on.h"
#include "hal_dmashdl_mt7935.h"
#include "coda/mt7935/wf2ap_conn_infra_on_ccif4.h"
#include "coda/mt7935/ap2wf_conn_infra_on_ccif4.h"
#include "coda/mt7935/wf_wtblon_top.h"
#include "coda/mt7935/wf_uwtbl_top.h"

#include "wlan_pinctrl.h"

#if CFG_MTK_MDDP_SUPPORT
#include "mddp_export.h"
#endif

#if CFG_MTK_CCCI_SUPPORT
#include "mtk_ccci_common.h"
#endif

#include "gl_coredump.h"
#if CFG_MTK_WIFI_SUPPORT_IPC
#include "wlan_ipc.h"
#endif
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#if (CFG_MTK_WIFI_SUPPORT_IPC == 1)
#define IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(_field) \
				OFFSET_OF(\
				struct mt7935_conn_von_sysram_layout_t,\
				_field)
#define IPC_GET_WFMCU_DOORBELL_BIT_OFFSET(_field) \
				OFFSET_OF(\
				struct mt7935_wfmcu_doorbell_layout_t,\
				_field)
#define IPC_GET_CBMCU_DOORBELL_BIT_OFFSET(_field) \
				OFFSET_OF(\
				struct mt7935_cbmcu_doorbell_layout_t,\
				_field)
#define IPC_GET_BITMAP_BIT_OFFSET(_field) \
				OFFSET_OF(\
				struct mt7935_bitmap_layout_t,\
				_field)
#define IPC_GET_CONN_VON_SYSRAM_FIELD_SIZE(_addr, _field) \
	sizeof(((struct mt7935_conn_von_sysram_layout_t *)_addr)->_field)

#define MT7935_WF_CB_MCU_FW_BIN_NAME "WIFI_RAM_CODE_MT7935_1_1.bin"
#endif /* CFG_MTK_WIFI_SUPPORT_IPC */
/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static uint32_t mt7935GetFlavorVer(uint8_t *flavor);

static void mt7935_ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx);

static void mt7935_ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);

#if CFG_MTK_WIFI_SUPPORT_DSP_FWDL
static void mt7935_ConstructDspName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);
#endif

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
static void mt7935_ConstructIdxLogBinName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName);
#endif

#if defined(_HIF_PCIE)
static uint8_t mt7935SetRxRingHwAddr(struct RTMP_RX_RING *prRxRing,
		struct BUS_INFO *prBusInfo, uint32_t u4SwRingIdx);

static bool mt7935WfdmaAllocRxRing(struct GLUE_INFO *prGlueInfo,
		bool fgAllocMem);

static void mt7935ProcessTxInterrupt(
		struct ADAPTER *prAdapter);

static void mt7935ProcessRxInterrupt(
	struct ADAPTER *prAdapter);

static void mt7935WfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo);

static void mt7935ReadIntStatusByMsi(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus);

static void mt7935ReadIntStatus(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus);

#if (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
static void mt7935EnableInterruptViaPcie(struct ADAPTER *prAdapter);
#endif
static void mt7935EnableInterrupt(struct ADAPTER *prAdapter);
static void mt7935DisableInterrupt(struct ADAPTER *prAdapter);

static void mt7935ConfigIntMask(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable);

static void mt7935ConfigWfdmaRxRingThreshold(
	struct ADAPTER *prAdapter, uint32_t u4Num, u_int8_t fgIsData);

static void mt7935WpdmaConfig(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable, bool fgResetHif);

#if CFG_MTK_WIFI_WFDMA_WB
static void mt7935ProcessTxInterruptByEmi(struct ADAPTER *prAdapter);
static void mt7935ProcessRxInterruptByEmi(struct ADAPTER *prAdapter);
static void mt7935ProcessSoftwareInterruptByEmi(struct ADAPTER *prAdapter);
static void mt7935ReadIntStatusByEmi(struct ADAPTER *prAdapter,
				     uint32_t *pu4IntStatus);
static void mt7935ConfigEmiIntMask(struct GLUE_INFO *prGlueInfo,
				   u_int8_t enable);
static void mt7935RunWfdmaCidxFetch(struct GLUE_INFO *prGlueInfo);
static void mt7935TriggerWfdmaTxCidx(struct GLUE_INFO *prGlueInfo,
				     struct RTMP_TX_RING *prTxRing);
static void mt7935TriggerWfdmaRxCidx(struct GLUE_INFO *prGlueInfo,
				     struct RTMP_RX_RING *prRxRing);
static void mt79353EnableWfdmaWb(struct GLUE_INFO *prGlueInfo);
#endif /* CFG_MTK_WIFI_WFDMA_WB */

static void mt7935SetupMcuEmiAddr(struct ADAPTER *prAdapter);
#if CFG_MTK_WIFI_SUPPORT_IPC
static void mt7935SetupWiFiMcuEmiAddr(struct ADAPTER *prAdapter);
#endif /* CFG_MTK_WIFI_SUPPORT_IPC */
static void mt7935WfdmaTxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *prTxRing,
	u_int32_t index);
static void mt7935WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	u_int32_t index);

static void mt7935InitPcieInt(struct GLUE_INFO *prGlueInfo);

static void mt7935ShowPcieDebugInfo(struct GLUE_INFO *prGlueInfo);

static u_int8_t mt7935_get_sw_interrupt_status(struct ADAPTER *prAdapter,
	uint32_t *pu4Status);

static int32_t mt7935_trigger_fw_assert(struct ADAPTER *prAdapter);
static uint32_t mt7935_mcu_init(struct ADAPTER *ad);
static void mt7935_mcu_deinit(struct ADAPTER *ad);
static int mt7935_CheckBusHang(void *priv, uint8_t rst_enable);
static uint32_t mt7935_wlanDownloadPatch(struct ADAPTER *prAdapter);
static void mt7935WiFiNappingCtrl(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn);

static void mt7935LowPowerOwnInit(struct ADAPTER *prAdapter);
static void mt7935LowPowerOwnRead(struct ADAPTER *prAdapter,
				  u_int8_t *pfgResult);
static void mt7935LowPowerOwnSet(struct ADAPTER *prAdapter,
				 u_int8_t *pfgResult);
static void mt7935LowPowerOwnClear(struct ADAPTER *prAdapter,
				   u_int8_t *pfgResult);

#if (CFG_ENABLE_IPC_FW_DOWNLOAD == 1)
static uint32_t mt7935IPCFirmwareDownload(struct ADAPTER *prAdapter);
static uint32_t mt7935IPCLoadFirmware(struct ADAPTER *prAdapter,
				      uint8_t **apucFwNameTable);
#endif /* CFG_ENABLE_IPC_FW_DOWNLOAD */
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct ECO_INFO mt7935_eco_table[] = {
	/* HW version,  ROM version,    Factory version */
	{0x00, 0x00, 0xA, 0x1},	/* E1 */
	{0x00, 0x00, 0x0, 0x0}	/* End of table */
};

uint8_t *apucmt7935FwName[] = {
	(uint8_t *) CFG_FW_FILENAME "_7935",
	NULL
};

#if defined(_HIF_PCIE)
struct PCIE_CHIP_CR_MAPPING mt7935_bus2chip_cr_mapping[] = {
	/* chip addr, bus addr, range */
	{0x830C0000, 0x00000, 0x1000}, /* WF_MCU_BUS_CR_REMAP */
	{0x54000000, 0x02000, 0x1000}, /* WFDMA_0 (PCIE0 MCU DMA0) */
	{0x55000000, 0x03000, 0x1000}, /* WFDMA_1 (PCIE0 MCU DMA1) */
	{0x56000000, 0x04000, 0x1000}, /* WFDMA_2 (Reserved) */
	{0x57000000, 0x05000, 0x1000}, /* WFDMA_3 (MCU wrap CR) */
	{0x58000000, 0x06000, 0x1000}, /* WFDMA_4 (PCIE1 MCU DMA0 (MEM_DMA)) */
	{0x59000000, 0x07000, 0x1000}, /* WFDMA_5 (PCIE1 MCU DMA1) */
	{0x820C0000, 0x08000, 0x4000}, /* WF_UMAC_TOP (PLE) */
	{0x820C8000, 0x0C000, 0x2000}, /* WF_UMAC_TOP (PSE) */
	{0x820CC000, 0x0E000, 0x2000}, /* WF_UMAC_TOP (PP) */
	{0x83000000, 0x110000, 0x10000}, /* WF_PHY_MAP3 */
	{0x820E0000, 0x20000, 0x0400}, /* WF_LMAC_TOP (WF_CFG) */
	{0x820E1000, 0x20400, 0x0200}, /* WF_LMAC_TOP (WF_TRB) */
	{0x820E2000, 0x20800, 0x0400}, /* WF_LMAC_TOP (WF_AGG) */
	{0x820E3000, 0x20C00, 0x0400}, /* WF_LMAC_TOP (WF_ARB) */
	{0x820E4000, 0x21000, 0x0400}, /* WF_LMAC_TOP (WF_TMAC) */
	{0x820E5000, 0x21400, 0x0800}, /* WF_LMAC_TOP (WF_RMAC) */
	{0x820CE000, 0x21C00, 0x0200}, /* WF_LMAC_TOP (WF_SEC) */
	{0x820E7000, 0x21E00, 0x0200}, /* WF_LMAC_TOP (WF_DMA) */
	{0x820CF000, 0x22000, 0x1000}, /* WF_LMAC_TOP (WF_PF) */
	{0x820E9000, 0x23400, 0x0200}, /* WF_LMAC_TOP (WF_WTBLOFF) */
	{0x820EA000, 0x24000, 0x0200}, /* WF_LMAC_TOP (WF_ETBF) */
	{0x820EB000, 0x24200, 0x0400}, /* WF_LMAC_TOP (WF_LPON) */
	{0x820EC000, 0x24600, 0x0200}, /* WF_LMAC_TOP (WF_INT) */
	{0x820ED000, 0x24800, 0x0800}, /* WF_LMAC_TOP (WF_MIB) */
	{0x820CA000, 0x26000, 0x2000}, /* WF_LMAC_TOP (WF_MUCOP) */
	{0x820D0000, 0x30000, 0x10000}, /* WF_LMAC_TOP (WF_WTBLON ) */
	{0x830A0000, 0x40000, 0x10000}, /* WF_PHY_MAP0 */
	{0x83080000, 0x50000, 0x10000}, /* WF_PHY_MAP1 */
	{0x83090000, 0x60000, 0x10000}, /* WF_PHY_MAP2 */
	{0xE0400000, 0x70000, 0x10000}, /* WF_UMCA_SYSRAM */
	{0x00400000, 0x80000, 0x10000}, /* WF_MCU_SYSRAM */
	{0x00410000, 0x90000, 0x10000}, /* WF_MCU_SYSRAM (Common driver usage only) */
	{0x820F0000, 0xA0000, 0x0400}, /* WF_LMAC_TOP (WF_CFG) */
	{0x820F1000, 0xA0600, 0x0200}, /* WF_LMAC_TOP (WF_TRB) */
	{0x820F2000, 0xA0800, 0x0400}, /* WF_LMAC_TOP (WF_AGG) */
	{0x820F3000, 0xA0C00, 0x0400}, /* WF_LMAC_TOP (WF_ARB) */
	{0x820F4000, 0xA1000, 0x0400}, /* WF_LMAC_TOP (WF_TMAC) */
	{0x820F5000, 0xA1400, 0x0800}, /* WF_LMAC_TOP (WF_RMAC) */
	{0x820CE000, 0xA1C00, 0x0200}, /* WF_LMAC_TOP (WF_SEC) */
	{0x820F7000, 0xA1E00, 0x0200}, /* WF_LMAC_TOP (WF_DMA) */
	{0x820CF000, 0xA2000, 0x1000}, /* WF_LMAC_TOP (WF_PF) */
	{0x820F9000, 0xA3400, 0x0200}, /* WF_LMAC_TOP (WF_WTBLOFF) */
	{0x820FA000, 0xA4000, 0x0200}, /* WF_LMAC_TOP (WF_ETBF) */
	{0x820FB000, 0xA4200, 0x0400}, /* WF_LMAC_TOP (WF_LPON) */
	{0x820FC000, 0xA4600, 0x0200}, /* WF_LMAC_TOP (WF_INT) */
	{0x820FD000, 0xA4800, 0x0800}, /* WF_LMAC_TOP (WF_MIB) */
	{0x820CC000, 0xA5000, 0x2000}, /* WF_LMAC_TOP (WF_MUCOP) */
	{0x820C4000, 0xA8000, 0x4000}, /* WF_LMAC_TOP (WF_UWTBL ) */
	{0x81030000, 0xAE000, 0x0100}, /* WFSYS_AON */
	{0x81031000, 0xAE100, 0x0100}, /* WFSYS_AON */
	{0x81032000, 0xAE200, 0x0100}, /* WFSYS_AON */
	{0x81033000, 0xAE300, 0x0100}, /* WFSYS_AON */
	{0x81034000, 0xAE400, 0x0100}, /* WFSYS_AON */
	{0x80020000, 0xB0000, 0x10000}, /* WF_TOP_MISC_OFF */
	{0x81020000, 0xC0000, 0x10000}, /* WF_TOP_MISC_ON */
	{0x81040000, 0x120000, 0x1000}, /* WF_MCU_CFG_ON */
	{0x81050000, 0x121000, 0x1000}, /* WF_MCU_EINT */
	{0x81060000, 0x122000, 0x1000}, /* WF_MCU_GPT */
	{0x81070000, 0x123000, 0x1000}, /* WF_MCU_WDT */
	{0x80010000, 0x124000, 0x1000}, /* WF_AXIDMA */
	{0x83010000, 0x130000, 0x10000}, /* WF_PHY_MAP4 */
	{0x88000000, 0x140000, 0x10000}, /* WF_MCU_CFG_LS */
	{0x20020000, 0xd0000, 0x10000}, /* CONN_INFRA wf_dma_host_side_cr */
	{0x200B0000, 0x50000, 0x10000}, /* CONN_INFRA conn_von_sysram */
	{0x20060000, 0xe0000, 0x10000}, /* CONN_INFRA conn_host_csr_top */
	{0x7c000000, 0xf0000, 0x10000}, /* CONN_INFRA (io_top bus_cr rgu_on cfg_on) */
	{0x7c010000, 0x100000, 0x10000}, /* CONN_INFRA (gpio  clkgen cfg) */
	{0x20090000, 0x150000, 0x10000}, /* CONN_INFRA VON (RO) */
	{0x20030000, 0x160000, 0x10000}, /* CONN_INFRA CCIF */
	{0x7c040000, 0x170000, 0x10000}, /* CONN_INFRA (bus, afe) */
	{0x7c070000, 0x180000, 0x10000}, /* CONN_INFRA Semaphore */
	{0x7c080000, 0x190000, 0x10000}, /* CONN_INFRA (coex, pta) */
	{0x7c050000, 0x1a0000, 0x10000}, /* CONN_INFRA SYSRAM */
#if (CFG_MTK_FPGA_PLATFORM == 1)
	{0x74040000, 0x010000, 0x10000}, /* PCIe MAC (conninfra remap) */
#else
	{0x74040000, 0x1d0000, 0x10000}, /* CB PCIe (cbtop remap) */
#endif
#if (CFG_MTK_FPGA_PLATFORM != 1)
	{0x70010000, 0x1c0000, 0x10000}, /* CB Infra1 */
	{0x70000000, 0x1e0000, 0x10000}, /* CB TOP */
	{0x70020000, 0x1f0000, 0x10000}, /* CB Infra2 (RO) */
#endif
	{0x7c500000, MT7935_PCIE2AP_REMAP_BASE_ADDR, 0x200000}, /* remap */
	{0x00000000, 0x000000, 0x00000}, /* END */
};
#endif

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
struct pcie2ap_remap mt7935_pcie2ap_remap = {
	.reg_base = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_ADDR,
	.reg_mask = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_MASK,
	.reg_shift = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_SHFT,
	.base_addr = MT7935_PCIE2AP_REMAP_BASE_ADDR
};

struct ap2wf_remap mt7935_ap2wf_remap = {
	.reg_base = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_ADDR,
	.reg_mask = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_MASK,
	.reg_shift = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_SHFT,
	.base_addr = MT7935_REMAP_BASE_ADDR
};

struct PCIE_CHIP_CR_REMAPPING mt7935_bus2chip_cr_remapping = {
	.pcie2ap = &mt7935_pcie2ap_remap,
	.ap2wf = &mt7935_ap2wf_remap,
};

struct wfdma_group_info mt7935_wfmda_host_tx_group[] = {
	{"P0T0:AP DATA0", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL0_ADDR, true},
	{"P0T1:AP DATA1", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING1_CTRL0_ADDR, true},
	{"P0T2:AP DATA2", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING2_CTRL0_ADDR, true},
	{"P0T3:AP DATA3", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING3_CTRL0_ADDR, true},
	{"P0T4:AP DATA4", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING4_CTRL0_ADDR, true},
	{"P0T5:AP DATA5", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING5_CTRL0_ADDR, true},
	{"P0T15:AP CMD", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING15_CTRL0_ADDR},
	{"P0T16:FWDL", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING16_CTRL0_ADDR},
};

struct wfdma_group_info mt7935_wfmda_host_rx_group[] = {
	{"P0R0:DATA", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL0_ADDR, true},
	{"P0R3:EVT", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING3_CTRL0_ADDR, true},
	{"P0R2:HPPDATA", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_CTRL0_ADDR, true},
	{"P0R1:TDone", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING1_CTRL0_ADDR, true},
	{"P0R4:ICS", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_CTRL0_ADDR, true},
	{"P0R5:Coredump", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING5_CTRL0_ADDR, true},
	{"P0R6:Fw log", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_CTRL0_ADDR, true},
};

struct wfdma_group_info mt7935_wfmda_wm_tx_group[] = {
};

struct wfdma_group_info mt7935_wfmda_wm_rx_group[] = {
	{"P0R0:FWDL", WF_WFDMA_MCU_DMA0_WPDMA_RX_RING0_CTRL0_ADDR},
};

struct pse_group_info mt7935_pse_group[] = {
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
struct pcie_msi_layout mt7935_pcie_msi_layout[] = {
#if (WFDMA_AP_MSI_NUM == 8)
	{"conn_hif_tx_data0_int", mtk_pci_isr,
	 mtk_pci_isr_tx_data0_thread, AP_INT, 0},
	{"conn_hif_tx_data1_int", mtk_pci_isr,
	 mtk_pci_isr_tx_data0_thread, AP_INT, 0},
	{"conn_hif_tx_free_done_int", mtk_pci_isr,
	 mtk_pci_isr_tx_free_done_thread, AP_INT, 0},
	{"conn_hif_rx_data0_int", mtk_pci_isr,
	 mtk_pci_isr_rx_data0_thread, AP_INT, 0},
	{"conn_hif_rx_data1_int", mtk_pci_isr,
	 mtk_pci_isr_rx_data1_thread, AP_INT, 0},
	{"conn_hif_event_int", mtk_pci_isr,
	 mtk_pci_isr_rx_event_thread, AP_INT, 0},
	{"conn_hif_cmd_int", mtk_pci_isr,
	 mtk_pci_isr_tx_cmd_thread, AP_INT, 0},
	{"conn_hif_lump_int", mtk_pci_isr,
	 mtk_pci_isr_lump_thread, AP_INT, 0},
#else
	{"conn_hif_host_int", mtk_pci_isr,
	 mtk_pci_isr_thread, AP_INT, 0},
	{"conn_hif_host_int", NULL, NULL, AP_INT, 0},
	{"conn_hif_host_int", NULL, NULL, AP_INT, 0},
	{"conn_hif_host_int", NULL, NULL, AP_INT, 0},
	{"conn_hif_host_int", NULL, NULL, AP_INT, 0},
	{"conn_hif_host_int", NULL, NULL, AP_INT, 0},
	{"conn_hif_host_int", NULL, NULL, AP_INT, 0},
	{"conn_hif_host_int", NULL, NULL, AP_INT, 0},
#endif
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0}, /* image response */
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0}, /* boot stage */
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"wm_conn2ap_wdt_irq", NULL, NULL, NONE_INT, 0},
	{"wf_mcu_jtag_det_eint", NULL, NULL, NONE_INT, 0},
	{"pmic_eint", NULL, NULL, NONE_INT, 0},
	{"wf_msi_dfd_en", NULL, NULL, NONE_INT, 0},
	{"cb_mcu_wdt_irq_b", NULL, NULL, NONE_INT, 0},
	{"mbu_c3", NULL, NULL, NONE_INT, 0},
	{"dtm_attach", NULL, NULL, NONE_INT, 0},
	{"cbmcu_jtag_attach", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0}, /* wf driver own */
	{"reserved", NULL, NULL, NONE_INT, 0}, /* md driver own */
	{"reserved", NULL, NULL, NONE_INT, 0}, /* wf log notify */
	{"reserved", NULL, NULL, NONE_INT, 0}, /* coredump start */
	{"reserved", NULL, NULL, NONE_INT, 0}, /* coredump finish */
};
#endif

struct BUS_INFO mt7935_bus_info = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.top_cfg_base = MT7935_TOP_CFG_BASE,

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
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_4_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_5_MASK |
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0 */
#if (CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0)
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_15_MASK |
#endif /* CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0 */
#if (WFDMA_AP_MSI_NUM == 1)
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_16_MASK |
#endif
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_mcu2host_sw_int_sts_MASK),
	.host_int_rxdone_bits =
	(WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_0_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_1_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_2_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_3_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_4_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_5_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_6_MASK),

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

	.bus2chip = mt7935_bus2chip_cr_mapping,
	.bus2chip_remap = &mt7935_bus2chip_cr_remapping,
	.max_static_map_addr = 0x00200000,

	.tx_ring_fwdl_idx = CONNAC3X_FWDL_TX_RING_IDX,
	.tx_ring_cmd_idx = 15,
	.tx_ring0_data_idx = 0,
	.tx_ring1_data_idx = 1,
	.tx_ring2_data_idx = 2,
	.tx_ring3_data_idx = 3,
	.tx_prio_data_idx = 4,
	.tx_altx_data_idx = 5,
	.rx_data_ring_num = 2,
	.rx_evt_ring_num = 5,
	.rx_data_ring_size = 3072,
	.rx_evt_ring_size = 128,
	.rx_data_ring_prealloc_size = 1024,
	.fw_own_clear_addr = CONN_HOST_CSR_TOP_WF_BAND0_IRQ_STAT_ADDR,
	.fw_own_clear_bit = CONN_HOST_CSR_TOP_WF_BAND0_IRQ_STAT_WF_B0_HOST_LPCR_FW_OWN_CLR_STAT_MASK,
	.fgCheckDriverOwnInt = FALSE,
	.u4DmaMask = 34,
	.wfmda_host_tx_group = mt7935_wfmda_host_tx_group,
	.wfmda_host_tx_group_len = ARRAY_SIZE(mt7935_wfmda_host_tx_group),
	.wfmda_host_rx_group = mt7935_wfmda_host_rx_group,
	.wfmda_host_rx_group_len = ARRAY_SIZE(mt7935_wfmda_host_rx_group),
	.wfmda_wm_tx_group = mt7935_wfmda_wm_tx_group,
	.wfmda_wm_tx_group_len = ARRAY_SIZE(mt7935_wfmda_wm_tx_group),
	.wfmda_wm_rx_group = mt7935_wfmda_wm_rx_group,
	.wfmda_wm_rx_group_len = ARRAY_SIZE(mt7935_wfmda_wm_rx_group),
	.prDmashdlCfg = &rMt7935DmashdlCfg,
	.prPpTopCr = &rMt7935PpTopCr,
	.prPseGroup = mt7935_pse_group,
	.u4PseGroupLen = ARRAY_SIZE(mt7935_pse_group),
	.pdmaSetup = mt7935WpdmaConfig,
#if defined(_HIF_PCIE)
#if CFG_MTK_WIFI_WFDMA_WB
	.enableInterrupt = asicConnac3xEnablePlatformIRQ,
	.disableInterrupt = asicConnac3xDisablePlatformIRQ,
#elif (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
	.enableInterrupt = mt7935EnableInterruptViaPcie,
	.disableInterrupt = asicConnac3xDisablePlatformIRQ,
#else
	.enableInterrupt = mt7935EnableInterrupt,
	.disableInterrupt = mt7935DisableInterrupt,
#endif /* CFG_SUPPORT_PCIE_PLAT_INT_FLOW */
#else /* !_HIF_PCIE */
	.enableInterrupt = mt7935EnableInterrupt,
	.disableInterrupt = mt7935DisableInterrupt,
#endif /* _HIF_PCIE */
#if CFG_MTK_WIFI_WFDMA_WB
	.configWfdmaIntMask = NULL,
#else
	.configWfdmaIntMask = mt7935ConfigIntMask,
#endif /* CFG_MTK_WIFI_WFDMA_WB */
#if defined(_HIF_PCIE)
	.initPcieInt = mt7935InitPcieInt,
	.pdmaStop = asicConnac3xWfdmaStop,
	.pdmaPollingIdle = asicConnac3xWfdmaPollingAllIdle,
	.pcie_msi_info = {
		.prMsiLayout = mt7935_pcie_msi_layout,
		.u4MaxMsiNum = ARRAY_SIZE(mt7935_pcie_msi_layout),
	},
	.showDebugInfo = mt7935ShowPcieDebugInfo,
#endif /* _HIF_PCIE */
#if CFG_MTK_WIFI_WFDMA_WB
	.processTxInterrupt = mt7935ProcessTxInterruptByEmi,
	.processRxInterrupt = mt7935ProcessRxInterruptByEmi,
	.processSoftwareInterrupt = mt7935ProcessSoftwareInterruptByEmi,
#else
	.processTxInterrupt = mt7935ProcessTxInterrupt,
	.processRxInterrupt = mt7935ProcessRxInterrupt,
	.processSoftwareInterrupt = asicConnac3xProcessSoftwareInterrupt,
#endif /* CFG_MTK_WIFI_WFDMA_WB */
	.tx_ring_ext_ctrl = mt7935WfdmaTxRingExtCtrl,
	.rx_ring_ext_ctrl = mt7935WfdmaRxRingExtCtrl,
	/* null wfdmaManualPrefetch if want to disable manual mode */
	.wfdmaManualPrefetch = mt7935WfdmaManualPrefetch,
	.lowPowerOwnInit = mt7935LowPowerOwnInit,
	.lowPowerOwnRead = mt7935LowPowerOwnRead,
	.lowPowerOwnSet = mt7935LowPowerOwnSet,
	.lowPowerOwnClear = mt7935LowPowerOwnClear,
	.wakeUpWiFi = asicWakeUpWiFi,
	.softwareInterruptMcu = asicConnac3xSoftwareInterruptMcu,
	.hifRst = asicConnac3xHifRst,
#if defined(_HIF_PCIE) && (WFDMA_AP_MSI_NUM == 8)
	.devReadIntStatus = mt7935ReadIntStatusByMsi,
#else
#if CFG_MTK_WIFI_WFDMA_WB
	.devReadIntStatus = mt7935ReadIntStatusByEmi,
#else
	.devReadIntStatus = mt7935ReadIntStatus,
#endif /* CFG_MTK_WIFI_WFDMA_WB */
#endif /* _HIF_PCIE */
	.setRxRingHwAddr = mt7935SetRxRingHwAddr,
	.wfdmaAllocRxRing = mt7935WfdmaAllocRxRing,
#if CFG_MTK_WIFI_SUPPORT_IPC
	.setupMcuEmiAddr = NULL,
#else
	.setupMcuEmiAddr = mt7935SetupMcuEmiAddr,
#endif
#endif /*_HIF_PCIE || _HIF_AXI */
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.DmaShdlInit = mt7935DmashdlInit,
#endif

#if defined(_HIF_NONE)
	/* for compiler need one entry */
	.DmaShdlInit = NULL
#endif
};

#if CFG_ENABLE_FW_DOWNLOAD
struct FWDL_OPS_T mt7935_fw_dl_ops = {
#if CFG_MTK_WIFI_SUPPORT_IPC
	.constructFirmwarePrio = NULL,
	.constructPatchName = NULL,
#else
	.constructFirmwarePrio = mt7935_ConstructFirmwarePrio,
	.constructPatchName = mt7935_ConstructPatchName,
#endif
#if CFG_SUPPORT_SINGLE_FW_BINARY
#if CFG_MTK_WIFI_SUPPORT_IPC
	.parseSingleBinaryFile = NULL,
#else
	.parseSingleBinaryFile = wlanParseSingleBinaryFile,
#endif
#endif
#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
	.constrcutIdxLogBin = NULL,
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */
#if defined(_HIF_PCIE)
#if CFG_MTK_WIFI_SUPPORT_IPC
	.downloadPatch = NULL,
#else
	.downloadPatch = mt7935_wlanDownloadPatch,
#endif
#endif
#if CFG_MTK_WIFI_SUPPORT_IPC
	.downloadFirmware = NULL,
	.downloadByDynMemMap = NULL,
	.getFwInfo = NULL,
	.getFwDlInfo = NULL,
	.downloadEMI = NULL,
#else
	.downloadFirmware = wlanConnacFormatDownload,
	.downloadByDynMemMap = NULL,
	.getFwInfo = wlanGetConnacFwInfo,
	.getFwDlInfo = asicGetFwDlInfo,
	.downloadEMI = wlanDownloadEMISectionViaDma,
#endif
#if (CFG_SUPPORT_PRE_ON_PHY_ACTION == 1)
#if CFG_MTK_WIFI_SUPPORT_IPC
	.phyAction = NULL,
#else
	.phyAction = wlanPhyAction,
#endif
#else
	.phyAction = NULL,
#endif
#if defined(_HIF_PCIE)
	.mcu_init = mt7935_mcu_init,
	.mcu_deinit = mt7935_mcu_deinit,
#endif
#if CFG_MTK_WIFI_SUPPORT_IPC
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
	.constructBtPatchName = NULL,
	.downloadBtPatch = NULL,
#if (CFG_SUPPORT_CONNAC3X == 1)
	.configBtImageSection = NULL,
#endif
#endif
	.getFwVerInfo = NULL,
#if CFG_MTK_WIFI_SUPPORT_DSP_FWDL
	.constructDspName = NULL,
	.downloadDspFw = NULL,
#endif
#else
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
	.constructBtPatchName = asicConnac3xConstructBtPatchName,
	.downloadBtPatch = asicConnac3xDownloadBtPatch,
#if (CFG_SUPPORT_CONNAC3X == 1)
	.configBtImageSection = asicConnac3xConfigBtImageSection,
#endif
#endif
	.getFwVerInfo = wlanParseRamCodeReleaseManifest,
#if CFG_MTK_WIFI_SUPPORT_DSP_FWDL
	.constructDspName = mt7935_ConstructDspName,
	.downloadDspFw = wlanDownloadDspFw,
#endif
#endif
};
#endif /* CFG_ENABLE_FW_DOWNLOAD */

struct TX_DESC_OPS_T mt7935_TxDescOps = {
	.fillNicAppend = fillNicTxDescAppend,
	.fillHifAppend = fillTxDescAppendByHostV2,
	.fillTxByteCount = fillConnac3xTxDescTxByteCount,
};

struct RX_DESC_OPS_T mt7935_RxDescOps = {0};

struct CHIP_DBG_OPS mt7935_DebugOps = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.showPdmaInfo = connac3x_show_wfdma_info,
#endif
	.showTxdInfo = connac3x_show_txd_Info,
	.showWtblInfo = connac3x_show_wtbl_info,
	.showUmacWtblInfo = connac3x_show_umac_wtbl_info,
	.showCsrInfo = NULL,
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.showDmaschInfo = connac3x_show_dmashdl_lite_info,
#endif
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.getFwDebug = NULL,
	.setFwDebug = NULL,
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
	.show_wfdma_dbg_probe_info = mt7935_show_wfdma_dbg_probe_info,
	.show_wfdma_wrapper_info = mt7935_show_wfdma_wrapper_info,
	.dumpwfsyscpupcr = mt7935_dumpWfsyscpupcr,
	.dumpBusHangCr = mt7935_DumpBusHangCr,
#endif
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	.get_rx_rate_info = mt7935_get_rx_rate_info,
#endif
#if CFG_SUPPORT_LLS
	.get_rx_link_stats = mt7935_get_rx_link_stats,
#endif
	.dumpTxdInfo = connac3x_dump_tmac_info,
};

#if CFG_SUPPORT_QA_TOOL
#if (CONFIG_WLAN_SERVICE == 1)
struct test_capability mt7935_toolCapability = {
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

		/* BIT[15:0]: Band0 2.4G, 0x1 */
		/* BIT[31:16]: Band1 5G, 6G, 0x6 */
		0x00060001,	/* u_int32 channel_band_dbdc;*/

		/* BIT[15:0]: Band2 N/A, 0x0 */
		/* BIT[31:16]: Band3 2.4G, 5G, 6G, 0x7 */
		0x00070000,	/* u_int32 channel_band_dbdc_ext; */

		/* BIT[7:0]: Support phy 0xB (bitwise),
		 *           phy0, phy1, phy3(little)
		 */
		/* BIT[15:8]: Support Adie 0x1 (bitwise) */
		0x010B,	/* u_int32 phy_adie_index; CFG_SUPPORT_CONNAC3X */

		/* BIT[7:0]: Band0 TX path 2 */
		/* BIT[15:8]: Band0 RX path 2 */
		/* BIT[23:16]: Band1 TX path 2 */
		/* BIT[31:24]: Band1 RX path 2 */
		0x02020202,	/* u_int32 band_0_1_wf_path_num; */

		/* BIT[7:0]: Band2 TX path 0 */
		/* BIT[15:8]: Band2 RX path 0 */
		/* BIT[23:16]: Band3 TX path 0 */
		/* BIT[31:24]: Band3 RX path 1 */
		0x01000000,	/* u_int32 band_2_3_wf_path_num; */

		/* BIT[7:0]: Band0 BW20, 0x1 */
		/* BIT[15:8]: Band1 BW160, 0xF */
		/* BIT[23:16]: Band2 N/A, 0x0 */
		/* BIT[31:24]: Band3 BW20, 0x1 */
		0x01000F01,	/* u_int32 band_bandwidth; */

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
		/* BIT2: Little core support 1 */
		/* BIT3: XTAL trim support 1 */
		/* BIT4: DBDC/MIMO switch support 0 */
		/* BIT5: eMLSR support 1 */
		/* BIT6: MLR+, ALR support 0 */
		/* BIT7: Bandwidth duplcate debug support 0 */
		/* BIT8: dRU support 1 */
		0x12C,	/*u_int32 feature1; */

		/* u_int32 reserved[15]; */
		{ 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0 }
	}
};
#endif

struct ATE_OPS_T mt7935_AteOps = {
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
	.icapRiseVcoreClockRate = mt7935_icapRiseVcoreClockRate,
	.icapDownVcoreClockRate = mt7935_icapDownVcoreClockRate,
#if (CONFIG_WLAN_SERVICE == 1)
	.tool_capability = &mt7935_toolCapability,
#endif
};
#endif /* CFG_SUPPORT_QA_TOOL */

#if CFG_NEW_HIF_DEV_REG_IF
enum HIF_DEV_REG_REASON mt7935ValidMmioReadReason[] = {
	HIF_DEV_REG_HIF_DBG,
	HIF_DEV_REG_HIF_EXTDBG,
	HIF_DEV_REG_ONOFF_READ,
	HIF_DEV_REG_ONOFF_DBG,
	HIF_DEV_REG_RESET_READ,
	HIF_DEV_REG_COREDUMP_DBG,
	HIF_DEV_REG_LPOWN_READ,
	HIF_DEV_REG_PLAT_DBG,
	HIF_DEV_REG_WTBL_DBG,
	HIF_DEV_REG_OID_DBG,
#if (CFG_MTK_FPGA_PLATFORM != 0)
	HIF_DEV_REG_HIF_RING,
#endif
#if (CFG_MTK_WIFI_WFDMA_WB == 0)
	HIF_DEV_REG_HIF_READ,
	HIF_DEV_REG_SER_READ,
#endif
};
#endif /* CFG_NEW_HIF_DEV_REG_IF */

static struct sw_sync_emi_info mt7935_sw_sync_emi_info[SW_SYNC_TAG_NUM];

#if CFG_MTK_WIFI_SUPPORT_IPC
static struct mt7935_conn_von_sysram_layout_t mt7935_conn_von_sysram_layout;
static struct mt7935_wfmcu_doorbell_layout_t mt7935_wfmcu_doorbell_layout;
static struct mt7935_cbmcu_doorbell_layout_t mt7935_cbmcu_doorbell_layout;
static struct mt7935_bitmap_layout_t mt7935_bitmap_layout;

struct WLAN_IPC_INFO mt7935_ipc_info = {
	.wfmcu_doorbell_pci_cfg_space_base_offset =
		MT7935_WFMCU_DOORBELL_PCI_CFG_SPACE_BASE_OFFSET,
	.cbmcu_doorbell_pci_cfg_space_base_offset =
		MT7935_CBMCU_DOORBELL_PCI_CFG_SPACE_BASE_OFFSET,
	.bitmap_pci_cfg_space_base_offset =
		MT7935_BITMAP_PCI_CFG_SPACE_BASE_OFFSET,
	.conn_von_sysram_base_addr = MT7935_CONN_VON_SYSRAM_BASE_ADDR,
	.conn_von_sysram_layout = &mt7935_conn_von_sysram_layout,
	.bitmap_layout = &mt7935_bitmap_layout,
	.wfmcu_doorbell_layout = &mt7935_wfmcu_doorbell_layout,
	.cbmcu_doorbell_layout = &mt7935_cbmcu_doorbell_layout,
	.ipcCheckStatus = wlanIPCCheckStatus,
	.ipcAccessConnVonSysRam = wlanIPCAccessConnVonSysRam,
	.ipcAccessPciCfgSpace = wlanIPCAccessPciCfgSpace,
	.ipcLoadFirmware = mt7935IPCLoadFirmware,
	.ipcSetupWiFiMcuEmiAddr = mt7935SetupWiFiMcuEmiAddr,
	.ipcSetupCbMcuEmiAddr = NULL,
};
#endif /* CFG_MTK_WIFI_SUPPORT_IPC */

struct mt66xx_chip_info mt66xx_chip_info_mt7935 = {
	.bus_info = &mt7935_bus_info,
#if CFG_ENABLE_FW_DOWNLOAD
	.fw_dl_ops = &mt7935_fw_dl_ops,
#endif /* CFG_ENABLE_FW_DOWNLOAD */
#if CFG_SUPPORT_QA_TOOL
	.prAteOps = &mt7935_AteOps,
#endif /* CFG_SUPPORT_QA_TOOL */
	.prTxDescOps = &mt7935_TxDescOps,
	.prRxDescOps = &mt7935_RxDescOps,
	.prDebugOps = &mt7935_DebugOps,
#if (CFG_MTK_WIFI_SUPPORT_IPC == 1)
	.ipc_info = &mt7935_ipc_info,
#endif /* CFG_MTK_WIFI_SUPPORT_IPC */
	.chip_id = MT7935_CHIP_ID,
	.should_verify_chip_id = FALSE,
	.sw_sync0 = CONNAC3X_CONN_CFG_ON_CONN_ON_MISC_ADDR,
	.sw_ready_bits = WIFI_FUNC_NO_CR4_READY_BITS,
	.sw_ready_bit_offset =
		Connac3x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT,
	.patch_addr = MT7935_PATCH_START_ADDR,
	.is_support_cr4 = FALSE,
	.is_support_wacpu = FALSE,
	.is_support_dmashdl_lite = TRUE,
	.sw_sync_emi_info = mt7935_sw_sync_emi_info,
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
	.wifi_off_magic_num = MT7935_WIFI_OFF_MAGIC_NUM,
#endif /* CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI */
#if defined(_HIF_PCIE)
	.is_en_wfdma_no_mmio_read = FALSE,
#endif /* _HIF_PCIE */
#if CFG_MTK_WIFI_WFDMA_WB
	.is_support_wfdma_write_back = TRUE,
	.is_support_wfdma_cidx_fetch = TRUE,
	.wb_didx_size = sizeof(struct WFDMA_EMI_RING_DIDX),
	.wb_cidx_size = sizeof(struct WFDMA_EMI_RING_CIDX),
	.wb_hw_done_flag_size = sizeof(struct WFDMA_EMI_DONE_FLAG),
	.wb_sw_done_flag_size = sizeof(struct WFDMA_EMI_DONE_FLAG),
	.allocWfdmaWbBuffer = asicConnac3xAllocWfdmaWbBuffer,
	.freeWfdmaWbBuffer = asicConnac3xFreeWfdmaWbBuffer,
	.enableWfdmaWb = mt79353EnableWfdmaWb,
	.runWfdmaCidxFetch = mt7935RunWfdmaCidxFetch,
#endif
	.txd_append_size = MT7935_TX_DESC_APPEND_LENGTH,
	.hif_txd_append_size = MT7935_HIF_TX_DESC_APPEND_LENGTH,
	.rxd_size = MT7935_RX_DESC_LENGTH,
	.init_evt_rxd_size = MT7935_RX_INIT_DESC_LENGTH,
	.pse_header_length = CONNAC3X_NIC_TX_PSE_HEADER_LENGTH,
	.init_event_size = CONNAC3X_RX_INIT_EVENT_LENGTH,
	.eco_info = mt7935_eco_table,
	.isNicCapV1 = FALSE,
	.is_support_efuse = TRUE,
	.top_hcr = CONNAC3X_TOP_HCR,
	.top_hvr = CONNAC3X_TOP_HVR,
	.top_fvr = CONNAC3X_TOP_FVR,
#if (CFG_SUPPORT_802_11AX == 1)
	.arb_ac_mode_addr = MT7935_ARB_AC_MODE_ADDR,
#endif
	.asicCapInit = asicConnac3xCapInit,
#if CFG_ENABLE_FW_DOWNLOAD
	.asicEnableFWDownload = NULL,
#endif /* CFG_ENABLE_FW_DOWNLOAD */

	.downloadBufferBin = NULL,
	.is_support_hw_amsdu = TRUE,
	.is_support_nvram_fragment = TRUE,
	.is_support_asic_lp = TRUE,
	.asicWfdmaReInit = asicConnac3xWfdmaReInit,
	.asicWfdmaReInit_handshakeInit = asicConnac3xWfdmaDummyCrWrite,
	.group5_size = sizeof(struct HW_MAC_RX_STS_GROUP_5),
	.u4LmacWtblDUAddr = CONNAC3X_WIFI_LWTBL_BASE,
	.u4UmacWtblDUAddr = CONNAC3X_WIFI_UWTBL_BASE,
#if CFG_MTK_MDDP_SUPPORT
	.isSupportMddpAOR = false,
	.isSupportMddpSHM = true,
#else
	.isSupportMddpAOR = false,
	.isSupportMddpSHM = false,
#endif
	.u4HostWfdmaBaseAddr = WF_WFDMA_HOST_DMA0_BASE,
	.u4HostWfdmaWrapBaseAddr = WF_WFDMA_EXT_WRAP_CSR_BASE,
	.u4McuWfdmaBaseAddr = WF_WFDMA_MCU_DMA0_BASE,
	.u4DmaShdlBaseAddr = WF_HIF_DMASHDL_TOP_BASE,
	.cmd_max_pkt_size = CFG_TX_MAX_PKT_SIZE, /* size 1600 */
#if defined(CFG_MTK_WIFI_PMIC_QUERY)
	.queryPmicInfo = asicConnac3xQueryPmicInfo,
#endif

	.prTxPwrLimitFile = "TxPwrLimit_MT66x9.dat",
#if (CFG_SUPPORT_SINGLE_SKU_6G == 1)
	.prTxPwrLimit6GFile = "TxPwrLimit6G_MT66x9.dat",
#if (CFG_SUPPORT_SINGLE_SKU_6G_1SS1T == 1)
	.prTxPwrLimit6G1ss1tFile = "TxPwrLimit6G_MT66x9_1ss1t.dat",
#endif
#endif

	.ucTxPwrLimitBatchSize = 3,
#if defined(_HIF_PCIE)
	.chip_capability = BIT(CHIP_CAPA_FW_LOG_TIME_SYNC) |
		BIT(CHIP_CAPA_FW_LOG_TIME_SYNC_BY_CCIF) |
		BIT(CHIP_CAPA_XTAL_TRIM),
	.checkbushang = mt7935_CheckBusHang,
	.rEmiInfo = {
#if CFG_MTK_ANDROID_EMI
		.type = EMI_ALLOC_TYPE_LK,
		.coredump_size = (7 * 1024 * 1024),
#else
		.type = EMI_ALLOC_TYPE_IN_DRIVER,
#endif /* CFG_MTK_ANDROID_EMI */
	},
	.trigger_fw_assert = mt7935_trigger_fw_assert,
	.get_sw_interrupt_status = mt7935_get_sw_interrupt_status,
#else
	.chip_capability = BIT(CHIP_CAPA_FW_LOG_TIME_SYNC) |
		BIT(CHIP_CAPA_XTAL_TRIM),
#endif /* _HIF_PCIE */
	.custom_oid_interface_version = MTK_CUSTOM_OID_INTERFACE_VERSION,
	.em_interface_version = MTK_EM_INTERFACE_VERSION,
#if CFG_CHIP_RESET_SUPPORT
	.asicWfsysRst = NULL,
	.asicPollWfsysSwInitDone = NULL,
#endif
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	/* owner set true when feature is ready. */
	.fgIsSupportL0p5Reset = TRUE,
#elif defined(_HIF_SDIO)
	/* owner set true when feature is ready. */
	.fgIsSupportL0p5Reset = FALSE,
#endif
	.u4MinTxLen = 2,
	.wifiNappingCtrl = mt7935WiFiNappingCtrl,
#if CFG_NEW_HIF_DEV_REG_IF
	.fgIsResetInvalidMmioRead = TRUE,
	.isValidMmioReadReason = connac3xIsValidMmioReadReason,
	.prValidMmioReadReason = mt7935ValidMmioReadReason,
	.u4ValidMmioReadReasonSize = ARRAY_SIZE(mt7935ValidMmioReadReason),
#endif /* CFG_NEW_HIF_DEV_REG_IF */
};

struct mt66xx_hif_driver_data mt66xx_driver_data_mt7935 = {
	.chip_info = &mt66xx_chip_info_mt7935,
};

void mt7935_icapRiseVcoreClockRate(void)
{
	DBGLOG(HAL, STATE, "icapRiseVcoreClockRate skip\n");
}

void mt7935_icapDownVcoreClockRate(void)
{
	DBGLOG(HAL, STATE, "icapDownVcoreClockRate skip\n");
}

#if (CFG_ENABLE_IPC_FW_DOWNLOAD == 1)
static uint32_t mt7935IPCLoadFirmware(struct ADAPTER *prAdapter,
	uint8_t **apucFwNameTable)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct WLAN_IPC_INFO *prIPCInfo = NULL;
	dma_addr_t rPhyAddr = 0;
	uint32_t u4Ret = 0, u4FwSize = 0, *pu4FwBuffer = NULL, u4Val = 0;

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "NULL prGlueInfo\n");
		u4Ret = WLAN_STATUS_INVALID_DATA;
		goto exit;
	}

	glGetChipInfo((void **)&prChipInfo);
	if (prChipInfo == NULL) {
		DBGLOG(INIT, ERROR, "NULL prChipInfo\n");
		u4Ret = WLAN_STATUS_INVALID_DATA;
		goto exit;
	}

	prIPCInfo = prChipInfo->ipc_info;
	if (prIPCInfo == NULL) {
		DBGLOG(INIT, ERROR, "NULL prIPCInfo\n");
		u4Ret = WLAN_STATUS_INVALID_DATA;
		goto exit;
	}

	prHifInfo = &prGlueInfo->rHifInfo;
	if (prHifInfo->pdev == NULL) {
		DBGLOG(INIT, ERROR, "NULL pdev\n");
		u4Ret = WLAN_STATUS_INVALID_DATA;
		goto exit;
	}

	u4Ret = kalFirmwareOpen(prGlueInfo, apucFwNameTable);
	if (u4Ret != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "Open FW failed.\n");
		u4Ret = WLAN_STATUS_INVALID_DATA;
		goto exit;
	}

	/* Set FW size */
	kalFirmwareSize(prGlueInfo, &u4FwSize);
	/* ALIGN 4 */
	u4FwSize = ALIGN_4(u4FwSize);
	pu4FwBuffer = KAL_DMA_ALLOC_COHERENT(prHifInfo->pdev,
		u4FwSize,
		&rPhyAddr);
	if (pu4FwBuffer == NULL) {
		DBGLOG(INIT, ERROR,
			"Alloc Physically continuous memory failed, size=%u.\n",
			u4FwSize);
		u4Ret = WLAN_STATUS_RESOURCES;
		goto close_fw;
	}

	/* Copy FW binary data to local buffer */
	u4Ret = kalFirmwareLoad(prGlueInfo,
				pu4FwBuffer,
				0,
				&u4FwSize);
	if (u4Ret != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR,
			"Copy FW binary to buffer failed.\n");
		KAL_DMA_FREE_COHERENT(prHifInfo->pdev,
			u4FwSize,
			pu4FwBuffer,
			rPhyAddr);
		goto free_mem;
	}

	/* Get FW header information */
	wlanGetUniFwHeaderInfo(pu4FwBuffer);

	/*Setup FW image addr and size */
	if (prIPCInfo->ipcAccessConnVonSysRam) {
		u4Val = rPhyAddr & 0xffffffff;
		prIPCInfo->ipcAccessConnVonSysRam(prGlueInfo,
			WLAN_IPC_WRITE,
			IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(lo_image_addr),
			&u4Val,
			sizeof(u4Val));
		u4Val = (rPhyAddr >> 32) & 0xffffffff;
		prIPCInfo->ipcAccessConnVonSysRam(prGlueInfo,
			WLAN_IPC_WRITE,
			IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(hi_image_addr),
			&u4Val,
			sizeof(u4Val));
		prIPCInfo->ipcAccessConnVonSysRam(prGlueInfo,
			WLAN_IPC_WRITE,
			IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(image_size),
			&u4FwSize,
			sizeof(u4FwSize));
	}

	/* Trigger Doorbell */
	if (prIPCInfo->ipcAccessPciCfgSpace) {
		u4Ret = prIPCInfo->ipcAccessPciCfgSpace(WLAN_IPC_SET,
			prIPCInfo->cbmcu_doorbell_pci_cfg_space_base_offset,
			IPC_GET_CBMCU_DOORBELL_BIT_OFFSET(cb_image_doorbell),
			&u4Val);
		if (u4Ret != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"Write image doorbell failed, ret=%u\n",
				u4Ret);
			goto free_mem;
		}
	}

	if (prIPCInfo->ipcCheckStatus) {
		DBGLOG(INIT, INFO,
			"Start polling image response = success, Time:%u\n",
			kalGetTimeTick());
		u4Ret = prIPCInfo->ipcCheckStatus(prAdapter->prGlueInfo,
			CONN_VON_SYSRAM,
			FALSE,
			0,
			&u4Val,
			IMG_RESP_SUCCESS,
			IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(image_response),
			sizeof(u4Val),
			50,
			200);
		if (u4Ret != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"Polling image response failed, img resp:%u\n",
				u4Val);
			goto free_mem;
		}
	}

	if (prIPCInfo->ipcCheckStatus) {
		DBGLOG(INIT, INFO,
			"Start polling boot stage = OS, Time:%u\n",
			kalGetTimeTick());
		u4Ret = prIPCInfo->ipcCheckStatus(prAdapter->prGlueInfo,
			CONN_VON_SYSRAM,
			FALSE,
			0,
			&u4Val,
			BOOT_STAGE_OS,
			IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(boot_stage),
			sizeof(u4Val),
			10,
			200);
		if (u4Ret != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"Polling boot stage failed, boot stage:%u\n",
				u4Val);
			goto free_mem;
		}
	}

	u4Ret = WLAN_STATUS_SUCCESS;

free_mem:
	KAL_DMA_FREE_COHERENT(prHifInfo->pdev, u4FwSize,
				pu4FwBuffer, rPhyAddr);
close_fw:
	kalFirmwareClose(prGlueInfo);
exit:
	return u4Ret;
}

static uint32_t mt7935IPCFirmwareDownload(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct WLAN_IPC_INFO *prIPCInfo = NULL;
	uint32_t u4Status = WLAN_STATUS_SUCCESS, u4Val = 0, u4Size = 0;
	uint32_t *pu4EfuseInfo = NULL;
	uint8_t *apucFwNameTable[] = {
		MT7935_WF_CB_MCU_FW_BIN_NAME,
		NULL
	};
	enum ENUM_IPC_FWDL_FAIL_REASON {
		POLLING_SW_INIT_DONE_FAIL,
		CHECK_HW_FW_ID_FAIL,
		GET_EFUSE_INFO_FAIL,
		POLLING_ROM_STAGE_FAIL,
		LOAD_FW_IMAGE_FAIL,
		IPC_FWDL_FAIL_REASON_NUM
	} eFailReason = IPC_FWDL_FAIL_REASON_NUM;

	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "NULL prAdapter.\n");
		return WLAN_STATUS_FAILURE;
	}

	prChipInfo = prAdapter->chip_info;
	if (prChipInfo == NULL) {
		DBGLOG(INIT, ERROR, "NULL prChipInfo.\n");
		return WLAN_STATUS_FAILURE;
	}

	prIPCInfo = prChipInfo->ipc_info;
	if (prIPCInfo == NULL) {
		DBGLOG(INIT, WARN, "IPC FWDL is not supported.");
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	/* Setup Wi-Fi MCU EMI Addr */
	if (prIPCInfo->ipcSetupWiFiMcuEmiAddr)
		prIPCInfo->ipcSetupWiFiMcuEmiAddr(prAdapter);

	DBGLOG(INIT, INFO, "IPC FWDL LAUNCHED!! Time:%u\n", kalGetTimeTick());
	do {

		/* <1> Polling Wi-Fi SW init done */
		if (prIPCInfo->ipcCheckStatus) {
			u4Status = prIPCInfo->ipcCheckStatus(
				prAdapter->prGlueInfo,
				PCI_CFG_SPACE,
				TRUE,
				IPC_GET_BITMAP_BIT_OFFSET(wifi_sw_init_done),
				&u4Val,
				1,
				prIPCInfo->bitmap_pci_cfg_space_base_offset,
				sizeof(u4Val),
				10,
				200);
			if (u4Status != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR,
					"Polling WiFi SW init done failed.\n");
				eFailReason = POLLING_SW_INIT_DONE_FAIL;
				break;
			}
		}

		/* <2> Check HW/FW ID */
		if (prIPCInfo->ipcCheckStatus) {
			DBGLOG(INIT, INFO, "Check HW ID:\n");
			u4Status = prIPCInfo->ipcCheckStatus(
				prAdapter->prGlueInfo,
				CONN_VON_SYSRAM,
				FALSE,
				0,
				&u4Val,
				0,
				IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(hw_version),
				sizeof(u4Val),
				0,
				0);
			if (u4Status != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR, "Check HW ID failed.\n");
				eFailReason = CHECK_HW_FW_ID_FAIL;
				break;
			}

			DBGLOG(INIT, INFO, "Check FW ID:\n");
			u4Status = prIPCInfo->ipcCheckStatus(
				prAdapter->prGlueInfo,
				CONN_VON_SYSRAM,
				FALSE,
				0,
				&u4Val,
				0,
				IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(fw_version),
				sizeof(u4Val),
				0,
				0);
			if (u4Status != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR, "Check FW ID failed.\n");
				eFailReason = CHECK_HW_FW_ID_FAIL;
				break;
			}
		}

		/* <3> Get Efuse info */
		DBGLOG(INIT, INFO, "Get Efuse info:\n");
		if (prIPCInfo->ipcCheckStatus) {
			u4Size = IPC_GET_CONN_VON_SYSRAM_FIELD_SIZE(
				prIPCInfo->conn_von_sysram_layout,
				wifi_efuse_info);
			pu4EfuseInfo = kalMemAlloc(u4Size, VIR_MEM_TYPE);
			if (pu4EfuseInfo == NULL) {
				DBGLOG(INIT, ERROR,
					"Alloc memory for eFuse info failed.\n");
				eFailReason = GET_EFUSE_INFO_FAIL;
				u4Status = WLAN_STATUS_FAILURE;
				break;
			}
			u4Status = prIPCInfo->ipcCheckStatus(
				prAdapter->prGlueInfo,
				CONN_VON_SYSRAM,
				0,
				0,
				pu4EfuseInfo,
				0,
				IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(
					wifi_efuse_info),
				u4Size,
				0,
				0);
			if (u4Status != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR,
					"Get Wi-Fi eFuse info failed.\n");
				eFailReason = GET_EFUSE_INFO_FAIL;
				break;
			}
		}

		/* <4> Polling Boot Stage is ROM */
		u4Val = BOOT_STAGE_NUM;
		if (prIPCInfo->ipcCheckStatus) {
			u4Status = prIPCInfo->ipcCheckStatus(
				prAdapter->prGlueInfo,
				CONN_VON_SYSRAM,
				FALSE,
				0,
				&u4Val,
				BOOT_STAGE_ROM,
				IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(boot_stage),
				sizeof(u4Val),
				10,
				200);
			if (u4Status != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR,
					"Polling WiFi SW init done failed.\n");
				eFailReason = POLLING_ROM_STAGE_FAIL;
				break;
			}
		}

		/* <5-1> Load FW image and write the doorbell
		 * <5-2> Polling Image Response = Success (Security check pass)
		 * <5-3> Polling Boot Stage = OS (Wi-Fi RAM code init done)
		 */
		if (prIPCInfo->ipcLoadFirmware) {
			u4Status = prIPCInfo->ipcLoadFirmware(prAdapter,
				apucFwNameTable);
			if (u4Status != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR,
					"Load FW image and write doorbell failed.\n");
				eFailReason = LOAD_FW_IMAGE_FAIL;
				break;
			}
		}
	} while (FALSE);

	if (u4Status != WLAN_STATUS_SUCCESS) {
		switch (eFailReason) {
		case LOAD_FW_IMAGE_FAIL:
		case POLLING_ROM_STAGE_FAIL:
		case GET_EFUSE_INFO_FAIL:
			if (pu4EfuseInfo)
				kalMemFree(pu4EfuseInfo, VIR_MEM_TYPE, u4Size);
		case CHECK_HW_FW_ID_FAIL:
		case POLLING_SW_INIT_DONE_FAIL:
		default:
			break;
		}
		return u4Status;
	}

	DBGLOG(INIT, INFO, "IPC FWDL SUCCESS !! Time: %u\n", kalGetTimeTick());
	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_ENABLE_IPC_FW_DOWNLOAD */

static void mt7935_ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx)
{
	int ret = 0;
	uint8_t ucIdx = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7935GetFlavorVer(&aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt7935_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7935_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt7935_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7935_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2. WIFI_RAM_CODE_MT7935_1_1.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"WIFI_RAM_CODE_MT%x_%s_%u.bin",
			MT7935_CHIP_ID,
			aucFlavor,
			MT7935_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	for (ucIdx = 0; apucmt7935FwName[ucIdx]; ucIdx++) {
		if ((*pucNameIdx + 3) >= ucMaxNameIdx) {
			/* the table is not large enough */
			DBGLOG(INIT, ERROR,
				"kalFirmwareImageMapping >> file name array is not enough.\n");
			ASSERT(0);
			continue;
		}

		/* Type 3. WIFI_RAM_CODE_7935.bin */
		ret = kalSnprintf(*(apucName + (*pucNameIdx)),
				CFG_FW_NAME_MAX_LEN, "%s.bin",
				apucmt7935FwName[ucIdx]);
		if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
			(*pucNameIdx) += 1;
		else
			DBGLOG(INIT, ERROR,
				"[%u] kalSnprintf failed, ret: %d\n",
				__LINE__, ret);
	}
}

static void mt7935_ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7935GetFlavorVer(&aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt7935_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7935_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt7935_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7935_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2. WIFI_MT7935_PATCH_MCU_1_1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_MT%x_PATCH_MCU_%s_%u_hdr.bin",
			  MT7935_CHIP_ID,
			  aucFlavor,
			  MT7935_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 3. mt7935_patch_e1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "mt7935_patch_e1_hdr.bin");
	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
}

#if CFG_MTK_WIFI_SUPPORT_DSP_FWDL
static void mt7935_ConstructDspName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7935GetFlavorVer(&aucFlavor[0]);

	/* Type 1. WIFI_MT7935_PHY_RAM_CODE_1_1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_MT%x_PHY_RAM_CODE_%s_%u.bin",
			  MT7935_CHIP_ID,
			  aucFlavor,
			  MT7935_ROM_VERSION);
	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
	else
		(*pucNameIdx) += 1;
}
#endif

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
static void mt7935_ConstructIdxLogBinName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	mt7935GetFlavorVer(&aucFlavor[0]);

	/* ex: WIFI_RAM_CODE_MT7935_2_1_idxlog.bin */
	ret = kalSnprintf(apucName[0],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_RAM_CODE_MT%x_%s_%u_idxlog.bin",
			  MT7935_CHIP_ID,
			  aucFlavor,
			  MT7935_ROM_VERSION);

	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
}
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */

#if defined(_HIF_PCIE)
static uint32_t mt7935RxRingSwIdx2HwIdx(uint32_t u4SwRingIdx)
{
	uint32_t u4HwRingIdx = 0;

	/*
	 * RX_RING_DATA0   (RX_Ring0) - Rx Data
	 * RX_RING_DATA1   (RX_Ring2) - HPP Rx Data
	 * RX_RING_EVT     (RX_Ring3) - Rx Event
	 * RX_RING_TXDONE0 (RX_Ring1) - Tx Free Done Event
	 * RX_RING_TXDONE1 (RX_Ring4) - ICS / RXPRT
	 * RX_RING_TXDONE2 (RX_Ring5) - Coredump event
	 * RX_RING_WAEVT0  (RX_Ring6) - FW log
	*/
	switch (u4SwRingIdx) {
	case RX_RING_EVT:
		u4HwRingIdx = 3;
		break;
	case RX_RING_DATA0:
		u4HwRingIdx = 0;
		break;
	case RX_RING_DATA1:
		u4HwRingIdx = 2;
		break;
	case RX_RING_TXDONE0:
		u4HwRingIdx = 1;
		break;
	case RX_RING_TXDONE1:
		u4HwRingIdx = 4;
		break;
	case RX_RING_TXDONE2:
		u4HwRingIdx = 5;
		break;
	case RX_RING_WAEVT0:
		u4HwRingIdx = 6;
		break;
	default:
		return RX_RING_MAX;
	}

	return u4HwRingIdx;
}

static uint8_t mt7935SetRxRingHwAddr(struct RTMP_RX_RING *prRxRing,
		struct BUS_INFO *prBusInfo, uint32_t u4SwRingIdx)
{
	uint32_t u4RingIdx = mt7935RxRingSwIdx2HwIdx(u4SwRingIdx);

	if (u4RingIdx >= RX_RING_MAX)
		return FALSE;

	halSetRxRingHwAddr(prRxRing, prBusInfo, u4RingIdx);

	return TRUE;
}

static bool mt7935WfdmaAllocRxRing(struct GLUE_INFO *prGlueInfo,
		bool fgAllocMem)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;

	/* HPP Data Rx path */
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_DATA1, prHifInfo->u4RxDataRingSize,
			RXD_SIZE, CFG_RX_MAX_PKT_SIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[2] fail\n");
		return false;
	}

	/* TX Free Done */
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_TXDONE0, prHifInfo->u4RxEvtRingSize,
			RXD_SIZE, RX_BUFFER_AGGRESIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[3] fail\n");
		return false;
	}

	/* ICS log */
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_TXDONE1, prHifInfo->u4RxEvtRingSize,
			RXD_SIZE, RX_BUFFER_AGGRESIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[4] fail\n");
		return false;
	}

	/* Coredump */
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_TXDONE2, prHifInfo->u4RxEvtRingSize,
			RXD_SIZE, RX_BUFFER_AGGRESIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[6] fail\n");
		return false;
	}

	/* Fw log */
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_WAEVT0, prHifInfo->u4RxEvtRingSize,
			RXD_SIZE, RX_BUFFER_AGGRESIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[7] fail\n");
		return false;
	}
	return true;
}

static void mt7935ProcessTxInterrupt(
		struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if (u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_16_MASK)
		halWpdmaProcessCmdDmaDone(
			prAdapter->prGlueInfo, TX_RING_FWDL);

#if (CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0)
	if (u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_15_MASK)
		halWpdmaProcessCmdDmaDone(
			prAdapter->prGlueInfo, TX_RING_CMD);
#endif /* CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0 */

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
			prAdapter->prGlueInfo, TX_RING_DATA2);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}
	if (u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_3_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA3);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}

	if (u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_4_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA_PRIO);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}

	if (u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_5_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA_ALTX);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0 */
}

static void mt7935ProcessRxDataInterrupt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if ((u4Sta &
	     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_0_MASK) ||
	    (KAL_TEST_BIT(RX_RING_DATA0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_DATA0, TRUE);

	if ((u4Sta &
	     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_2_MASK) ||
	    (KAL_TEST_BIT(RX_RING_DATA1, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_DATA1, TRUE);
}

static void mt7935ProcessRxInterrupt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_1_MASK) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE0, FALSE);

	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_3_MASK) ||
	    (KAL_TEST_BIT(RX_RING_EVT, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_EVT, FALSE);

	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_4_MASK) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE1, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE1, FALSE);

	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_5_MASK) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE2, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE2, FALSE);

	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_6_MASK) ||
	    (KAL_TEST_BIT(RX_RING_WAEVT0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_WAEVT0, FALSE);

	mt7935ProcessRxDataInterrupt(prAdapter);
}

static void mt7935SetTRXRingPriorityInterrupt(struct ADAPTER *prAdapter)
{
	uint32_t u4Val = 0;

#if (WFDMA_AP_MSI_NUM == 8)
	u4Val |= 0xF0;
#endif
#if CFG_MTK_MDDP_SUPPORT && (WFDMA_MD_MSI_NUM == 8)
	u4Val |= 0xF00;
#endif
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_INT_RX_PRI_SEL_ADDR, u4Val);

	u4Val = 0;
#if (WFDMA_AP_MSI_NUM == 8)
	u4Val |= 0x180FF;
#endif
#if CFG_MTK_MDDP_SUPPORT && (WFDMA_MD_MSI_NUM == 8)
	u4Val |= 0x7F00;
#endif
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_INT_TX_PRI_SEL_ADDR, u4Val);
}

static void mt7935WfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint32_t u4WrVal = 0, u4Addr = 0;
	uint32_t u4PrefetchCnt = 0x4, u4TxDataPrefetchCnt = 0x10;
	uint32_t u4PrefetchBase = 0x00400000, u4TxDataPrefetchBase = 0x01000000;
	uint32_t u4RxDataPrefetchCnt = 0x8;
	uint32_t u4RxDataPrefetchBase = 0x00800000;

	/* Rx ring */
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4RxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4RxDataPrefetchBase;
	}

	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING3_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4PrefetchBase;
	}

	/* Tx ring */
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_EXT_CTRL_ADDR;
		 u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_TX_RING5_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4TxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4TxDataPrefetchBase;
	}

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING15_EXT_CTRL_ADDR;
	u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
	u4WrVal += u4PrefetchBase;

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING16_EXT_CTRL_ADDR;
	u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
	u4WrVal += u4PrefetchBase;

	mt7935SetTRXRingPriorityInterrupt(prAdapter);

	/* reset dma TRX idx */
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_RST_DTX_PTR_ADDR, 0xFFFFFFFF);
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_RST_DRX_PTR_ADDR, 0xFFFFFFFF);
}

#if defined(_HIF_PCIE)
static void mt7935ReadIntStatusByMsi(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct BUS_INFO *prBusInfo = prChipInfo->bus_info;
	struct pcie_msi_info *prMsiInfo = &prBusInfo->pcie_msi_info;
	uint32_t u4Value = 0, u4WrValue = 0;

	*pu4IntStatus = 0;

	if (KAL_TEST_BIT(PCIE_MSI_TX_FREE_DONE, prMsiInfo->ulEnBits)) {
		*pu4IntStatus |= WHISR_RX0_DONE_INT;
		u4Value |=
			WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_7_MASK;
	}

	if (KAL_TEST_BIT(PCIE_MSI_RX_DATA_BAND0, prMsiInfo->ulEnBits)) {
		*pu4IntStatus |= WHISR_RX0_DONE_INT;
		u4Value |=
			WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_4_MASK;
	}

	if (KAL_TEST_BIT(PCIE_MSI_RX_DATA_BAND1, prMsiInfo->ulEnBits)) {
		*pu4IntStatus |= WHISR_RX0_DONE_INT;
		u4Value |=
			WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_5_MASK;
	}

	if (KAL_TEST_BIT(PCIE_MSI_EVENT, prMsiInfo->ulEnBits)) {
		*pu4IntStatus |= WHISR_RX0_DONE_INT;
		u4Value |=
			WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_6_MASK;
	}

#if (CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0)
	if (KAL_TEST_BIT(PCIE_MSI_CMD, prMsiInfo->ulEnBits)) {
		*pu4IntStatus |= WHISR_TX_DONE_INT;
		u4WrValue |=
			WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_15_MASK;
	}
#endif /* CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0 */

	if (KAL_TEST_BIT(PCIE_MSI_LUMP, prMsiInfo->ulEnBits)) {
		*pu4IntStatus |= WHISR_D2H_SW_INT;
		u4WrValue |= CONNAC_MCU_SW_INT;
	}

	/* force process all interrupt */
	if (KAL_TEST_BIT(GLUE_FLAG_HALT_BIT, prAdapter->prGlueInfo->ulFlag)) {
		*pu4IntStatus |= WHISR_RX0_DONE_INT | WHISR_TX_DONE_INT |
			WHISR_D2H_SW_INT;
		u4WrValue |= prBusInfo->host_int_rxdone_bits |
			prBusInfo->host_int_txdone_bits |
			CONNAC_MCU_SW_INT;
	}

	prHifInfo->u4IntStatus = u4Value | u4WrValue;

	/* clear interrupt */
	if (u4WrValue)
		HAL_MCR_WR(prAdapter, WF_WFDMA_HOST_DMA0_HOST_INT_STA_ADDR,
			   u4WrValue);
}
#endif

static void mt7935ReadIntStatus(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct BUS_INFO *prBusInfo = prChipInfo->bus_info;
	uint32_t u4RegValue = 0, u4WrValue = 0, u4Addr;

	*pu4IntStatus = 0;

	u4Addr = WF_WFDMA_HOST_DMA0_HOST_INT_STA_ADDR;
	HAL_RMCR_RD(HIF_READ, prAdapter, u4Addr, &u4RegValue);

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
}

static void mt7935ConfigIntMask(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_VAR *prWifiVar;
	uint32_t u4Addr = 0, u4WrVal = 0;

	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;

	u4Addr = enable ? WF_WFDMA_HOST_DMA0_HOST_INT_ENA_SET_ADDR :
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_CLR_ADDR;
	u4WrVal =
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA0_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA1_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA2_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA3_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA4_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA5_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA6_MASK |
#if (CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0)
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA0_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA1_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA2_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA3_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA4_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA5_MASK |
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0 */
#if (WFDMA_AP_MSI_NUM == 1)
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA16_MASK |
#endif
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_mcu2host_sw_int_ena_MASK;

	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4WrVal);
}

#if CFG_MTK_WIFI_WFDMA_WB
static void mt7935ReadIntStatusByEmi(struct ADAPTER *prAdapter,
				     uint32_t *pu4IntStatus)
{
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prHwDoneFlagBuf, *prSwDoneFlagBuf;
	struct WFDMA_EMI_DONE_FLAG *prHwDoneFlag, *prSwDoneFlag, *prIntFlag;

	prChipInfo = prAdapter->chip_info;
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	if (!prChipInfo->is_support_wfdma_write_back)
		return;

	prHwDoneFlagBuf = &prHifInfo->rHwDoneFlag;
	prSwDoneFlagBuf = &prHifInfo->rSwDoneFlag;
	prHwDoneFlag = (struct WFDMA_EMI_DONE_FLAG *)prHwDoneFlagBuf->AllocVa;
	prSwDoneFlag = (struct WFDMA_EMI_DONE_FLAG *)prSwDoneFlagBuf->AllocVa;
	prIntFlag = &prHifInfo->rIntFlag;

	*pu4IntStatus = 0;
	kalMemZero(prIntFlag, sizeof(struct WFDMA_EMI_DONE_FLAG));

	prIntFlag->tx_int0 = prHwDoneFlag->tx_int0 ^ prSwDoneFlag->tx_int0;
	prIntFlag->tx_int1 = prHwDoneFlag->tx_int1 ^ prSwDoneFlag->tx_int1;
	prIntFlag->rx_int0 = prHwDoneFlag->rx_int0 ^ prSwDoneFlag->rx_int0;
	prIntFlag->err_int = prHwDoneFlag->err_int ^ prSwDoneFlag->err_int;
	prIntFlag->sw_int = prHwDoneFlag->sw_int ^ prSwDoneFlag->sw_int;
	prIntFlag->subsys_int =
		prHwDoneFlag->subsys_int ^ prSwDoneFlag->subsys_int;
	prIntFlag->rro = prHwDoneFlag->rro ^ prSwDoneFlag->rro;

#if (CFG_SUPPORT_DISABLE_TX_DDONE_INTR == 0)
	if (prIntFlag->tx_int0) {
		*pu4IntStatus |= WHISR_TX_DONE_INT;
		prSwDoneFlag->tx_int0 = prHwDoneFlag->tx_int0;
	}
	if (prIntFlag->tx_int1) {
		*pu4IntStatus |= WHISR_TX_DONE_INT;
		prSwDoneFlag->tx_int1 = prHwDoneFlag->tx_int1;
	}
#endif

	if (prIntFlag->rx_int0) {
		*pu4IntStatus |= WHISR_RX0_DONE_INT;
		prSwDoneFlag->rx_int0 = prHwDoneFlag->rx_int0;
	}

	if (prIntFlag->err_int)
		prSwDoneFlag->err_int = prHwDoneFlag->err_int;

	if (prIntFlag->sw_int)
		*pu4IntStatus |= WHISR_D2H_SW_INT;

	if (prIntFlag->subsys_int)
		prSwDoneFlag->subsys_int = prHwDoneFlag->subsys_int;

	if (prIntFlag->rro)
		prSwDoneFlag->rro = prHwDoneFlag->rro;
}

static void mt7935ProcessTxInterruptByEmi(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_DISABLE_TX_DDONE_INTR == 0)
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->rIntFlag.tx_int0;

#if (CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0)
	if (u4Sta & BIT(prBusInfo->tx_ring_fwdl_idx))
		halWpdmaProcessCmdDmaDone(prGlueInfo, TX_RING_FWDL);

	if (u4Sta & BIT(prBusInfo->tx_ring_cmd_idx))
		halWpdmaProcessCmdDmaDone(prGlueInfo, TX_RING_CMD);
#endif /* CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0 */
#if (CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0)
	if (u4Sta & BIT(0)) {
		halWpdmaProcessDataDmaDone(pGlueInfo, TX_RING_DATA0);
		kalSetTxEvent2Hif(prGlueInfo);
	}

	if (u4Sta & BIT(1)) {
		halWpdmaProcessDataDmaDone(prGlueInfo, TX_RING_DATA1);
		kalSetTxEvent2Hif(prGlueInfo);
	}

	if (u4Sta & BIT(2)) {
		halWpdmaProcessDataDmaDone(pGlueInfo, TX_RING_DATA2);
		kalSetTxEvent2Hif(prGlueInfo);
	}

	if (u4Sta & BIT(3)) {
		halWpdmaProcessDataDmaDone(prGlueInfo, TX_RING_DATA3);
		kalSetTxEvent2Hif(prGlueInfo);
	}

	if (u4Sta & BIT(4)) {
		halWpdmaProcessDataDmaDone(prGlueInfo, TX_RING_DATA_PRIO);
		kalSetTxEvent2Hif(prGlueInfo);
	}

	if (u4Sta & BIT(5)) {
		halWpdmaProcessDataDmaDone(prGlueInfo, TX_RING_DATA_ALTX);
		kalSetTxEvent2Hif(prGlueInfo);
	}
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0 */
#endif /* CFG_SUPPORT_DISABLE_TX_DDONE_INTR == 0 */
}

static void mt7935ProcessRxInterruptByEmi(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->rIntFlag.rx_int0;

	if ((u4Sta & BIT(0)) ||
	    (KAL_TEST_BIT(RX_RING_DATA0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_DATA0, TRUE);

	if ((u4Sta & BIT(1)) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE0, FALSE);

	if ((u4Sta & BIT(2)) ||
	    (KAL_TEST_BIT(RX_RING_DATA1, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_DATA1, TRUE);

	if ((u4Sta & BIT(3)) ||
	    (KAL_TEST_BIT(RX_RING_EVT, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_EVT, FALSE);

	if ((u4Sta & BIT(4)) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE1, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE1, FALSE);

	if ((u4Sta & BIT(5)) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE2, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE2, FALSE);

	if ((u4Sta & BIT(6)) ||
	    (KAL_TEST_BIT(RX_RING_WAEVT0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_WAEVT0, FALSE);
}

static void mt7935ProcessSoftwareInterruptByEmi(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct ERR_RECOVERY_CTRL_T *prErrRecoveryCtrl;
	struct RTMP_DMABUF *prHwDoneFlagBuf, *prSwDoneFlagBuf;
	struct WFDMA_EMI_DONE_FLAG *prHwDoneFlag, *prSwDoneFlag;
	uint32_t u4Sta = 0, u4Addr = 0;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;

	prHwDoneFlagBuf = &prHifInfo->rHwDoneFlag;
	prSwDoneFlagBuf = &prHifInfo->rSwDoneFlag;
	prHwDoneFlag = (struct WFDMA_EMI_DONE_FLAG *)prHwDoneFlagBuf->AllocVa;
	prSwDoneFlag = (struct WFDMA_EMI_DONE_FLAG *)prSwDoneFlagBuf->AllocVa;
	prErrRecoveryCtrl = &prHifInfo->rErrRecoveryCtl;
	u4Sta = prHwDoneFlag->sw_int ^ prSwDoneFlag->sw_int;
	u4Sta = ((u4Sta & BITS(2, 17)) >> 2) |
		((u4Sta & BIT(0)) << 31) |
		((u4Sta & BIT(1)) << 29);

	DBGLOG(HAL, TRACE, "sw_int[0x%x]hwdone[0x%x]swdone[0x%x]\n",
	       u4Sta, prHwDoneFlag->sw_int, prSwDoneFlag->sw_int);

	prSwDoneFlag->sw_int = prHwDoneFlag->sw_int;
	prErrRecoveryCtrl->u4BackupStatus = u4Sta;
	if (u4Sta & ERROR_DETECT_SUBSYS_BUS_TIMEOUT) {
		DBGLOG(INIT, ERROR, "[SER][L0.5] wfsys timeout!!\n");
		GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_SUBSYS_BUS_HANG);
	} else if (u4Sta & ERROR_DETECT_MASK) {
		/* reset the done flag to zero when wfdma resetting */
		if (u4Sta & ERROR_DETECT_STOP_PDMA) {
			kalMemZero(prHwDoneFlagBuf->AllocVa,
				   prHwDoneFlagBuf->AllocSize);
			kalMemZero(prSwDoneFlagBuf->AllocVa,
				   prSwDoneFlagBuf->AllocSize);
		}
		prErrRecoveryCtrl->u4Status = u4Sta;
		halHwRecoveryFromError(prAdapter);
	} else
		DBGLOG(HAL, TRACE, "undefined SER status[0x%x].\n", u4Sta);

	u4Addr = WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_ADDR;
	HAL_MCR_WR(prAdapter, u4Addr, u4Sta);
}

static void mt7935WfdmaConfigWriteBack(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prRingDmyRd, *prRingDidx;
	struct RTMP_DMABUF *prHwDoneFlag, *prSwDoneFlag;
	struct WFDMA_EMI_DONE_FLAG *prSwDone;
	uint32_t u4Addr = 0, u4WrVal = 0;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;

	prRingDmyRd = &prHifInfo->rRingDmyRd;
	prRingDidx = &prHifInfo->rRingDidx;
	prHwDoneFlag = &prHifInfo->rHwDoneFlag;
	prSwDoneFlag = &prHifInfo->rSwDoneFlag;
	prSwDone = (struct WFDMA_EMI_DONE_FLAG *)prSwDoneFlag->AllocVa;

	/* set err_int enable */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA2HOST_ERR_INT_ENA_ADDR;
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set sw_int enable */
	u4Addr = WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_ENA_ADDR;
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set subsys_int enable */
	u4Addr = WF_WFDMA_HOST_DMA0_SUBSYS2HOST_INT_ENA_ADDR;
	u4WrVal = 0x200;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set periodic int */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL3_ADDR;
	u4WrVal = (50 <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL3_TRINFO_WB_PER_RD_TIME_SHFT) &
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL3_TRINFO_WB_PER_RD_TIME_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set dmy read cmd start address */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_DMY_CTRL2_ADDR;
	u4WrVal = ((uint64_t)prRingDmyRd->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set dmy read ext address */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_DMY_CTRL3_ADDR;
	u4WrVal = (((uint64_t)prRingDmyRd->AllocPa >> DMA_BITS_OFFSET) <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_DMY_CTRL3_DMY_RD_BASE_PTR_EXT_SHFT) &
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_DMY_CTRL3_DMY_RD_BASE_PTR_EXT_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set DIDX_WB_BASE_PTR */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL0_ADDR;
	u4WrVal = ((uint64_t)prRingDidx->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set HW_DONE_FLAG_WB_BASE_PTR */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL1_ADDR;
	u4WrVal = ((uint64_t)prHwDoneFlag->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set SW_DONE_FLAG_BASE_PTR */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_SW_DONE_BASE_PTR_ADDR;
	u4WrVal = ((uint64_t)prSwDoneFlag->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set SW_DONE_FLAG_BASE_PTR ext */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_SW_DONE_BASE_PTR_EXT_ADDR;
	u4WrVal = (((uint64_t)prSwDoneFlag->AllocPa >> DMA_BITS_OFFSET) <<
WF_WFDMA_HOST_DMA0_WPDMA_SW_DONE_BASE_PTR_EXT_SW_DONE_FLAG_BASR_PTR_EXT_SHFT) &
WF_WFDMA_HOST_DMA0_WPDMA_SW_DONE_BASE_PTR_EXT_SW_DONE_FLAG_BASR_PTR_EXT_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	kalMemZero(prSwDone, sizeof(struct WFDMA_EMI_DONE_FLAG));

#if (CFG_SUPPORT_DISABLE_TX_DDONE_INTR == 1)
	/* disable tx done interrupt */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_TX_INT_EN0_ADDR;
	u4WrVal = 0;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_TX_INT_EN1_ADDR;
	u4WrVal = 0;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
#endif /* CFG_SUPPORT_DISABLE_TX_DDONE_INTR == 1 */

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_ADDR;
	u4WrVal = 0x2 <<
		WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_INT_TRI_TIME_SHFT;

	/* set NOC */
	u4WrVal |= WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_NOC_BUS_SEL_MASK |
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_TRINFO_WB_AP_ONLY_MASK |
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_TRINFO_WB_DONE_FLAG_MODE_MASK |
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_TRINFO_WB_EN_MASK;

	/* set DIDX_WB_BASE_PTR ext */
	u4WrVal |= (((uint64_t)prRingDidx->AllocPa >> DMA_BITS_OFFSET) <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_DIDX_WB_BASE_PTR_EXT_SHFT) &
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_DIDX_WB_BASE_PTR_EXT_MASK;

	/* set HW_DONE_FLAG_WB_BASE_PTR ext */
	u4WrVal |= (((uint64_t)prRingDidx->AllocPa >> DMA_BITS_OFFSET) <<
WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_HW_DONE_FLAG_WB_BASE_PTR_EXT_SHFT) &
WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_HW_DONE_FLAG_WB_BASE_PTR_EXT_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
}

static void mt7935WfdmaConfigCidxFetch(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prRingCidx;
	uint32_t u4Addr = 0, u4WrVal = 0, u4RxPps = 45;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prRingCidx = &prHifInfo->rRingCidx;

	/* set rxring cidx fetch threshold */
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_CFET_TH_01_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_RX_CFET_TH_67_ADDR;
	     u4Addr += 4) {
		u4WrVal = u4RxPps;
		u4WrVal |= u4RxPps <<
		WF_WFDMA_HOST_DMA0_WPDMA_RX_CFET_TH_01_RX_CFET_TH_1_SHFT;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
	}

	/* set rx periodic fetch timer */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_CFET_CTRL0_ADDR;
	u4WrVal = 0x32; /* 50 * 20us */
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* enable pcie txp first qos priority */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_QOS_QTM_CFG1_ADDR;
	u4WrVal =
	WF_WFDMA_HOST_DMA0_WPDMA_TX_QOS_QTM_CFG1_CSR_QOS_DYNAMIC_SET_MASK |
	WF_WFDMA_HOST_DMA0_WPDMA_TX_QOS_QTM_CFG1_CSR_QOS_PRI_SEL_MASK |
	(3 << WF_WFDMA_HOST_DMA0_WPDMA_TX_QOS_QTM_CFG1_CSR_TXD_RSVD_QTM_SHFT) |
	(13 << WF_WFDMA_HOST_DMA0_WPDMA_TX_QOS_QTM_CFG1_CSR_TXD_FFA_QTM_SHFT) |
	(1 << WF_WFDMA_HOST_DMA0_WPDMA_TX_QOS_QTM_CFG1_CSR_DMAD_RSVD_QTM_SHFT) |
	(7 << WF_WFDMA_HOST_DMA0_WPDMA_TX_QOS_QTM_CFG1_CSR_DMAD_FFA_QTM_SHFT);
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set tx periodic fetch mode */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_QOS_FSM_ADDR;
	u4WrVal = 0x32; /* 50 * 20us */
	u4WrVal |= WF_WFDMA_HOST_DMA0_WPDMA_TX_QOS_FSM_CSR_PCIE_LP_QOS_EN_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set SW_DONE_FLAG_BASE_PTR */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_CIDX_FET_CTRL0_ADDR;
	u4WrVal = ((uint64_t)prRingCidx->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set SW_DONE_FLAG_BASE_PTR ext */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_CIDX_FET_CTRL1_ADDR;
	u4WrVal = (((uint64_t)prRingCidx->AllocPa >> DMA_BITS_OFFSET) <<
	WF_WFDMA_HOST_DMA0_WPDMA_CIDX_FET_CTRL1_CFET_BASE_PTR_EXT_SHFT) &
	WF_WFDMA_HOST_DMA0_WPDMA_CIDX_FET_CTRL1_CFET_BASE_PTR_EXT_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* enable TRX CIDX fetch */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_CIDX_FET_CTRL2_ADDR;
	u4WrVal = WF_WFDMA_HOST_DMA0_WPDMA_CIDX_FET_CTRL2_CFET_RX_EN_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_CIDX_FET_CTRL2_CFET_TX_EN_MASK;
	u4WrVal |= 0x0 <<
		WF_WFDMA_HOST_DMA0_WPDMA_CIDX_FET_CTRL2_CFET_DLY_TIME_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
}

static void mt7935ConfigEmiIntMask(struct GLUE_INFO *prGlueInfo,
				   u_int8_t enable)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	uint32_t u4Addr = 0, u4WrVal = 0;

	if (!prChipInfo->is_support_wfdma_write_back ||
	    !prChipInfo->is_enable_wfdma_write_back)
		return;

	/* Clr HOST interrupt enable */
	u4Addr = WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR;
	u4WrVal = 0;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	mt7935WfdmaConfigWriteBack(prGlueInfo);
	if (prChipInfo->is_support_wfdma_cidx_fetch)
		mt7935WfdmaConfigCidxFetch(prGlueInfo);
}

static void mt7935TriggerWfdmaCidxFetch(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint32_t u4Addr = 0, u4WrVal = 0;

	u4Addr = CONN_HOST_CSR_TOP_WF_DRV_CIDX_TRIG_ADDR;
	u4WrVal = CONN_HOST_CSR_TOP_WF_DRV_CIDX_TRIG_WF_DRV_CIDX_TRIG_MASK;
	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4WrVal);

	prHifInfo->fgIsCidxFetchNewTx = FALSE;
	prHifInfo->ulCidxFetchTimeout =
		jiffies +
		prAdapter->rWifiVar.u4WfdmaCidxFetchTimeout * HZ / 1000;
}

static u_int8_t mt7935CheckWfdmaCidxFetchTimeout(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct RTMP_TX_RING *prTxRing;
	uint32_t u4Idx;

	if (time_before(jiffies, prHifInfo->ulCidxFetchTimeout))
		return FALSE;

	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
		prTxRing = &prHifInfo->TxRing[u4Idx];
		HAL_GET_RING_CIDX(HIF_RING, prAdapter,
				  prTxRing, &prTxRing->TxCpuIdx);
		HAL_GET_RING_DIDX(HIF_RING, prAdapter,
				  prTxRing, &prTxRing->TxDmaIdx);

		if (prTxRing->TxCpuIdx != prTxRing->TxDmaIdx)
			return TRUE;
	}

	return FALSE;
}

static void mt7935RunWfdmaCidxFetch(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct HIF_STATS *prHifStats = &prAdapter->rHifStats;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (prHifInfo->fgIsNeedCidxFetchFlag) {
		GLUE_INC_REF_CNT(prHifStats->u4CidxFetchByNewTx);
		if (IS_FEATURE_ENABLED(prWifiVar->fgWfdmaCidxFetchDbg))
			DBGLOG(HAL, INFO, "Trigger cidx fetch by new tx");
		goto fetch;
	}

	if (prHifInfo->fgIsCidxFetchNewTx &&
	    mt7935CheckWfdmaCidxFetchTimeout(prGlueInfo)) {
		GLUE_INC_REF_CNT(prHifStats->u4CidxFetchByTimeout);
		if (IS_FEATURE_ENABLED(prWifiVar->fgWfdmaCidxFetchDbg))
			DBGLOG(HAL, INFO, "Trigger cidx fetch by timeout");
		goto fetch;
	}

	return;
fetch:
	prHifInfo->fgIsNeedCidxFetchFlag = FALSE;
	mt7935TriggerWfdmaCidxFetch(prGlueInfo);
}

static void mt7935TriggerWfdmaTxCidx(struct GLUE_INFO *prGlueInfo,
				     struct RTMP_TX_RING *prTxRing)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct HIF_STATS *prHifStats = &prAdapter->rHifStats;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (!prChipInfo->is_support_wfdma_cidx_fetch)
		return;

	if (!halIsDataRing(TX_RING, prTxRing->u4RingIdx)) {
		mt7935TriggerWfdmaCidxFetch(prGlueInfo);
		GLUE_INC_REF_CNT(prHifStats->u4CidxFetchByCmd);
		if (IS_FEATURE_ENABLED(prWifiVar->fgWfdmaCidxFetchDbg))
			DBGLOG(HAL, INFO, "Trigger cidx fetch by cmd");
		goto exit;
	}

	if (prHifInfo->fgIsUrgentCidxFetch) {
		prHifInfo->fgIsUrgentCidxFetch = FALSE;
		prHifInfo->fgIsNeedCidxFetchFlag = TRUE;
	}

	if (prTxRing->u4LastCidx == prTxRing->u4LastDidx)
		prHifInfo->fgIsNeedCidxFetchFlag = TRUE;

	prHifInfo->fgIsCidxFetchNewTx = TRUE;

exit:
	if (IS_FEATURE_ENABLED(prWifiVar->fgWfdmaCidxFetchDbg)) {
		DBGLOG(HAL, INFO,
		       "Ring[%u]: old cidx[%u]didx[%u]",
		       prTxRing->u4RingIdx,
		       prTxRing->u4LastCidx,
		       prTxRing->u4LastDidx);
	}
}

static void mt7935TriggerWfdmaRxCidx(struct GLUE_INFO *prGlueInfo,
				     struct RTMP_RX_RING *prRxRing)
{
}

static void mt79353EnableWfdmaWb(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct RTMP_TX_RING *prTxRing;
	struct RTMP_RX_RING *prRxRing;
	uint32_t u4Idx = 0;

	prHifInfo = &prGlueInfo->rHifInfo;
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	if (!prChipInfo->is_support_wfdma_write_back)
		return;

	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
		prTxRing = &prHifInfo->TxRing[u4Idx];
		if (!prTxRing->pu2EmiDidx)
			continue;

		prTxRing->fgEnEmiDidx = TRUE;
		*prTxRing->pu2EmiDidx = prTxRing->TxDmaIdx;
	}

	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		prRxRing = &prHifInfo->RxRing[u4Idx];
		if (!prRxRing->pu2EmiDidx)
			continue;

		prRxRing->fgEnEmiDidx = TRUE;
		*prRxRing->pu2EmiDidx = prRxRing->RxDmaIdx;
	}

	if (!prChipInfo->is_support_wfdma_cidx_fetch)
		goto enable;

	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
		prTxRing = &prHifInfo->TxRing[u4Idx];
		if (!prTxRing->pu2EmiCidx)
			continue;

		prTxRing->fgEnEmiCidx = TRUE;
		*prTxRing->pu2EmiCidx = prTxRing->TxCpuIdx;
		prTxRing->triggerCidx = mt7935TriggerWfdmaTxCidx;
	}

	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		prRxRing = &prHifInfo->RxRing[u4Idx];
		if (!prRxRing->pu2EmiCidx)
			continue;

		prRxRing->fgEnEmiCidx = TRUE;
		*prRxRing->pu2EmiCidx = prRxRing->RxCpuIdx;
		prRxRing->triggerCidx = mt7935TriggerWfdmaRxCidx;
	}

enable:
	prChipInfo->is_enable_wfdma_write_back = TRUE;
	if (prBusInfo->pdmaSetup)
		prBusInfo->pdmaSetup(prGlueInfo, TRUE, FALSE);
}
#endif /* CFG_MTK_WIFI_WFDMA_WB */

#if defined(_HIF_PCIE) && (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
static void mt7935EnableInterruptViaPcie(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct BUS_INFO *prBusInfo = prChipInfo->bus_info;
	/*
	 * Problem Statement:
	 * Current rx driver own flow is disable wfdma
	 * interrupt, then set driver own.
	 * It may cause Falcon enter sleep after disable
	 * wfdma interrupt and cause read driver own timeout.
	 *
	 * Solution:
	 * Confirmed with DE, correct rx driver own flow
	 * Set driver own and read driver own before disable/enable
	 * wfdma interrupt
	 */

	if (prBusInfo->configWfdmaIntMask) {
		prBusInfo->configWfdmaIntMask(prAdapter->prGlueInfo, FALSE);
		prBusInfo->configWfdmaIntMask(prAdapter->prGlueInfo, TRUE);
	}
	asicConnac3xEnablePlatformIRQ(prAdapter);
}
#endif

static void mt7935EnableInterrupt(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->configWfdmaIntMask)
		prBusInfo->configWfdmaIntMask(prAdapter->prGlueInfo, TRUE);
	asicConnac3xEnablePlatformIRQ(prAdapter);
}

static void mt7935DisableInterrupt(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->configWfdmaIntMask)
		prBusInfo->configWfdmaIntMask(prAdapter->prGlueInfo, FALSE);
	asicConnac3xDisablePlatformIRQ(prAdapter);
}

static void mt7935WpdmaMsiConfig(struct ADAPTER *prAdapter)
{
/*
 * ilog2(WFDMA_AP_MSI 1) = WFDMA_AP_MSI_NUM for CR shitf
 * please do NOT use linux API, which is finally implemented in assembly
 */
#if (WFDMA_AP_MSI_NUM == 8)
#define WFDMA_AP_MSI_SETTING_VAL		3
#else
#define WFDMA_AP_MSI_SETTING_VAL		0
#endif

#if (WFDMA_MD_MSI_NUM == 8)
#define WFDMA_MD_MSI_SETTING_VAL		3
#else
#define WFDMA_MD_MSI_SETTING_VAL		0
#endif

	struct mt66xx_chip_info *prChipInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct pcie_msi_info *prMsiInfo = NULL;
	uint32_t u4Value = 0;

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	if (!prMsiInfo->fgMsiEnabled)
		return;

#if (WFDMA_AP_MSI_NUM == 8)
	/* No need to read int status if msi num is 8 */
	prAdapter->rWifiVar.u4HifIstLoopCount = 1;

	/* enable msi 2~5 auto clear feature */
	u4Value = 0x3C;
	/* enable deassert timer */
	u4Value |= (0x40 <<
	WF_WFDMA_EXT_WRAP_CSR_WFDMA_MSI_CONFIG_msi_deassert_tmr_ticks_SHFT) |
	(1 << WF_WFDMA_EXT_WRAP_CSR_WFDMA_MSI_CONFIG_msi_deassert_tmr_en_SHFT);

	HAL_MCR_WR(prAdapter,
		   WF_WFDMA_EXT_WRAP_CSR_WFDMA_MSI_CONFIG_ADDR,
		   u4Value);
#endif

	/* configure MSI number */
	u4Value = ((WFDMA_AP_MSI_SETTING_VAL <<
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_pcie0_msi_num_SHFT) &
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_pcie0_msi_num_MASK);
#if CFG_MTK_MDDP_SUPPORT
	u4Value |= ((WFDMA_MD_MSI_SETTING_VAL <<
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_pcie0_md_msi_num_SHFT) &
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_pcie0_md_msi_num_MASK);
#endif
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_ADDR,
		u4Value);

	/* Set WFDMA MSI_Ring Mapping */
	u4Value = 0x00660077;
#if CFG_MTK_MDDP_SUPPORT
	u4Value |= 0xAA00BB00;
#endif
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_EXT_WRAP_CSR_MSI_INT_CFG0_ADDR,
		u4Value);

	u4Value = 0x00001100;
#if CFG_MTK_MDDP_SUPPORT
	u4Value |= 0x99880000;
#endif
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_EXT_WRAP_CSR_MSI_INT_CFG1_ADDR,
		u4Value);

	u4Value = 0x0030004F;
#if CFG_MTK_MDDP_SUPPORT
	u4Value |= 0x00005E00;
#endif
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_EXT_WRAP_CSR_MSI_INT_CFG2_ADDR,
		u4Value);

	u4Value = 0x00542200;
#if CFG_MTK_MDDP_SUPPORT
	u4Value |= 0x98000800;
#endif
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_EXT_WRAP_CSR_MSI_INT_CFG3_ADDR,
		u4Value);

#if CFG_MTK_MDDP_SUPPORT
#if (WFDMA_MD_MSI_NUM == 1)
	u4Value = 0x0F00087F;
#else
	u4Value = 0x0000083C;
#endif
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_MD_INT_LUMP_SEL_ADDR,
		u4Value);
#endif
}

static void mt7935ConfigWfdmaRxRingThreshold(
	struct ADAPTER *prAdapter, uint32_t u4Th, u_int8_t fgIsData)
{
	uint32_t u4Addr = 0, u4Val = 0, u4Num = 2;

	/* set rxq th to 1 if tput is high */
	if (u4Th == 2)
		u4Num = 1;

	u4Val = u4Num | (u4Num <<
		 WF_WFDMA_HOST_DMA0_WPDMA_PAUSE_RX_Q_TH10_RX_DMAD_TH1_SHFT);
	if (fgIsData) {
		u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_PAUSE_RX_Q_TH54_ADDR;
		HAL_MCR_WR(prAdapter, u4Addr, u4Val);
		goto exit;
	}

	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_PAUSE_RX_Q_TH10_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_PAUSE_RX_Q_TH76_ADDR;
	     u4Addr += 0x4)
		HAL_MCR_WR(prAdapter, u4Addr, u4Val);

exit:
	DBGLOG(HAL, INFO, "Set WFDMA RxQ[%u] threshold[0x%08x]\n",
	       fgIsData, u4Val);
}

static void mt7935WpdmaDlyInt(struct GLUE_INFO *prGlueInfo)
{
}

static void mt7935WpdmaConfigExt0(struct ADAPTER *prAdapter)
{
#if CFG_MTK_WIFI_WFDMA_WB
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
#endif /* CFG_MTK_WIFI_WFDMA_WB */
	uint32_t u4Addr = 0, u4Val = 0;

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_ADDR;
	/* default settings */
	u4Val =
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_AXI_AWUSER_LOCK_EN_MASK |
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_RX_INFO_WB_EN_MASK |
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_BID_CHECK_BYPASS_EN_MASK |
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_RX_WB_KEEP_RSVD_MASK |
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_TX_DMASHDL_LITE_ENABLE_MASK |
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_MEM_ARB_LOCK_EN_MASK;
	u4Val |= 0x8 <<
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_AXI_AW_OUTSTANDING_NUM_SHFT;
	u4Val |= 0x3 <<
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_MEM_BST_SIZE_SHFT;
	u4Val |= 0x3 <<
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_MAX_PREFETCH_CNT_SHFT;

#if CFG_MTK_WIFI_WFDMA_WB
	/* enable cidx fetch */
	if (prChipInfo->is_enable_wfdma_write_back &&
	    prChipInfo->is_support_wfdma_cidx_fetch)
		u4Val |= WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_CFET_EN_MASK;
#endif /* CFG_MTK_WIFI_WFDMA_WB */

	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

static void mt7935WpdmaConfigExt1(struct ADAPTER *prAdapter)
{
}

static void mt7935WpdmaConfigExt2(struct ADAPTER *prAdapter)
{
}

static void mt7935WfdmaControl(struct ADAPTER *prAdapter, u_int8_t fgEn)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	union WPDMA_GLO_CFG_STRUCT *prGloCfg = &prHifInfo->GloCfg;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct BUS_INFO *prBusInfo = prChipInfo->bus_info;
	uint32_t u4Addr = 0;

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR;

	/* default settings */
	prGloCfg->word =
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_OMIT_TX_INFO_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_CSR_LBK_RX_Q_SEL_EN_MASK |
	WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_CSR_RX_WB_DDONE_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_FIFO_LITTLE_ENDIAN_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_CSR_AXI_BUFRDY_BYP_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_WB_DDONE_MASK;

	/* axi v3 => 3:256 bytes, 2:128 bytes */
	prGloCfg->word |=
		(3 << WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_PDMA_BT_SIZE_SHFT);

	if (prBusInfo->u4DmaMask > 32) {
		prGloCfg->word |=
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_PDMA_ADDR_EXT_EN_MASK;
	}

	if (fgEn) {
		prGloCfg->word |=
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_RX_DMA_EN_MASK;
	}

	HAL_MCR_WR(prAdapter, u4Addr, prGloCfg->word);
	prHifInfo->GloCfg.word = prGloCfg->word;
}

static void mt7935WpdmaConfig(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable, bool fgResetHif)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

	mt7935WfdmaControl(prAdapter, enable);

	if (!enable)
		return;

#if CFG_MTK_WIFI_WFDMA_WB
	mt7935ConfigEmiIntMask(prGlueInfo, TRUE);
#endif /* CFG_MTK_WIFI_WFDMA_WB */

#if defined(_HIF_PCIE)
	mt7935WpdmaMsiConfig(prAdapter);
#endif
	mt7935ConfigWfdmaRxRingThreshold(prAdapter, 0, FALSE);

	mt7935WpdmaConfigExt0(prAdapter);
	mt7935WpdmaConfigExt1(prAdapter);
	mt7935WpdmaConfigExt2(prAdapter);

	mt7935WpdmaDlyInt(prGlueInfo);
}

#if CFG_MTK_WIFI_WFDMA_WB
static void mt7935WfdmaTxRingWbExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *prTxRing,
	uint32_t u4Idx)
{
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct WFDMA_EMI_RING_DIDX *prRingDidx;
	struct WFDMA_EMI_RING_CIDX *prRingCidx;

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prRingDidx = (struct WFDMA_EMI_RING_DIDX *)
		prHifInfo->rRingDidx.AllocVa;
	prRingCidx = (struct WFDMA_EMI_RING_CIDX *)
		prHifInfo->rRingCidx.AllocVa;

	if (!prChipInfo->is_support_wfdma_write_back)
		return;

	if (u4Idx >= WFDMA_TX_RING_MAX_NUM) {
		DBGLOG(HAL, ERROR, "idx[%u] > max number\n", u4Idx);
		return;
	}

	prTxRing->pu2EmiDidx = &prRingDidx->tx_ring[u4Idx];
	prTxRing->fgEnEmiDidx = TRUE;
	*prTxRing->pu2EmiDidx = 0;

	if (!prChipInfo->is_support_wfdma_cidx_fetch)
		return;

	prTxRing->pu2EmiCidx = &prRingCidx->tx_ring[u4Idx];
	*prTxRing->pu2EmiCidx = 0;
}

static void mt7935WfdmaRxRingWbExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *prRxRing,
	uint32_t u4Idx)
{
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct WFDMA_EMI_RING_DIDX *prRingDidx;
	struct WFDMA_EMI_RING_CIDX *prRingCidx;

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prRingDidx = (struct WFDMA_EMI_RING_DIDX *)
		prHifInfo->rRingDidx.AllocVa;
	prRingCidx = (struct WFDMA_EMI_RING_CIDX *)
		prHifInfo->rRingCidx.AllocVa;

	if (!prChipInfo->is_support_wfdma_write_back)
		return;

	if (u4Idx >= WFDMA_RX_RING_MAX_NUM) {
		DBGLOG(HAL, ERROR, "idx[%u] > max number\n", u4Idx);
		return;
	}

	prRxRing->pu2EmiDidx = &prRingDidx->rx_ring[u4Idx];
	*prRxRing->pu2EmiDidx = 0;

	if (!prChipInfo->is_support_wfdma_cidx_fetch)
		return;

	prRxRing->pu2EmiCidx = &prRingCidx->rx_ring[u4Idx];
	*prRxRing->pu2EmiCidx = prRxRing->u4RingSize - 1;
}
#endif /* CFG_MTK_WIFI_WFDMA_WB */

static void mt7935WfdmaTxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *prTxRing,
	u_int32_t index)
{
	struct BUS_INFO *prBusInfo;
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4Offset = 0, u4RingIdx = 0;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	switch (index) {
	case TX_RING_DATA0:
		u4RingIdx = prBusInfo->tx_ring0_data_idx;
		break;
	case TX_RING_DATA1:
		u4RingIdx = prBusInfo->tx_ring1_data_idx;
		break;
	case TX_RING_DATA2:
		u4RingIdx = prBusInfo->tx_ring2_data_idx;
		break;
	case TX_RING_DATA3:
		u4RingIdx = prBusInfo->tx_ring3_data_idx;
		break;
	case TX_RING_DATA_PRIO:
		u4RingIdx = prBusInfo->tx_prio_data_idx;
		break;
	case TX_RING_DATA_ALTX:
		u4RingIdx = prBusInfo->tx_altx_data_idx;
		break;
	case TX_RING_CMD:
		u4RingIdx = prBusInfo->tx_ring_cmd_idx;
		break;
	case TX_RING_FWDL:
		u4RingIdx = prBusInfo->tx_ring_fwdl_idx;
		break;
	default:
		u4RingIdx = index;
		break;

	}
	u4Offset = u4RingIdx * 4;

	prTxRing->hw_desc_base_ext =
		prBusInfo->host_tx_ring_ext_ctrl_base + u4Offset;
	HAL_MCR_WR(prAdapter, prTxRing->hw_desc_base_ext,
		   CONNAC3X_TX_RING_DISP_MAX_CNT);

	asicConnac3xWfdmaTxRingBasePtrExtCtrl(
		prGlueInfo, prTxRing, index, prTxRing->u4RingSize);

#if CFG_MTK_WIFI_WFDMA_WB
	mt7935WfdmaTxRingWbExtCtrl(prGlueInfo, prTxRing, u4RingIdx);
#endif /* CFG_MTK_WIFI_WFDMA_WB */
}

static void mt7935WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *prRxRing,
	u_int32_t index)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	uint32_t u4Offset = 0, u4RingIdx = 0;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	u4RingIdx = mt7935RxRingSwIdx2HwIdx(index);
	if (u4RingIdx >= RX_RING_MAX)
		return;

	u4Offset = u4RingIdx * 4;

	prRxRing->hw_desc_base_ext =
		prBusInfo->host_rx_ring_ext_ctrl_base + u4Offset;
	HAL_MCR_WR(prAdapter, prRxRing->hw_desc_base_ext,
		   CONNAC3X_RX_RING_DISP_MAX_CNT);

	asicConnac3xWfdmaRxRingBasePtrExtCtrl(
		prGlueInfo, prRxRing, index, prRxRing->u4RingSize);

#if CFG_MTK_WIFI_WFDMA_WB
	mt7935WfdmaRxRingWbExtCtrl(prGlueInfo, prRxRing, u4RingIdx);
#endif /* CFG_MTK_WIFI_WFDMA_WB */
}

static void mt7935InitPcieInt(struct GLUE_INFO *prGlueInfo)
{
}

static void mt7935ShowPcieDebugInfo(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4Addr, u4Val = 0;

	if (!in_interrupt()) {
		u4Addr = 0x112F0184;
		wf_ioremap_read(u4Addr, &u4Val);
		DBGLOG(HAL, INFO, "PCIE CR [0x%08x]=[0x%08x]", u4Addr, u4Val);
		for (u4Addr = 0x112F0C04; u4Addr <= 0x112F0C1C; u4Addr += 4) {
			wf_ioremap_read(u4Addr, &u4Val);
			DBGLOG(HAL, INFO, "PCIE CR [0x%08x]=[0x%08x]",
			       u4Addr, u4Val);
		}
	}
}

#if (CFG_MTK_WIFI_SUPPORT_IPC == 1)
static void mt7935SetupWiFiMcuEmiAddr(struct ADAPTER *prAdapter)
{
	phys_addr_t base = emi_mem_get_phy_base(prAdapter->chip_info);
	uint32_t u4Addr = 0;
	uint32_t size = emi_mem_get_size(prAdapter->chip_info);
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct WLAN_IPC_INFO *prIPCInfo = NULL;

	prChipInfo = prAdapter->chip_info;
	if (prChipInfo == NULL)
		return;

	prIPCInfo = prChipInfo->ipc_info;
	if (prIPCInfo == NULL || !base)
		return;

	DBGLOG(HAL, INFO, "base: 0x%llx, size: 0x%x\n", base, size);

	/* Update EMI's pa and size of WFMCU to conn von sysram */
	if (prIPCInfo->ipcAccessConnVonSysRam) {
		u4Addr = base & 0xffffffff;
		prIPCInfo->ipcAccessConnVonSysRam(prAdapter->prGlueInfo,
			WLAN_IPC_WRITE,
			IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(lo_host_emi_addr),
			&u4Addr,
			sizeof(u4Addr));
		u4Addr = (base >> 32) & 0xffffffff;
		prIPCInfo->ipcAccessConnVonSysRam(prAdapter->prGlueInfo,
			WLAN_IPC_WRITE,
			IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(hi_host_emi_addr),
			&u4Addr,
			sizeof(u4Addr));
		prIPCInfo->ipcAccessConnVonSysRam(prAdapter->prGlueInfo,
			WLAN_IPC_WRITE,
			IPC_GET_CONN_VON_SYSRAM_ADDR_OFFSET(wifi_host_emi_size),
			&size,
			sizeof(size));
	}
}
#endif /* CFG_MTK_WIFI_SUPPORT_IPC */

static void mt7935SetupMcuEmiAddr(struct ADAPTER *prAdapter)
{
	phys_addr_t base = emi_mem_get_phy_base(prAdapter->chip_info);
	uint32_t size = emi_mem_get_size(prAdapter->chip_info);

	if (!base)
		return;

	DBGLOG(HAL, INFO, "base: 0x%llx, size: 0x%x\n", base, size);

	HAL_MCR_WR(prAdapter,
		   CONNAC3X_CONN_CFG_ON_CONN_ON_EMI_ADDR,
		   ((uint32_t)base >> 16));

	HAL_MCR_WR(prAdapter,
		   MT7935_EMI_SIZE_ADDR,
		   size);
}

static u_int8_t mt7935_get_sw_interrupt_status(struct ADAPTER *prAdapter,
	uint32_t *pu4Status)
{
	*pu4Status = 0;
	return TRUE;
}

u_int8_t mt7935_is_ap2conn_off_readable(struct ADAPTER *ad)
{
#define MAX_POLLING_COUNT		4

	uint32_t value = 0, retry = 0;

	while (TRUE) {
		if (retry >= MAX_POLLING_COUNT) {
			DBGLOG(HAL, ERROR,
				"Conninfra off bus clk: 0x%08x\n",
				value);
			return FALSE;
		}

		HAL_MCR_WR(ad,
			   CONN_DBG_CTL_CONN_INFRA_BUS_CLK_DETECT_ADDR,
			   BIT(0));
		HAL_RMCR_RD(PLAT_DBG, ad,
			   CONN_DBG_CTL_CONN_INFRA_BUS_CLK_DETECT_ADDR,
			   &value);
		if ((value & BIT(1)) && (value & BIT(3)))
			break;

		retry++;
		kalMdelay(1);
	}

	HAL_RMCR_RD(PLAT_DBG, ad,
		   CONN_CFG_IP_VERSION_IP_VERSION_ADDR,
		   &value);
	if (value != MT7935_CONNINFRA_VERSION_ID) {
		DBGLOG(HAL, ERROR,
			"Conninfra ver id: 0x%08x\n",
			value);
		return FALSE;
	}

	HAL_RMCR_RD(PLAT_DBG, ad,
		   CONN_DBG_CTL_CONN_INFRA_BUS_DBG_CR_00_ADDR,
		   &value);
	if ((value & BITS(0, 9)) == 0x3FF)
		DBGLOG(HAL, ERROR,
			"Conninfra bus hang irq status: 0x%08x\n",
			value);

	return TRUE;
}

u_int8_t mt7935_is_conn2wf_readable(struct ADAPTER *ad)
{
	uint32_t value = 0;

	HAL_RMCR_RD(PLAT_DBG, ad,
		   CONN_BUS_CR_ADDR_CONN2SUBSYS_0_AHB_GALS_DBG_ADDR,
		   &value);
	if ((value & BIT(26)) != 0x0) {
		DBGLOG(HAL, ERROR,
			"conn2wf sleep protect: 0x%08x\n",
			value);
		return FALSE;
	}

	HAL_RMCR_RD(PLAT_DBG, ad,
		   WF_TOP_CFG_IP_VERSION_ADDR,
		   &value);
	if (value != MT7935_WF_VERSION_ID) {
		DBGLOG(HAL, ERROR,
			"WF ver id: 0x%08x\n",
			value);
		return FALSE;
	}

	HAL_RMCR_RD(PLAT_DBG, ad,
		   CONN_DBG_CTL_WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_BUS_TIMEOUT_IRQ_ADDR,
		   &value);
	if ((value & BIT(0)) != 0x0) {
		DBGLOG(HAL, WARN,
			"WF mcusys bus hang irq status: 0x%08x\n",
			value);
		HAL_RMCR_RD(PLAT_DBG, ad,
			   CONN_DBG_CTL_CONN_INFRA_BUS_DBG_CR_00_ADDR,
			   &value);
		if (value == 0x100)
			DBGLOG(HAL, INFO,
				"Skip conn_infra_vdnr timeout irq.\n");
		else
			return FALSE;
	}

	return TRUE;
}

#if (CFG_MTK_ANDROID_WMT == 0)
static uint32_t mt7935_mcu_reset(struct ADAPTER *ad)
{
	uint32_t u4Value = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO, "mt7935_mcu_reset..\n");

	HAL_RMCR_RD(RESET_READ, ad,
		CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		&u4Value);
	u4Value &= ~CB_INFRA_RGU_WF_SUBSYS_RST_WF_SUBSYS_RST_MASK;
	u4Value |= (0x1 << CB_INFRA_RGU_WF_SUBSYS_RST_WF_SUBSYS_RST_SHFT);
	HAL_MCR_WR(ad,
		CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		u4Value);

	kalMdelay(1);

	HAL_RMCR_RD(RESET_READ, ad,
		CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		&u4Value);
	u4Value &= ~CB_INFRA_RGU_WF_SUBSYS_RST_WF_SUBSYS_RST_MASK;
	u4Value |= (0x0 << CB_INFRA_RGU_WF_SUBSYS_RST_WF_SUBSYS_RST_SHFT);
	HAL_MCR_WR(ad,
		CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		u4Value);

	HAL_RMCR_RD(RESET_READ, ad,
		CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M0_STA_REP_1_ADDR,
		&u4Value);
	DBGLOG(INIT, INFO, "0x%08x=0x%08x.\n",
		CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M0_STA_REP_1_ADDR,
		u4Value);
	if ((u4Value &
	     CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M0_STA_REP_1_CONN_SEMA00_OWN_BY_M0_STA_REP_MASK) != 0x0)
		DBGLOG(INIT, ERROR, "L0.5 reset failed.\n");

	return rStatus;
}
#endif

static uint32_t mt7935_mcu_init(struct ADAPTER *ad)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS, u4Val = 0;

#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
	struct mt66xx_chip_info *prChipInfo = NULL;
#endif /* CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI */

	if (!ad) {
		DBGLOG(INIT, ERROR, "NULL ADAPTER.\n");
		rStatus = WLAN_STATUS_FAILURE;
		goto exit;
	}
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
	prChipInfo = ad->chip_info;
	if (prChipInfo == NULL) {
		DBGLOG(INIT, ERROR, "NULL prChipInfo.\n");
		rStatus = WLAN_STATUS_FAILURE;
		goto exit;
	}
#endif /* CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI */
	switch (MT7935_WIFI_PWR_ON_OFF_MODE) {
	case 0:
		break;
	/* FLR */
	case 1:
		glReadPcieCfgSpace(0x4BC, &u4Val);
		u4Val |= BIT(28);
		glWritePcieCfgSpace(0x4BC, u4Val);
		break;
	/* Config Space */
	case 2:
		glReadPcieCfgSpace(0x4BC, &u4Val);
		u4Val |= BIT(28);
		glWritePcieCfgSpace(0x4BC, u4Val);
		break;
	default:
		DBGLOG(INIT, ERROR, "%d not supported.\n",
			MT7935_WIFI_PWR_ON_OFF_MODE);
		break;
	}
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
	kalMemZero(prChipInfo->sw_sync_emi_info,
		sizeof(struct sw_sync_emi_info) * SW_SYNC_TAG_NUM);
#endif /* CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI */
#if (CFG_ENABLE_IPC_FW_DOWNLOAD == 1)
	rStatus = mt7935IPCFirmwareDownload(ad);
#endif /* CFG_ENABLE_IPC_FW_DOWNLOAD */
exit:
	return rStatus;
}

static void mt7935_mcu_deinit(struct ADAPTER *ad)
{
#define MAX_WAIT_COREDUMP_COUNT 10

	int retry = 0;
	uint32_t u4Val = 0;

	while (is_wifi_coredump_processing()) {
		if (retry >= MAX_WAIT_COREDUMP_COUNT) {
			DBGLOG(INIT, WARN,
				"Coredump spend long time, retry = %d\n",
				retry);
		}
		kalMsleep(100);
		retry++;
	}

	pci_save_state(ad->prGlueInfo->rHifInfo.pdev);
	switch (MT7935_WIFI_PWR_ON_OFF_MODE) {
	case 0:
		break;
	/* FLR */
	case 1:
		glReadPcieCfgSpace(0x88, &u4Val);
		u4Val |= BIT(15);
		glWritePcieCfgSpace(0x88, u4Val);
		break;
	/* Config Space */
	case 2:
		glReadPcieCfgSpace(0x4BC, &u4Val);
		u4Val |= BIT(29);
		glWritePcieCfgSpace(0x4BC, u4Val);
		break;
	default:
		DBGLOG(INIT, ERROR, "%d not supported.\n",
			MT7935_WIFI_PWR_ON_OFF_MODE);
	}
	pci_restore_state(ad->prGlueInfo->rHifInfo.pdev);
	wifi_coredump_set_enable(FALSE);
}

static int32_t mt7935_trigger_fw_assert(struct ADAPTER *prAdapter)
{
	int32_t ret = 0;

#if CFG_WMT_RESET_API_SUPPORT
	ret = reset_wait_for_trigger_completion();
#endif

	return ret;
}

static int mt7935_CheckBusHang(void *priv, uint8_t rst_enable)
{
	struct ADAPTER *ad = priv;
	u_int8_t readable = FALSE;

	if (fgIsBusAccessFailed) {
		readable = FALSE;
		goto exit;
	}

	if (mt7935_is_ap2conn_off_readable(ad) &&
	    mt7935_is_conn2wf_readable(ad))
		readable = TRUE;
	else
		readable = FALSE;

exit:
	return readable ? 0 : 1;
}

static uint32_t mt7935_wlanDownloadPatch(struct ADAPTER *prAdapter)
{
	uint32_t status  = wlanDownloadPatch(prAdapter);

	if (status == WLAN_STATUS_SUCCESS)
		wifi_coredump_set_enable(TRUE);

	return status;
}
#endif /* _HIF_PCIE */

static uint32_t mt7935GetFlavorVer(uint8_t *flavor)
{
	uint32_t ret = WLAN_STATUS_FAILURE;
	uint32_t u4StrLen = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN] = {0};

	if (kalGetFwFlavor(&aucFlavor[0]) == 1) {
		u4StrLen = kalStrnLen(aucFlavor,
						CFG_FW_FLAVOR_MAX_LEN);
		if (u4StrLen == 1) {
			kalScnprintf(flavor,
					CFG_FW_FLAVOR_MAX_LEN,
					"%u%s", CFG_WIFI_IP_SET, aucFlavor);
		} else {
			kalScnprintf(flavor,
					CFG_FW_FLAVOR_MAX_LEN,
					"%s", aucFlavor);
		}
		ret = WLAN_STATUS_SUCCESS;
	} else if (kalScnprintf(flavor,
					CFG_FW_FLAVOR_MAX_LEN,
					"1") > 0) {
		ret = WLAN_STATUS_SUCCESS;
	} else {
		ret = WLAN_STATUS_FAILURE;
	}

	return ret;
}

static void mt7935WiFiNappingCtrl(
	struct GLUE_INFO *prGlueInfo, u_int8_t fgEn)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint32_t u4value = 0;
	u_int8_t fgNappingEn = FALSE;

	if (!prGlueInfo->prAdapter) {
		DBGLOG(HAL, ERROR, "adapter is null\n");
		return;
	}

	prChipInfo = prGlueInfo->prAdapter->chip_info;

	if (prChipInfo->fgWifiNappingForceDisable)
		fgNappingEn = FALSE;
	else
		fgNappingEn = fgEn;

	/* return if setting no chang */
	if (prChipInfo->fgWifiNappingEn == fgNappingEn)
		return;

	prChipInfo->fgWifiNappingEn = fgNappingEn;

	/*
	 * [0]: 1: set wf bus active from wf napping sleep by driver.
	 *      0: set wf bus goes back to napping sleep by driver
	 */
	if (fgNappingEn)
		u4value = CONN_AON_WF_NAPPING_ENABLE;
	else
		u4value = CONN_AON_WF_NAPPING_DISABLE;

	DBGLOG(INIT, TRACE,
		"fgEn[%u] fgNappingEn[%u], WrAddr[0x%08x]=[0x%08x]\n",
		fgEn, fgNappingEn,
		CONN_HOST_CSR_TOP_ADDR_CR_CONN_AON_TOP_RESERVE_ADDR,
		u4value);
	HAL_MCR_WR(prGlueInfo->prAdapter,
		   CONN_HOST_CSR_TOP_ADDR_CR_CONN_AON_TOP_RESERVE_ADDR,
		   u4value);
}

static void mt7935LowPowerOwnInit(struct ADAPTER *prAdapter)
{
	HAL_MCR_WR(prAdapter,
		   CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_ADDR,
		   BIT(0));
}

static void mt7935LowPowerOwnRead(struct ADAPTER *prAdapter,
				  u_int8_t *pfgResult)
{
	uint32_t u4RegValue = 0;

	HAL_RMCR_RD(LPOWN_READ, prAdapter,
		    CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		    &u4RegValue);

	*pfgResult = (u4RegValue &
		PCIE_LPCR_AP_HOST_OWNER_STATE_SYNC) == 0 ?
		TRUE : FALSE;
}

static void mt7935LowPowerOwnSet(struct ADAPTER *prAdapter,
				 u_int8_t *pfgResult)
{
	uint32_t u4RegValue = 0;

	HAL_MCR_WR(prAdapter,
		   CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		   PCIE_LPCR_HOST_SET_OWN);

	HAL_RMCR_RD(LPOWN_READ, prAdapter,
		    CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		    &u4RegValue);

	*pfgResult = (u4RegValue &
		PCIE_LPCR_AP_HOST_OWNER_STATE_SYNC) == 0x4;
}

static void mt7935LowPowerOwnClear(struct ADAPTER *prAdapter,
				   u_int8_t *pfgResult)
{
	uint32_t u4RegValue = 0;

	HAL_MCR_WR(prAdapter,
		   CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		   PCIE_LPCR_HOST_CLR_OWN);

	HAL_RMCR_RD(LPOWN_READ, prAdapter,
		    CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		    &u4RegValue);

	*pfgResult = (u4RegValue &
		PCIE_LPCR_AP_HOST_OWNER_STATE_SYNC) == 0;
}
#endif  /* MT7935 */
