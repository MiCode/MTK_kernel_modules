// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   mt6639.c
*    \brief  Internal driver stack will export
*    the required procedures here for GLUE Layer.
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifdef MT6639

#include "precomp.h"
#include "mt6639.h"
#include "coda/mt6639/cb_ckgen_top.h"
#include "coda/mt6639/cb_infra_misc0.h"
#include "coda/mt6639/cb_infra_rgu.h"
#include "coda/mt6639/cb_infra_slp_ctrl.h"
#include "coda/mt6639/cbtop_gpio_sw_def.h"
#include "coda/mt6639/conn_bus_cr.h"
#include "coda/mt6639/conn_cfg.h"
#include "coda/mt6639/conn_dbg_ctl.h"
#include "coda/mt6639/conn_host_csr_top.h"
#include "coda/mt6639/conn_semaphore.h"
#include "coda/mt6639/eef_top.h"
#include "coda/mt6639/wf_cr_sw_def.h"
#include "coda/mt6639/wf_top_cfg.h"
#include "coda/mt6639/wf_wfdma_ext_wrap_csr.h"
#include "coda/mt6639/wf_wfdma_host_dma0.h"
#include "coda/mt6639/wf_wfdma_mcu_dma0.h"
#include "coda/mt6639/wf_hif_dmashdl_top.h"
#include "coda/mt6639/wf_pse_top.h"
#include "coda/mt6639/pcie_mac_ireg.h"
#include "coda/mt6639/conn_mcu_bus_cr.h"
#include "coda/mt6639/conn_bus_cr_von.h"
#include "coda/mt6639/conn_host_csr_top.h"
#include "coda/mt6639/vlp_uds_ctrl.h"
#include "coda/mt6639/mawd_reg.h"
#include "coda/mt6639/wf_rro_top.h"
#include "coda/mt6639/wf_top_cfg_on.h"
#include "hal_dmashdl_mt6639.h"
#include "coda/mt6639/wf2ap_conn_infra_on_ccif4.h"
#include "coda/mt6639/ap2wf_conn_infra_on_ccif4.h"
#include "coda/mt6639/wf_top_cfg_on.h"
#include "coda/mt6639/wf_wtblon_top.h"
#include "coda/mt6639/wf_uwtbl_top.h"
#include "coda/mt6639/top_misc.h"
#include "hal_wfsys_reset_mt6639.h"
#include "coda/mt6639/cb_infra_slp_ctrl.h"
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
#include "connv3.h"
#endif
#if CFG_MTK_WIFI_FW_LOG_MMIO
#include "fw_log_mmio.h"
#endif
#if CFG_MTK_WIFI_FW_LOG_EMI
#include "fw_log_emi.h"
#endif

#if (CFG_SUPPORT_DEBUG_SOP == 1)
#include "dbg_mt6639.h"
#endif

#include "wlan_pinctrl.h"

#if CFG_MTK_MDDP_SUPPORT
#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 0)
#include "mddp_export.h"
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP == 0 */
#include "mddp.h"
#endif /* CFG_MTK_MDDP_SUPPORT */

#if CFG_MTK_CCCI_SUPPORT
#include "mtk_ccci_common.h"
#endif

#include "gl_coredump.h"

#define CFG_SUPPORT_VCODE_VDFS 0

#if (CFG_SUPPORT_VCODE_VDFS == 1)
#include <linux/pm_qos.h>

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#endif /*#ifndef CFG_SUPPORT_VCODE_VDFS*/

#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
#include <linux/msi.h>
#endif /* CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE */

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define WM_RAM_TYPE_MOBILE			0
#define WM_RAM_TYPE_CE				1

#define IS_MOBILE_SEGMENT \
		(CONFIG_WM_RAM_TYPE == WM_RAM_TYPE_MOBILE)

#define IS_CE_SEGMENT \
		(CONFIG_WM_RAM_TYPE == WM_RAM_TYPE_CE)

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static uint32_t mt6639GetFlavorVer(uint8_t *flavor);

static void mt6639_ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx);

static void mt6639_ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
static void mt6639_ConstructIdxLogBinName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName);
#endif

#if defined(_HIF_PCIE)
static uint8_t mt6639SetRxRingHwAddr(struct RTMP_RX_RING *prRxRing,
		struct BUS_INFO *prBusInfo, uint32_t u4SwRingIdx);

static bool mt6639WfdmaAllocRxRing(struct GLUE_INFO *prGlueInfo,
		bool fgAllocMem);

static void mt6639ProcessTxInterrupt(
		struct ADAPTER *prAdapter);

static void mt6639ProcessRxInterrupt(
	struct ADAPTER *prAdapter);

static void mt6639WfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo);

static void mt6639ReadIntStatusByMsi(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus);

static void mt6639ReadIntStatus(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus);

#if defined(_HIF_PCIE) && (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
static void mt6639EnableInterruptViaPcie(struct ADAPTER *prAdapter);
static void mt6639DisableInterruptViaPcie(struct ADAPTER *prAdapter);
#endif

static void mt6639EnableInterrupt(struct ADAPTER *prAdapter);
static void mt6639DisableInterrupt(struct ADAPTER *prAdapter);

static void mt6639ConfigIntMask(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable);

static void mt6639ConfigWfdmaRxRingThreshold(
	struct ADAPTER *prAdapter, uint32_t u4Num, u_int8_t fgIsData);

static void mt6639WpdmaConfig(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable, bool fgResetHif);

static void mt6639SetupMcuEmiAddr(struct ADAPTER *prAdapter);

#if CFG_MTK_WIFI_SW_EMI_RING
static void mt6639triggerInt(struct GLUE_INFO *prGlueInfo);
#endif /* CFG_MTK_WIFI_SW_EMI_RING */

static void mt6639WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	u_int32_t index);

static void mt6639CheckFwOwnMsiStatus(struct ADAPTER *prAdapter);
static void mt6639RecoveryMsiStatus(struct ADAPTER *prAdapter);

static void mt6639InitPcieInt(struct GLUE_INFO *prGlueInfo);
static void mt6639PowerOffPcieMac(struct ADAPTER *prAdpater);
static void mt6639PcieHwControlVote(
	struct ADAPTER *prAdapter,
	uint8_t enable,
	uint32_t u4WifiUser);

#if CFG_SUPPORT_PCIE_ASPM
static u_int8_t mt6639SetL1ssEnable(struct ADAPTER *prAdapter, u_int role,
	u_int8_t fgEn);
static uint32_t mt6639ConfigPcieAspm(struct GLUE_INFO *prGlueInfo,
	u_int8_t fgEn, u_int enable_role);
static void mt6639UpdatePcieAspm(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn);
static void mt6639KeepPcieWakeup(struct GLUE_INFO *prGlueInfo,
	u_int8_t fgWakeup);
static u_int8_t mt6639DumpPcieDateFlowStatus(struct GLUE_INFO *prGlueInfo);
#endif

static void mt6639ShowPcieDebugInfo(struct GLUE_INFO *prGlueInfo);

#if CFG_MTK_WIFI_DEVAPC
static void mt6639ShowDevapcDebugInfo(void);
#endif

static u_int8_t mt6639_get_sw_interrupt_status(struct ADAPTER *prAdapter,
	uint32_t *pu4Status);

static void mt6639_set_crypto(struct ADAPTER *prAdapter);
static void mt6639_ccif_notify_utc_time_to_fw(struct ADAPTER *ad,
	uint32_t sec,
	uint32_t usec);
static uint32_t mt6639_ccif_get_interrupt_status(struct ADAPTER *ad);
static void mt6639_ccif_set_fw_log_read_pointer(struct ADAPTER *ad,
	enum ENUM_FW_LOG_CTRL_TYPE type,
	uint32_t read_pointer);
static uint32_t mt6639_ccif_get_fw_log_read_pointer(struct ADAPTER *ad,
	enum ENUM_FW_LOG_CTRL_TYPE type);
static int32_t mt6639_ccif_trigger_fw_assert(struct ADAPTER *ad);

#if CFG_SUPPORT_PCIE_GEN_SWITCH
static void mt6639SetPcieSpeed(struct GLUE_INFO *prGlueInfo, uint32_t speed);
#endif

#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
static void mt6639PcieMsiMaskIrq(uint32_t u4Irq, uint32_t u4Bit);
static void mt6639PcieMsiUnmaskIrq(uint32_t u4Irq, uint32_t u4Bit);
#endif /* CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE */

#if IS_MOBILE_SEGMENT
static int32_t mt6639_trigger_fw_assert(struct ADAPTER *prAdapter);
static uint32_t mt6639_mcu_init(struct ADAPTER *ad);
static void mt6639_mcu_deinit(struct ADAPTER *ad);
static int mt6639ConnacPccifOn(struct ADAPTER *prAdapter);
static int mt6639ConnacPccifOff(struct ADAPTER *prAdapter);
static int mt6639_CheckBusHang(void *priv, uint8_t rst_enable);
static void mt6639_CheckMcuOff(struct ADAPTER *ad);
static uint32_t mt6639_wlanDownloadPatch(struct ADAPTER *prAdapter);
#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
static void mt6639PcieMsiMaskIrq(uint32_t u4Irq, uint32_t u4Bit);
static void mt6639PcieMsiUnmaskIrq(uint32_t u4Irq, uint32_t u4Bit);
#endif /* CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE */
#endif
#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
static int mt6639ConnacPccifOn(struct ADAPTER *prAdapter);
static int mt6639ConnacPccifOff(struct ADAPTER *prAdapter);
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */
#if CFG_PCIE_LTR_UPDATE
static void mt6639PcieLTRValue(struct ADAPTER *prAdapter, uint8_t ucState);
#endif
#endif

#if (CFG_SUPPORT_APS == 1)
static uint8_t mt6639_apsLinkPlanDecision(struct ADAPTER *prAdapter,
	struct AP_COLLECTION *prAp, enum ENUM_MLO_LINK_PLAN eLinkPlan,
	uint8_t ucBssIndex);
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct ECO_INFO mt6639_eco_table[] = {
	/* HW version,  ROM version,    Factory version */
	{0x00, 0x00, 0xA, 0x1},	/* E1 */
	{0x00, 0x00, 0x0, 0x0}	/* End of table */
};

uint8_t *apucmt6639FwName[] = {
	(uint8_t *) CFG_FW_FILENAME "_6639",
	NULL
};

#if CFG_SUPPORT_PCIE_ASPM
static spinlock_t rPCIELock;
#define WIFI_ROLE	(1)
#define MD_ROLE		(2)
#define POLLING_TIMEOUT		(200)
#define DUMP_PCIE_CR	"0x1F_5004=0x%08x, 0x1F_500C=0x%08x,"\
		"0x1F_5014=0x%08x, 0x1F_5400=0x%08x, 0x1F_5404=0x%08x,"\
		"0x1F_6008=0x%08x, 0x1F_6000=0x%08x, 0x1F_6100=0x%08x,"\
		"0x1F_5300=0x%08x, 0x1F_6550=0x%08x, 0x1F_801C=0x%08x,"\
		"0x1D_0E48=0x%08x, 0x1D_0E40=0x%08x, 0x1D_0E44=0x%08x,"\
		"0x7403018C=0x%08x, 0x7403002C=0x%08x, 0x740310f0=0x%08x,"\
		"0x740310f4=0x%08x\n"
#endif

#if defined(_HIF_PCIE)
struct PCIE_CHIP_CR_MAPPING mt6639_bus2chip_cr_mapping[] = {
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
#if IS_MOBILE_SEGMENT
	{0x74030000, 0x1d0000, 0x2000},  /* PCIe MAC */
#else
	{0x74030000, 0x10000, 0x1000},  /* PCIe MAC */
#endif
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
	{0x7c010000, 0x100000, 0x10000}, /* CONN_INFRA */
	{0x7c030000, 0x1a0000, 0x10000}, /* CONN_INFRA_ON_CCIF */
	{0x70020000, 0x1f0000, 0x10000}, /* Reserved for CBTOP, can't switch */
	{0x7c500000, MT6639_PCIE2AP_REMAP_BASE_ADDR, 0x200000}, /* remap */
	{0x70000000, 0x1e0000, 0x9000},
	{0x0, 0x0, 0x0} /* End */
};
#endif

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
struct pcie2ap_remap mt6639_pcie2ap_remap = {
	.reg_base = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_cr_pcie2ap_public_remapping_wf_06_ADDR,
	.reg_mask = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_cr_pcie2ap_public_remapping_wf_06_MASK,
	.reg_shift = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_cr_pcie2ap_public_remapping_wf_06_SHFT,
	.base_addr = MT6639_PCIE2AP_REMAP_BASE_ADDR
};

struct ap2wf_remap mt6639_ap2wf_remap = {
	.reg_base = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_ADDR,
	.reg_mask = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_MASK,
	.reg_shift = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_SHFT,
	.base_addr = MT6639_REMAP_BASE_ADDR
};

struct PCIE_CHIP_CR_REMAPPING mt6639_bus2chip_cr_remapping = {
	.pcie2ap = &mt6639_pcie2ap_remap,
	.ap2wf = &mt6639_ap2wf_remap,
};

struct wfdma_group_info mt6639_wfmda_host_tx_group[] = {
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

struct wfdma_group_info mt6639_wfmda_host_rx_group[] = {
	{"P0R4:AP DATA0", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_CTRL0_ADDR},
	{"P0R6:AP EVT/TDONE", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_CTRL0_ADDR,
	 true},
	{"P0R5:AP DATA1", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING5_CTRL0_ADDR},
	{"P0R7:AP ICS", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING7_CTRL0_ADDR},
	{"P0R8:MD DATA0", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING8_CTRL0_ADDR},
	{"P0R9:MD DATA1", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING9_CTRL0_ADDR},
	{"P0R10:MD EVENT", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING10_CTRL0_ADDR},
	{"P0R11:MD TDONE", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING11_CTRL0_ADDR},
};

struct wfdma_group_info mt6639_wfmda_wm_tx_group[] = {
	{"P0T6:LMAC TXD", WF_WFDMA_MCU_DMA0_WPDMA_TX_RING6_CTRL0_ADDR},
};

struct wfdma_group_info mt6639_wfmda_wm_rx_group[] = {
	{"P0R0:FWDL", WF_WFDMA_MCU_DMA0_WPDMA_RX_RING0_CTRL0_ADDR},
	{"P0R2:TXD0", WF_WFDMA_MCU_DMA0_WPDMA_RX_RING2_CTRL0_ADDR},
	{"P0R3:TXD1", WF_WFDMA_MCU_DMA0_WPDMA_RX_RING3_CTRL0_ADDR},
};

struct pse_group_info mt6639_pse_group[] = {
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
struct pcie_msi_layout mt6639_pcie_msi_layout[] = {
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
#if CFG_MTK_MDDP_SUPPORT
	{"conn_hif_md_int", mtk_md_dummy_pci_interrupt, NULL, MDDP_INT, 0},
	{"conn_hif_md_int", mtk_md_dummy_pci_interrupt, NULL, MDDP_INT, 0},
	{"conn_hif_md_int", mtk_md_dummy_pci_interrupt, NULL, MDDP_INT, 0},
	{"conn_hif_md_int", mtk_md_dummy_pci_interrupt, NULL, MDDP_INT, 0},
	{"conn_hif_md_int", mtk_md_dummy_pci_interrupt, NULL, MDDP_INT, 0},
	{"conn_hif_md_int", mtk_md_dummy_pci_interrupt, NULL, MDDP_INT, 0},
	{"conn_hif_md_int", mtk_md_dummy_pci_interrupt, NULL, MDDP_INT, 0},
	{"conn_hif_md_int", mtk_md_dummy_pci_interrupt, NULL, MDDP_INT, 0},
#else
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
	{"conn_hif_host_int", NULL, NULL, NONE_INT, 0},
#endif
	{"wm_conn2ap_wdt_irq", NULL, NULL, NONE_INT, 0},
	{"wf_mcu_jtag_det_eint", NULL, NULL, NONE_INT, 0},
	{"pmic_eint", NULL, NULL, NONE_INT, 0},
#if CFG_MTK_CCCI_SUPPORT && CFG_MTK_MDDP_SUPPORT
	{"ccif_bgf2ap_sw_irq", mtk_md_dummy_pci_interrupt, NULL, CCIF_INT, 0},
#else
	{"ccif_bgf2ap_sw_irq", NULL, NULL, NONE_INT, 0},
#endif
	{"ccif_wf2ap_sw_irq", pcie_sw_int_top_handler,
	 pcie_sw_int_thread_handler, AP_MISC_INT, 0},
#if CFG_MTK_CCCI_SUPPORT && CFG_MTK_MDDP_SUPPORT
	{"ccif_bgf2ap_irq_0", mtk_md_dummy_pci_interrupt, NULL, CCIF_INT, 0},
	{"ccif_bgf2ap_irq_1", mtk_md_dummy_pci_interrupt, NULL, CCIF_INT, 0},
#else
	{"ccif_bgf2ap_irq_0", NULL, NULL, NONE_INT, 0},
	{"ccif_bgf2ap_irq_1", NULL, NULL, NONE_INT, 0},
#endif
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
#if IS_ENABLED(CFG_MTK_WIFI_DRV_OWN_INT_MODE)
	{"drv_own_host_timeout_irq", pcie_drv_own_top_handler,
		pcie_drv_own_thread_handler, AP_DRV_OWN, 0},
#else
	{"reserved", NULL, NULL, NONE_INT, 0},
#endif
#if CFG_MTK_MDDP_SUPPORT
	{"drv_own_md_timeout_irq", mtk_md_dummy_pci_interrupt,
	 NULL, MDDP_INT, 0},
#else
	{"reserved", NULL, NULL, NONE_INT, 0},
#endif
#if CFG_MTK_WIFI_FW_LOG_MMIO || CFG_MTK_WIFI_FW_LOG_EMI
	{"fw_log_irq", pcie_fw_log_top_handler,
	 pcie_fw_log_thread_handler, AP_MISC_INT, 0},
#else
	{"reserved", NULL, NULL, NONE_INT, 0},
#endif
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
};
#endif

struct BUS_INFO mt6639_bus_info = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.top_cfg_base = MT6639_TOP_CFG_BASE,

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
#if (CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0)
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_15_MASK |
#endif /* CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0 */
#if (WFDMA_AP_MSI_NUM == 1)
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_16_MASK |
#endif
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_mcu2host_sw_int_sts_MASK),
	.host_int_rxdone_bits =
	(WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_4_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_5_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_6_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_7_MASK),

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

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	/* MAWD */
	.mawd_rx_blk_ctrl0 = MAWD_AP_RX_BLK_CTRL0,
	.mawd_rx_blk_ctrl1 = MAWD_AP_RX_BLK_CTRL1,
	.mawd_rx_blk_ctrl2 = MAWD_AP_RX_BLK_CTRL2,
	.mawd_ring_ctrl0 = MAWD_WFDMA_RING_MD_CTRL0,
	.mawd_ring_ctrl1 = MAWD_WFDMA_RING_MD_CTRL1,
	.mawd_ring_ctrl2 = MAWD_WFDMA_RING_MD_CTRL2,
	.mawd_ring_ctrl3 = MAWD_WFDMA_RING_MD_CTRL3,
	.mawd_ring_ctrl4 = MAWD_WFDMA_RING_MD_CTRL4,
	.mawd_hif_txd_ctrl0 = MAWD_HIF_TXD_MD_CTRL0,
	.mawd_hif_txd_ctrl1 = MAWD_HIF_TXD_MD_CTRL1,
	.mawd_hif_txd_ctrl2 = MAWD_HIF_TXD_MD_CTRL2,
	.mawd_err_rpt_ctrl0 = MAWD_ERR_RPT_CTRL0,
	.mawd_err_rpt_ctrl1 = MAWD_ERR_RPT_CTRL1,
	.mawd_err_rpt_ctrl2 = MAWD_ERR_RPT_CTRL2,
	.mawd_settings0 = MAWD_SETTING0,
	.mawd_settings1 = MAWD_SETTING1,
	.mawd_settings2 = MAWD_SETTING2,
	.mawd_settings3 = MAWD_SETTING3,
	.mawd_settings4 = MAWD_SETTING4,
	.mawd_settings5 = MAWD_SETTING5,
	.mawd_settings6 = MAWD_SETTING6,
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

	.bus2chip = mt6639_bus2chip_cr_mapping,
	.bus2chip_remap = &mt6639_bus2chip_cr_remapping,
	.max_static_map_addr = 0x00200000,

	.tx_ring_fwdl_idx = CONNAC3X_FWDL_TX_RING_IDX,
	.tx_ring_cmd_idx = 15,
	.tx_ring0_data_idx = 0,
	.tx_ring1_data_idx = 1,
	.tx_prio_data_idx = 2,
	.tx_altx_data_idx = 3,
	.rx_data_ring_num = 2,
	.rx_evt_ring_num = 2,
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	.rx_data_ring_size = 4095,
#elif CFG_SUPPORT_RX_PAGE_POOL
	.rx_data_ring_size = 3072,
#else
	.rx_data_ring_size = 1024,
#endif
	.rx_evt_ring_size = 128,
	.rx_data_ring_prealloc_size = 1024,
	.fw_own_clear_addr = CONNAC3X_BN0_IRQ_STAT_ADDR,
	.fw_own_clear_bit = PCIE_LPCR_FW_CLR_OWN,
#if IS_ENABLED(CFG_MTK_WIFI_DRV_OWN_INT_MODE)
	.fgCheckDriverOwnInt = TRUE,
#else
	.fgCheckDriverOwnInt = FALSE,
#endif /* IS_ENABLED(CFG_MTK_WIFI_DRV_OWN_INT_MODE) */
#if defined(_HIF_PCIE)
	.checkFwOwnMsiStatus = mt6639CheckFwOwnMsiStatus,
#endif
	.u4DmaMask = 32,
	.wfmda_host_tx_group = mt6639_wfmda_host_tx_group,
	.wfmda_host_tx_group_len = ARRAY_SIZE(mt6639_wfmda_host_tx_group),
	.wfmda_host_rx_group = mt6639_wfmda_host_rx_group,
	.wfmda_host_rx_group_len = ARRAY_SIZE(mt6639_wfmda_host_rx_group),
	.wfmda_wm_tx_group = mt6639_wfmda_wm_tx_group,
	.wfmda_wm_tx_group_len = ARRAY_SIZE(mt6639_wfmda_wm_tx_group),
	.wfmda_wm_rx_group = mt6639_wfmda_wm_rx_group,
	.wfmda_wm_rx_group_len = ARRAY_SIZE(mt6639_wfmda_wm_rx_group),
	.prDmashdlCfg = &rMt6639DmashdlCfg,
#if (DBG_DISABLE_ALL_INFO == 0)
	.prPleTopCr = &rMt6639PleTopCr,
	.prPseTopCr = &rMt6639PseTopCr,
	.prPpTopCr = &rMt6639PpTopCr,
#endif
	.prPseGroup = mt6639_pse_group,
	.u4PseGroupLen = ARRAY_SIZE(mt6639_pse_group),
	.pdmaSetup = mt6639WpdmaConfig,
#if defined(_HIF_PCIE) && (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
	.enableInterrupt = mt6639EnableInterruptViaPcie,
	.disableInterrupt = mt6639DisableInterruptViaPcie,
#else
	.enableInterrupt = mt6639EnableInterrupt,
	.disableInterrupt = mt6639DisableInterrupt,
#endif
	.configWfdmaIntMask = mt6639ConfigIntMask,
	.configWfdmaRxRingTh = mt6639ConfigWfdmaRxRingThreshold,
#if defined(_HIF_PCIE)
	.initPcieInt = mt6639InitPcieInt,
	.powerOffPcieMac = mt6639PowerOffPcieMac,
	.hwControlVote = mt6639PcieHwControlVote,
#if CFG_SUPPORT_PCIE_ASPM
	.configPcieAspm = mt6639ConfigPcieAspm,
	.updatePcieAspm = mt6639UpdatePcieAspm,
	.keepPcieWakeup = mt6639KeepPcieWakeup,
	.fgWifiEnL1_2 = TRUE,
	.fgMDEnL1_2 = TRUE,
#endif
	.pdmaStop = asicConnac3xWfdmaStop,
	.pdmaPollingIdle = asicConnac3xWfdmaPollingAllIdle,
	.pcie_msi_info = {
		.prMsiLayout = mt6639_pcie_msi_layout,
		.u4MaxMsiNum = ARRAY_SIZE(mt6639_pcie_msi_layout),
	},
#if CFG_MTK_WIFI_PCIE_SUPPORT
	.is_en_drv_ctrl_pci_msi_irq = FALSE,
#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
	.pcieMsiMaskIrq = mt6639PcieMsiMaskIrq,
	.pcieMsiUnmaskIrq = mt6639PcieMsiUnmaskIrq,
#endif /* CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE */
#endif
#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
	.pcieMsiMaskIrq = mt6639PcieMsiMaskIrq,
	.pcieMsiUnmaskIrq = mt6639PcieMsiUnmaskIrq,
#endif /* CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE */
	.showDebugInfo = mt6639ShowPcieDebugInfo,
	.disableDevice = mtk_pci_disable_device,
#if CFG_SUPPORT_PCIE_GEN_SWITCH
	.setPcieSpeed = mt6639SetPcieSpeed,
#endif
#if CFG_SUPPORT_WIFI_SLEEP_COUNT
	.wf_power_dump_start = mt6639PowerDumpStart,
	.wf_power_dump_end = mt6639PowerDumpEnd,
#endif
#if CFG_PCIE_LTR_UPDATE
	.pcieLTRValue = mt6639PcieLTRValue,
#endif
#endif /* _HIF_PCIE */
	.processTxInterrupt = mt6639ProcessTxInterrupt,
	.processRxInterrupt = mt6639ProcessRxInterrupt,
	.tx_ring_ext_ctrl = asicConnac3xWfdmaTxRingExtCtrl,
	.rx_ring_ext_ctrl = mt6639WfdmaRxRingExtCtrl,
	/* null wfdmaManualPrefetch if want to disable manual mode */
	.wfdmaManualPrefetch = mt6639WfdmaManualPrefetch,
	.lowPowerOwnRead = asicConnac3xLowPowerOwnRead,
	.lowPowerOwnSet = asicConnac3xLowPowerOwnSet,
	.lowPowerOwnClear = asicConnac3xLowPowerOwnClear,
	.wakeUpWiFi = asicWakeUpWiFi,
	.processSoftwareInterrupt = asicConnac3xProcessSoftwareInterrupt,
	.softwareInterruptMcu = asicConnac3xSoftwareInterruptMcu,
	.getMdSwIntSta = asicConnac3xGetMdSoftwareInterruptStatus,
	.hifRst = asicConnac3xHifRst,
#if defined(_HIF_PCIE) && (WFDMA_AP_MSI_NUM == 8)
	.devReadIntStatus = mt6639ReadIntStatusByMsi,
#else
	.devReadIntStatus = mt6639ReadIntStatus,
#endif /* _HIF_PCIE */
	.setRxRingHwAddr = mt6639SetRxRingHwAddr,
	.wfdmaAllocRxRing = mt6639WfdmaAllocRxRing,
	.clearEvtRingTillCmdRingEmpty = connac3xClearEvtRingTillCmdRingEmpty,
	.setupMcuEmiAddr = mt6639SetupMcuEmiAddr,
#endif /*_HIF_PCIE || _HIF_AXI */
#if defined(_HIF_PCIE) || defined(_HIF_AXI) || defined(_HIF_USB)
	.DmaShdlInit = mt6639DmashdlInit,
#endif
#if CFG_MTK_WIFI_SW_EMI_RING
	.rSwEmiRingInfo = {
		.rOps = {
			.init = halSwEmiInit,
			.read = halSwEmiRead,
			.triggerInt = mt6639triggerInt,
			.debug = halSwEmiDebug,
		},
		.fgIsSupport = TRUE,
		.u4CcifTchnumAddr =
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_TCHNUM_ADDR,
		.u4CcifChlNum = 3,
	},
#endif /* CFG_MTK_WIFI_SW_EMI_RING */
#if defined(_HIF_USB)
	.prDmashdlCfg = &rMt6639DmashdlCfg,
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
	.updateTxRingMaxQuota = mt6639UpdateDmashdlQuota,
#if CFG_CHIP_RESET_SUPPORT
	.asicUsbEpctlRstOpt = mt6639HalUsbEpctlRstOpt,
#endif
#endif
#if defined(_HIF_NONE)
	/* for compiler need one entry */
	.DmaShdlInit = NULL
#endif
#if (CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1)
	.updateTxRingMaxQuota = asicConnac3xUpdateDynamicDmashdlQuota,
#endif
};

#if CFG_ENABLE_FW_DOWNLOAD
struct FWDL_OPS_T mt6639_fw_dl_ops = {
	.constructFirmwarePrio = mt6639_ConstructFirmwarePrio,
	.constructPatchName = mt6639_ConstructPatchName,
#if CFG_SUPPORT_SINGLE_FW_BINARY
	.parseSingleBinaryFile = wlanParseSingleBinaryFile,
#endif
#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
	.constrcutIdxLogBin = mt6639_ConstructIdxLogBinName,
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */
#if defined(_HIF_PCIE) && IS_MOBILE_SEGMENT
	.downloadPatch = mt6639_wlanDownloadPatch,
#else
	.downloadPatch = wlanDownloadPatch,
#endif
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
#if defined(_HIF_PCIE) && IS_MOBILE_SEGMENT
	.mcu_init = mt6639_mcu_init,
	.mcu_deinit = mt6639_mcu_deinit,
#endif
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
	.constructBtPatchName = asicConnac3xConstructBtPatchName,
	.downloadBtPatch = asicConnac3xDownloadBtPatch,
#if (CFG_SUPPORT_CONNAC3X == 1)
	.configBtImageSection = asicConnac3xConfigBtImageSection,
#endif
#endif
	.getFwVerInfo = wlanParseRamCodeReleaseManifest,
};
#endif /* CFG_ENABLE_FW_DOWNLOAD */

struct TX_DESC_OPS_T mt6639_TxDescOps = {
	.fillNicAppend = fillNicTxDescAppend,
	.fillHifAppend = fillTxDescAppendByHostV2,
	.fillTxByteCount = fillConnac3xTxDescTxByteCount,
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	.fillNicSdoAppend = fillConnac3xNicTxDescAppendWithSdo,
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
};

struct RX_DESC_OPS_T mt6639_RxDescOps = {
	.getRxModeMcs = mt6639_get_rx_mode_mcs,
};

#if (DBG_DISABLE_ALL_INFO == 0)
struct CHIP_DBG_OPS mt6639_DebugOps = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.showPdmaInfo = connac3x_show_wfdma_info,
#endif
	.showPseInfo = connac3x_show_pse_info,
	.showPleInfo = connac3x_show_ple_info,
	.showTxdInfo = connac3x_show_txd_Info,
	.showWtblInfo = connac3x_show_wtbl_info,
	.showUmacWtblInfo = connac3x_show_umac_wtbl_info,
	.get_rssi_from_wtbl = connac3x_get_rssi_from_wtbl,
	.showCsrInfo = NULL,
#if defined(_HIF_PCIE) || defined(_HIF_AXI) || defined(_HIF_USB)
	.showDmaschInfo = connac3x_show_dmashdl_info,
#endif
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.getFwDebug = NULL,
	.setFwDebug = connac3x_set_ple_int_no_read,
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
	.show_wfdma_dbg_probe_info = mt6639_show_wfdma_dbg_probe_info,
	.show_wfdma_wrapper_info = mt6639_show_wfdma_wrapper_info,
	.dumpwfsyscpupcr = mt6639_dumpWfsyscpupcr,
#if (CFG_SUPPORT_DEBUG_SOP == 0)
	.dumpBusHangCr = mt6639_DumpBusHangCr,
	.dumpPcieStatus = mt6639DumpPcieDateFlowStatus,
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	.dumpPcieCr = mt6639_dumpPcieReg,
	.checkDumpViaBt = mt6639_CheckDumpViaBt,
#endif
#endif
#endif
#if (CFG_SUPPORT_DEBUG_SOP == 1)
	.show_debug_sop_info = mt6639_show_debug_sop_info,
#if defined(_HIF_PCIE)
	.show_mcu_debug_info = mt6639_pcie_show_mcu_debug_info,
#elif defined(_HIF_USB)
	.show_mcu_debug_info = mt6639_usb_show_mcu_debug_info,
#endif
#endif
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	.get_rx_rate_info = mt6639_get_rx_rate_info,
#endif
#if CFG_SUPPORT_LLS
	.get_rx_link_stats = mt6639_get_rx_link_stats,
#endif
	.dumpTxdInfo = connac3x_dump_tmac_info,
#if defined(_HIF_PCIE)
	.dumpWfBusSectionA = mt6639_dumpHostVdnrTimeoutInfo,
#endif
#if CFG_MTK_WIFI_DEVAPC
	.showDevapcDebugInfo = mt6639ShowDevapcDebugInfo,
#endif
};
#endif /* DBG_DISABLE_ALL_INFO */

#if CFG_SUPPORT_QA_TOOL
#if (CONFIG_WLAN_SERVICE == 1)
struct test_capability mt6639_toolCapability = {
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
		0x2F,	/* u_int32 bandwidth; */

		/* BIT[15:0]: Band0 2.4G, 0x1 */
		/* BIT[31:16]: Band1 5G, 6G, 0x6 */
		0x00060001,	/* u_int32 channel_band_dbdc;*/

		/* BIT[15:0]: Band2 N/A, 0x0 */
		/* BIT[31:16]: Band3 2.4G, 5G, 6G, 0x7 */
		0x00070000,	/* u_int32 channel_band_dbdc_ext */

		/* BIT[7:0]: Support phy 0xB (bitwise),
		 *           phy0, phy1, phy3(little)
		 */
		/* BIT[15:8]: Support Adie 0x1 (bitwise)*/
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

		/* BIT[7:0]: Band0 BW40, 0x3 */
		/* BIT[15:8]: Band1 BW320, 0x2F */
		/* BIT[23:16]: Band2 N/A, 0x0 */
		/* BIT[31:24]: Band3 BW20, 0x1 */
		0x01002F03,	/* u_int32 band_bandwidth; */

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
		/* BIT5: eMLSR support 0 */
		/* BIT6: MLR+, ALR support 0 */
		/* BIT7: Bandwidth duplcate debug support 0 */
		/* BIT8: dRU support */
		0xC,	/*u_int32 feature1; */

		/* u_int32 reserved[15]; */
		{ 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0 }
	}
};
#endif

struct ATE_OPS_T mt6639_AteOps = {
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
	.icapRiseVcoreClockRate = mt6639_icapRiseVcoreClockRate,
	.icapDownVcoreClockRate = mt6639_icapDownVcoreClockRate,
#if (CONFIG_WLAN_SERVICE == 1)
	.tool_capability = &mt6639_toolCapability,
#endif
};
#endif /* CFG_SUPPORT_QA_TOOL */

#if defined(_HIF_PCIE)
static struct CCIF_OPS mt6639_ccif_ops = {
	.get_interrupt_status = mt6639_ccif_get_interrupt_status,
	.notify_utc_time_to_fw = mt6639_ccif_notify_utc_time_to_fw,
	.set_fw_log_read_pointer = mt6639_ccif_set_fw_log_read_pointer,
	.get_fw_log_read_pointer = mt6639_ccif_get_fw_log_read_pointer,
	.trigger_fw_assert = mt6639_ccif_trigger_fw_assert,
};

#if CFG_MTK_WIFI_FW_LOG_MMIO
static struct FW_LOG_OPS mt6639_fw_log_mmio_ops = {
	.init = fwLogMmioInitMcu,
	.deinit = fwLogMmioDeInitMcu,
	.start = fwLogMmioStart,
	.stop = fwLogMmioStop,
	.handler = fwLogMmioHandler,
};
#endif

#if CFG_MTK_WIFI_FW_LOG_EMI
static struct FW_LOG_OPS mt6639_fw_log_emi_ops = {
	.init = fw_log_emi_init,
	.deinit = fw_log_emi_deinit,
	.start = fw_log_emi_start,
	.stop = fw_log_emi_stop,
	.set_enabled = fw_log_emi_set_enabled,
	.handler = fw_log_emi_handler,
};
#endif
#endif

#if CFG_SUPPORT_THERMAL_QUERY
struct thermal_sensor_info mt6639_thermal_sensor_info[] = {
	{"wifi_adie_0", THERMAL_TEMP_TYPE_ADIE, 0},
	{"wifi_ddie_0", THERMAL_TEMP_TYPE_DDIE, 0},
	{"wifi_ddie_1", THERMAL_TEMP_TYPE_DDIE, 1},
	{"wifi_ddie_2", THERMAL_TEMP_TYPE_DDIE, 2},
	{"wifi_ddie_3", THERMAL_TEMP_TYPE_DDIE, 3},
};
#endif

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
struct EMI_WIFI_MISC_RSV_MEM_INFO mt6639_wifi_misc_rsv_mem_info[] = {
	{WIFI_MISC_MEM_BLOCK_NON_MMIO, 2048, {0}},
	{WIFI_MISC_MEM_BLOCK_TX_POWER_LIMIT, 20480, {0}}
};
#endif

struct mt66xx_chip_info mt66xx_chip_info_mt6639 = {
	.bus_info = &mt6639_bus_info,
#if CFG_ENABLE_FW_DOWNLOAD
	.fw_dl_ops = &mt6639_fw_dl_ops,
#endif /* CFG_ENABLE_FW_DOWNLOAD */
#if CFG_SUPPORT_QA_TOOL
	.prAteOps = &mt6639_AteOps,
#endif /* CFG_SUPPORT_QA_TOOL */
	.prTxDescOps = &mt6639_TxDescOps,
	.prRxDescOps = &mt6639_RxDescOps,
#if (DBG_DISABLE_ALL_INFO == 0)
	.prDebugOps = &mt6639_DebugOps,
#endif
	.chip_id = MT6639_CHIP_ID,
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
	.patch_addr = MT6639_PATCH_START_ADDR,
	.is_support_cr4 = FALSE,
	.is_support_wacpu = FALSE,
	.sw_sync_emi_info = NULL,
#if defined(_HIF_PCIE)
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	.is_support_mawd = TRUE,
	.is_support_sdo = TRUE,
	.is_support_rro = TRUE,
	.is_en_fix_rro_amsdu_error = TRUE,
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
	.is_en_wfdma_no_mmio_read = TRUE,
#if CFG_MTK_WIFI_SW_EMI_RING
	.is_en_sw_emi_read = TRUE,
#endif
	.fgDumpViaBtOnlyForDbgSOP = TRUE,
#endif /* _HIF_PCIE */
	.txd_append_size = MT6639_TX_DESC_APPEND_LENGTH,
	.hif_txd_append_size = MT6639_HIF_TX_DESC_APPEND_LENGTH,
	.rxd_size = MT6639_RX_DESC_LENGTH,
	.init_evt_rxd_size = MT6639_RX_INIT_DESC_LENGTH,
	.pse_header_length = CONNAC3X_NIC_TX_PSE_HEADER_LENGTH,
	.init_event_size = CONNAC3X_RX_INIT_EVENT_LENGTH,
	.eco_info = mt6639_eco_table,
	.isNicCapV1 = FALSE,
	.is_support_efuse = TRUE,
	.top_hcr = CONNAC3X_TOP_HCR,
	.top_hvr = CONNAC3X_TOP_HVR,
	.top_fvr = CONNAC3X_TOP_FVR,
#if (CFG_SUPPORT_802_11AX == 1)
	.arb_ac_mode_addr = MT6639_ARB_AC_MODE_ADDR,
#endif
	.asicCapInit = asicConnac3xCapInit,
#if CFG_ENABLE_FW_DOWNLOAD
	.asicEnableFWDownload = NULL,
#endif /* CFG_ENABLE_FW_DOWNLOAD */
#if IS_CE_SEGMENT
	.downloadBufferBin = wlanConnac3XDownloadBufferBin,
#else
	.downloadBufferBin = NULL,
#endif
	.is_support_hw_amsdu = TRUE,
	.is_support_nvram_fragment = TRUE,
	.is_support_asic_lp = TRUE,
	.asicWfdmaReInit = asicConnac3xWfdmaReInit,
	.asicWfdmaReInit_handshakeInit = asicConnac3xWfdmaDummyCrWrite,
	.group5_size = sizeof(struct HW_MAC_RX_STS_GROUP_5),
	.u4LmacWtblDUAddr = CONNAC3X_WIFI_LWTBL_BASE,
	.u4UmacWtblDUAddr = CONNAC3X_WIFI_UWTBL_BASE,
#if defined(CFG_MTK_WIFI_CONNV3_SUPPORT) || (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
	.coexpccifon = mt6639ConnacPccifOn,
	.coexpccifoff = mt6639ConnacPccifOff,
#endif /* CFG_MTK_WIFI_CONNV3_SUPPORT || CFG_MTK_SUPPORT_LIGHT_MDDP */
#if CFG_MTK_MDDP_SUPPORT
	.isSupportMddpAOR = false,
	.isSupportMddpSHM = true,
	.u4MdLpctlAddr = CONN_HOST_CSR_TOP_WF_MD_LPCTL_ADDR,
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
#if (CFG_SUPPORT_APS == 1)
	.apsLinkPlanDecision = mt6639_apsLinkPlanDecision,
#endif
#if defined(_HIF_USB)
	.asicUsbInit = asicConnac3xWfdmaInitForUSB,
	.asicUsbInit_ic_specific = NULL,
	.u4SerUsbMcuEventAddr = WF_SW_DEF_CR_USB_MCU_EVENT_ADDR,
	.u4SerUsbHostAckAddr = WF_SW_DEF_CR_USB_HOST_ACK_ADDR,
	.dmashdlQuotaDecision = mt6639dmashdlQuotaDecision,
#endif
#if defined(_HIF_PCIE)
#if IS_MOBILE_SEGMENT
	.chip_capability = BIT(CHIP_CAPA_FW_LOG_TIME_SYNC) |
		BIT(CHIP_CAPA_FW_LOG_TIME_SYNC_BY_CCIF) |
		BIT(CHIP_CAPA_XTAL_TRIM),
	.checkbushang = mt6639_CheckBusHang,
	.checkmcuoff = mt6639_CheckMcuOff,
	.setCrypto = mt6639_set_crypto,
	.rEmiInfo = {
#if CFG_MTK_ANDROID_EMI
		.type = EMI_ALLOC_TYPE_LK,
		.coredump_size = (7 * 1024 * 1024),
		.coredump2_size = 0,
#else
		.type = EMI_ALLOC_TYPE_IN_DRIVER,
#endif /* CFG_MTK_ANDROID_EMI */
	},
#if CFG_SUPPORT_THERMAL_QUERY
	.thermal_info = {
		.sensor_num = ARRAY_SIZE(mt6639_thermal_sensor_info),
		.sensor_info = mt6639_thermal_sensor_info,
	},
#endif
	.trigger_fw_assert = mt6639_trigger_fw_assert,
	.fw_log_info = {
#if CFG_MTK_WIFI_FW_LOG_MMIO
		.ops = &mt6639_fw_log_mmio_ops,
#endif
#if CFG_MTK_WIFI_FW_LOG_EMI
		.base = 0x538000,
		.ops = &mt6639_fw_log_emi_ops,
#endif
		.path = ENUM_LOG_READ_POINTER_PATH_CCIF,
	},
#else
	.chip_capability = BIT(CHIP_CAPA_FW_LOG_TIME_SYNC) |
		BIT(CHIP_CAPA_XTAL_TRIM),
#endif
	.ccif_ops = &mt6639_ccif_ops,
	.get_sw_interrupt_status = mt6639_get_sw_interrupt_status,
#else
	.chip_capability = BIT(CHIP_CAPA_FW_LOG_TIME_SYNC) |
		BIT(CHIP_CAPA_XTAL_TRIM),
#endif /* _HIF_PCIE */
	.custom_oid_interface_version = MTK_CUSTOM_OID_INTERFACE_VERSION,
	.em_interface_version = MTK_EM_INTERFACE_VERSION,
#if CFG_CHIP_RESET_SUPPORT
	.asicWfsysRst = mt6639HalCbInfraRguWfRst,
	.asicPollWfsysSwInitDone = mt6639HalPollWfsysSwInitDone,
#endif
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	/* owner set true when feature is ready. */
	.fgIsSupportL0p5Reset = TRUE,
#elif defined(_HIF_USB)
	.fgIsSupportL0p5Reset = FALSE,
#elif defined(_HIF_SDIO)
	/* owner set true when feature is ready. */
	.fgIsSupportL0p5Reset = FALSE,
#endif
	.u4MinTxLen = 2,

	.eDefaultDbdcMode = ENUM_DBDC_MODE_STATIC,

	.fgCheckRxDropThreshold = TRUE,

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.rsvMemWiFiMisc = mt6639_wifi_misc_rsv_mem_info,
	.rsvMemWiFiMiscSize = ARRAY_SIZE(mt6639_wifi_misc_rsv_mem_info),
#endif
#if (CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1)
	.dmashdlQuotaDecision = asicConnac3xDynamicDmashdlQuotaDecision,
	.eMloMaxQuotaHwBand = ENUM_BAND_1,
	.u4DefaultMinQuota = 0x10,
	.u4DefaultMaxQuota = 0x100,
	.au4DmaMaxQuotaBand = {0x100, 0x7E0},
#if (CFG_SUPPORT_WIFI_6G == 1)
	.au4DmaMaxQuotaRfBand = {0x100, 0x2d0, 0x590},
#else
	.au4DmaMaxQuotaRfBand = {0x100, 0x2d0},
#endif /* CFG_SUPPORT_WIFI_6G */
#endif
#if CFG_SUPPORT_CONNAC3X
	/* Platform custom config for conninfra */
	.rPlatcfgInfraSysram = {
		.addr = CONNAC3X_PLAT_CFG_ADDR,
		.size = CONNAC3X_PLAT_CFG_SIZE,
	}
#endif
};

struct mt66xx_hif_driver_data mt66xx_driver_data_mt6639 = {
	.chip_info = &mt66xx_chip_info_mt6639,
};

void mt6639_icapRiseVcoreClockRate(void)
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

void mt6639_icapDownVcoreClockRate(void)
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

static void mt6639_ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx)
{
	int ret = 0;
	uint8_t ucIdx = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt6639GetFlavorVer(&aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt6639_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt6639_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt6639_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt6639_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2. WIFI_RAM_CODE_MT6639_1_1.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"WIFI_RAM_CODE_MT%x_%s_%u.bin",
			MT6639_CHIP_ID,
			aucFlavor,
			MT6639_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	for (ucIdx = 0; apucmt6639FwName[ucIdx]; ucIdx++) {
		if ((*pucNameIdx + 3) >= ucMaxNameIdx) {
			/* the table is not large enough */
			DBGLOG(INIT, ERROR,
				"kalFirmwareImageMapping >> file name array is not enough.\n");
			ASSERT(0);
			continue;
		}

		/* Type 3. WIFI_RAM_CODE_6639.bin */
		ret = kalSnprintf(*(apucName + (*pucNameIdx)),
				CFG_FW_NAME_MAX_LEN, "%s.bin",
				apucmt6639FwName[ucIdx]);
		if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
			(*pucNameIdx) += 1;
		else
			DBGLOG(INIT, ERROR,
				"[%u] kalSnprintf failed, ret: %d\n",
				__LINE__, ret);
	}
}

static void mt6639_ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt6639GetFlavorVer(&aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt6639_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt6639_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt6639_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt6639_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2. WIFI_MT6639_PATCH_MCU_1_1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_MT%x_PATCH_MCU_%s_%u_hdr.bin",
			  MT6639_CHIP_ID,
			  aucFlavor,
			  MT6639_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 3. mt6639_patch_e1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "mt6639_patch_e1_hdr.bin");
	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
}

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
static void mt6639_ConstructIdxLogBinName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	mt6639GetFlavorVer(&aucFlavor[0]);

	/* ex: WIFI_RAM_CODE_MT6639_2_1_idxlog.bin */
	ret = kalSnprintf(apucName[0],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_RAM_CODE_MT%x_%s_%u_idxlog.bin",
			  MT6639_CHIP_ID,
			  aucFlavor,
			  MT6639_ROM_VERSION);

	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
}
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */

#if defined(_HIF_PCIE)
static uint8_t mt6639SetRxRingHwAddr(struct RTMP_RX_RING *prRxRing,
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
		offset = 6;
		break;
	case RX_RING_DATA0:
		offset = 4;
		break;
	case RX_RING_DATA1:
		offset = 5;
		break;
	case RX_RING_TXDONE0:
		offset = 7;
		break;
	default:
		return FALSE;
	}

	halSetRxRingHwAddr(prRxRing, prBusInfo, offset);

	return TRUE;
}

static bool mt6639WfdmaAllocRxRing(struct GLUE_INFO *prGlueInfo,
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

	/* ICS log */
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_TXDONE0, prHifInfo->u4RxEvtRingSize,
			RXD_SIZE, RX_BUFFER_AGGRESIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[3] fail\n");
		return false;
	}
	return true;
}

static void mt6639ProcessTxInterrupt(
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

static void mt6639ProcessRxDataInterrupt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro)) {
		if (prHifInfo->u4OffloadIntStatus ||
		    (KAL_TEST_BIT(RX_RRO_DATA, prAdapter->ulNoMoreRfb)))
			halRroReadRxData(prAdapter);

		if ((u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_4_MASK) ||
		    (u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_5_MASK))
			halRroReadRxData(prAdapter);
	} else
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
#endif /* _HIF_PCIE || _HIF_AXI */
	{
		if ((u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_4_MASK) ||
		    (KAL_TEST_BIT(RX_RING_DATA0, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA0, TRUE);

		if ((u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_5_MASK) ||
		    (KAL_TEST_BIT(RX_RING_DATA1, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA1, TRUE);
	}
}

static void mt6639ProcessRxInterrupt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_6_MASK) ||
	    (KAL_TEST_BIT(RX_RING_EVT, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_EVT, FALSE);

	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_7_MASK) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE0, FALSE);

	mt6639ProcessRxDataInterrupt(prAdapter);
}

static void mt6639SetTRXRingPriorityInterrupt(struct ADAPTER *prAdapter)
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

static void mt6639WfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint32_t u4WrVal = 0, u4Addr = 0;
	uint32_t u4PrefetchCnt = 0x4, u4TxDataPrefetchCnt = 0x10;
	uint32_t u4PrefetchBase = 0x00400000, u4TxDataPrefetchBase = 0x01000000;
	uint32_t u4RxDataPrefetchCnt = 0x8;
	uint32_t u4RxDataPrefetchBase = 0x00800000;

	/* Rx ring */
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4RxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4RxDataPrefetchBase;
	}

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING7_EXT_CTRL_ADDR;
	u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
	u4WrVal += u4PrefetchBase;

#if CFG_MTK_MDDP_SUPPORT
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING8_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_RX_RING11_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4PrefetchBase;
	}
#endif

	/* Tx ring */
	/* fw download reuse tx data ring */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING16_EXT_CTRL_ADDR;
	u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_TX_RING1_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4TxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4TxDataPrefetchBase;
	}
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING2_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_TX_RING3_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4PrefetchBase;
	}

#if CFG_MTK_MDDP_SUPPORT
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING8_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_TX_RING9_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4TxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4TxDataPrefetchBase;
	}

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING10_EXT_CTRL_ADDR;
	u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
	u4WrVal += u4PrefetchBase;

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING14_EXT_CTRL_ADDR;
	u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
	u4WrVal += u4PrefetchBase;
#endif

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING15_EXT_CTRL_ADDR;
	u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
	u4WrVal += u4PrefetchBase;

	mt6639SetTRXRingPriorityInterrupt(prAdapter);

	/* reset dma TRX idx */
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_RST_DTX_PTR_ADDR, 0xFFFFFFFF);
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_RST_DRX_PTR_ADDR, 0xFFFFFFFF);
}

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
static void mt6639ReadOffloadIntStatus(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t u4RegValue = 0, u4WrValue = 0, u4Addr = 0, u4MawdOffSet;

	if (!IS_FEATURE_ENABLED(prWifiVar->fgEnableRro))
		return;

	u4WrValue = 0;
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd)) {
		u4MawdOffSet = prChipInfo->u4HostCsrOffset;
		if (KAL_TEST_BIT(1, prHifInfo->ulHifIntEnBits)) {
			*pu4IntStatus |= WHISR_RX0_DONE_INT;
			u4WrValue = BIT(0);
		}
		u4Addr = MAWD_AP_INTERRUPT_SETTING1 + u4MawdOffSet;
	} else {
		u4Addr = WF_RRO_TOP_HOST_INT_STS_ADDR;
		HAL_MCR_RD(prAdapter, u4Addr, &u4RegValue);
		if (u4RegValue &
		    WF_RRO_TOP_HOST_INT_STS_HOST_RRO_DONE_INT_MASK) {
			*pu4IntStatus |= WHISR_RX0_DONE_INT;
			u4WrValue =
				(u4RegValue &
			WF_RRO_TOP_HOST_INT_STS_HOST_RRO_DONE_INT_MASK);
		}
	}
	prHifInfo->u4OffloadIntStatus = u4WrValue;
	if (u4WrValue)
		HAL_MCR_WR(prAdapter, u4Addr, u4WrValue);
}
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

#if defined(_HIF_PCIE)
static void mt6639ReadIntStatusByMsi(struct ADAPTER *prAdapter,
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

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	mt6639ReadOffloadIntStatus(prAdapter, pu4IntStatus);
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
}
#endif

static void mt6639ReadIntStatus(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct BUS_INFO *prBusInfo = prChipInfo->bus_info;
	uint32_t u4RegValue = 0, u4WrValue = 0, u4Addr;

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

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	mt6639ReadOffloadIntStatus(prAdapter, pu4IntStatus);
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
}

static void mt6639ConfigIntMask(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_VAR *prWifiVar;
	uint32_t u4Addr = 0, u4WrVal = 0;

	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;

#if CFG_SUPPORT_WED_PROXY
	u4Addr = WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR;
#else
	u4Addr = enable ? WF_WFDMA_HOST_DMA0_HOST_INT_ENA_SET_ADDR :
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_CLR_ADDR;
#endif
	u4WrVal =
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA6_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA7_MASK |
#if (CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0)
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA0_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA1_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA2_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA3_MASK |
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0 */
#if (CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0)
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA15_MASK |
#endif /* CFG_SUPPORT_DISABLE_CMD_DDONE_INTR */
#if (WFDMA_AP_MSI_NUM == 1)
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA16_MASK |
#endif
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_mcu2host_sw_int_ena_MASK;

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro)) {
		if (!IS_FEATURE_ENABLED(prWifiVar->fgEnableMawd)) {
			u4WrVal |=
			WF_WFDMA_HOST_DMA0_HOST_INT_ENA_subsys_int_ena_MASK;
		}
	}
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

	u4WrVal |=
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA4_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA5_MASK;

#if CFG_SUPPORT_WED_PROXY
	if (!enable)
		u4WrVal = 0;

	wedUpdateIntMask(u4WrVal);
#endif

	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4WrVal);
}

#if defined(_HIF_PCIE) && (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
static void mt6639EnableInterruptViaPcie(struct ADAPTER *prAdapter)
{
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

	mt6639ConfigIntMask(prAdapter->prGlueInfo, FALSE);
	mt6639ConfigIntMask(prAdapter->prGlueInfo, TRUE);
	asicConnac3xEnablePlatformIRQ(prAdapter);
}

static void mt6639DisableInterruptViaPcie(struct ADAPTER *prAdapter)
{
	asicConnac3xDisablePlatformIRQ(prAdapter);
}
#endif

static void mt6639EnableInterrupt(struct ADAPTER *prAdapter)
{
	mt6639ConfigIntMask(prAdapter->prGlueInfo, TRUE);
	asicConnac3xEnablePlatformIRQ(prAdapter);
}

static void mt6639DisableInterrupt(struct ADAPTER *prAdapter)
{
	mt6639ConfigIntMask(prAdapter->prGlueInfo, FALSE);
	asicConnac3xDisablePlatformIRQ(prAdapter);
}

static void mt6639WpdmaMsiConfig(struct ADAPTER *prAdapter)
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
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_MD_INT_LUMP_SEL,
		u4Value);
#endif
}

static void mt6639ConfigWfdmaRxRingThreshold(
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
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_PAUSE_RX_Q_TH1110_ADDR;
	     u4Addr += 0x4)
		HAL_MCR_WR(prAdapter, u4Addr, u4Val);

exit:
	DBGLOG(HAL, INFO, "Set WFDMA RxQ[%u] threshold[0x%08x]\n",
	       fgIsData, u4Val);
}

static void mt6639WpdmaDlyInt(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t u4Addr, u4Val;

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	/* disable delay interrupt if enable rro */
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro))
		return;
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

	/* Enable RX periodic delayed interrupt (unit: 20us) */
	u4Val = 0xF00000 | prWifiVar->u4PrdcIntTime;
#if CFG_SUPPORT_WIFI_MDDP_NO_MMIO_READ
#if CFG_MTK_MDDP_SUPPORT && (WFDMA_MD_MSI_NUM == 8)
	u4Val |= 0xF000000;
#endif
#endif
	u4Addr = WF_WFDMA_HOST_DMA0_HOST_PER_DLY_INT_CFG_ADDR;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	/* setup ring 4-7 delay interrupt */
	u4Addr = WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_ADDR;
	u4Val = (4 <<
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_RING_IDX0_SHFT) |
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_DIR0_MASK |
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_ENA0_MASK |
		(5 <<
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_RING_IDX1_SHFT) |
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_DIR1_MASK |
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_ENA1_MASK |
		(6 <<
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_RING_IDX2_SHFT) |
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_DIR2_MASK |
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_ENA2_MASK |
		(7 <<
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_RING_IDX3_SHFT) |
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_DIR3_MASK |
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_ENA3_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG0_ADDR;
	u4Val = prWifiVar->u4DlyIntTime <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG0_PRI0_MAX_PTIME_SHFT |
		prWifiVar->u4DlyIntCnt <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG0_PRI0_MAX_PINT_SHFT |
		prWifiVar->fgEnDlyInt <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG0_PRI0_DLY_INT_EN_SHFT |
		prWifiVar->u4DlyIntTime <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG0_PRI1_MAX_PTIME_SHFT |
		prWifiVar->u4DlyIntCnt <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG0_PRI1_MAX_PINT_SHFT |
		prWifiVar->fgEnDlyInt <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG0_PRI1_DLY_INT_EN_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG1_ADDR;
	u4Val = prWifiVar->u4DlyIntTime <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG1_PRI0_MAX_PTIME_SHFT |
		prWifiVar->u4DlyIntCnt <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG1_PRI0_MAX_PINT_SHFT |
		prWifiVar->fgEnDlyInt <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG1_PRI0_DLY_INT_EN_SHFT |
		prWifiVar->u4DlyIntTime <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG1_PRI1_MAX_PTIME_SHFT |
		prWifiVar->u4DlyIntCnt <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG1_PRI1_MAX_PINT_SHFT |
		prWifiVar->fgEnDlyInt <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG1_PRI1_DLY_INT_EN_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	DBGLOG(HAL, INFO, "prdc int: %uus, dly int[%u]: %uus, cnt=%u",
	       prWifiVar->u4PrdcIntTime * 20,
	       prWifiVar->fgEnDlyInt,
	       prWifiVar->u4DlyIntTime * 20,
	       prWifiVar->u4DlyIntCnt);
}

static void mt6639WpdmaConfigExt0(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t u4Addr = 0, u4Val = 0;

	if (!IS_FEATURE_ENABLED(prWifiVar->fgEnableSdo))
		return;

	/* enable SDO */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_ADDR;
	/* default settings */
	u4Val = 0x28C004DF |
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_SDO_DISP_MODE_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
}

static void mt6639WpdmaConfigExt1(struct ADAPTER *prAdapter)
{
	uint32_t u4Addr = 0, u4Val = 0;

	/* packet based TX flow control */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT1_ADDR;
	/* default settings */
	u4Val = 0x8C800404 |
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT1_CSR_TX_FCTRL_MODE_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

static void mt6639WpdmaConfigExt2(struct ADAPTER *prAdapter)
{
	uint32_t u4Addr = 0, u4Val = 0;

	/* enable performance monitor */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT2_ADDR;
	u4Val = 0x44;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HIF_PERF_MAVG_DIV_ADDR;
	u4Val = 0x36;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

static void mt6639WpdmaConfig(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable, bool fgResetHif)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	union WPDMA_GLO_CFG_STRUCT GloCfg;
	uint32_t u4Addr = 0;

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	asicConnac3xWfdmaControl(prGlueInfo, 0, enable);
	GloCfg.word = prGlueInfo->rHifInfo.GloCfg.word;
#endif
	if (!enable)
		return;

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR;
#if defined(_HIF_PCIE)
	mt6639WpdmaMsiConfig(prGlueInfo->prAdapter);
#else
	HAL_MCR_RD(prAdapter, u4Addr, &GloCfg.word);
#endif
	GloCfg.field_conn3x.tx_dma_en = 1;
	GloCfg.field_conn3x.rx_dma_en = 1;
	HAL_MCR_WR(prAdapter, u4Addr, GloCfg.word);
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	prGlueInfo->rHifInfo.GloCfg.word = GloCfg.word;
#endif
	mt6639ConfigWfdmaRxRingThreshold(prAdapter, 0, FALSE);

	mt6639WpdmaConfigExt0(prAdapter);
	mt6639WpdmaConfigExt1(prAdapter);
	mt6639WpdmaConfigExt2(prAdapter);

	mt6639WpdmaDlyInt(prGlueInfo);
}

static void mt6639WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *prRxRing,
	u_int32_t index)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	struct WIFI_VAR *prWifiVar;
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
	uint32_t ext_offset;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	prWifiVar = &prAdapter->rWifiVar;
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

	switch (index) {
	case RX_RING_EVT:
		ext_offset = 6 * 4;
		break;
	case RX_RING_DATA0:
		ext_offset = 4 * 4;
		break;
	case RX_RING_DATA1:
		ext_offset = 5 * 4;
		break;
	case RX_RING_TXDONE0:
		ext_offset = 7 * 4;
		break;
	default:
		DBGLOG(RX, ERROR, "Error index=%d\n", index);
		return;
	}

	prRxRing->hw_desc_base_ext =
		prBusInfo->host_rx_ring_ext_ctrl_base + ext_offset;

	HAL_MCR_WR(prAdapter, prRxRing->hw_desc_base_ext,
		   CONNAC3X_RX_RING_DISP_MAX_CNT);

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	/* enable wfdma magic cnt */
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro) &&
	    halIsDataRing(RX_RING, index)) {
		uint32_t u4Val = 0;

		u4Val = prRxRing->u4RingSize |
			WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL1_MGC_ENA_MASK;
		HAL_MCR_WR(prAdapter, prRxRing->hw_cnt_addr, u4Val);
	}
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
}

#if defined(_HIF_PCIE)
static void mt6639RecoveryMsiStatus(struct ADAPTER *prAdapter)
{
	struct PERF_MONITOR *perf = &prAdapter->rPerMonitor;
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct pcie_msi_info *prMsiInfo = &prBusInfo->pcie_msi_info;
	uint32_t u4Val = 0, u4Cnt = 0;

	/* tput < 10mbps */
	if (perf->u4CurrPerfLevel > 0)
		return;

	/* check wfdma bits(0-7) */
	if (prMsiInfo->ulEnBits & 0xff)
		return;

	if (time_before(jiffies, prBusInfo->ulRecoveryMsiCheckTime))
		return;

	prBusInfo->ulRecoveryMsiCheckTime = jiffies +
		prAdapter->rWifiVar.u4RecoveryMsiTime * HZ / 1000;

	u4Cnt = halGetWfdmaRxCnt(prAdapter);
	if (u4Cnt < prWifiVar->u4RecoveryMsiRxCnt)
		return;

	/* read PCIe EP MSI status */
	u4Val = mtk_pci_read_msi_mask(prAdapter->prGlueInfo);
	if ((u4Val & 0xff) == 0)
		return;

	mtk_pci_msi_unmask_all_irq(prAdapter->prGlueInfo);
	DBGLOG(HAL, WARN, "Rx[%u] MSI_MASK=[0x%08x], unmask all msi irq",
	       u4Cnt, u4Val);
}

static void mt6639CheckFwOwnMsiStatus(struct ADAPTER *prAdapter)
{
	mt6639RecoveryMsiStatus(prAdapter);
}

#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
static void mt6639PcieMsiMaskIrq(uint32_t u4Irq, uint32_t u4Bit)
{
	struct irq_data *data;
	struct msi_desc *entry;
	raw_spinlock_t *lock = NULL;
	unsigned long flags;

	data = irq_get_irq_data(u4Irq);
	if (data) {
		entry = irq_data_get_msi_desc(data);
		lock = &to_pci_dev(entry->dev)->msi_lock;

		raw_spin_lock_irqsave(lock, flags);
		entry->pci.msi_mask |= BIT(u4Bit);
		HAL_MCR_WR(NULL, 0x740310F0, entry->pci.msi_mask);
		raw_spin_unlock_irqrestore(lock, flags);
	}
}

static void mt6639PcieMsiUnmaskIrq(uint32_t u4Irq, uint32_t u4Bit)
{
	struct irq_data *data;
	struct msi_desc *entry;
	raw_spinlock_t *lock = NULL;
	unsigned long flags;

	data = irq_get_irq_data(u4Irq);
	if (data) {
		entry = irq_data_get_msi_desc(data);
		lock = &to_pci_dev(entry->dev)->msi_lock;

		raw_spin_lock_irqsave(lock, flags);
		entry->pci.msi_mask &= ~(BIT(u4Bit));
		HAL_MCR_WR(NULL, 0x740310F0, entry->pci.msi_mask);
		raw_spin_unlock_irqrestore(lock, flags);
	}
}
#endif /* CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE */
#endif

#if CFG_SUPPORT_PCIE_ASPM
void *pcie_vir_addr;
#endif

static void mt6639InitPcieInt(struct GLUE_INFO *prGlueInfo)
{
#if CFG_SUPPORT_PCIE_ASPM
	HAL_MCR_WR(prGlueInfo->prAdapter, 0x74030074, 0x08021000);
	if (pcie_vir_addr) {
		writel(0x08021000, (pcie_vir_addr + 0x74));
		DBGLOG(HAL, INFO, "pcie_vir_addr=0x%llx\n",
		       (uint64_t)pcie_vir_addr);
	} else {
		DBGLOG(HAL, INFO, "pcie_vir_addr is null\n");
	}
#endif
}

static void mt6639PowerOffPcieMac(struct ADAPTER *prAdpater)
{
#if IS_MOBILE_SEGMENT
	HAL_MCR_WR(prAdpater, PCIE_MAC_IREG_IMASK_HOST_ADDR, 0);
#endif /* IS_MOBILE_SEGMENT */
}

static void mt6639PcieHwControlVote(
	struct ADAPTER *prAdapter,
	uint8_t enable,
	uint32_t u4WifiUser)
{
	halPcieHwControlVote(prAdapter, enable, u4WifiUser);
}

#if CFG_SUPPORT_PCIE_ASPM
static u_int8_t mt6639SetL1ssEnable(struct ADAPTER *prAdapter,
				u_int role, u_int8_t fgEn)
{
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	if (role == WIFI_ROLE)
		prChipInfo->bus_info->fgWifiEnL1_2 = fgEn;
	else if (role == MD_ROLE)
		prChipInfo->bus_info->fgMDEnL1_2 = fgEn;

	DBGLOG(HAL, TRACE, "fgWifiEnL1_2 = %d, fgMDEnL1_2=%d\n",
		prChipInfo->bus_info->fgWifiEnL1_2,
		prChipInfo->bus_info->fgMDEnL1_2);

	if (prChipInfo->bus_info->fgWifiEnL1_2
		&& prChipInfo->bus_info->fgMDEnL1_2)
		return TRUE;
	else
		return FALSE;
}
static uint32_t mt6639ConfigPcieAspm(struct GLUE_INFO *prGlueInfo,
				u_int8_t fgEn, u_int enable_role)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t value = 0, delay = 0, value1 = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	u_int8_t enableL1ss = FALSE;
	u_int8_t isL0Status = FALSE;
	unsigned long flags = 0;

	if (pcie_vir_addr == NULL)
		return WLAN_STATUS_FAILURE;

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	spin_lock_irqsave(&rPCIELock, flags);
	enableL1ss =
		mt6639SetL1ssEnable(prGlueInfo->prAdapter, enable_role, fgEn);

	if (fgEn) {
		/* Restore original setting*/
		if (enableL1ss) {
			value = readl(pcie_vir_addr + 0x194);
			value1 = readl(pcie_vir_addr + 0x150);
			isL0Status = ((value1 & BITS(24, 28)) >> 24) == 0x10;
			if ((value & BITS(0, 11)) == 0xc0f ||
				((value & BITS(0, 11)) == 0x20f &&
				isL0Status)) {
				writel(0xe0f, (pcie_vir_addr + 0x194));
			} else {
				DBGLOG(HAL, INFO,
					"enable isL0Status=%d, value=0x%08x, value1=0x%08x\n",
					isL0Status, value, value1);
				goto exit;
			}

			delay += 10;
			udelay(10);

			/* Polling RC 0x112f0150[28:24] until =0x10 */
			while (1) {
				value = readl(pcie_vir_addr + 0x150);

				if (((value & BITS(24, 28))
					>> 24) == 0x10)
					break;

				if (delay >= POLLING_TIMEOUT) {
					DBGLOG(HAL, INFO,
						"Enable L1.2 POLLING_TIMEOUT\n");
					rStatus = WLAN_STATUS_FAILURE;
					goto exit;
				}

				delay += 10;
				udelay(10);
			}

			HAL_MCR_WR(prGlueInfo->prAdapter,
				PCIE_MAC_IREG_PCIE_LOW_POWER_CTRL_ADDR, 0xf);
			HAL_MCR_RD(prGlueInfo->prAdapter,
				PCIE_MAC_IREG_PCIE_LOW_POWER_CTRL_ADDR, &value);
			writel(0xf, (pcie_vir_addr + 0x194));


			DBGLOG(HAL, TRACE, "Enable aspm L1.1/L1.2..\n");
		} else {
			DBGLOG(HAL, TRACE, "Not to enable aspm L1.1/L1.2..\n");
		}
	} else {
		/*
		 *	Backup original setting then
		 *	disable L1.1, L1.2 and set LTR to 0
		 */
			value = readl(pcie_vir_addr + 0x194);
			value1 = readl(pcie_vir_addr + 0x150);
			isL0Status = ((value1 & BITS(24, 28)) >> 24) == 0x10;
			if ((value & BITS(0, 11)) == 0xf ||
				((value & BITS(0, 11)) == 0xe0f &&
				isL0Status)) {
				writel(0x20f, (pcie_vir_addr + 0x194));
			} else {
				DBGLOG(HAL, INFO,
					"disable isL0Status=%d, value=0x%08x, value1=0x%08x\n",
					isL0Status, value, value1);
				goto exit;
			}

		delay += 10;
		udelay(10);

		/* Polling RC 0x112f0150[28:24] until =0x10 */
		while (1) {
			value = readl(pcie_vir_addr + 0x150);

			if (((value & BITS(24, 28))
				>> 24) == 0x10)
				break;

			if (delay >= POLLING_TIMEOUT) {
				DBGLOG(HAL, INFO,
					"Disable L1.2 POLLING_TIMEOUT\n");
				rStatus = WLAN_STATUS_FAILURE;
				goto exit;
			}

			delay += 10;
			udelay(10);
		}

		HAL_MCR_WR(prGlueInfo->prAdapter,
			PCIE_MAC_IREG_PCIE_LOW_POWER_CTRL_ADDR, 0xc0f);
		HAL_MCR_RD(prGlueInfo->prAdapter,
			PCIE_MAC_IREG_PCIE_LOW_POWER_CTRL_ADDR, &value);
		writel(0xc0f, (pcie_vir_addr + 0x194));

		if (prHifInfo->eCurPcieState == PCIE_STATE_L0)
			DBGLOG(HAL, TRACE, "Disable aspm L1..\n");
		else
			DBGLOG(HAL, TRACE, "Disable aspm L1.1/L1.2..\n");
	}

exit:
	spin_unlock_irqrestore(&rPCIELock, flags);
	return rStatus;
}

static void mt6639UpdatePcieAspm(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;

	if (fgEn) {
		prHifInfo->eNextPcieState = PCIE_STATE_L1_2;
	} else {
		if (prHifInfo->eNextPcieState != PCIE_STATE_L0)
			prHifInfo->eNextPcieState = PCIE_STATE_L1;
	}

	if (prHifInfo->eCurPcieState != prHifInfo->eNextPcieState) {
		if (prHifInfo->eNextPcieState == PCIE_STATE_L1_2)
			mt6639ConfigPcieAspm(prGlueInfo, TRUE, 1);
		else
			mt6639ConfigPcieAspm(prGlueInfo, FALSE, 1);
		prHifInfo->eCurPcieState = prHifInfo->eNextPcieState;
	}
}

static void mt6639KeepPcieWakeup(struct GLUE_INFO *prGlueInfo,
				u_int8_t fgWakeup)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;

	if (fgWakeup) {
		prHifInfo->eNextPcieState = PCIE_STATE_L0;
	} else {
		if (prHifInfo->eCurPcieState == PCIE_STATE_L0)
			prHifInfo->eNextPcieState = PCIE_STATE_L1;
	}
}

static u_int8_t mt6639DumpPcieDateFlowStatus(struct GLUE_INFO *prGlueInfo)
{
	struct pci_dev *pci_dev = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	uint32_t u4RegVal[25] = {0};
#if CFG_MTK_WIFI_PCIE_SUPPORT
	uint32_t link_info = mtk_pcie_dump_link_info(0);
#endif

#if CFG_MTK_WIFI_PCIE_SUPPORT
	if (!(link_info & BIT(5)))
		return FALSE;
#endif

	/*read pcie cfg.space 0x488 // level1: pcie*/
	prHifInfo = &prGlueInfo->rHifInfo;
	if (prHifInfo)
		pci_dev = prHifInfo->pdev;

	if (pci_dev) {
		pci_read_config_dword(pci_dev, 0x0, &u4RegVal[0]);
		if (u4RegVal[0] == 0 || u4RegVal[0] == 0xffff) {
			DBGLOG(HAL, INFO,
				"PCIE link down 0x0=0x%08x\n", u4RegVal[0]);
			/* block pcie to prevent access */
#if CFG_MTK_WIFI_PCIE_SUPPORT
			mtk_pcie_disable_data_trans(0);
#endif
			return FALSE;
		}

		/*1. read pcie cfg.space 0x488 // level1: pcie*/
		pci_read_config_dword(pci_dev, 0x488, &u4RegVal[1]);
		if (u4RegVal[1] != 0xC0093301)
			DBGLOG(HAL, INFO,
				"state mismatch 0x488=0x%08x\n", u4RegVal[1]);
	}

	/*2. cb_infra/cbtop status*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1E7204, &u4RegVal[2]);
	if (u4RegVal[2] < 0x20220811) {
		DBGLOG(HAL, INFO, "version error 0x1E7204=0x%08x\n",
			u4RegVal[2]);
		return FALSE;
	}

	/*3. cb_infra_slp_status*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F500C, &u4RegVal[3]);
	if ((u4RegVal[3] & BITS(1, 3)) != BITS(1, 3)) {
		DBGLOG(HAL, INFO, "cb_infra_slp error=0x%08x\n", u4RegVal[3]);
		return FALSE;
	}

	/*4. MMIO dump slp_ctrl setting*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F5004, &u4RegVal[4]);

	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F500C, &u4RegVal[5]);

	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F5014, &u4RegVal[6]);

	/*5. MMIO dump slp_ctrl cnt:*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F5400, &u4RegVal[7]);

	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F5404, &u4RegVal[8]);

	/*6. MMIO dump ap2conn gals dbg*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F6008, &u4RegVal[9]);

	/*6. MMIO dump conn2ap gals dbg*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F6000, &u4RegVal[10]);

	/*6. MMIO dump dma2ap gals dbg*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F6100, &u4RegVal[11]);

	/*7. MMIO dump 0x1F_5300*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F5300, &u4RegVal[12]);

	/*5. MMIO dump 0x1F_6550*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F6550, &u4RegVal[13]);

	/*6. MMIO dump 0x1F_801C*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1F801C, &u4RegVal[14]);
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1D0E48, &u4RegVal[16]);
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1D0E40, &u4RegVal[17]);
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1D0E44, &u4RegVal[18]);

	/*pcie msi status*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x7403018C, &u4RegVal[19]);
	HAL_MCR_WR(prGlueInfo->prAdapter, 0x74030168, 0xcccc0100);
	HAL_MCR_WR(prGlueInfo->prAdapter, 0x74030164, 0x03020100);
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x7403002C, &u4RegVal[20]);
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x740310f0, &u4RegVal[21]);
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x740310f4, &u4RegVal[22]);

	DBGLOG(HAL, INFO, DUMP_PCIE_CR,
	u4RegVal[4], u4RegVal[5], u4RegVal[6], u4RegVal[7],
	u4RegVal[8], u4RegVal[9], u4RegVal[10], u4RegVal[11],
	u4RegVal[12], u4RegVal[13], u4RegVal[14], u4RegVal[16],
	u4RegVal[17], u4RegVal[18], u4RegVal[19], u4RegVal[20],
	u4RegVal[21], u4RegVal[22]);

	/*7. MMIO write 0x1E_3020 = 0x0*/
	HAL_MCR_WR(prGlueInfo->prAdapter, 0x1E3020, 0x0);
	/*8. MMIO write 0x1E_7150 = 0x2*/
	HAL_MCR_WR(prGlueInfo->prAdapter, 0x1E7150, 0x2);
	/*9. CBTOP REGs dump  0x1E_7154*/
	HAL_MCR_RD(prGlueInfo->prAdapter, 0x1E7154, &u4RegVal[15]);
	if (u4RegVal[15] != 0x0) {
		DBGLOG(HAL, INFO, "0x1E7154=0x%08x\n", u4RegVal[15]);
		return FALSE;
	}

	if ((u4RegVal[6] & BITS(12, 13)) == BITS(12, 13)) {
		DBGLOG(HAL, INFO, "MCU off, 0x1F5014=0x%08x\n", u4RegVal[6]);
		/* MCU OFF, set dump via BT */
		fgIsMcuOff = TRUE;
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
		fgTriggerDebugSop = TRUE;
#endif
		return FALSE;
	}

#if CFG_MTK_WIFI_PCIE_SUPPORT
	/* MalfTLP */
	if (link_info & BIT(8)) {
		wlanUpdateBusAccessStatus(TRUE);
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
		fgTriggerDebugSop = TRUE;
#endif
		return FALSE;
	}
#endif

	return TRUE;
}
#endif /* CFG_SUPPORT_PCIE_ASPM */

static void mt6639_set_crypto(struct ADAPTER *prAdapter)
{
	if (!prAdapter->fgIsWiFiOnDrvOwn)
		HAL_MCR_WR(prAdapter,
			CB_INFRA_SLP_CTRL_CB_INFRA_CRYPTO_TOP_MCU_OWN_SET_ADDR,
			BIT(0));
}

static void mt6639ShowPcieDebugInfo(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4Addr, u4Val = 0;

#if CFG_MTK_WIFI_PCIE_SUPPORT
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
#endif
	u4Addr = PCIE_MAC_IREG_PCIE_DEBUG_SEL_1_ADDR;
	u4Val = 0xcccc0100;
	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4Val);
	u4Addr = PCIE_MAC_IREG_PCIE_DEBUG_SEL_0_ADDR;
	u4Val = 0x23220302;
	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4Val);
	u4Addr = PCIE_MAC_IREG_PCIE_DEBUG_MONITOR_ADDR;
	HAL_MCR_RD(prGlueInfo->prAdapter, u4Addr, &u4Val);
	DBGLOG(HAL, INFO,
	       "PCIE W[0x74030164] = [0x23220302], CR[0x%08x]=[0x%08x]",
	       u4Addr, u4Val);

	u4Addr = PCIE_MAC_IREG_PCIE_DEBUG_SEL_1_ADDR;
	u4Val = 0xcccc0100;
	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4Val);
	u4Addr = PCIE_MAC_IREG_PCIE_DEBUG_SEL_0_ADDR;
	u4Val = 0x21200100;
	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4Val);
	u4Addr = PCIE_MAC_IREG_PCIE_DEBUG_MONITOR_ADDR;
	HAL_MCR_RD(prGlueInfo->prAdapter, u4Addr, &u4Val);
	DBGLOG(HAL, INFO,
	       "PCIE W[0x74030164] = [0x21200100], CR[0x%08x]=[0x%08x]",
	       u4Addr, u4Val);

	u4Addr = PCIE_MAC_IREG_IMASK_HOST_ADDR;
	HAL_MCR_RD(prGlueInfo->prAdapter, u4Addr, &u4Val);
	DBGLOG(HAL, INFO, "CR[0x%08x]=[0x%08x]", u4Addr, u4Val);

	u4Addr = PCIE_MAC_IREG_ISTATUS_HOST_ADDR;
	HAL_MCR_RD(prGlueInfo->prAdapter, u4Addr, &u4Val);
	DBGLOG(HAL, INFO, "CR[0x%08x]=[0x%08x]", u4Addr, u4Val);

	u4Addr = 0x740310E0;
	HAL_MCR_RD(prGlueInfo->prAdapter, u4Addr, &u4Val);
	DBGLOG(HAL, INFO, "CR[0x%08x]=[0x%08x]", u4Addr, u4Val);

	u4Addr = 0x740310F0;
	HAL_MCR_RD(prGlueInfo->prAdapter, u4Addr, &u4Val);
	DBGLOG(HAL, INFO, "CR[0x%08x]=[0x%08x]", u4Addr, u4Val);

	u4Addr = 0x740310F4;
	HAL_MCR_RD(prGlueInfo->prAdapter, u4Addr, &u4Val);
	DBGLOG(HAL, INFO, "CR[0x%08x]=[0x%08x]", u4Addr, u4Val);
}

#if CFG_MTK_WIFI_DEVAPC
static void mt6639ShowDevapcDebugInfo(void)
{
	uint32_t u4Val = 0;

	HAL_MCR_RD(NULL, PCIE_MAC_IREG_IMASK_HOST_ADDR, &u4Val);
	DBGLOG(HAL, INFO, "PCIE_MAC_IREG_IMASK_HOST_ADDR[0x%08x]=[0x%08x]\n",
		PCIE_MAC_IREG_IMASK_HOST_ADDR, u4Val);
}
#endif

static void mt6639SetupMcuEmiAddr(struct ADAPTER *prAdapter)
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
		   MT6639_EMI_SIZE_ADDR,
		   size);
}

#if CFG_MTK_WIFI_SW_EMI_RING
static void mt6639triggerInt(struct GLUE_INFO *prGlueInfo)
{
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct SW_EMI_RING_INFO *prSwEmiRingInfo;

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prSwEmiRingInfo = &prBusInfo->rSwEmiRingInfo;

	HAL_MCR_WR(prGlueInfo->prAdapter,
		   prSwEmiRingInfo->u4CcifTchnumAddr,
		   prSwEmiRingInfo->u4CcifChlNum);
}
#endif /* CFG_MTK_WIFI_SW_EMI_RING */

static u_int8_t mt6639_get_sw_interrupt_status(struct ADAPTER *prAdapter,
	uint32_t *pu4Status)
{
	*pu4Status = ccif_get_interrupt_status(prAdapter);
	return TRUE;
}

static uint32_t mt6639_ccif_get_interrupt_status(struct ADAPTER *ad)
{
	uint32_t u4Status = 0;

	HAL_MCR_RD(ad,
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_RCHNUM_ADDR,
		&u4Status);
	HAL_MCR_WR(ad,
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_ACK_ADDR,
		u4Status);

	return u4Status;
}

static void mt6639_ccif_notify_utc_time_to_fw(struct ADAPTER *ad,
	uint32_t sec,
	uint32_t usec)
{
	ACQUIRE_POWER_CONTROL_FROM_PM(ad);
	if (ad->fgIsFwOwn == TRUE)
		goto exit;

	HAL_MCR_WR(ad,
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_DUMMY1_ADDR,
		sec);
	HAL_MCR_WR(ad,
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_DUMMY2_ADDR,
		usec);
	HAL_MCR_WR(ad,
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_TCHNUM_ADDR,
		SW_INT_TIME_SYNC);

exit:
	RECLAIM_POWER_CONTROL_TO_PM(ad, FALSE);
}

static void mt6639_ccif_set_fw_log_read_pointer(struct ADAPTER *ad,
	enum ENUM_FW_LOG_CTRL_TYPE type,
	uint32_t read_pointer)
{
	uint32_t u4Addr = 0;

	if (type == ENUM_FW_LOG_CTRL_TYPE_MCU)
		u4Addr = WF2AP_CONN_INFRA_ON_CCIF4_WF2AP_PCCIF_DUMMY2_ADDR;
	else
		u4Addr = WF2AP_CONN_INFRA_ON_CCIF4_WF2AP_PCCIF_DUMMY1_ADDR;

	HAL_MCR_WR(ad, u4Addr, read_pointer);
}

static uint32_t mt6639_ccif_get_fw_log_read_pointer(struct ADAPTER *ad,
	enum ENUM_FW_LOG_CTRL_TYPE type)
{
	uint32_t u4Addr = 0, u4Value = 0;

	if (type == ENUM_FW_LOG_CTRL_TYPE_MCU)
		u4Addr = WF2AP_CONN_INFRA_ON_CCIF4_WF2AP_PCCIF_DUMMY2_ADDR;
	else
		u4Addr = WF2AP_CONN_INFRA_ON_CCIF4_WF2AP_PCCIF_DUMMY1_ADDR;

	HAL_MCR_RD(ad, u4Addr, &u4Value);

	return u4Value;
}

static int32_t mt6639_ccif_trigger_fw_assert(struct ADAPTER *ad)
{
	HAL_MCR_WR(ad,
		CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_1_BA_ADDR,
		0x18051803);
	HAL_MCR_WR(ad,
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_TCHNUM_ADDR,
		SW_INT_SUBSYS_RESET);

	return 0;
}

u_int8_t mt6639_is_ap2conn_off_readable(struct ADAPTER *ad)
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
		HAL_MCR_RD(ad,
			   CONN_DBG_CTL_CONN_INFRA_BUS_CLK_DETECT_ADDR,
			   &value);
		if ((value & BIT(1)) && (value & BIT(3)))
			break;

		retry++;
		kalMdelay(1);
	}

	HAL_MCR_RD(ad,
		   CONN_CFG_IP_VERSION_IP_VERSION_ADDR,
		   &value);
	if (value != MT6639_CONNINFRA_VERSION_ID &&
	    value != MT6639_CONNINFRA_VERSION_ID_E2) {
		DBGLOG(HAL, ERROR,
			"Conninfra ver id: 0x%08x\n",
			value);
		return FALSE;
	}

	HAL_MCR_RD(ad,
		   CONN_DBG_CTL_CONN_INFRA_BUS_TIMEOUT_IRQ_ADDR,
		   &value);
	if ((value & BITS(0, 9)) != 0)
		DBGLOG(HAL, ERROR,
			"Conninfra bus hang irq status: 0x%08x\n",
			value);

	return TRUE;
}

u_int8_t mt6639_is_conn2wf_readable(struct ADAPTER *ad)
{
	uint32_t value = 0;

	HAL_MCR_RD(ad,
		   CONN_BUS_CR_ADDR_CONN2SUBSYS_0_AHB_GALS_DBG_ADDR,
		   &value);
	if ((value & BIT(26)) != 0x0) {
		DBGLOG(HAL, ERROR,
			"conn2wf sleep protect: 0x%08x\n",
			value);
		return FALSE;
	}

	HAL_MCR_RD(ad,
		   WF_TOP_CFG_IP_VERSION_ADDR,
		   &value);
	if (value != MT6639_WF_VERSION_ID) {
		DBGLOG(HAL, ERROR,
			"WF ver id: 0x%08x\n",
			value);
		return FALSE;
	}

	HAL_MCR_RD(ad,
		   CONN_DBG_CTL_WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_BUS_TIMEOUT_IRQ_ADDR,
		   &value);
	if ((value & BIT(0)) != 0x0) {
		DBGLOG(HAL, WARN,
			"WF mcusys bus hang irq status: 0x%08x\n",
			value);
		HAL_MCR_RD(ad,
			   CONN_DBG_CTL_CONN_INFRA_BUS_TIMEOUT_IRQ_ADDR,
			   &value);
		if (value == 0x100)
			DBGLOG(HAL, INFO,
				"Skip conn_infra_vdnr timeout irq.\n");
		else
			return FALSE;
	}

	return TRUE;
}

#if CFG_SUPPORT_PCIE_GEN_SWITCH
static void mt6639SetPcieSpeed(struct GLUE_INFO *prGlueInfo, uint32_t speed)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct pci_dev *pdev = NULL;
	int32_t prv = 0, ret = 0;

	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(INIT, INFO, "%s no glue info\n", __func__);
		return;
	}
	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	pdev = prHifInfo->pdev;
	prv = prBusInfo->pcie_current_speed;
	ret = mtk_pcie_speed(pdev, speed);
	if (ret) {
		prBusInfo->pcie_current_speed = speed;
		DBGLOG(INIT, INFO, "[Gen_Switch]be[%d]af[%d]\n", prv, speed);
	} else if (ret == 0) {
		DBGLOG(INIT, INFO, "[Gen_Switch]not changed[%d]\n", speed);
	}
}
#endif

#if IS_MOBILE_SEGMENT
static u_int8_t mt6639_check_recovery_needed(struct ADAPTER *ad)
{
	uint32_t u4Value = 0;
	u_int8_t fgResult = FALSE;

	/*
	 * if (0x81021604[31:16]==0xdead &&
	 *     (0x70005350[30:28]!=0x0 || 0x70005360[6:4]!=0x0)) == 0x1
	 * do recovery flow
	 */

	HAL_MCR_RD(ad, WF_TOP_CFG_ON_ROMCODE_INDEX_ADDR,
		&u4Value);
	DBGLOG(INIT, INFO, "0x%08x=0x%08x\n",
		WF_TOP_CFG_ON_ROMCODE_INDEX_ADDR, u4Value);
	if ((u4Value & 0xFFFF0000) != 0xDEAD0000) {
		fgResult = FALSE;
		goto exit;
	}

	HAL_MCR_RD(ad, CBTOP_GPIO_MODE5_ADDR,
		&u4Value);
	DBGLOG(INIT, INFO, "0x%08x=0x%08x\n",
		CBTOP_GPIO_MODE5_ADDR, u4Value);
	if (((u4Value & CBTOP_GPIO_MODE5_GPIO47_MASK) >>
	    CBTOP_GPIO_MODE5_GPIO47_SHFT) != 0x0) {
		fgResult = TRUE;
		goto exit;
	}

	HAL_MCR_RD(ad, CBTOP_GPIO_MODE6_ADDR,
		&u4Value);
	DBGLOG(INIT, INFO, "0x%08x=0x%08x\n",
		CBTOP_GPIO_MODE6_ADDR, u4Value);
	if (((u4Value & CBTOP_GPIO_MODE6_GPIO49_MASK) >>
	    CBTOP_GPIO_MODE6_GPIO49_SHFT) != 0x0) {
		fgResult = TRUE;
		goto exit;
	}

exit:
	return fgResult;
}

static uint32_t mt6639_mcu_reinit(struct ADAPTER *ad)
{
#define CONNINFRA_ID_MAX_POLLING_COUNT		10

	uint32_t u4Value = 0, u4PollingCnt = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	/* Check recovery needed */
	if (mt6639_check_recovery_needed(ad) == FALSE)
		goto exit;

	DBGLOG(INIT, INFO, "mt6639_mcu_reinit.\n");

	/* Force on conninfra */
	HAL_MCR_WR(ad,
		CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR,
		0x1);

	/* Wait conninfra wakeup */
	while (TRUE) {
		HAL_MCR_RD(ad, CONN_CFG_IP_VERSION_IP_VERSION_ADDR,
			&u4Value);

		if (u4Value == MT6639_CONNINFRA_VERSION_ID ||
		    u4Value == MT6639_CONNINFRA_VERSION_ID_E2)
			break;

		u4PollingCnt++;
		if (u4PollingCnt >= CONNINFRA_ID_MAX_POLLING_COUNT) {
			rStatus = WLAN_STATUS_FAILURE;
			DBGLOG(INIT, ERROR,
				"Conninfra ID polling failed, value=0x%x\n",
				u4Value);
			goto exit;
		}

		kalUdelay(1000);
	}

	/* Switch to GPIO mode */
	HAL_MCR_WR(ad,
		CBTOP_GPIO_MODE5_MOD_ADDR,
		0x80000000);
	HAL_MCR_WR(ad,
		CBTOP_GPIO_MODE6_MOD_ADDR,
		0x80);
	kalUdelay(100);

	/* Reset */
	HAL_MCR_WR(ad,
		CB_INFRA_RGU_BT_SUBSYS_RST_ADDR,
		0x10351);
	HAL_MCR_WR(ad,
		CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		0x10351);
	kalMdelay(10);
	HAL_MCR_WR(ad,
		CB_INFRA_RGU_BT_SUBSYS_RST_ADDR,
		0x10340);
	HAL_MCR_WR(ad,
		CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		0x10340);

	kalMdelay(50);

	HAL_MCR_RD(ad, CBTOP_GPIO_MODE5_ADDR, &u4Value);
	DBGLOG(INIT, INFO, "0x%08x=0x%08x\n",
		CBTOP_GPIO_MODE5_ADDR, u4Value);

	HAL_MCR_RD(ad, CBTOP_GPIO_MODE6_ADDR, &u4Value);
	DBGLOG(INIT, INFO, "0x%08x=0x%08x\n",
		CBTOP_GPIO_MODE6_ADDR, u4Value);

	/* Clean force on conninfra */
	HAL_MCR_WR(ad,
		CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR,
		0x0);

exit:
	return rStatus;
}

#if (CFG_MTK_ANDROID_WMT == 0)
static uint32_t mt6639_mcu_reset(struct ADAPTER *ad)
{
	uint32_t u4Value = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO, "mt6639_mcu_reset..\n");

	HAL_MCR_RD(ad,
		CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		&u4Value);
	u4Value &= ~CB_INFRA_RGU_WF_SUBSYS_RST_WF_SUBSYS_RST_MASK;
	u4Value |= (0x1 << CB_INFRA_RGU_WF_SUBSYS_RST_WF_SUBSYS_RST_SHFT);
	HAL_MCR_WR(ad,
		CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		u4Value);

	kalMdelay(1);

	HAL_MCR_RD(ad,
		CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		&u4Value);
	u4Value &= ~CB_INFRA_RGU_WF_SUBSYS_RST_WF_SUBSYS_RST_MASK;
	u4Value |= (0x0 << CB_INFRA_RGU_WF_SUBSYS_RST_WF_SUBSYS_RST_SHFT);
	HAL_MCR_WR(ad,
		CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		u4Value);

	HAL_MCR_RD(ad,
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

static void set_cbinfra_remap(struct ADAPTER *ad)
{
	DBGLOG(INIT, INFO, "set_cbinfra_remap.\n");

	HAL_MCR_WR(ad,
		CB_INFRA_MISC0_CBTOP_PCIE_REMAP_WF_ADDR,
		0x74037001);
	HAL_MCR_WR(ad,
		CB_INFRA_MISC0_CBTOP_PCIE_REMAP_WF_BT_ADDR,
		0x70007000);
}

static uint32_t mt6639_mcu_init(struct ADAPTER *ad)
{
#define MCU_IDLE		0x1D1E

	uint32_t u4Value = 0, u4PollingCnt = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct CHIP_DBG_OPS *prDbgOps = NULL;

	if (!ad) {
		DBGLOG(INIT, ERROR, "NULL ADAPTER.\n");
		rStatus = WLAN_STATUS_FAILURE;
		goto exit;
	}

	set_cbinfra_remap(ad);

#if CFG_MTK_ANDROID_WMT
	HAL_MCR_RD(ad, TOP_MISC_EFUSE_MBIST_LATCH_16_ADDR, &u4Value);
	if ((u4Value & MT6639_MEMOEY_REPAIR_CHECK_MASK) !=
		MT6639_MEMOEY_REPAIR_CHECK_MASK) {
		DBGLOG(INIT, ERROR,
			"Unexpected memory repair pattern\n");
		rStatus = WLAN_STATUS_FAILURE;
		goto exit;
	}
#endif

	rStatus = mt6639_mcu_reinit(ad);
	if (rStatus != WLAN_STATUS_SUCCESS)
		goto dump;

#if (CFG_MTK_ANDROID_WMT == 0)
	rStatus = mt6639_mcu_reset(ad);
	if (rStatus != WLAN_STATUS_SUCCESS)
		goto dump;
#endif

	HAL_MCR_WR(ad,
		   CB_INFRA_SLP_CTRL_CB_INFRA_CRYPTO_TOP_MCU_OWN_SET_ADDR,
		   BIT(0));

	while (TRUE) {
		if (u4PollingCnt >= 1000) {
			DBGLOG(INIT, ERROR, "timeout.\n");
			rStatus = WLAN_STATUS_FAILURE;
			goto dump;
		}

		HAL_MCR_RD(ad, WF_TOP_CFG_ON_ROMCODE_INDEX_ADDR,
			&u4Value);
		if (u4Value == MCU_IDLE)
			break;

		u4PollingCnt++;
		kalUdelay(1000);
	}

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (connv3_ext_32k_on()) {
		DBGLOG(INIT, ERROR, "connv3_ext_32k_on failed.\n");
		rStatus = WLAN_STATUS_FAILURE;
		goto dump;
	}
#endif

	/* setup 1a/1b remapping for mcif interrupt */
	HAL_MCR_WR(ad,
		   CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_1_BA_ADDR,
		   0x18051803);

	if (ad->chip_info->coexpccifon)
		ad->chip_info->coexpccifon(ad);
#if CFG_SUPPORT_PCIE_ASPM
	pcie_vir_addr = ioremap(0x112f0000, 0x2000);
	spin_lock_init(&rPCIELock);
#endif
dump:
	if (rStatus != WLAN_STATUS_SUCCESS) {
		WARN_ON_ONCE(TRUE);
		DBGLOG(INIT, ERROR, "u4Value: 0x%x\n",
			u4Value);
		WARN_ON_ONCE(TRUE);

		prChipInfo = ad->chip_info;
		prDbgOps = prChipInfo->prDebugOps;
		if (prDbgOps && prDbgOps->dumpBusHangCr)
			prDbgOps->dumpBusHangCr(ad);

		/* Clock detection for ULPOSC */
		HAL_MCR_WR(ad,
			   VLP_UDS_CTRL_CBTOP_ULPOSC_CTRL1_ADDR,
			   0x06030138);
		HAL_MCR_WR(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			   0x000f0000);
		HAL_MCR_WR(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			   0x001f0000);
		kalUdelay(10);
		HAL_MCR_WR(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			   0x011f0000);
		kalUdelay(10);
		HAL_MCR_RD(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_2_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_CKGEN_TOP_CBTOP_ULPOSC_2_ADDR,
			u4Value);
		HAL_MCR_RD(ad,
			   CB_INFRA_SLP_CTRL_CB_INFRA_CRYPTO_TOP_MCU_OWN_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_INFRA_SLP_CTRL_CB_INFRA_CRYPTO_TOP_MCU_OWN_ADDR,
			u4Value);

		HAL_MCR_RD(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_0_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_CKGEN_TOP_CBTOP_ULPOSC_0_ADDR,
			u4Value);
		HAL_MCR_RD(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			u4Value);
		HAL_MCR_RD(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_2_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_CKGEN_TOP_CBTOP_ULPOSC_2_ADDR,
			u4Value);
		HAL_MCR_RD(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_3_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_CKGEN_TOP_CBTOP_ULPOSC_3_ADDR,
			u4Value);
		HAL_MCR_RD(ad,
			   VLP_UDS_CTRL_CBTOP_ULPOSC_CTRL0_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			VLP_UDS_CTRL_CBTOP_ULPOSC_CTRL0_ADDR,
			u4Value);
		HAL_MCR_RD(ad,
			   VLP_UDS_CTRL_CBTOP_ULPOSC_CTRL1_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			VLP_UDS_CTRL_CBTOP_ULPOSC_CTRL1_ADDR,
			u4Value);
		HAL_MCR_RD(ad,
			   VLP_UDS_CTRL_CBTOP_UDS_RSV_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			VLP_UDS_CTRL_CBTOP_UDS_RSV_ADDR,
			u4Value);

		HAL_MCR_WR(ad,
			   EEF_TOP_EFUSE_CTRL_ADDR,
			   0x41B00000);
		kalUdelay(100);
		HAL_MCR_RD(ad,
			   EEF_TOP_EFUSE_RDATA0_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			EEF_TOP_EFUSE_RDATA0_ADDR,
			u4Value);
		HAL_MCR_RD(ad,
			   EEF_TOP_EFUSE_RDATA1_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			EEF_TOP_EFUSE_RDATA1_ADDR,
			u4Value);

		HAL_MCR_WR(ad,
			   VLP_UDS_CTRL_CBTOP_ULPOSC_CTRL1_ADDR,
			   0x06030138);
		HAL_MCR_WR(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			   0x000f0000);
		HAL_MCR_WR(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			   0x001f0000);
		kalUdelay(10);
		HAL_MCR_WR(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			   0x011f0000);
		kalUdelay(10);
		HAL_MCR_RD(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_2_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_CKGEN_TOP_CBTOP_ULPOSC_2_ADDR,
			u4Value);

		HAL_MCR_RD(ad,
			   CB_INFRA_MISC0_CBTOP_FREQ_METER_DET_CTL_ADDR,
			   &u4Value);
		u4Value &= ~BIT(3);
		HAL_MCR_WR(ad,
			   CB_INFRA_MISC0_CBTOP_FREQ_METER_DET_CTL_ADDR,
			   u4Value);
		HAL_MCR_WR(ad,
			   CB_INFRA_MISC0_CBTOP_FREQ_METER_DET_CTL_ADDR,
			   0xBC);
		kalMdelay(1);
		HAL_MCR_RD(ad,
			   CB_INFRA_MISC0_CBTOP_FREQ_METER_STATUS_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_INFRA_MISC0_CBTOP_FREQ_METER_STATUS_ADDR,
			u4Value);

		HAL_MCR_WR(ad,
			   VLP_UDS_CTRL_CBTOP_ULPOSC_CTRL1_ADDR,
			   0x06030138);
		HAL_MCR_WR(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			   0x000f0000);
		HAL_MCR_WR(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			   0x001f0000);
		kalUdelay(10);
		HAL_MCR_WR(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			   0x011f0000);
		kalUdelay(10);
		HAL_MCR_RD(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_2_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_CKGEN_TOP_CBTOP_ULPOSC_2_ADDR,
			u4Value);

		HAL_MCR_RD(ad,
			   CB_INFRA_MISC0_CBTOP_FREQ_METER_DET_CTL_ADDR,
			   &u4Value);
		u4Value &= ~BIT(3);
		HAL_MCR_WR(ad,
			   CB_INFRA_MISC0_CBTOP_FREQ_METER_DET_CTL_ADDR,
			   u4Value);
		kalUdelay(150);
		HAL_MCR_WR(ad,
			   CB_INFRA_MISC0_CBTOP_FREQ_METER_DET_CTL_ADDR,
			   0x1C);
		kalMdelay(1);
		HAL_MCR_RD(ad,
			   CB_INFRA_MISC0_CBTOP_FREQ_METER_STATUS_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_INFRA_MISC0_CBTOP_FREQ_METER_STATUS_ADDR,
			u4Value);
	}

exit:
	return rStatus;
}

static void mt6639_mcu_deinit(struct ADAPTER *ad)
{
#define MAX_WAIT_COREDUMP_COUNT 10

	int retry = 0;

	while (is_wifi_coredump_processing()
#if CFG_MTK_ANDROID_WMT
		&& !kalGetShutdownState()
#endif
		) {
		if (retry >= MAX_WAIT_COREDUMP_COUNT) {
			DBGLOG(INIT, WARN,
				"Coredump spend long time, retry = %d\n",
				retry);
			break;
		}
		kalMsleep(100);
		retry++;
	}
	wifi_coredump_set_enable(FALSE);

	if (retry >= MAX_WAIT_COREDUMP_COUNT)
		kalSendAeeWarning("WLAN", "Coredump Wait Locked\n");

	if (ad->chip_info->coexpccifoff)
		ad->chip_info->coexpccifoff(ad);

#if CFG_SUPPORT_PCIE_ASPM
	if (pcie_vir_addr)
		iounmap(pcie_vir_addr);
#endif
}

static int32_t mt6639_trigger_fw_assert(struct ADAPTER *prAdapter)
{
	int32_t ret = 0;

	ccif_trigger_fw_assert(prAdapter);

#if CFG_WMT_RESET_API_SUPPORT
	ret = reset_wait_for_trigger_completion();
#endif

	return ret;
}
#endif /* IS_MOBILE_SEGMENT */

#if IS_MOBILE_SEGMENT || (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
#define MCIF_EMI_MEMORY_SIZE 128
#define MCIF_EMI_COEX_SWMSG_OFFSET 0xF8518000
#define MCIF_EMI_BASE_OFFSET 0xE4
static int mt6639ConnacPccifOn(struct ADAPTER *prAdapter)
{
#if CFG_MTK_CCCI_SUPPORT && CFG_MTK_MDDP_SUPPORT
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	uint32_t ccif_base = 0x1a0000, pcie2ap_base = 0x1b0000;
	uint32_t mcif_emi_base, u4Val = 0, u4WifiEmi = 0;
	void *vir_addr = NULL;
	int ret = 0;

#if CFG_MTK_ANDROID_WMT
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (is_pwr_on_notify_processing())
		return -1;
#endif
#endif
	ccif_base += (uint32_t)(prChipInfo->u8CsrOffset);
	pcie2ap_base += (uint32_t)(prChipInfo->u8CsrOffset);
	mcif_emi_base = get_smem_phy_start_addr(
		MD_SYS1, SMEM_USER_RAW_MD_CONSYS, &ret);
	if (!mcif_emi_base) {
		DBGLOG(INIT, ERROR, "share memory is NULL.\n");
		return -1;
	}

	vir_addr = ioremap(mcif_emi_base, MCIF_EMI_MEMORY_SIZE);
	if (!vir_addr) {
		DBGLOG(INIT, ERROR, "ioremap fail.\n");
		return -1;
	}

#if CFG_SUPPORT_WIFI_MCIF_NO_MMIO_READ
	u4WifiEmi = (uint32_t)emi_mem_get_phy_base(prAdapter->chip_info) +
		emi_mem_offset_convert(0x518001);
#endif

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
	mddpStartMdRxThread();
	kalDevRegWrite(
		prAdapter->prGlueInfo,
		CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_1_BA_ADDR,
		0x18051803);
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */

	kalMemSetIo(vir_addr, 0xFF, MCIF_EMI_MEMORY_SIZE);
	writel(0x4D4D434D, vir_addr);
	writel(0x4D4D434D, vir_addr + 0x4);
	writel(0x00000000, vir_addr + 0x8);
	writel(0x00000000, vir_addr + 0xC);
#if CFG_SUPPORT_WIFI_MCIF_NO_MMIO_READ
	writel(u4WifiEmi, vir_addr + 0x10);
	writel(0x02000080, vir_addr + 0x14);
#else
	writel(pcie2ap_base + 0x5801, vir_addr + 0x10);
	writel(0x02000010, vir_addr + 0x14);
#endif
	writel(ccif_base + 0xF00C, vir_addr + 0x18);
	writel(0x00000001, vir_addr + 0x1C);
	writel(0x00000000, vir_addr + 0x70);
	writel(0x00000000, vir_addr + 0x74);
	writel(0x4D434D4D, vir_addr + 0x78);
	writel(0x4D434D4D, vir_addr + 0x7C);

	u4Val = readl(vir_addr + MCIF_EMI_BASE_OFFSET);

	if (mddpIsSupportMcifWifi()) {
		HAL_MCR_WR(prAdapter,
			   MT6639_MCIF_MD_STATE_WHEN_WIFI_ON_ADDR,
			   u4Val);
	}

	DBGLOG(INIT, TRACE, "MCIF_EMI_BASE_OFFSET=[0x%08x] WIFI EMI=[0x%08x]\n",
	       u4Val, u4WifiEmi);
	DBGLOG_MEM128(HAL, TRACE, vir_addr, MCIF_EMI_MEMORY_SIZE);

	iounmap(vir_addr);
#else
	DBGLOG(INIT, ERROR, "[%s] ECCCI Driver is not supported.\n", __func__);
#endif
	return 0;
}

static int mt6639ConnacPccifOff(struct ADAPTER *prAdapter)
{
#if CFG_MTK_CCCI_SUPPORT && CFG_MTK_MDDP_SUPPORT
	uint32_t mcif_emi_base;
	void *vir_addr = NULL;
	int ret = 0;

	mcif_emi_base =	get_smem_phy_start_addr(
		MD_SYS1, SMEM_USER_RAW_MD_CONSYS, &ret);
	if (!mcif_emi_base) {
		DBGLOG(INIT, ERROR, "share memory is NULL.\n");
		return -1;
	}

	vir_addr = ioremap(mcif_emi_base, MCIF_EMI_MEMORY_SIZE);
	if (!vir_addr) {
		DBGLOG(INIT, ERROR, "ioremap fail.\n");
		return -1;
	}

	writel(0, vir_addr + 0x10);
	writel(0, vir_addr + 0x14);
	writel(0, vir_addr + 0x18);
	writel(0, vir_addr + 0x1C);

	iounmap(vir_addr);
#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
	mddpStopMdRxThread();
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */
#else /* CFG_MTK_CCCI_SUPPORT && CFG_MTK_MDDP_SUPPORT */
	DBGLOG(INIT, ERROR, "[%s] ECCCI Driver is not supported.\n", __func__);
#endif /* CFG_MTK_CCCI_SUPPORT && CFG_MTK_MDDP_SUPPORT */
	return 0;
}
#endif /* IS_MOBILE_SEGMENT || (CFG_MTK_SUPPORT_LIGHT_MDDP == 1) */

#if IS_MOBILE_SEGMENT
static int mt6639_CheckBusHang(void *priv, uint8_t rst_enable)
{
	struct ADAPTER *ad = priv;
	struct mt66xx_chip_info *chip_info = NULL;
	struct CHIP_DBG_OPS *debug_ops = NULL;
	u_int8_t readable = FALSE;

	if (fgIsBusAccessFailed) {
		readable = FALSE;
		goto exit;
	}

	chip_info = ad->chip_info;
	debug_ops = chip_info->prDebugOps;

	if (debug_ops && debug_ops->dumpPcieStatus)
		readable = debug_ops->dumpPcieStatus(ad->prGlueInfo);

	if (readable == FALSE)
		goto exit;

	if (mt6639_is_ap2conn_off_readable(ad) &&
	    mt6639_is_conn2wf_readable(ad))
		readable = TRUE;
	else
		readable = FALSE;

exit:
	return readable ? 0 : 1;
}

static void mt6639_CheckMcuOff(struct ADAPTER *ad)
{
	uint32_t u4RegVal = 0;

	if (!ad) {
		DBGLOG(HAL, ERROR, "prAdapter is NULL\n");
		return;
	}

	HAL_MCR_RD(ad, 0x1F5014, &u4RegVal);

	if ((u4RegVal & BITS(12, 13)) == BITS(12, 13)) {
		DBGLOG(HAL, INFO, "MCU off, 0x1F5014=0x%08x\n", u4RegVal);
		/* block pcie to prevent access */
#if CFG_MTK_WIFI_PCIE_SUPPORT
		mtk_pcie_disable_data_trans(0);
#endif
	}
}

static uint32_t mt6639_wlanDownloadPatch(struct ADAPTER *prAdapter)
{
	uint32_t status  = wlanDownloadPatch(prAdapter);

	if (status == WLAN_STATUS_SUCCESS)
		wifi_coredump_set_enable(TRUE);

	return status;
}
#endif /* IS_MOBILE_SEGMENT */

#if CFG_PCIE_LTR_UPDATE
uint8_t g_ucLTRStat;

static void mt6639PcieLTRValue(struct ADAPTER *prAdapter, uint8_t ucState)
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

static uint32_t mt6639GetFlavorVer(uint8_t *flavor)
{
	uint32_t ret = WLAN_STATUS_FAILURE;
	uint32_t u4StrLen = 0;

	if (IS_MOBILE_SEGMENT) {
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
	} else if (kalScnprintf(flavor,
				CFG_FW_FLAVOR_MAX_LEN,
				"2") > 0) {
		ret = WLAN_STATUS_SUCCESS;
	} else {
		ret = WLAN_STATUS_FAILURE;
	}

	return ret;
}

#if CFG_SUPPORT_WIFI_SLEEP_COUNT
int mt6639PowerDumpStart(void *priv_data, unsigned int force_dump)
{
	struct GLUE_INFO *glue = priv_data;
	struct ADAPTER *ad = glue->prAdapter;
	uint32_t u4Val = 0;

	if (ad == NULL) {
		DBGLOG(REQ, ERROR, "prAdapter is NULL!\n");
		return 1;
	}

	if (wlanIsChipNoAck(ad)) {
		DBGLOG(REQ, ERROR,
			"Chip reset and chip no response.\n");
		return 1;
	}

	if (pcie_vir_addr && glue->u4ReadyFlag)
		u4Val = readl(pcie_vir_addr + 0x150);
	else
		return 1;

	if (force_dump == TRUE) {
		DBGLOG(REQ, INFO, "wlan_power_dump_start force_dump\n");

		ad->fgIsPowerDumpDrvOwn = TRUE;
		/* ACQUIRE_POWER_CONTROL_FROM_PM(ad); */
		halSetDriverOwn(ad, DRV_OWN_SRC_POWER_DUMP);
		if (ad->fgWiFiInSleepyState == TRUE)
			ad->fgWiFiInSleepyState = FALSE;
		ad->fgIsPowerDumpDrvOwn = FALSE;

		if (ad->fgIsFwOwn == TRUE) {
			DBGLOG(REQ, ERROR,
				"wlan_power_dump_start end: driver own fail!\n");
			return 1;
		}

		return 0;

	} else {
		u4Val = ((u4Val & 0x1F000000) >> 24);
		DBGLOG(REQ, INFO,
			"wlan_power_dump_start PCIE status: 0x%08x\n", u4Val);

		if (u4Val == 0x10) {
			ad->fgIsPowerDumpDrvOwn = TRUE;
			/* ACQUIRE_POWER_CONTROL_FROM_PM(ad); */
			halSetDriverOwn(ad, DRV_OWN_SRC_POWER_DUMP);
			if (ad->fgWiFiInSleepyState == TRUE)
				ad->fgWiFiInSleepyState = FALSE;
			ad->fgIsPowerDumpDrvOwn = FALSE;

			if (ad->fgIsFwOwn == TRUE) {
				DBGLOG(REQ, ERROR,
					"wlan_power_dump_start end: driver own fail!\n");
				return 1;
			}
			return 0;
		}

		return 1;
	}

}

int mt6639PowerDumpEnd(void *priv_data)
{
	struct GLUE_INFO *glue = priv_data;
	struct ADAPTER *ad = glue->prAdapter;

	if (ad == NULL) {
		DBGLOG(REQ, ERROR, "prAdapter is NULL!\n");
		return 0;
	}

	if (ad->fgIsFwOwn == FALSE && glue->u4ReadyFlag)
		RECLAIM_POWER_CONTROL_TO_PM(ad, FALSE);

	return 0;
}
#endif  /* CFG_SUPPORT_WIFI_SLEEP_COUNT */
#if (CFG_SUPPORT_APS == 1)
uint8_t mt6639_apsLinkPlanDecision(struct ADAPTER *prAdapter,
	struct AP_COLLECTION *prAp, enum ENUM_MLO_LINK_PLAN eLinkPlan,
	uint8_t ucBssIndex)
{
	uint32_t u4TmpLinkPlanBmap;
	uint32_t u4LinkPlanBmap =
		BIT(MLO_LINK_PLAN_2_5)
#if (CFG_SUPPORT_WIFI_6G == 1)
		| BIT(MLO_LINK_PLAN_2_6)
#endif
	;
#if (CFG_SUPPORT_DUAL_SAP_SINGLE_LINK_MLO == 1)
	uint32_t u4LinkPlanBmapSingleLink =
		BIT(MLO_LINK_PLAN_2)
		| BIT(MLO_LINK_PLAN_5)
#if (CFG_SUPPORT_WIFI_6G == 1)
		| BIT(MLO_LINK_PLAN_6)
#endif
	;
#endif

	u4TmpLinkPlanBmap = u4LinkPlanBmap;

#if (CFG_SUPPORT_DUAL_SAP_SINGLE_LINK_MLO == 1)
	if (p2pFuncIsDualAPActive(prAdapter))
		u4TmpLinkPlanBmap = u4LinkPlanBmapSingleLink;
#endif

	return !!(u4TmpLinkPlanBmap & BIT(eLinkPlan));
}
#endif /* CFG_SUPPORT_APS */
#endif  /* MT6639 */
