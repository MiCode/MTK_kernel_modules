// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

/*! \file   mt7926.c
 *    \brief  Internal driver stack will export
 *    the required procedures here for GLUE Layer.
 *
 *    This file contains all routines which are exported
 *    from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
 */

#ifdef MT7926

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "coda/mt7926/wf_wfdma_host_dma0.h"
#include "coda/mt7926/wf_cr_sw_def.h"
#include "precomp.h"
#include "mt7961.h"
#include "mt7902.h"
#include "mt7926.h"
#include "hal_dmashdl_mt7902.h"
#include "hal_wfsys_reset_mt7961.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define CONN_MCU_CONFG_BASE 0x88000000
#define CONN_MCU_CONFG_COM_REG0_ADDR (CONN_MCU_CONFG_BASE + 0x200)

#define PATCH_SEMAPHORE_COMM_REG 0
#define PATCH_SEMAPHORE_COMM_REG_PATCH_DONE 1 /* bit0 is for patch. */

#define RX_DATA_RING_BASE_IDX 0

#define FLAVOR_VERSION_DEFAULT (1)

#define MT7926_CMD_TX_RING_IDX 15

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define MT7926_BUS2CHIP_MEM_MAP_TBLE_SIZE    \
	(sizeof(mt7926_bus2chip_cr_mapping) /	\
	 sizeof(mt7926_bus2chip_cr_mapping[0]))

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

struct ECO_INFO mt7926_eco_table[] = {
	/* HW version,  ROM version, Factory version, EcoVer */
	{0x00, 0x00, 0xA, 0x1},	/* E1 */
	{0x00, 0x00, 0x0, 0x0}	/* End of table */
};

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
struct PCIE_CHIP_CR_MAPPING mt7926_bus2chip_cr_mapping[] = {
	/* chip addr, bus addr, range */
	{0x54000000, 0x02000, 0x1000},  /* WFDMA PCIE0 MCU DMA0 */
	{0x55000000, 0x03000, 0x1000},  /* WFDMA PCIE0 MCU DMA1 */
	{0x56000000, 0x04000, 0x1000},  /* WFDMA reserved */
	{0x57000000, 0x05000, 0x1000},  /* WFDMA MCU wrap CR */
	{0x58000000, 0x06000, 0x1000},  /* WFDMA PCIE1 MCU DMA0 (MEM_DMA) */
	{0x59000000, 0x07000, 0x1000},  /* WFDMA PCIE1 MCU DMA1 */
	{0x820c0000, 0x08000, 0x4000},  /* WF_UMAC_TOP (PLE) */
	{0x820c8000, 0x0c000, 0x2000},  /* WF_UMAC_TOP (PSE) */
	{0x820cc000, 0x0e000, 0x2000},  /* WF_UMAC_TOP (PP) */
	{0x74030000, 0x10000, 0x10000}, /* PCIE_MAC_IREG */
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
	{0x88000000, 0x40000, 0x10000}, /* CONN_MCU_CONFG_LS */
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
	{0x820cc000, 0xa5000, 0x2000},  /* WF_LMAC_TOP BN1 (WF_MUCOP) */
	{0x820c4000, 0xa8000, 0x4000},  /* WF_LMAC_TOP BN1 (WF_MUCOP) */
	{0x820b0000, 0xae000, 0x1000},  /* [APB2] WFSYS_ON */
	{0x80020000, 0xb0000, 0x10000}, /* WF_TOP_MISC_OFF */
	{0x81020000, 0xc0000, 0x10000}, /* WF_TOP_MISC_ON */
	{0x7c020000, 0xd0000, 0x10000}, /* CONN_INFRA, wfdma */
	{0x7c060000, 0xe0000, 0x10000}, /* CONN_INFRA, conn_host_csr_top */
	{0x7c000000, 0xf0000, 0x10000}, /* CONN_INFRA */
};

#endif /*_HIF_PCIE || _HIF_AXI */

uint32_t mt7926GetDongleType(struct ADAPTER *prAdapter)
{
	return MT7926_CHIP_ID;
}

struct BUS_INFO mt7926_bus_info = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.top_cfg_base = MT7926_TOP_CFG_BASE,

	/* host_dma0 for TXP */
	.host_dma0_base = WF_WFDMA_HOST_DMA0_BASE,
	.host_int_status_addr = WF_WFDMA_HOST_DMA0_HOST_INT_STA_ADDR,

	.host_int_txdone_bits =
		(CONNAC2X_WFDMA_TX_DONE_INT0 | CONNAC2X_WFDMA_TX_DONE_INT1 |
		CONNAC2X_WFDMA_TX_DONE_INT2 | CONNAC2X_WFDMA_TX_DONE_INT3 |
		CONNAC2X_WFDMA_TX_DONE_INT4 | CONNAC2X_WFDMA_TX_DONE_INT5 |
		CONNAC2X_WFDMA_TX_DONE_INT6 | CONNAC2X_WFDMA_TX_DONE_INT16 |
		/* MT7926 CMD TX ring id is 15, but its interrupt id is 17 */
		CONNAC2X_WFDMA_TX_DONE_INT17),
	.host_int_rxdone_bits =
		(CONNAC2X_WFDMA_RX_DONE_INT0 | CONNAC2X_WFDMA_RX_DONE_INT1 |
		 CONNAC2X_WFDMA_RX_DONE_INT2 | CONNAC2X_WFDMA_RX_DONE_INT3),

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

	.bus2chip = mt7926_bus2chip_cr_mapping,
	.bus2chip_tbl_size = MT7926_BUS2CHIP_MEM_MAP_TBLE_SIZE,
	.max_static_map_addr = 0x00100000,

	.tx_ring_fwdl_idx = CONNAC2X_FWDL_TX_RING_IDX,
	.tx_ring_cmd_idx = MT7926_CMD_TX_RING_IDX,
	.tx_ring0_data_idx = 0,
	.tx_ring1_data_idx = 1,
	.fw_own_clear_addr = CONNAC2X_BN0_IRQ_STAT_ADDR,
	.fw_own_clear_bit = PCIE_LPCR_FW_CLR_OWN,
	.fgCheckDriverOwnInt = FALSE,
	.u4DmaMask = 32,
	.pdmaSetup = asicConnac2xWpdmaConfig,
	.pdmaStop = asicConnac2xWfdmaStop,
	.pdmaPollingIdle = asicConnac2xWfdmaPollingAllIdle,
	.enableInterrupt = mt7902EnableInterrupt,
	.disableInterrupt = mt7902DisableInterrupt,
	.processTxInterrupt = mt7902Connac2xProcessTxInterrupt,
	.processRxInterrupt = mt7902Connac2xProcessRxInterrupt,
	.tx_ring_ext_ctrl = mt7902WfdmaTxRingExtCtrl,
	.rx_ring_ext_ctrl = mt7902WfdmaRxRingExtCtrl,
	/* MT7926 event & rx freed done evt use the same ring.
	 * Dut to the sw rfb usage, the buf size is set as CFG_RX_MAX_PKT_SIZE.
	 */
	.rx_evt_ring_buf_size = CFG_RX_MAX_PKT_SIZE,
	/* null wfdmaManualPrefetch if want to disable manual mode */
	.wfdmaManualPrefetch = mt7902Connac2xWfdmaManualPrefetch,
	.lowPowerOwnRead = asicConnac2xLowPowerOwnRead,
	.lowPowerOwnSet = asicConnac2xLowPowerOwnSet,
	.lowPowerOwnClear = asicConnac2xLowPowerOwnClear,
	.wakeUpWiFi = asicWakeUpWiFi,
	.getSoftwareInterrupt = asicConnac2xGetSoftwareInterrupt,
	.processSoftwareInterrupt = asicConnac2xProcessSoftwareInterrupt,
	.softwareInterruptMcu = asicConnac2xSoftwareInterruptMcu,
	.hifRst = asicConnac2xHifRst,
	.initPcieInt = wlanBuzzardInitPcieInt,
	.devReadIntStatus = mt7902ReadIntStatus,
	.DmaShdlInit = mt7902DmashdlInit,
	.DmaShdlReInit = mt7902DmashdlReInit,
	.setRxRingHwAddr = mt7902SetRxRingHwAddr,
	.wfdmaAllocRxRing = mt7902LiteWfdmaAllocRxRing,
#if CFG_SUPPORT_PCIE_ASPM_IMPROVE
	.configPcieASPM = mt7961ConfigPcieASPM,
	.setCTSbyRate = mt7961SetCTSbyRate,
#endif
#if (CFG_COALESCING_INTERRUPT == 1)
#if defined(_HIF_PCIE)
	.setWfdmaCoalescingInt = mt7961setWfdmaCoalescingInt,
#else
	.setWfdmaCoalescingInt = NULL,
#endif
#endif
#endif /*_HIF_PCIE || _HIF_AXI */

#if defined(_HIF_USB)
	.u4UdmaWlCfg_0_Addr = CONNAC2X_UDMA_WLCFG_0,
	.u4UdmaWlCfg_1_Addr = CONNAC2X_UDMA_WLCFG_1,
	.u4UdmaWlCfg_0 =
	    (CONNAC2X_UDMA_WLCFG_0_WL_TX_EN(1) |
	     CONNAC2X_UDMA_WLCFG_0_WL_RX_EN(1) |
	     CONNAC2X_UDMA_WLCFG_0_WL_RX_MPSZ_PAD0(1) |
	     CONNAC2X_UDMA_WLCFG_0_TICK_1US_EN(1)),
	.u4UdmaTxQsel = CONNAC2X_UDMA_TX_QSEL,
	.u4UdmaConnInfraStatusSelAddr = MT7926_UDMA_CONN_INFRA_STATUS_SEL,
	.u4UdmaConnInfraStatusSelVal = MT7926_UDMA_CONN_INFRA_STATUS_VAL,
	.u4UdmaConnInfraStatusAddr = MT7926_UDMA_CONN_INFRA_STATUS,
	.u4device_vender_request_in = DEVICE_VENDOR_REQUEST_IN_CONNAC2,
	.u4device_vender_request_out = DEVICE_VENDOR_REQUEST_OUT_CONNAC2,
	.fgIsSupportWdtEp = TRUE,
	.asicUsbEventEpDetected = mt7902Connac2xUsbEventEpDetected,
	.asicUsbRxByteCount = mt7902Connac2xUsbRxByteCount,
	.u4SuspendVer = SUSPEND_V2,
	.asicUsbSuspend = NULL, /*asicUsbSuspend*/
	.asicUsbResume = asicConnac2xUsbResume,
	.DmaShdlInit = mt7902DmashdlInit,
	.DmaShdlReInit = mt7902DmashdlReInit,
	.updateTxRingMaxQuota = mt7902UpdateDmashdlQuota,
	.asicUdmaRxFlush = asicConnac2xUdmaRxFlush,
#if CFG_CHIP_RESET_SUPPORT
	.asicUsbEpctlRstOpt = mt7961HalUsbEpctlRstOpt,
#endif
#endif /* _HIF_USB */
#if defined(_HIF_SDIO)
	.halTxGetFreeResource = halTxGetFreeResource_v1,
	.halTxReturnFreeResource = halTxReturnFreeResource_v1,
	.halRestoreTxResource = halRestoreTxResource_v1,
	.halUpdateTxDonePendingCount = halUpdateTxDonePendingCount_v1,
#endif /* _HIF_SDIO */
};

#if CFG_ENABLE_FW_DOWNLOAD
struct FWDL_OPS_T mt7926_fw_dl_ops = {
	.constructFirmwarePrio = mt7902ConstructFirmwarePrio,
	.constructPatchName = mt7902ConstructPatchName,
	.downloadPatch = wlanDownloadPatch,
	.downloadFirmware = wlanConnacFormatDownload,
	.getFwInfo = wlanGetConnacFwInfo,
	.getFwDlInfo = asicGetFwDlInfo,
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
	.constructBtPatchName = mt7902ConstructBtPatchName,
	.downloadBtPatch = mt7902DownloadBtPatch,
#endif
#if CFG_SUPPORT_WIFI_DL_ZB_PATCH
	.constructZbPatchName = mt7902ConstructZbPatchName,
	.downloadZbPatch = mt7902DownloadZbPatch,
#endif
};
#endif /* CFG_ENABLE_FW_DOWNLOAD */

struct TX_DESC_OPS_T mt7926TxDescOps = {
	.fillNicAppend = fillNicTxDescAppend,
	.fillHifAppend = fillTxDescAppendByHostV2,
	.fillTxByteCount = fillConnac2xTxDescTxByteCount,
};

struct RX_DESC_OPS_T mt7926RxDescOps = {};

struct CHIP_DBG_OPS mt7926DebugOps = {
	.showPdmaInfo = mt7902_show_wfdma_info,
	.showPseInfo = mt7961_show_pse_info,
	.showPleInfo = mt7902_show_ple_info,
	.showTxdInfo = connac2x_show_txd_Info,
	.showWtblInfo = connac2x_show_wtbl_info,
	.showUmacFwtblInfo = connac2x_show_umac_wtbl_info,
	.showCsrInfo = NULL,
	.showDmaschInfo = NULL,
	.showHifInfo = NULL,
	.printHifDbgInfo = NULL,
	.show_rx_rate_info = connac2x_show_rx_rate_info,
	.show_rx_rssi_info = connac2x_show_rx_rssi_info,
	.show_stat_info = connac2x_show_stat_info,
#if (CFG_SUPPORT_LINK_QUALITY_MONITOR == 1)
	.get_rx_rate_info = connac2x_get_rx_rate_info,
#endif
#if defined(_HIF_SDIO)
	.show_mcu_debug_info = mt7902_sdio_show_mcu_debug_info,
#elif defined(_HIF_USB)
	.show_mcu_debug_info = usb_show_mcu_debug_info,
#elif defined(_HIF_PCIE)
	.show_mcu_debug_info = pcie_show_mcu_debug_info,
#endif
	.show_conninfra_debug_info = NULL,
#if (CFG_SUPPORT_DEBUG_SOP == 1)
	.show_debug_sop_info = mt7902_show_debug_sop_info,
#endif
};

#if CFG_SUPPORT_QA_TOOL
struct ATE_OPS_T mt7926_AteOps = {
	/*ICapStart phase out , wlan_service instead*/
	.setICapStart = connacSetICapStart,
	/*ICapStatus phase out , wlan_service instead*/
	.getICapStatus = connacGetICapStatus,
	/*CapIQData phase out , wlan_service instead*/
	.getICapIQData = connacGetICapIQData,
	.getRbistDataDumpEvent = nicExtEventICapIQData,
	.icapRiseVcoreClockRate = mt7902_icapRiseVcoreClockRate,
	.icapDownVcoreClockRate = mt7902_icapDownVcoreClockRate,
	.u4EnBitWidth = 4, /*no translate, keep 128 bit*/
	.u4Architech = 0,  /*0:on-chip*/
	.u4PhyIdx = 0,
	.u4EmiStartAddress = 0,
	.u4EmiEndAddress = 0,
	.u4EmiMsbAddress = 0,
};
#endif

/* Litien code refine to support multi chip */
struct mt66xx_chip_info mt66xx_chip_info_mt7926 = {
	.bus_info = &mt7926_bus_info,
#if CFG_ENABLE_FW_DOWNLOAD
	.fw_dl_ops = &mt7926_fw_dl_ops,
#endif /* CFG_ENABLE_FW_DOWNLOAD */
#if CFG_SUPPORT_QA_TOOL
	.prAteOps = &mt7926_AteOps,
#endif
	.prTxDescOps = &mt7926TxDescOps,
	.prRxDescOps = &mt7926RxDescOps,
	.prDebugOps = &mt7926DebugOps,
	.chip_id = MT7926_CHIP_ID,
	.should_verify_chip_id = FALSE,
	.sw_sync0 = MT7926_SW_SYNC0,
	.sw_ready_bits = WIFI_FUNC_NO_CR4_READY_BITS,
	.sw_ready_bit_offset = MT7926_SW_SYNC0_RDY_OFFSET,
	.patch_addr = MT7926_PATCH_START_ADDR,
	.is_support_cr4 = FALSE,
	.is_support_wacpu = FALSE,
	.txd_append_size = MT7926_TX_DESC_APPEND_LENGTH,
	.rxd_size = MT7926_RX_DESC_LENGTH,
	.pse_header_length = CONNAC2X_NIC_TX_PSE_HEADER_LENGTH,
	.init_event_size = CONNAC2X_RX_INIT_EVENT_LENGTH,
	.event_hdr_size = CONNAC2X_RX_EVENT_HDR_LENGTH,
	.eco_info = mt7926_eco_table,
	.isNicCapV1 = FALSE,
	.is_support_efuse = TRUE,
	.top_hcr = CONNAC2X_TOP_HCR,
	.top_hvr = CONNAC2X_TOP_HVR,
	.top_fvr = CONNAC2X_TOP_FVR,
	.arb_ac_mode_addr = MT7926_ARB_AC_MODE_ADDR,
	.asicCapInit = asicConnac2xCapInit,
#if CFG_ENABLE_FW_DOWNLOAD
	.asicEnableFWDownload = NULL,
#endif /* CFG_ENABLE_FW_DOWNLOAD */
#if defined(_HIF_USB) || defined(_HIF_SDIO)
	.asicDumpSerDummyCR = mt7961DumpSerDummyCR,
#endif
	.downloadBufferBin = wlanConnac2XDownloadBufferBin,
	.constructBufferBinFileName = mt7902ConstructBufferBinFileName,
	.is_support_hw_amsdu = TRUE,
	.is_support_asic_lp = TRUE,
	.is_support_wfdma1 = FALSE,
	.RxDbgInfoFromRxdGrp3 = mt7902GetRxDbgInfoSrc,
	.asicWfdmaReInit = asicConnac2xWfdmaReInit,
	.asicWfdmaReInit_handshakeInit = asicConnac2xWfdmaDummyCrWrite,
#if defined(_HIF_USB)
	.asicUsbInit = asicConnac2xWfdmaInitForUSB,
	.asicUsbInit_ic_specific = NULL,
	.u4SerUsbMcuEventAddr = WF_SW_DEF_CR_USB_MCU_EVENT_ADD,
	.u4SerUsbHostAckAddr = WF_SW_DEF_CR_USB_HOST_ACK_ADDR,
#endif
	.u4SerSuspendSyncAddr = CONNAC2X_HOST_MAILBOX_WF_ADDR,
	.group5_size = sizeof(struct HW_MAC_RX_STS_GROUP_5),
	.u4LmacWtblDUAddr = MT7926_WIFI_LWTBL_BASE,
	.u4UmacWtblDUAddr = MT7926_WIFI_UWTBL_BASE,

#if CFG_CHIP_RESET_SUPPORT
	.asicWfsysRst = mt7961HalCbtopRguWfRst,
	.asicPollWfsysSwInitDone = mt7961HalPollWfsysSwInitDone,
#if defined(_HIF_SDIO)
	.asicSetNoBTFwOwnEn = NULL,
#endif
#endif
	.asicSerInit = mt7961SerInit,
	.prTxPwrLimitFile = "TxPwrLimit_MT79x1.dat",
	.ucTxPwrLimitBatchSize = 8,

	/* buzzard capability:
	 * 1. MAC RX AMPDU max number is 256.
	 * 2. MAC RX MPDU max length is 11454.
	 * So, MAC RX AMPDU max length is 256 * 11454 = 2,932,224
	 *
	 * Set HE_CAP MAX_AMPDU_LEN_EXP to maximum which meets the condition
	 *     1. In 2.4G, 2 ^ (16 + MAX_AMPDU_LEN_EXP) - 1 <= 2,932,224
	 *     2. In 5G, 2 ^ (20 + MAX_AMPDU_LEN_EX) - 1 <= 2,932,224
	 */
	.uc2G4HeCapMaxAmpduLenExp = 3, /* 2 ^ (16 + 3) - 1 = 524287 */
	.uc5GHeCapMaxAmpduLenExp = 1, /* 2 ^ (20 + 1) - 1 = 2,097,151 */

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	/* owner set true when feature is ready. */
	.fgIsSupportL0p5Reset = TRUE,
#elif defined(_HIF_USB)
	.fgIsSupportL0p5Reset = TRUE,
#elif defined(_HIF_SDIO)
	/* owner set true when feature is ready. */
	.fgIsSupportL0p5Reset = TRUE,
#endif

	.loadCfgSetting = NULL,
	.getDongleType = mt7926GetDongleType,

};

struct mt66xx_hif_driver_data mt66xx_driver_data_mt7926 = {
	.chip_info = &mt66xx_chip_info_mt7926,
};

#endif /* MT7926 */
