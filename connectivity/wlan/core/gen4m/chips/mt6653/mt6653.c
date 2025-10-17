// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   mt6653.c
*    \brief  Internal driver stack will export
*    the required procedures here for GLUE Layer.
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifdef MT6653

#include "precomp.h"
#include "mt6653.h"
#include "coda/mt6653/cb_ckgen_top.h"
#include "coda/mt6653/cb_infra_misc0.h"
#include "coda/mt6653/cb_infra_rgu.h"
#include "coda/mt6653/cb_infra_slp_ctrl.h"
#include "coda/mt6653/cbtop_gpio_sw_def.h"
#include "coda/mt6653/conn_bus_cr.h"
#include "coda/mt6653/conn_cfg.h"
#include "coda/mt6653/conn_dbg_ctl.h"
#include "coda/mt6653/conn_host_csr_top.h"
#include "coda/mt6653/conn_semaphore.h"
#include "coda/mt6653/wf_cr_sw_def.h"
#include "coda/mt6653/wf_top_cfg.h"
#include "coda/mt6653/wf_wfdma_ext_wrap_csr.h"
#include "coda/mt6653/wf_wfdma_host_dma0.h"
#include "coda/mt6653/wf_wfdma_mcu_dma0.h"
#include "coda/mt6653/wf_hif_dmashdl_top.h"
#include "coda/mt6653/wf_pse_top.h"
#include "coda/mt6653/pcie_mac_ireg.h"
#include "coda/mt6653/conn_mcu_bus_cr.h"
#include "coda/mt6653/conn_bus_cr_von.h"
#include "coda/mt6653/conn_host_csr_top.h"
#include "coda/mt6653/vlp_uds_ctrl.h"
#include "coda/mt6653/mawd_reg.h"
#include "coda/mt6653/wf_rro_top.h"
#include "coda/mt6653/wf_top_cfg_on.h"
#include "hal_dmashdl_mt6653.h"
#include "coda/mt6653/wf2ap_conn_infra_on_ccif4.h"
#include "coda/mt6653/ap2wf_conn_infra_on_ccif4.h"
#include "coda/mt6653/wf_top_cfg_on.h"
#include "coda/mt6653/wf_wtblon_top.h"
#include "coda/mt6653/wf_uwtbl_top.h"
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
#include "connv3.h"
#endif
#if CFG_MTK_WIFI_FW_LOG_MMIO
#include "fw_log_mmio.h"
#endif
#if CFG_MTK_WIFI_FW_LOG_EMI
#include "fw_log_emi.h"
#endif

#include "wlan_pinctrl.h"

#if CFG_MTK_MDDP_SUPPORT
#include "mddp_export.h"
#include "mddp.h"
#endif

#if CFG_MTK_CCCI_SUPPORT
#include "mtk_ccci_common.h"
#endif

#include "gl_coredump.h"

#if (CFG_SUPPORT_CONNFEM == 1)
#include "connfem_api.h"
#endif

#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
#include <linux/msi.h>
#endif /* CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE */

#include "wlan_hw_dbg.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
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
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static uint32_t mt6653GetFlavorVer(uint8_t *flavor);

static void mt6653_ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx);

static void mt6653_ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);

#if CFG_MTK_WIFI_SUPPORT_DSP_FWDL
static void mt6653_ConstructDspName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);
#endif

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
static void mt6653_ConstructIdxLogBinName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName);
#endif

#if defined(_HIF_PCIE)
static uint8_t mt6653SetRxRingHwAddr(struct RTMP_RX_RING *prRxRing,
		struct BUS_INFO *prBusInfo, uint32_t u4SwRingIdx);

static bool mt6653WfdmaAllocRxRing(struct GLUE_INFO *prGlueInfo,
		bool fgAllocMem);

static void mt6653ProcessTxInterrupt(
		struct ADAPTER *prAdapter);

static void mt6653ProcessRxInterrupt(
	struct ADAPTER *prAdapter);

static void mt6653WfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo);

static void mt6653ReadIntStatusByMsi(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus);

static void mt6653ReadIntStatus(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus);

#if (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
static void mt6653EnableInterruptViaPcie(struct ADAPTER *prAdapter);
#endif
static void mt6653EnableInterrupt(struct ADAPTER *prAdapter);
static void mt6653DisableInterrupt(struct ADAPTER *prAdapter);

static void mt6653ConfigIntMask(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable);

static void mt6653ConfigWfdmaRxRingThreshold(
	struct ADAPTER *prAdapter, uint32_t u4Num, u_int8_t fgIsData);

static void mt6653UpdateWfdmaPrdcInt(
	struct GLUE_INFO *prGlueInfo, u_int8_t fgForceEn);

static void mt6653WpdmaConfig(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable, bool fgResetHif);

#if CFG_MTK_WIFI_WFDMA_WB
static u_int8_t mt6653IsWfdmaRxReady(struct ADAPTER *prAdapter);
static void mt6653ProcessTxInterruptByEmi(struct ADAPTER *prAdapter);
static void mt6653ProcessRxInterruptByEmi(struct ADAPTER *prAdapter);
static void mt6653ReadIntStatusByEmi(struct ADAPTER *prAdapter,
				     uint32_t *pu4IntStatus);
static void mt6653ConfigEmiIntMask(struct GLUE_INFO *prGlueInfo,
				   u_int8_t enable);
static void mt6653EnableInterruptViaPcieByEmi(struct ADAPTER *prAdapter);
#endif /* CFG_MTK_WIFI_WFDMA_WB */

static void mt6653SetupMcuEmiAddr(struct ADAPTER *prAdapter);

static void mt6653WfdmaTxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *prTxRing,
	u_int32_t index);
static void mt6653WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	u_int32_t index);

static void mt6653CheckFwOwnMsiStatus(struct ADAPTER *prAdapter);
static void mt6653PcieMsiDebugDump(struct ADAPTER *prAdapter);
static void mt6653RecoveryMsiStatus(struct ADAPTER *prAdapter,
				    u_int8_t fgForce);
static void mt6653RecoverSerStatus(struct ADAPTER *prAdapter);

static void mt6653InitPcieInt(struct GLUE_INFO *prGlueInfo);

static void mt6653PcieHwControlVote(
	struct ADAPTER *prAdapter,
	uint8_t enable,
	uint32_t u4WifiUser);

#if CFG_SUPPORT_PCIE_ASPM
static u_int8_t mt6653DumpPcieDateFlowStatus(struct GLUE_INFO *prGlueInfo);
#endif
static void mt6653ShowPcieDebugInfo(struct GLUE_INFO *prGlueInfo);

#if CFG_SUPPORT_PCIE_ASPM
static u_int8_t mt6653SetL1ssEnable(struct ADAPTER *prAdapter, u_int role,
		u_int8_t fgEn);
static uint32_t mt6653ConfigPcieAspm(struct GLUE_INFO *prGlueInfo,
	u_int8_t fgEn, u_int enable_role);
static void mt6653UpdatePcieAspm(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn);
static void mt6653KeepPcieWakeup(struct GLUE_INFO *prGlueInfo,
	u_int8_t fgWakeup);
#endif

static u_int8_t mt6653_get_sw_interrupt_status(struct ADAPTER *prAdapter,
	uint32_t *pu4Status);

static void mt6653_ccif_notify_utc_time_to_fw(struct ADAPTER *ad,
	uint32_t sec,
	uint32_t usec);
static uint32_t mt6653_ccif_get_interrupt_status(struct ADAPTER *ad);
static void mt6653_ccif_set_fw_log_read_pointer(struct ADAPTER *ad,
	enum ENUM_FW_LOG_CTRL_TYPE type,
	uint32_t read_pointer);
static uint32_t mt6653_ccif_get_fw_log_read_pointer(struct ADAPTER *ad,
	enum ENUM_FW_LOG_CTRL_TYPE type);
static int32_t mt6653_ccif_trigger_fw_assert(struct ADAPTER *ad);

static int32_t mt6653_trigger_fw_assert(struct ADAPTER *prAdapter);
static uint32_t mt6653_mcu_init(struct ADAPTER *ad);
static void mt6653_mcu_deinit(struct ADAPTER *ad);
static int mt6653ConnacPccifOn(struct ADAPTER *prAdapter);
static int mt6653ConnacPccifOff(struct ADAPTER *prAdapter);
static int mt6653_CheckBusHang(void *priv, uint8_t rst_enable);
static uint32_t mt6653_wlanDownloadPatch(struct ADAPTER *prAdapter);
static void mt6653WiFiNappingCtrl(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn);
#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
static void mt6653PcieMsiMaskIrq(uint32_t u4Irq, uint32_t u4Bit);
static void mt6653PcieMsiUnmaskIrq(uint32_t u4Irq, uint32_t u4Bit);
#endif /* CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE */
#endif

static u_int8_t mt6653_isUpgradeWholeChipReset(struct ADAPTER *prAdapter);

#if (CFG_SUPPORT_APS == 1)
static uint8_t mt6653_apsLinkPlanDecision(struct ADAPTER *prAdapter,
	struct AP_COLLECTION *prAp, enum ENUM_MLO_LINK_PLAN eLinkPlan,
	uint8_t ucBssIndex);
static void mt6653_apsUpdateTotalScore(struct ADAPTER *prAdapter,
	struct BSS_DESC *arLinks[], uint8_t ucLinkNum,
	enum ENUM_MLO_LINK_PLAN eCurrPlan, struct AP_COLLECTION *prAp,
	uint8_t ucBssidx);
static void mt6653_apsFillBssDescSet(struct ADAPTER *prAdapter,
		struct BSS_DESC_SET *prSet,
		uint8_t ucBssidx);
#endif

static void mt6653LowPowerOwnInit(struct ADAPTER *prAdapter);
static void mt6653LowPowerOwnRead(struct ADAPTER *prAdapter,
				  u_int8_t *pfgResult);
static void mt6653LowPowerOwnSet(struct ADAPTER *prAdapter,
				 u_int8_t *pfgResult);
static void mt6653LowPowerOwnClear(struct ADAPTER *prAdapter,
				   u_int8_t *pfgResult);
#if CFG_MTK_WIFI_MBU
static void mt6653MbuDumpDebugCr(struct GLUE_INFO *prGlueInfo);
#endif

#if CFG_MTK_MDDP_SUPPORT
static void mt6653CheckMdRxHang(struct ADAPTER *prAdapter);
#endif

#if defined(_HIF_PCIE)
static void mt6653_notify_fw_disable_SR(struct ADAPTER *prAdapter);
#endif

#if (CFG_SUPPORT_CONNFEM == 1)
u_int8_t mt6653_is_AA_DBDC_enable(void);
static u_int8_t mt6653_is_support_band2(void);
#endif
#if CFG_MTK_WIFI_PCIE_SR
u_int8_t fgIsL2Finished = FALSE;
#endif

#if CFG_SUPPORT_PCIE_ASPM
void *pcie_vir_addr;
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct ECO_INFO mt6653_eco_table[] = {
	/* HW version,  ROM version,    Factory version */
	{0x00, 0x00, 0xA, 0x1},	/* E1 */
	{0x00, 0x00, 0x0, 0x0}	/* End of table */
};

uint8_t *apucmt6653FwName[] = {
	(uint8_t *) CFG_FW_FILENAME "_6653",
	NULL
};

#if defined(_HIF_PCIE)
struct PCIE_CHIP_CR_MAPPING mt6653_bus2chip_cr_mapping[] = {
	/* chip addr, bus addr, range */
	{0x830c0000, 0x00000, 0x1000}, /* WF_MCU_BUS_CR_REMAP */
	{0x54000000, 0x02000, 0x1000}, /* WFDMA_0 (PCIE0 MCU DMA0) */
	{0x55000000, 0x03000, 0x1000}, /* WFDMA_1 (PCIE0 MCU DMA1) */
	{0x56000000, 0x04000, 0x1000}, /* WFDMA_2 (Reserved) */
	{0x57000000, 0x05000, 0x1000}, /* WFDMA_3 (MCU wrap CR) */
	{0x58000000, 0x06000, 0x1000}, /* WFDMA_4 (PCIE1 MCU DMA0 (MEM_DMA)) */
	{0x59000000, 0x07000, 0x1000}, /* WFDMA_5 (PCIE1 MCU DMA1) */
	{0x820c0000, 0x08000, 0x4000}, /* WF_UMAC_TOP (PLE)  */
	{0x820c8000, 0x0c000, 0x2000}, /* WF_UMAC_TOP (PSE) */
	{0x820cc000, 0x0e000, 0x2000}, /* WF_UMAC_TOP (PP) */
	{0x83000000, 0x110000, 0x10000}, /* WF_PHY_MAP3 */
	{0x820e0000, 0x20000, 0x0400}, /* WF_LMAC_TOP (WF_CFG)  */
	{0x820e1000, 0x20400, 0x0200}, /* WF_LMAC_TOP (WF_TRB)  */
	{0x820e2000, 0x20800, 0x0400}, /* WF_LMAC_TOP (WF_AGG)  */
	{0x820e3000, 0x20c00, 0x0400}, /* WF_LMAC_TOP (WF_ARB)  */
	{0x820e4000, 0x21000, 0x0400}, /* WF_LMAC_TOP (WF_TMAC)  */
	{0x820e5000, 0x21400, 0x0800}, /* WF_LMAC_TOP (WF_RMAC)  */
	{0x820ce000, 0x21c00, 0x0200}, /* WF_LMAC_TOP (WF_SEC)  */
	{0x820e7000, 0x21e00, 0x0200}, /* WF_LMAC_TOP (WF_DMA)  */
	{0x820cf000, 0x22000, 0x1000}, /* WF_LMAC_TOP (WF_PF)  */
	{0x820e9000, 0x23400, 0x0200}, /* WF_LMAC_TOP (WF_WTBLOFF)  */
	{0x820ea000, 0x24000, 0x0200}, /* WF_LMAC_TOP (WF_ETBF)  */
	{0x820eb000, 0x24200, 0x0400}, /* WF_LMAC_TOP (WF_LPON)  */
	{0x820ec000, 0x24600, 0x0200}, /* WF_LMAC_TOP (WF_INT)  */
	{0x820ed000, 0x24800, 0x0800}, /* WF_LMAC_TOP (WF_MIB)  */
	{0x820ca000, 0x26000, 0x2000}, /* WF_LMAC_TOP (WF_MUCOP)  */
	{0x820d0000, 0x30000, 0x10000}, /* WF_LMAC_TOP (WF_WTBLON )  */
	{0x830a0000, 0x40000, 0x10000}, /* WF_PHY_MAP0 */
	{0x83080000, 0x50000, 0x10000}, /* WF_PHY_MAP1 */
	{0x83090000, 0x60000, 0x10000}, /* WF_PHY_MAP2 */
	{0xe0400000, 0x70000, 0x10000}, /* WF_UMCA_SYSRAM */
	{0x00400000, 0x80000, 0x10000}, /* WF_MCU_SYSRAM */
	{0x00410000, 0x90000, 0x10000}, /* WF_MCU_SYSRAM (Common driver usage only)  */
	{0x820f0000, 0xa0000, 0x0400}, /* WF_LMAC_TOP (WF_CFG)  */
	{0x820f1000, 0xa0600, 0x0200}, /* WF_LMAC_TOP (WF_TRB)  */
	{0x820f2000, 0xa0800, 0x0400}, /* WF_LMAC_TOP (WF_AGG)  */
	{0x820f3000, 0xa0c00, 0x0400}, /* WF_LMAC_TOP (WF_ARB)  */
	{0x820f4000, 0xa1000, 0x0400}, /* WF_LMAC_TOP (WF_TMAC)  */
	{0x820f5000, 0xa1400, 0x0800}, /* WF_LMAC_TOP (WF_RMAC)  */
	{0x820ce000, 0xa1c00, 0x0200}, /* WF_LMAC_TOP (WF_SEC)  */
	{0x820f7000, 0xa1e00, 0x0200}, /* WF_LMAC_TOP (WF_DMA)  */
	{0x820cf000, 0xa2000, 0x1000}, /* WF_LMAC_TOP (WF_PF)  */
	{0x820f9000, 0xa3400, 0x0200}, /* WF_LMAC_TOP (WF_WTBLOFF)  */
	{0x820fa000, 0xa4000, 0x0200}, /* WF_LMAC_TOP (WF_ETBF)  */
	{0x820fb000, 0xa4200, 0x0400}, /* WF_LMAC_TOP (WF_LPON)  */
	{0x820fc000, 0xa4600, 0x0200}, /* WF_LMAC_TOP (WF_INT)  */
	{0x820fd000, 0xa4800, 0x0800}, /* WF_LMAC_TOP (WF_MIB)  */
	{0x820cc000, 0xa5000, 0x2000}, /* WF_LMAC_TOP (WF_MUCOP)  */
	{0x820c4000, 0xa8000, 0x4000}, /* WF_LMAC_TOP (WF_UWTBL )  */
	{0x81030000, 0xae000, 0x0100}, /* WFSYS_AON  */
	{0x81031000, 0xae100, 0x0100}, /* WFSYS_AON  */
	{0x81032000, 0xae200, 0x0100}, /* WFSYS_AON  */
	{0x81033000, 0xae300, 0x0100}, /* WFSYS_AON  */
	{0x81034000, 0xae400, 0x0100}, /* WFSYS_AON  */
	{0x80020000, 0xb0000, 0x10000}, /* WF_TOP_MISC_OFF */
	{0x81020000, 0xc0000, 0x10000}, /* WF_TOP_MISC_ON */
	{0x81040000, 0x120000, 0x1000}, /* WF_MCU_CFG_ON */
	{0x81050000, 0x121000, 0x1000}, /* WF_MCU_EINT */
	{0x81060000, 0x122000, 0x1000}, /* WF_MCU_GPT */
	{0x81070000, 0x123000, 0x1000}, /* WF_MCU_WDT */
	{0x80010000, 0x124000, 0x1000}, /* WF_AXIDMA */
	{0x83010000, 0x130000, 0x10000}, /* WF_PHY_MAP4 */
	{0x88000000, 0x140000, 0x10000}, /* WF_MCU_CFG_LS */
	{0x20020000, 0xd0000, 0x10000}, /* CONN_INFRA WFDMA */
	{0x20060000, 0xe0000, 0x10000}, /* CONN_INFRA conn_host_csr_top */
	{0x7c000000, 0xf0000, 0x10000}, /* CONN_INFRA (io_top bus_cr rgu_on cfg_on) */
	{0x7c010000, 0x100000, 0x10000}, /* CONN_INFRA (gpio  clkgen cfg) */
	{0x20090000, 0x150000, 0x10000}, /* CONN_INFRA VON (RO) */
	{0x20030000, 0x160000, 0x10000}, /* CONN_INFRA CCIF */
	{0x7c040000, 0x170000, 0x10000}, /* CONN_INFRA (bus, afe) */
	{0x7c070000, 0x180000, 0x10000}, /* CONN_INFRA Semaphore */
	{0x7c080000, 0x190000, 0x10000}, /* CONN_INFRA (coex, pta) */
	{0x7c050000, 0x1a0000, 0x10000}, /* CONN_INFRA SYSRAM */
#if CFG_MTK_FPGA_PLATFORM == 1
	{0x74030000, 0x010000, 0x10000}, /* PCIe MAC (conninfra remap) */
#else
	{0x74030000, 0x1d0000, 0x10000}, /* CB PCIe (cbtop remap) */
#endif
#if CFG_MTK_FPGA_PLATFORM != 1
	{0x70010000, 0x1c0000, 0x10000}, /* CB Infra1 */
	{0x70000000, 0x1e0000, 0x10000}, /* CB TOP */
	{0x70020000, 0x1f0000, 0x10000}, /* CB Infra2 (RO) */
#endif
	{0x7c500000, MT6653_PCIE2AP_REMAP_BASE_ADDR, 0x200000}, /* remap */
	{0x00000000, 0x000000, 0x00000}, /* END */
};

#if CFG_SUPPORT_PCIE_ASPM
static spinlock_t rPCIELock;
#define WIFI_ROLE	(1)
#define MD_ROLE		(2)
#define WIFI_RST_ROLE	(3)
#define POLLING_TIMEOUT		(200)
#define POLLING_TIMEOUT_IN_UDS	(8000)
#endif //CFG_SUPPORT_PCIE_ASPM

#endif

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
struct pcie2ap_remap mt6653_pcie2ap_remap = {
	.reg_base = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_ADDR,
	.reg_mask = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_MASK,
	.reg_shift = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_SHFT,
	.base_addr = MT6653_PCIE2AP_REMAP_BASE_ADDR
};

struct pcie2ap_remap mt6653_pcie2ap_remap_cbtop = {
	.reg_base = CB_INFRA_MISC0_CBTOP_PCIE_REMAP_WF_pcie_remap_wf_rg0_ADDR,
	.reg_mask = CB_INFRA_MISC0_CBTOP_PCIE_REMAP_WF_pcie_remap_wf_rg0_MASK,
	.reg_shift = CB_INFRA_MISC0_CBTOP_PCIE_REMAP_WF_pcie_remap_wf_rg0_SHFT,
	.base_addr = MT6653_PCIE2AP_REMAP_CBTOP_BASE_ADDR
};

struct ap2wf_remap mt6653_ap2wf_remap = {
	.reg_base = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_ADDR,
	.reg_mask = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_MASK,
	.reg_shift = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_SHFT,
	.base_addr = MT6653_REMAP_BASE_ADDR
};

struct remap_range mt6653_cbtop_remap_ranges[] = {
	{0x70000000, 0x70ffffff},
	{0x74000000, 0x74ffffff},
	{0x75010000, 0x7501ffff},
	{0, 0}, /* END */
};

struct PCIE_CHIP_CR_REMAPPING mt6653_bus2chip_cr_remapping = {
	.pcie2ap = &mt6653_pcie2ap_remap,
	.pcie2ap_cbtop = &mt6653_pcie2ap_remap_cbtop,
	.ap2wf = &mt6653_ap2wf_remap,
	.cbtop_ranges = mt6653_cbtop_remap_ranges,
};

struct wfdma_group_info mt6653_wfmda_host_tx_group[] = {
	{"P0T0:AP DATA0", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL0_ADDR},
	{"P0T1:AP DATA1", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING1_CTRL0_ADDR},
	{"P0T2:AP DATA2", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING2_CTRL0_ADDR},
	{"P0T3:AP DATA3", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING3_CTRL0_ADDR},
	{"P0T3:AP DATA4", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING4_CTRL0_ADDR},
	{"P0T3:AP DATA5", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING5_CTRL0_ADDR},
	{"P0T15:AP CMD", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING15_CTRL0_ADDR, true},
	{"P0T16:FWDL", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING16_CTRL0_ADDR},
	{"P0T8:MD DATA0", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING8_CTRL0_ADDR},
	{"P0T9:MD DATA1", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING9_CTRL0_ADDR},
	{"P0T10:MD DATA2", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING10_CTRL0_ADDR},
	{"P0T11:MD DATA3", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING11_CTRL0_ADDR},
	{"P0T12:MD DATA4", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING12_CTRL0_ADDR},
	{"P0T14:MD CMD", WF_WFDMA_HOST_DMA0_WPDMA_TX_RING14_CTRL0_ADDR},
};

struct wfdma_group_info mt6653_wfmda_host_rx_group[] = {
	{"P0R4:AP DATA0", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_CTRL0_ADDR},
	{"P0R7:AP EVT/TDONE", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING7_CTRL0_ADDR,
	 true},
	{"P0R5:AP DATA1", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING5_CTRL0_ADDR},
	{"P0R8:AP ICS", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING8_CTRL0_ADDR},
	{"P0R6:AP DATA2", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_CTRL0_ADDR},
	{"P0R9:MD DATA0", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING9_CTRL0_ADDR},
	{"P0R10:MD DATA1", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING10_CTRL0_ADDR},
	{"P0R11:MD DATA2", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING11_CTRL0_ADDR},
	{"P0R12:MD EVT/TDONE", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING12_CTRL0_ADDR},
	{"P0R13:MD ICS", WF_WFDMA_HOST_DMA0_WPDMA_RX_RING13_CTRL0_ADDR},
};

struct wfdma_group_info mt6653_wfmda_wm_tx_group[] = {
	{"P0T6:LMAC TXD", WF_WFDMA_MCU_DMA0_WPDMA_TX_RING6_CTRL0_ADDR},
};

struct wfdma_group_info mt6653_wfmda_wm_rx_group[] = {
	{"P0R0:FWDL", WF_WFDMA_MCU_DMA0_WPDMA_RX_RING0_CTRL0_ADDR},
	{"P0R2:TXD0", WF_WFDMA_MCU_DMA0_WPDMA_RX_RING2_CTRL0_ADDR},
	{"P0R3:TXD1", WF_WFDMA_MCU_DMA0_WPDMA_RX_RING3_CTRL0_ADDR},
};

struct pse_group_info mt6653_pse_group[] = {
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
struct pcie_msi_layout mt6653_pcie_msi_layout[] = {
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
#if CFG_MTK_CCCI_SUPPORT
	{"ccif_bgf2ap_sw_irq", mtk_md_dummy_pci_interrupt, NULL, CCIF_INT, 0},
#else
	{"ccif_bgf2ap_sw_irq", NULL, NULL, NONE_INT, 0},
#endif
	{"ccif_wf2ap_sw_irq", pcie_sw_int_top_handler,
	 pcie_sw_int_thread_handler, AP_MISC_INT, 0},
#if CFG_MTK_CCCI_SUPPORT
	{"ccif_bgf2ap_irq_0", mtk_md_dummy_pci_interrupt, NULL, CCIF_INT, 0},
	{"ccif_bgf2ap_irq_1", mtk_md_dummy_pci_interrupt, NULL, CCIF_INT, 0},
#else
	{"ccif_bgf2ap_irq_0", NULL, NULL, NONE_INT, 0},
	{"ccif_bgf2ap_irq_1", NULL, NULL, NONE_INT, 0},
#endif
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},
	{"reserved", NULL, NULL, NONE_INT, 0},

#if (CFG_PCIE_GEN_SWITCH == 1)
	{"gen_switch_irq1", pcie_gen_switch_end_top_handler,
	pcie_gen_switch_end_thread_handler, PCIE_GEN_SWITCH_INT, 0},
#else
	{"reserved", NULL, NULL, NONE_INT, 0},
#endif

#if defined(CFG_MTK_WIFI_DRV_OWN_INT_MODE)
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

#if (CFG_PCIE_GEN_SWITCH == 1)
	{"gen_switch_irq", pcie_gen_switch_top_handler,
	pcie_gen_switch_thread_handler, PCIE_GEN_SWITCH_INT, 0},
#else
	{"reserved", NULL, NULL, NONE_INT, 0},
#endif

	{"reserved", NULL, NULL, NONE_INT, 0},

};
#endif

struct BUS_INFO mt6653_bus_info = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.top_cfg_base = MT6653_TOP_CFG_BASE,

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
	(WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_4_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_5_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_6_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_7_MASK |
#if CFG_ENABLE_MAWD_MD_RING
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_9_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_10_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_11_MASK |
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_12_MASK |
#endif
	 WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_8_MASK),

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
	.mawd_md_rx_blk_ctrl0 = MAWD_MD_RX_BLK_CTRL0,
	.mawd_md_rx_blk_ctrl1 = MAWD_MD_RX_BLK_CTRL1,
	.mawd_md_rx_blk_ctrl2 = MAWD_MD_RX_BLK_CTRL2,
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

	.bus2chip = mt6653_bus2chip_cr_mapping,
	.bus2chip_remap = &mt6653_bus2chip_cr_remapping,
	.max_static_map_addr = 0x00200000,

	.tx_ring_fwdl_idx = CONNAC3X_FWDL_TX_RING_IDX,
	.tx_ring_cmd_idx = 15,
	.tx_ring0_data_idx = 0,
	.tx_ring1_data_idx = 1,
	.tx_ring2_data_idx = 2,
#if CFG_ENABLE_MAWD_MD_RING
	.tx_ring3_data_idx = 8,
	.tx_prio_data_idx = 9,
	.tx_altx_data_idx = 10,
	.rx_data_ring_num = 6,
	.rx_evt_ring_num = 3,
	.rx_data_ring_size = 1024,
#else
	.tx_ring3_data_idx = 3,
	.tx_prio_data_idx = 4,
	.tx_altx_data_idx = 5,
	.rx_data_ring_num = 3,
	.rx_evt_ring_num = 2,
	.rx_data_ring_size = 3072,
#endif /* CFG_ENABLE_MAWD_MD_RING */
	.rx_evt_ring_size = 128,
	.rx_data_ring_prealloc_size = 1024,
	.fw_own_clear_addr = CONN_HOST_CSR_TOP_WF_BAND0_IRQ_STAT_ADDR,
	.fw_own_clear_bit = CONN_HOST_CSR_TOP_WF_BAND0_IRQ_STAT_WF_B0_HOST_LPCR_FW_OWN_CLR_STAT_MASK,
#if defined(CFG_MTK_WIFI_DRV_OWN_INT_MODE)
	.fgCheckDriverOwnInt = TRUE,
#else
	.fgCheckDriverOwnInt = FALSE,
#endif /* CFG_MTK_WIFI_DRV_OWN_INT_MODE */
#if CFG_MTK_WIFI_PCIE_SUPPORT
	.checkFwOwnMsiStatus = mt6653CheckFwOwnMsiStatus,
	.dumpPcieMsiStatus = mt6653PcieMsiDebugDump,
	.recoveryMsiStatus = mt6653RecoveryMsiStatus,
	.recoverSerStatus = mt6653RecoverSerStatus,
#endif
#if (CFG_MTK_ANDROID_WMT == 1)
	.u4DmaMask = 36,
#else /* !CFG_MTK_ANDROID_WMT */
	.u4DmaMask = 34,
#endif /* !CFG_MTK_ANDROID_WMT */
	.wfmda_host_tx_group = mt6653_wfmda_host_tx_group,
	.wfmda_host_tx_group_len = ARRAY_SIZE(mt6653_wfmda_host_tx_group),
	.wfmda_host_rx_group = mt6653_wfmda_host_rx_group,
	.wfmda_host_rx_group_len = ARRAY_SIZE(mt6653_wfmda_host_rx_group),
	.wfmda_wm_tx_group = mt6653_wfmda_wm_tx_group,
	.wfmda_wm_tx_group_len = ARRAY_SIZE(mt6653_wfmda_wm_tx_group),
	.wfmda_wm_rx_group = mt6653_wfmda_wm_rx_group,
	.wfmda_wm_rx_group_len = ARRAY_SIZE(mt6653_wfmda_wm_rx_group),
	.prDmashdlCfg = &rMt6653DmashdlCfg,
#if (DBG_DISABLE_ALL_INFO == 0)
	.prPleTopCr = &rMt6653PleTopCr,
	.prPseTopCr = &rMt6653PseTopCr,
	.prPpTopCr = &rMt6653PpTopCr,
#endif
#if CFG_SUPPORT_WIFI_SLEEP_COUNT
	.wf_power_dump_start = mt6653PowerDumpStart,
	.wf_power_dump_end = mt6653PowerDumpEnd,
#endif
	.prPseGroup = mt6653_pse_group,
	.u4PseGroupLen = ARRAY_SIZE(mt6653_pse_group),
	.pdmaSetup = mt6653WpdmaConfig,
#if defined(_HIF_PCIE)
#if CFG_MTK_WIFI_WFDMA_WB
	.enableInterrupt = mt6653EnableInterruptViaPcieByEmi,
	.disableInterrupt = asicConnac3xDisablePlatformIRQ,
#elif (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
	.enableInterrupt = mt6653EnableInterruptViaPcie,
	.disableInterrupt = asicConnac3xDisablePlatformIRQ,
#else
	.enableInterrupt = mt6653EnableInterrupt,
	.disableInterrupt = mt6653DisableInterrupt,
#endif /* CFG_SUPPORT_PCIE_PLAT_INT_FLOW */
#else /* !_HIF_PCIE */
	.enableInterrupt = mt6653EnableInterrupt,
	.disableInterrupt = mt6653DisableInterrupt,
#endif /* _HIF_PCIE */
#if CFG_MTK_WIFI_WFDMA_WB
	.configWfdmaIntMask = mt6653ConfigEmiIntMask,
#else
	.configWfdmaIntMask = mt6653ConfigIntMask,
#endif /* CFG_MTK_WIFI_WFDMA_WB */
	.configWfdmaRxRingTh = mt6653ConfigWfdmaRxRingThreshold,
#if defined(_HIF_PCIE)
	.initPcieInt = mt6653InitPcieInt,
	.hwControlVote = mt6653PcieHwControlVote,
	.pdmaStop = asicConnac3xWfdmaStop,
	.pdmaPollingIdle = asicConnac3xWfdmaPollingAllIdle,
	.pcie_msi_info = {
		.prMsiLayout = mt6653_pcie_msi_layout,
		.u4MaxMsiNum = ARRAY_SIZE(mt6653_pcie_msi_layout),
	},

#if CFG_SUPPORT_PCIE_ASPM
	.configPcieAspm = mt6653ConfigPcieAspm,
	.updatePcieAspm = mt6653UpdatePcieAspm,
	.keepPcieWakeup = mt6653KeepPcieWakeup,
	.fgWifiEnL1_2 = TRUE,
	.fgMDEnL1_2 = TRUE,
	.fgWifiRstEnL1_2 = TRUE,
#endif

#if CFG_MTK_WIFI_PCIE_SUPPORT
	.is_en_drv_ctrl_pci_msi_irq = TRUE,
#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
	.pcieMsiMaskIrq = mt6653PcieMsiMaskIrq,
	.pcieMsiUnmaskIrq = mt6653PcieMsiUnmaskIrq,
#endif /* CFG_MTK_ANDROID_WMT */
#endif
	.showDebugInfo = mt6653ShowPcieDebugInfo,
#endif /* _HIF_PCIE */
#if CFG_MTK_WIFI_WFDMA_WB
	.processTxInterrupt = mt6653ProcessTxInterruptByEmi,
	.processRxInterrupt = mt6653ProcessRxInterruptByEmi,
#else
	.processTxInterrupt = mt6653ProcessTxInterrupt,
	.processRxInterrupt = mt6653ProcessRxInterrupt,
#endif /* CFG_MTK_WIFI_WFDMA_WB */
	.tx_ring_ext_ctrl = mt6653WfdmaTxRingExtCtrl,
	.rx_ring_ext_ctrl = mt6653WfdmaRxRingExtCtrl,
	/* null wfdmaManualPrefetch if want to disable manual mode */
	.wfdmaManualPrefetch = mt6653WfdmaManualPrefetch,
	.lowPowerOwnInit = mt6653LowPowerOwnInit,
	.lowPowerOwnRead = mt6653LowPowerOwnRead,
	.lowPowerOwnSet = mt6653LowPowerOwnSet,
	.lowPowerOwnClear = mt6653LowPowerOwnClear,
	.wakeUpWiFi = asicWakeUpWiFi,
	.processSoftwareInterrupt = asicConnac3xProcessSoftwareInterrupt,
	.softwareInterruptMcu = asicConnac3xSoftwareInterruptMcu,
	.hifRst = asicConnac3xHifRst,
#if defined(_HIF_PCIE) && (WFDMA_AP_MSI_NUM == 8)
	.devReadIntStatus = mt6653ReadIntStatusByMsi,
#else
#if CFG_MTK_WIFI_WFDMA_WB
	.devReadIntStatus = mt6653ReadIntStatusByEmi,
#else
	.devReadIntStatus = mt6653ReadIntStatus,
#endif /* CFG_MTK_WIFI_WFDMA_WB */
#endif /* _HIF_PCIE */
	.setRxRingHwAddr = mt6653SetRxRingHwAddr,
	.wfdmaAllocRxRing = mt6653WfdmaAllocRxRing,
	.clearEvtRingTillCmdRingEmpty = connac3xClearEvtRingTillCmdRingEmpty,
	.setupMcuEmiAddr = mt6653SetupMcuEmiAddr,
#if (CFG_MTK_WIFI_SW_EMI_RING == 1) && (CFG_MTK_WIFI_MBU == 1)
	.rSwEmiRingInfo = {
		.rOps = {
			.init = halMbuInit,
			.uninit = halMbuUninit,
			.read = halMbuRead,
			.debug = halMbuDebug,
			.dumpDebugCr = mt6653MbuDumpDebugCr,
		},
		.fgIsSupport = TRUE,
		.u4RemapAddr = CB_INFRA_MISC0_CBTOP_PCIE_REMAP_WF_BT_ADDR,
		.u4RemapVal = 0x70027413,
		.u4RemapDefVal = 0x70027000,
		.u4RemapRegAddr = 0x74130000,
		.u4RemapBusAddr = 0x1e0000,

	},
#endif /* CFG_MTK_WIFI_SW_EMI_RING */
#endif /*_HIF_PCIE || _HIF_AXI */
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.DmaShdlInit = mt6653DmashdlInit,
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
struct FWDL_OPS_T mt6653_fw_dl_ops = {
	.constructFirmwarePrio = mt6653_ConstructFirmwarePrio,
	.constructPatchName = mt6653_ConstructPatchName,
#if CFG_SUPPORT_SINGLE_FW_BINARY
	.parseSingleBinaryFile = wlanParseSingleBinaryFile,
#endif
#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
	.constrcutIdxLogBin = mt6653_ConstructIdxLogBinName,
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */
#if defined(_HIF_PCIE)
	.downloadPatch = mt6653_wlanDownloadPatch,
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
#if defined(_HIF_PCIE)
	.mcu_init = mt6653_mcu_init,
	.mcu_deinit = mt6653_mcu_deinit,
#endif
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
	.constructBtPatchName = asicConnac3xConstructBtPatchName,
	.downloadBtPatch = asicConnac3xDownloadBtPatch,
#if (CFG_SUPPORT_CONNAC3X == 1)
	.configBtImageSection = asicConnac3xConfigBtImageSection,
#endif
#endif
	.getFwVerInfo = wlanParseRamCodeReleaseManifest,
#if CFG_MTK_WIFI_SUPPORT_DSP_FWDL
	.constructDspName = mt6653_ConstructDspName,
	.downloadDspFw = wlanDownloadDspFw,
#endif
};
#endif /* CFG_ENABLE_FW_DOWNLOAD */

struct TX_DESC_OPS_T mt6653_TxDescOps = {
	.fillNicAppend = fillNicTxDescAppend,
	.fillHifAppend = fillTxDescAppendByHostV2,
	.fillTxByteCount = fillConnac3xTxDescTxByteCount,
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	.fillNicSdoAppend = fillConnac3xNicTxDescAppendWithSdoV2,
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
};

struct RX_DESC_OPS_T mt6653_RxDescOps = {0};

#if (DBG_DISABLE_ALL_INFO == 0)
struct CHIP_DBG_OPS mt6653_DebugOps = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.showPdmaInfo = connac3x_show_wfdma_info,
#endif
	.showPseInfo = connac3x_show_pse_info,
	.showPleInfo = connac3x_show_ple_info,
	.showTxdInfo = connac3x_show_txd_Info,
	.showWtblInfo = connac3x_show_wtbl_info,
	.showUmacWtblInfo = connac3x_show_umac_wtbl_info,
	.showCsrInfo = NULL,
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
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
	.show_wfdma_dbg_probe_info = mt6653_show_wfdma_dbg_probe_info,
	.show_wfdma_wrapper_info = mt6653_show_wfdma_wrapper_info,
	.dumpwfsyscpupcr = mt6653_dumpWfsyscpupcr,
	.dumpBusHangCr = mt6653_DumpBusHangCr,
	.dumpPcieStatus = mt6653DumpPcieDateFlowStatus,
#ifdef CFG_MTK_WIFI_CONNV3_SUPPORT
	.dumpPcieCr = mt6653_dumpPcieReg,
	.checkDumpViaBt = mt6653_CheckDumpViaBt,
#endif
#if CFG_MTK_WIFI_WFDMA_WB
	.show_wfdma_wb_info = connac3x_show_wfdma_wb_info,
#endif
#if CFG_MTK_WIFI_MBU
	.getMbuTimeoutStatus = mt6653_get_mbu_timeout_status,
#endif
#endif
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	.get_rx_rate_info = mt6653_get_rx_rate_info,
#endif
#if CFG_SUPPORT_LLS
	.get_rx_link_stats = mt6653_get_rx_link_stats,
#endif
	.dumpTxdInfo = connac3x_dump_tmac_info,
};
#endif /* DBG_DISABLE_ALL_INFO */

#if CFG_SUPPORT_QA_TOOL
#if (CONFIG_WLAN_SERVICE == 1)
struct test_capability mt6653_toolCapability = {
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

		/* BIT[15:0]: Band2 5G, 6G, 0x6 */
		/* BIT[31:16]: Band3 2.4G, 5G, 6G, 0x7 */
		0x00070006,	/* u_int32 channel_band_dbdc_ext */

		/* BIT[7:0]: Support phy 0xF (bitwise),
		 *           phy0, phy1, phy2, phy3(little)
		 */
		/* BIT[15:8]: Support Adie 0x1 (bitwise) */
		0x010F,	/* u_int32 phy_adie_index; CFG_SUPPORT_CONNAC3X */

		/* BIT[7:0]: Band0 TX path 2 */
		/* BIT[15:8]: Band0 RX path 2 */
		/* BIT[23:16]: Band1 TX path 2 */
		/* BIT[31:24]: Band1 RX path 2 */
		0x02020202,	/* u_int32 band_0_1_wf_path_num; */

		/* BIT[7:0]: Band2 TX path 1 */
		/* BIT[15:8]: Band2 RX path 1*/
		/* BIT[23:16]: Band3 TX path 0 */
		/* BIT[31:24]: Band3 RX path 1 */
		0x02000101,	/* u_int32 band_2_3_wf_path_num; */

		/* BIT[7:0]: Band0 BW20, 0x1 */
		/* BIT[15:8]: Band1 BW320, 0x2F */
		/* BIT[23:16]: Band2 BW160, 0xF */
		/* BIT[31:24]: Band3 BW20, 0x1 */
		0x010F2F01,	/* u_int32 band_bandwidth; */

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

struct ATE_OPS_T mt6653_AteOps = {
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
	.icapRiseVcoreClockRate = mt6653_icapRiseVcoreClockRate,
	.icapDownVcoreClockRate = mt6653_icapDownVcoreClockRate,
#if (CONFIG_WLAN_SERVICE == 1)
	.tool_capability = &mt6653_toolCapability,
#endif
};
#endif /* CFG_SUPPORT_QA_TOOL */

#if defined(_HIF_PCIE)
#if (CFG_MTK_FPGA_PLATFORM == 0)
static struct CCIF_OPS mt6653_ccif_ops = {
	.get_interrupt_status = mt6653_ccif_get_interrupt_status,
	.notify_utc_time_to_fw = mt6653_ccif_notify_utc_time_to_fw,
	.set_fw_log_read_pointer = mt6653_ccif_set_fw_log_read_pointer,
	.get_fw_log_read_pointer = mt6653_ccif_get_fw_log_read_pointer,
	.trigger_fw_assert = mt6653_ccif_trigger_fw_assert,
};
#endif
#if CFG_MTK_WIFI_FW_LOG_MMIO
static struct FW_LOG_OPS mt6653_fw_log_mmio_ops = {
	.init = fwLogMmioInitMcu,
	.deinit = fwLogMmioDeInitMcu,
	.start = fwLogMmioStart,
	.stop = fwLogMmioStop,
	.handler = fwLogMmioHandler,
};
#endif

#if CFG_MTK_WIFI_FW_LOG_EMI
static struct FW_LOG_OPS mt6653_fw_log_emi_ops = {
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
struct thermal_sensor_info mt6653_thermal_sensor_info[] = {
	{"wifi_adie_0", THERMAL_TEMP_TYPE_ADIE, 0},
	{"wifi_ddie_0", THERMAL_TEMP_TYPE_DDIE, 0},
	{"wifi_ddie_1", THERMAL_TEMP_TYPE_DDIE, 1},
	{"wifi_ddie_2", THERMAL_TEMP_TYPE_DDIE, 2},
	{"wifi_ddie_3", THERMAL_TEMP_TYPE_DDIE, 3},
};
#endif

#if CFG_NEW_HIF_DEV_REG_IF
enum HIF_DEV_REG_REASON mt6653ValidMmioReadReason[] = {
	HIF_DEV_REG_HIF_DBG,
	HIF_DEV_REG_HIF_EXTDBG,
	HIF_DEV_REG_HIF_BT_DBG,
	HIF_DEV_REG_ONOFF_READ,
	HIF_DEV_REG_ONOFF_DBG,
	HIF_DEV_REG_RESET_READ,
	HIF_DEV_REG_COREDUMP_DBG,
	HIF_DEV_REG_LPOWN_READ,
	HIF_DEV_REG_SER_READ,
#if (CFG_MTK_FPGA_PLATFORM != 0)
	HIF_DEV_REG_OFFLOAD_READ,
#endif
	HIF_DEV_REG_OFFLOAD_HOST,
	HIF_DEV_REG_OFFLOAD_DBG,
	HIF_DEV_REG_CCIF_READ,
	HIF_DEV_REG_PLAT_DBG,
	HIF_DEV_REG_WTBL_DBG,
	HIF_DEV_REG_OID_DBG,
	HIF_DEV_REG_PCIEASPM_READ,
	HIF_DEV_REG_NOMMIO_DBG,
#if (CFG_MTK_WIFI_WFDMA_WB == 0)
	HIF_DEV_REG_HIF_READ,
	HIF_DEV_REG_HIF_RING,
#endif
};

#if (CFG_MTK_WIFI_SW_EMI_RING == 1) && (CFG_MTK_WIFI_MBU == 1)
enum HIF_DEV_REG_REASON mt6653NoMmioReadReason[] = {
	HIF_DEV_REG_HIF_DBG,
	HIF_DEV_REG_NOMMIO_DBG,
};
#endif
#endif /* CFG_NEW_HIF_DEV_REG_IF */

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
struct EMI_WIFI_MISC_RSV_MEM_INFO mt6653_wifi_misc_rsv_mem_info[] = {
	{WIFI_MISC_MEM_BLOCK_NON_MMIO, 2048, {0}},
	{WIFI_MISC_MEM_BLOCK_TX_POWER_LIMIT, 20480, {0}},
	{WIFI_MISC_MEM_BLOCK_TX_POWER_STATUS, 16, {0}},
	{WIFI_MISC_MEM_BLOCK_SER_STATUS, 16, {0}},
	{WIFI_MISC_MEM_BLOCK_SCREEN_STATUS, 16, {0}},
	{WIFI_MISC_MEM_BLOCK_WF_M_BRAIN, 20480, {0}},
	{WIFI_MISC_MEM_BLOCK_WF_GEN_SWITCH, 16, {0}},
	{WIFI_MISC_MEM_BLOCK_WF_RSVED, 22464, {0}},
	{WIFI_MISC_MEM_BLOCK_PRECAL, 589824, {0}}
};
#endif
static struct sw_sync_emi_info mt6653_sw_sync_emi_info[SW_SYNC_TAG_NUM];

struct mt66xx_chip_info mt66xx_chip_info_mt6653 = {
	.bus_info = &mt6653_bus_info,
#if CFG_ENABLE_FW_DOWNLOAD
	.fw_dl_ops = &mt6653_fw_dl_ops,
#endif /* CFG_ENABLE_FW_DOWNLOAD */
#if CFG_SUPPORT_QA_TOOL
	.prAteOps = &mt6653_AteOps,
#endif /* CFG_SUPPORT_QA_TOOL */
	.prTxDescOps = &mt6653_TxDescOps,
	.prRxDescOps = &mt6653_RxDescOps,
#if (DBG_DISABLE_ALL_INFO == 0)
	.prDebugOps = &mt6653_DebugOps,
#endif
	.chip_id = MT6653_CHIP_ID,
	.should_verify_chip_id = FALSE,
	.sw_sync0 = CONNAC3X_CONN_CFG_ON_CONN_ON_MISC_ADDR,
	.sw_ready_bits = WIFI_FUNC_NO_CR4_READY_BITS,
	.sw_ready_bit_offset =
		Connac3x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT,
	.patch_addr = MT6653_PATCH_START_ADDR,
	.is_support_cr4 = FALSE,
	.is_support_wacpu = FALSE,
	.sw_sync_emi_info = mt6653_sw_sync_emi_info,
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
	.wifi_off_magic_num = MT6653_WIFI_OFF_MAGIC_NUM,
#endif /* CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI */
#if defined(_HIF_PCIE)
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	.is_support_mawd = TRUE,
	.is_support_sdo = TRUE,
	.is_support_rro = TRUE,
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
	.is_en_wfdma_no_mmio_read = FALSE,
#if CFG_MTK_WIFI_SW_EMI_RING
	.is_en_sw_emi_read = TRUE,
#endif
	.fgDumpViaBtOnlyForDbgSOP = TRUE,
#endif /* _HIF_PCIE */
#if CFG_MTK_WIFI_WFDMA_WB
	.is_support_wfdma_write_back = TRUE,
	.wb_dmy_dbg_size = sizeof(struct WFDMA_EMI_RING_IDX_0) * 2,
	.wb_int_sta_size = sizeof(uint32_t),
	.wb_didx_size = sizeof(struct WFDMA_EMI_RING_IDX_0),
	.wb_md_int_sta_size = sizeof(uint32_t),
	.wb_md_didx_size = sizeof(struct WFDMA_EMI_RING_IDX_1),
	.allocWfdmaWbBuffer = asicConnac3xAllocWfdmaWbBuffer,
	.freeWfdmaWbBuffer = asicConnac3xFreeWfdmaWbBuffer,
#if defined(_HIF_PCIE)
	.isWfdmaRxReady = mt6653IsWfdmaRxReady,
#endif /* _HIF_PCIE */
#endif
#if CFG_SUPPORT_WFDMA_RX_DELAY_INT
	.updatePrdcInt = mt6653UpdateWfdmaPrdcInt,
#endif /* CFG_SUPPORT_WFDMA_RX_DELAY_INT */
	.txd_append_size = MT6653_TX_DESC_APPEND_LENGTH,
	.hif_txd_append_size = MT6653_HIF_TX_DESC_APPEND_LENGTH,
	.rxd_size = MT6653_RX_DESC_LENGTH,
	.init_evt_rxd_size = MT6653_RX_INIT_DESC_LENGTH,
	.pse_header_length = CONNAC3X_NIC_TX_PSE_HEADER_LENGTH,
	.init_event_size = CONNAC3X_RX_INIT_EVENT_LENGTH,
	.eco_info = mt6653_eco_table,
	.isNicCapV1 = FALSE,
	.is_support_efuse = TRUE,
	.top_hcr = CONNAC3X_TOP_HCR,
	.top_hvr = CONNAC3X_TOP_HVR,
	.top_fvr = CONNAC3X_TOP_FVR,
#if (CFG_SUPPORT_802_11AX == 1)
	.arb_ac_mode_addr = MT6653_ARB_AC_MODE_ADDR,
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
	.coexpccifon = mt6653ConnacPccifOn,
	.coexpccifoff = mt6653ConnacPccifOff,
#if CFG_MTK_MDDP_SUPPORT
	.isSupportMddpAOR = false,
	.isSupportMddpSHM = true,
	.u4MdLpctlAddr = CONN_HOST_CSR_TOP_WF_MD_LPCTL_ADDR,
	.u4MdDrvOwnTimeoutTime = 2000,
	.checkMdRxHang = mt6653CheckMdRxHang,
#else
	.isSupportMddpAOR = false,
	.isSupportMddpSHM = false,
#endif
	.u4HostWfdmaBaseAddr = WF_WFDMA_HOST_DMA0_BASE,
	.u4HostWfdmaWrapBaseAddr = WF_WFDMA_EXT_WRAP_CSR_BASE,
	.u4McuWfdmaBaseAddr = WF_WFDMA_MCU_DMA0_BASE,
	.u4DmaShdlBaseAddr = WF_HIF_DMASHDL_TOP_BASE,
	.cmd_max_pkt_size = CFG_TX_MAX_PKT_SIZE, /* size 1600 */
#if (CFG_SUPPORT_APS == 1)
	.apsLinkPlanDecision = mt6653_apsLinkPlanDecision,
	.apsUpdateTotalScore = mt6653_apsUpdateTotalScore,
	.apsFillBssDescSet = mt6653_apsFillBssDescSet,
#endif
#if defined(CFG_MTK_WIFI_PMIC_QUERY)
	.queryPmicInfo = asicConnac3xQueryPmicInfo,
#endif
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
	.queryDFDInfo = asicConnac3xQueryDFDInfo,
#endif
	.isUpgradeWholeChipReset = mt6653_isUpgradeWholeChipReset,

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
	.checkbushang = mt6653_CheckBusHang,
	.rEmiInfo = {
#if CFG_MTK_ANDROID_EMI
		.type = EMI_ALLOC_TYPE_LK,
		.coredump_size = (7 * 1024 * 1024),
		.coredump2_size = (1 * 1024 * 1024),
#else
		.type = EMI_ALLOC_TYPE_IN_DRIVER,
#endif /* CFG_MTK_ANDROID_EMI */
	},
#if CFG_SUPPORT_THERMAL_QUERY
	.thermal_info = {
		.sensor_num = ARRAY_SIZE(mt6653_thermal_sensor_info),
		.sensor_info = mt6653_thermal_sensor_info,
	},
#endif
	.trigger_fw_assert = mt6653_trigger_fw_assert,
	.fw_log_info = {
#if CFG_MTK_WIFI_FW_LOG_MMIO
		.ops = &mt6653_fw_log_mmio_ops,
#endif
#if CFG_MTK_WIFI_FW_LOG_EMI
		.base = 0x1A8000,
		.ops = &mt6653_fw_log_emi_ops,
#endif
		.path = ENUM_LOG_READ_POINTER_PATH_CCIF,
	},
#if ((CFG_SUPPORT_PHY_ICS_V3 == 1) || (CFG_SUPPORT_PHY_ICS_V4 == 1))
	.u4PhyIcsEmiBaseAddr = PHYICS_EMI_BASE_ADDR,
	.u4PhyIcsTotalCnt = PHYICS_TOTAL_CNT,
	.u4PhyIcsBufSize = PHYICS_BUF_SIZE,
	.u4MemoryPart = WIFI_MCU_MEMORY_PART_2,
#endif
#if (CFG_MTK_FPGA_PLATFORM == 0)
	.ccif_ops = &mt6653_ccif_ops,
#endif
	.get_sw_interrupt_status = mt6653_get_sw_interrupt_status,
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
#if CFG_NEW_HIF_DEV_REG_IF
	.fgIsWarnInvalidMmioRead = TRUE,
	.isValidMmioReadReason = connac3xIsValidMmioReadReason,
	.prValidMmioReadReason = mt6653ValidMmioReadReason,
	.u4ValidMmioReadReasonSize = ARRAY_SIZE(mt6653ValidMmioReadReason),
#if (CFG_MTK_WIFI_SW_EMI_RING == 1) && (CFG_MTK_WIFI_MBU == 1)
	.isNoMmioReadReason = connac3xIsNoMmioReadReason,
	.prNoMmioReadReason = mt6653NoMmioReadReason,
	.u4NoMmioReadReasonSize = ARRAY_SIZE(mt6653NoMmioReadReason),
#endif
#endif /* CFG_NEW_HIF_DEV_REG_IF */
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.rsvMemWiFiMisc = mt6653_wifi_misc_rsv_mem_info,
	.rsvMemWiFiMiscSize = ARRAY_SIZE(mt6653_wifi_misc_rsv_mem_info),
#endif
#if (CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1)
	.dmashdlQuotaDecision = asicConnac3xDynamicDmashdlQuotaDecision,
	.eMloMaxQuotaHwBand = ENUM_BAND_1,
	.u4DefaultMinQuota = 0x10,
	.u4DefaultMaxQuota = 0x100,
#if (CONFIG_BAND_NUM == 3)
	.au4DmaMaxQuotaBand = {0x100, 0x7E0, 0x280},
#else
	.au4DmaMaxQuotaBand = {0x100, 0x7E0},
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	.au4DmaMaxQuotaRfBand = {0x100, 0x2d0, 0x590},
#else
	.au4DmaMaxQuotaRfBand = {0x100, 0x590},
#endif /* CFG_SUPPORT_WIFI_6G */
	.isSupportBand2 = TRUE,
#endif /* CFG_DYNAMIC_DMASHDL_MAX_QUOTA */
#if CFG_SUPPORT_CONNAC3X
	/* Platform custom config for conninfra */
	.rPlatcfgInfraSysram = {
		.addr = CONNAC3X_PLAT_CFG_ADDR,
		.size = CONNAC3X_PLAT_CFG_SIZE,
	},
#endif
	.ucMaxSwapAntenna = 2
};

struct mt66xx_hif_driver_data mt66xx_driver_data_mt6653 = {
	.chip_info = &mt66xx_chip_info_mt6653,
};

void mt6653_icapRiseVcoreClockRate(void)
{
	DBGLOG(HAL, STATE, "icapRiseVcoreClockRate skip\n");
}

void mt6653_icapDownVcoreClockRate(void)
{
	DBGLOG(HAL, STATE, "icapDownVcoreClockRate skip\n");
}

static void mt6653_ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx)
{
	int ret = 0;
	uint8_t ucIdx = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];
	uint8_t aucTestmode[CFG_FW_FLAVOR_MAX_LEN] = {0};

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt6653GetFlavorVer(&aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt6653_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt6653_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt6653_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt6653_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2. WIFI_RAM_CODE_MT6653_1_1.bin */
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
	if (get_wifi_test_mode_fwdl() == 1)
		kalScnprintf(aucTestmode,
			CFG_FW_FLAVOR_MAX_LEN,
			"TESTMODE_");
#endif
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"WIFI_RAM_CODE_MT%x_%s%s_%u.bin",
			MT6653_CHIP_ID,
			aucTestmode,
			aucFlavor,
			MT6653_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	for (ucIdx = 0; apucmt6653FwName[ucIdx]; ucIdx++) {
		if ((*pucNameIdx + 3) >= ucMaxNameIdx) {
			/* the table is not large enough */
			DBGLOG(INIT, ERROR,
				"kalFirmwareImageMapping >> file name array is not enough.\n");
			ASSERT(0);
			continue;
		}

		/* Type 3. WIFI_RAM_CODE_6653.bin */
		ret = kalSnprintf(*(apucName + (*pucNameIdx)),
				CFG_FW_NAME_MAX_LEN, "%s.bin",
				apucmt6653FwName[ucIdx]);
		if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
			(*pucNameIdx) += 1;
		else
			DBGLOG(INIT, ERROR,
				"[%u] kalSnprintf failed, ret: %d\n",
				__LINE__, ret);
	}
}

static void mt6653_ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];
	uint8_t aucTestmode[CFG_FW_FLAVOR_MAX_LEN] = {0};

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt6653GetFlavorVer(&aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt6653_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt6653_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt6653_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt6653_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2. WIFI_MT6653_PATCH_MCU_1_1_hdr.bin */
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
	if (get_wifi_test_mode_fwdl() == 1)
		kalScnprintf(aucTestmode,
			CFG_FW_FLAVOR_MAX_LEN,
			"TESTMODE_");
#endif
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			CFG_FW_NAME_MAX_LEN,
			"WIFI_MT%x_PATCH_MCU_%s%s_%u_hdr.bin",
			MT6653_CHIP_ID,
			aucTestmode,
			aucFlavor,
			MT6653_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 3. mt6653_patch_e1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "mt6653_patch_e1_hdr.bin");
	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
}

#if CFG_MTK_WIFI_SUPPORT_DSP_FWDL
static void mt6653_ConstructDspName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt6653GetFlavorVer(&aucFlavor[0]);

	/* Type 1. WIFI_MT6653_PHY_RAM_CODE_1_1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_MT%x_PHY_RAM_CODE_%s_%u.bin",
			  MT6653_CHIP_ID,
			  aucFlavor,
			  MT6653_ROM_VERSION);
	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
	else
		(*pucNameIdx) += 1;
}
#endif

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
static void mt6653_ConstructIdxLogBinName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	mt6653GetFlavorVer(&aucFlavor[0]);

	/* ex: WIFI_RAM_CODE_MT6653_2_1_idxlog.bin */
	ret = kalSnprintf(apucName[0],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_RAM_CODE_MT%x_%s_%u_idxlog.bin",
			  MT6653_CHIP_ID,
			  aucFlavor,
			  MT6653_ROM_VERSION);

	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
}
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */

#if defined(_HIF_PCIE)
static uint32_t mt6653RxRingSwIdx2HwIdx(uint32_t u4SwRingIdx)
{
	uint32_t u4HwRingIdx = 0;

	/*
	 * RX_RING_DATA0   (RX_Ring4) - Band0 Rx Data
	 * RX_RING_DATA1   (RX_Ring5) - Band1 Rx Data
	 * RX_RING_DATA2   (RX_Ring6) - Band2 Rx Data
	 * RX_RING_EVT     (RX_Ring7) - Band0 Tx Free Done Event / Rx Event
	 * RX_RING_TXDONE0 (RX_Ring8) - ICS / RXPRT
	*/
	switch (u4SwRingIdx) {
	case RX_RING_EVT:
		u4HwRingIdx = 7;
		break;
	case RX_RING_DATA0:
		u4HwRingIdx = 4;
		break;
	case RX_RING_DATA1:
		u4HwRingIdx = 5;
		break;
	case RX_RING_DATA2:
		u4HwRingIdx = 6;
		break;
	case RX_RING_TXDONE0:
		u4HwRingIdx = 8;
		break;
#if CFG_ENABLE_MAWD_MD_RING
	case RX_RING_DATA3:
		u4HwRingIdx = 9;
		break;
	case RX_RING_DATA4:
		u4HwRingIdx = 10;
		break;
	case RX_RING_DATA5:
		u4HwRingIdx = 11;
		break;
	case RX_RING_TXDONE1:
		u4HwRingIdx = 12;
		break;
	case RX_RING_TXDONE2:
		u4HwRingIdx = 13;
		break;
#endif
	default:
		return RX_RING_MAX;
	}

	return u4HwRingIdx;
}

static uint8_t mt6653SetRxRingHwAddr(struct RTMP_RX_RING *prRxRing,
		struct BUS_INFO *prBusInfo, uint32_t u4SwRingIdx)
{
	uint32_t u4RingIdx = mt6653RxRingSwIdx2HwIdx(u4SwRingIdx);

	if (u4RingIdx >= RX_RING_MAX)
		return FALSE;

	halSetRxRingHwAddr(prRxRing, prBusInfo, u4RingIdx);

	return TRUE;
}

static bool mt6653WfdmaAllocRxRing(struct GLUE_INFO *prGlueInfo,
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

	/* Band2 Data Rx path */
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_DATA2, prHifInfo->u4RxDataRingSize,
			RXD_SIZE, CFG_RX_MAX_PKT_SIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[5] fail\n");
		return false;
	}

	/* ICS log */
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_TXDONE0, prHifInfo->u4RxEvtRingSize,
			RXD_SIZE, RX_BUFFER_AGGRESIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[3] fail\n");
		return false;
	}
#if CFG_ENABLE_MAWD_MD_RING
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_DATA3, prHifInfo->u4RxDataRingSize,
			RXD_SIZE, CFG_RX_MAX_PKT_SIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[9] fail\n");
		return false;
	}
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_DATA4, prHifInfo->u4RxDataRingSize,
			RXD_SIZE, CFG_RX_MAX_PKT_SIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[10] fail\n");
		return false;
	}
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_DATA5, prHifInfo->u4RxDataRingSize,
			RXD_SIZE, CFG_RX_MAX_PKT_SIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[11] fail\n");
		return false;
	}
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_TXDONE1, prHifInfo->u4RxEvtRingSize,
			RXD_SIZE, RX_BUFFER_AGGRESIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[4] fail\n");
		return false;
	}
	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_TXDONE2, prHifInfo->u4RxEvtRingSize,
			RXD_SIZE, RX_BUFFER_AGGRESIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[6] fail\n");
		return false;
	}
#endif /* CFG_ENABLE_MAWD_MD_RING */
	return true;
}

static void mt6653ProcessTxInterrupt(
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

static void mt6653ProcessRxDataInterrupt(struct ADAPTER *prAdapter)
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
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_5_MASK) ||
		    (u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_6_MASK))
			halRroReadRxData(prAdapter);
#if CFG_ENABLE_MAWD_MD_RING
		if ((u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_9_MASK) ||
		    (u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_10_MASK) ||
		    (u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_11_MASK))
			halRroReadRxData(prAdapter);
#endif /* CFG_ENABLE_MAWD_MD_RING */
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

		if ((u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_6_MASK) ||
		    (KAL_TEST_BIT(RX_RING_DATA2, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA2, TRUE);

#if CFG_ENABLE_MAWD_MD_RING
		if ((u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_9_MASK) ||
		    (KAL_TEST_BIT(RX_RING_DATA3, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA3, TRUE);

		if ((u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_10_MASK) ||
		    (KAL_TEST_BIT(RX_RING_DATA4, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA4, TRUE);

		if ((u4Sta &
		     WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_11_MASK) ||
		    (KAL_TEST_BIT(RX_RING_DATA5, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA5, TRUE);
#endif /* CFG_ENABLE_MAWD_MD_RING */
	}
}

static void mt6653ProcessRxInterrupt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_7_MASK) ||
	    (KAL_TEST_BIT(RX_RING_EVT, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_EVT, FALSE);

	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_8_MASK) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE0, FALSE);

#if CFG_ENABLE_MAWD_MD_RING
	if ((u4Sta & WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_12_MASK) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE1, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE1, FALSE);
#endif /* CFG_ENABLE_MAWD_MD_RING */

	mt6653ProcessRxDataInterrupt(prAdapter);
}

static void mt6653SetTRXRingPriorityInterrupt(struct ADAPTER *prAdapter)
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

static void mt6653WfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint32_t u4WrVal = 0, u4Addr = 0;
	uint32_t u4PrefetchCnt = 0x4, u4TxDataPrefetchCnt = 0x10;
	uint32_t u4PrefetchBase = 0x00400000, u4TxDataPrefetchBase = 0x01000000;
	uint32_t u4RxDataPrefetchCnt = 0x8;
	uint32_t u4RxDataPrefetchBase = 0x00800000;
#if CFG_MTK_MDDP_SUPPORT || CFG_ENABLE_MAWD_MD_RING
	uint32_t u4MdTxDataPrefetchCnt = 0x8;
	uint32_t u4MdTxDataPrefetchBase = 0x00800000;
#endif

	/* Rx ring */
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4RxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4RxDataPrefetchBase;
	}

	/* Rx Evt/ICS */
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING7_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_RX_RING8_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4PrefetchBase;
	}

#if CFG_MTK_MDDP_SUPPORT || CFG_ENABLE_MAWD_MD_RING
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING9_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_RX_RING11_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4RxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4RxDataPrefetchBase;
	}

	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING12_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_RX_RING13_EXT_CTRL_ADDR;
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
		 u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_TX_RING3_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4TxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4TxDataPrefetchBase;
	}
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING4_EXT_CTRL_ADDR;
		 u4Addr <=
			WF_WFDMA_HOST_DMA0_WPDMA_TX_RING5_EXT_CTRL_ADDR;
		 u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4PrefetchBase;
	}

#if CFG_MTK_MDDP_SUPPORT || CFG_ENABLE_MAWD_MD_RING
	for (u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING8_EXT_CTRL_ADDR;
	     u4Addr <= WF_WFDMA_HOST_DMA0_WPDMA_TX_RING11_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4MdTxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4MdTxDataPrefetchBase;
	}

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING12_EXT_CTRL_ADDR;
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

	mt6653SetTRXRingPriorityInterrupt(prAdapter);

	/* reset dma TRX idx */
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_RST_DTX_PTR_ADDR, 0xFFFFFFFF);
	HAL_MCR_WR(prAdapter,
		WF_WFDMA_HOST_DMA0_WPDMA_RST_DRX_PTR_ADDR, 0xFFFFFFFF);
}

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
static void mt6653ReadOffloadIntStatus(struct ADAPTER *prAdapter,
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
		HAL_RMCR_RD(OFFLOAD_READ, prAdapter, u4Addr, &u4RegValue);
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
static void mt6653ReadIntStatusByMsi(struct ADAPTER *prAdapter,
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
	mt6653ReadOffloadIntStatus(prAdapter, pu4IntStatus);
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
}
#endif

static void mt6653ReadIntStatus(struct ADAPTER *prAdapter,
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

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	mt6653ReadOffloadIntStatus(prAdapter, pu4IntStatus);
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
}

static void mt6653ConfigIntMask(struct GLUE_INFO *prGlueInfo,
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
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA4_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA5_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA6_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA7_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_RX_DONE_INT_ENA8_MASK |
#if (CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0)
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA0_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA1_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA2_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA3_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA4_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_HOST_TX_DONE_INT_ENA5_MASK |
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

	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4WrVal);
}

#if CFG_MTK_WIFI_WFDMA_WB
static u_int8_t mt6653IsWfdmaRxReady(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct RTMP_DMABUF *prRingIntSta;
	struct HIF_MEM_OPS *prMemOps;
	struct HIF_MEM *prMem = NULL;
	uint32_t u4Idx, u4IntSta = 0;

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	prChipInfo = prAdapter->chip_info;
	prMemOps = &prHifInfo->rMemOps;
	prRingIntSta = &prHifInfo->rRingIntSta;

	if (prRingIntSta->AllocVa)
		u4IntSta = *((uint32_t *)prRingIntSta->AllocVa);

	/* rx int & sw int */
	if (u4IntSta & (BITS(11, 15) | BIT(27)))
		return TRUE;

	if (!halIsWfdmaRxRingsEmpty(prAdapter->prGlueInfo))
		return TRUE;

	if (prMemOps->getWifiMiscRsvEmi) {
		prMem = prMemOps->getWifiMiscRsvEmi(
			prChipInfo, WIFI_MISC_MEM_BLOCK_SER_STATUS);
	}
	if (prMem && prMem->va) {
		struct SER_EMI_STATUS *prEmiSta =
			(struct SER_EMI_STATUS *)prMem->va;

		for (u4Idx = 0; u4Idx < HIF_EMI_SER_STATUS_SIZE; u4Idx++) {
			if (prEmiSta->ucStatus[u4Idx])
				return TRUE;
		}
	}
	return FALSE;
}

static void mt6653ReadIntStatusByEmi(struct ADAPTER *prAdapter,
				     uint32_t *pu4IntStatus)
{
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct HIF_MEM *prMem = NULL;
	struct RTMP_DMABUF *prRingIntSta;
#if CFG_ENABLE_MAWD_MD_RING
	struct RTMP_DMABUF *prRingMdIntSta;
#endif
	uint32_t u4Addr, u4RegValue = 0, u4WrValue = 0, u4SwIntSta = 0, u4Idx;
	u_int8_t fgClrCr = FALSE;

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	prChipInfo = prAdapter->chip_info;
	prMemOps = &prHifInfo->rMemOps;
	prRingIntSta = &prHifInfo->rRingIntSta;
#if CFG_ENABLE_MAWD_MD_RING
	prRingMdIntSta = &prHifInfo->rRingMdIntSta;
#endif

	*pu4IntStatus = 0;
	if (prRingIntSta->AllocVa)
		u4RegValue = *((uint32_t *)prRingIntSta->AllocVa);
	prHifInfo->u4IntStatus = u4RegValue & 0xFFFF;

#if (CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0)
	if (u4RegValue & BITS(0, 10)) {
		*pu4IntStatus |= WHISR_TX_DONE_INT;
		fgClrCr = TRUE;
	}
	u4WrValue |= BITS(0, 6);
#endif
#if (CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0)
	if (u4RegValue & BITS(0, 10)) {
		*pu4IntStatus |= WHISR_TX_DONE_INT;
		fgClrCr = TRUE;
	}
	u4WrValue |= BITS(7, 10);
#endif
	if (u4RegValue & BITS(11, 15)) {
		*pu4IntStatus |= WHISR_RX0_DONE_INT;
		fgClrCr = TRUE;
	}
	u4WrValue |= BITS(11, 15);

	prHifInfo->u4WbIntSta = u4WrValue;
	/* clear interrupt */
	if (fgClrCr) {
		DBGLOG(HAL, LOUD,
		       "EmiIntSta[0x%08x][0x%08x] WrValue[0x%08x]\n",
		       prHifInfo->u4IntStatus, u4RegValue, u4WrValue);
	}

	if (prMemOps->getWifiMiscRsvEmi) {
		prMem = prMemOps->getWifiMiscRsvEmi(
			prChipInfo, WIFI_MISC_MEM_BLOCK_SER_STATUS);
	}
	if (prMem && prMem->va) {
		struct SER_EMI_STATUS *prEmiSta =
			(struct SER_EMI_STATUS *)prMem->va;

		for (u4Idx = 0; u4Idx < HIF_EMI_SER_STATUS_SIZE; u4Idx++) {
			if (prEmiSta->ucStatus[u4Idx])
				u4SwIntSta |= BIT(u4Idx);
		}
	}

	/* clear err int */
	if ((u4RegValue & BIT(27)) || u4SwIntSta) {
		*pu4IntStatus |= WHISR_D2H_SW_INT;
		u4Addr = WF_WFDMA_HOST_DMA0_HOST_INT_STA_ADDR;
		u4WrValue = CONNAC_MCU_SW_INT | CONNAC_SUBSYS_INT;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrValue);
	}

#if CFG_ENABLE_MAWD_MD_RING
	fgClrCr = FALSE;
	u4RegValue = *((uint32_t *)prRingMdIntSta->AllocVa);
	prHifInfo->u4IntStatus |= (u4RegValue << 16);

	u4WrValue = 0;
	if (u4RegValue & BITS(8, 12)) {
		fgClrCr = TRUE;
		*pu4IntStatus |= WHISR_RX0_DONE_INT;
	}
	u4WrValue |= BITS(8, 12);

	prHifInfo->u4WbMdIntSta = u4WrValue;
	/* clear interrupt */
	if (fgClrCr) {
		DBGLOG(HAL, LOUD,
		       "MdEmiIntSta[0x%08x][0x%08x] WrValue[0x%08x]\n",
		       prHifInfo->u4IntStatus, u4RegValue, u4WrValue);
	}
#endif

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	mt6653ReadOffloadIntStatus(prAdapter, pu4IntStatus);
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
}

static void mt6653ProcessTxInterruptByEmi(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if (u4Sta & BIT(8))
		halWpdmaProcessCmdDmaDone(prGlueInfo, TX_RING_FWDL);

	if (u4Sta & BIT(7))
		halWpdmaProcessCmdDmaDone(prGlueInfo, TX_RING_CMD);

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
}

static void mt6653ProcessRxDataInterruptByEmi(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro)) {
		if (prHifInfo->u4OffloadIntStatus ||
		    (u4Sta & BITS(11, 13)) ||
		    (KAL_TEST_BIT(RX_RRO_DATA, prAdapter->ulNoMoreRfb)))
			halRroReadRxData(prAdapter);
#if CFG_ENABLE_MAWD_MD_RING
		if (u4Sta & BITS(24, 26))
			halRroReadRxData(prAdapter);
#endif
	} else
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
#endif /* _HIF_PCIE || _HIF_AXI */
	{
		if ((u4Sta & BIT(11)) ||
		    (KAL_TEST_BIT(RX_RING_DATA0, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA0, TRUE);

		if ((u4Sta & BIT(12)) ||
		    (KAL_TEST_BIT(RX_RING_DATA1, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA1, TRUE);

		if ((u4Sta & BIT(13)) ||
		    (KAL_TEST_BIT(RX_RING_DATA2, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA2, TRUE);

#if CFG_ENABLE_MAWD_MD_RING
		if ((u4Sta & BIT(24)) ||
		    (KAL_TEST_BIT(RX_RING_DATA3, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA3, TRUE);

		if ((u4Sta & BIT(25)) ||
		    (KAL_TEST_BIT(RX_RING_DATA4, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA4, TRUE);

		if ((u4Sta & BIT(26)) ||
		    (KAL_TEST_BIT(RX_RING_DATA5, prAdapter->ulNoMoreRfb)))
			halRxReceiveRFBs(prAdapter, RX_RING_DATA5, TRUE);
#endif /* CFG_ENABLE_MAWD_MD_RING */
	}
}

static void mt6653ProcessRxInterruptByEmi(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if ((u4Sta & BIT(14)) ||
	    (KAL_TEST_BIT(RX_RING_EVT, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_EVT, FALSE);

	if ((u4Sta & BIT(15)) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE0, FALSE);

#if CFG_ENABLE_MAWD_MD_RING
	if ((u4Sta & BIT(27)) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE1, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE1, FALSE);

	if ((u4Sta & BIT(28)) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE2, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE2, FALSE);
#endif /* CFG_ENABLE_MAWD_MD_RING */

	mt6653ProcessRxDataInterruptByEmi(prAdapter);
}

static void mt6653WfdmaConfigWriteBack(struct GLUE_INFO *prGlueInfo,
				       u_int8_t enable)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prRingDidx, *prRingIntSta;
	struct RTMP_DMABUF *prRingMdDidx, *prRingMdIntSta;
	struct RTMP_DMABUF *prRingDmyRd;
	uint32_t u4Addr = 0, u4WrVal = 0;
	uint32_t u4DmyRdExt = 0;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;

	prRingDidx = &prHifInfo->rRingDidx;
	prRingIntSta = &prHifInfo->rRingIntSta;
	prRingMdDidx = &prHifInfo->rRingMdDidx;
	prRingMdIntSta = &prHifInfo->rRingMdIntSta;
	prRingDmyRd = &prHifInfo->rRingDmyRd;

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_DMY_CTRL2_ADDR;
	u4WrVal = ((uint64_t)prRingDmyRd->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4DmyRdExt = (((uint64_t)prRingDmyRd->AllocPa >> DMA_BITS_OFFSET) <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_DMY_CTRL3_DMY_RD_BASE_PTR_EXT_SHFT) &
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_DMY_CTRL3_DMY_RD_BASE_PTR_EXT_MASK;
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_DMY_CTRL3_ADDR;
	u4WrVal = u4DmyRdExt |
		WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_MSI_DBG_MASK;
#if CFG_MTK_MDDP_SUPPORT
	/* enable md write back */
	u4WrVal |= BIT(10);
#endif
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL0_ADDR;
	u4WrVal = ((uint64_t)prRingDidx->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL1_ADDR;
	u4WrVal = ((uint64_t)prRingIntSta->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_ADDR;
	u4WrVal = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_TRINFO_WB_EN_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_NOC_BUS_SEL_MASK;
	u4WrVal |= (((uint64_t)prRingDidx->AllocPa >> DMA_BITS_OFFSET) &
		 DMA_HIGHER_4BITS_MASK) <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_DIDX_WB_BASE_PTR_EXT_SHFT;
	u4WrVal |= (((uint64_t)prRingIntSta->AllocPa >> DMA_BITS_OFFSET) &
		 DMA_HIGHER_4BITS_MASK) <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_INT_WB_BASE_PTR_EXT_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* disable tx done interrupt */
	u4Addr = WF_WFDMA_HOST_DMA0_HOST_TX_INT_PCIE_SEL_ADDR;
	HAL_MCR_WR(prAdapter, u4Addr, 0xffffffff);

	u4Addr = WF_WFDMA_HOST_DMA0_HOST_TX_INT_WB_EN_ADDR;
	u4WrVal = enable ? 0xF800 : 0;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

#if (CFG_MTK_MDDP_SUPPORT == 1) && (CFG_MTK_CCCI_SUPPORT == 1) && \
	(CFG_ENABLE_MAWD_MD_RING == 0)
	if (!prAdapter->u8MdRingIdxBase)
		prAdapter->u8MdRingIdxBase = (uint64_t)prRingMdDidx->AllocPa;
	if (!prAdapter->u8MdRingStaBase)
		prAdapter->u8MdRingStaBase = (uint64_t)prRingMdIntSta->AllocPa;

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_MD_CTRL0_ADDR;
	u4WrVal = (prAdapter->u8MdRingIdxBase) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_MD_CTRL1_ADDR;
	u4WrVal = (prAdapter->u8MdRingStaBase) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_MD_CTRL2_ADDR;
	u4WrVal = ((prAdapter->u8MdRingIdxBase >> DMA_BITS_OFFSET) &
		 DMA_HIGHER_4BITS_MASK) <<
WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_MD_CTRL2_DIDX_MD_WB_BASE_PTR_EXT_SHFT;
	u4WrVal |= ((prAdapter->u8MdRingStaBase >> DMA_BITS_OFFSET) &
		 DMA_HIGHER_4BITS_MASK) <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_MD_CTRL2_INT_MD_WB_BASE_PTR_EXT_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_ADDR;
	u4WrVal = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_TRINFO_WB_EN_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_NOC_BUS_SEL_MASK;
	u4WrVal |= ((prAdapter->u8MdRingIdxBase >> DMA_BITS_OFFSET) &
		 DMA_HIGHER_4BITS_MASK) <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_DIDX_WB_BASE_PTR_EXT_SHFT;
	u4WrVal |= ((prAdapter->u8MdRingStaBase >> DMA_BITS_OFFSET) &
		 DMA_HIGHER_4BITS_MASK) <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_INT_WB_BASE_PTR_EXT_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
#else
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_MD_CTRL0_ADDR;
	u4WrVal = ((uint64_t)prRingMdDidx->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_MD_CTRL1_ADDR;
	u4WrVal = ((uint64_t)prRingMdIntSta->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_MD_CTRL2_ADDR;
	u4WrVal = (((uint64_t)prRingMdDidx->AllocPa >> DMA_BITS_OFFSET) &
		 DMA_HIGHER_4BITS_MASK) <<
WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_MD_CTRL2_DIDX_MD_WB_BASE_PTR_EXT_SHFT;
	u4WrVal |= (((uint64_t)prRingMdIntSta->AllocPa >> DMA_BITS_OFFSET) &
		 DMA_HIGHER_4BITS_MASK) <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_MD_CTRL2_INT_MD_WB_BASE_PTR_EXT_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_ADDR;
	u4WrVal = WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_TRINFO_WB_EN_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_NOC_BUS_SEL_MASK;
	u4WrVal |= (((uint64_t)prRingMdDidx->AllocPa >> DMA_BITS_OFFSET) &
		 DMA_HIGHER_4BITS_MASK) <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_DIDX_WB_BASE_PTR_EXT_SHFT;
	u4WrVal |= (((uint64_t)prRingMdIntSta->AllocPa >> DMA_BITS_OFFSET) &
		 DMA_HIGHER_4BITS_MASK) <<
	WF_WFDMA_HOST_DMA0_WPDMA_TRINFO_WB_CTRL2_INT_WB_BASE_PTR_EXT_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

#if CFG_ENABLE_MAWD_MD_RING
	u4Addr = WF_WFDMA_HOST_DMA0_HOST_RX_INT_WB_EN_ADDR;
	u4WrVal = 0x3E00;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
#endif /* CFG_ENABLE_MAWD_MD_RING */
#endif /* CFG_MTK_MDDP_SUPPORT */
}

static void mt6653ConfigEmiIntMask(struct GLUE_INFO *prGlueInfo,
				   u_int8_t enable)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint32_t u4Addr = 0, u4WrVal = 0;

	u4Addr = WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR;
	u4WrVal = 0;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	mt6653WfdmaConfigWriteBack(prGlueInfo, enable);
}
#endif /* CFG_MTK_WIFI_WFDMA_WB */

#if CFG_MTK_WIFI_WFDMA_WB
static void mt6653EnableInterruptViaPcieByEmi(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	uint32_t u4Addr;

	asicConnac3xEnablePlatformIRQ(prAdapter);

	/* enable wfdma writeback interrupt */
	u4Addr = WF_WFDMA_HOST_DMA0_HOST_TX_INT_WB_EN_ADDR;
	HAL_MCR_WR(prAdapter, u4Addr, prHifInfo->u4WbIntSta);

#if CFG_ENABLE_MAWD_MD_RING
	u4Addr = WF_WFDMA_HOST_DMA0_HOST_RX_INT_WB_EN_ADDR;
	HAL_MCR_WR(prAdapter, u4Addr, prHifInfo->u4WbMdIntSta);
#endif
}
#endif /* CFG_MTK_WIFI_WFDMA_WB */

#if defined(_HIF_PCIE) && (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
static void mt6653EnableInterruptViaPcie(struct ADAPTER *prAdapter)
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

static void mt6653EnableInterrupt(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->configWfdmaIntMask)
		prBusInfo->configWfdmaIntMask(prAdapter->prGlueInfo, TRUE);
	asicConnac3xEnablePlatformIRQ(prAdapter);
}

static void mt6653DisableInterrupt(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->configWfdmaIntMask)
		prBusInfo->configWfdmaIntMask(prAdapter->prGlueInfo, FALSE);
	asicConnac3xDisablePlatformIRQ(prAdapter);
}

static void mt6653WpdmaMsiConfig(struct ADAPTER *prAdapter)
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

	/* MSI only need to read int status once */
	prAdapter->rWifiVar.u4HifIstLoopCount = 1;

#if (WFDMA_AP_MSI_NUM == 8)
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

static void mt6653ConfigWfdmaRxRingThreshold(
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

static void mt6653UpdateWfdmaPrdcInt(
	struct GLUE_INFO *prGlueInfo, u_int8_t fgForceEn)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t u4Addr, u4Val, u4Time;

	u4Time = prGlueInfo->fgIsInSuspendMode ?
		prWifiVar->u4SuspendPrdcIntTime : prWifiVar->u4PrdcIntTime;

	if (!fgForceEn && u4Time == prAdapter->u4CurPrdcIntTime)
		return;

	prAdapter->u4CurPrdcIntTime = u4Time;

	/* clear before set */
	u4Addr = WF_WFDMA_HOST_DMA0_HOST_PER_DLY_INT_CFG_ADDR;
	HAL_MCR_WR(prAdapter, u4Addr, 0);

	/* Enable RX periodic delayed interrupt (unit: 20us) */
	u4Val = 0x1F00000 | u4Time;
#if CFG_MTK_MDDP_SUPPORT
	u4Val |= 0x3E000000;
#endif
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	DBGLOG(HAL, INFO, "prdc int: %uus", u4Time * 20);
}

static void mt6653WpdmaDlyInt(struct GLUE_INFO *prGlueInfo)
{
#if CFG_SUPPORT_WFDMA_RX_DELAY_INT
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t u4Addr, u4Val;

	/* setup ring 4-8 delay interrupt */
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

	u4Addr = WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_1_ADDR;
	u4Val = (8 <<
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_1_DLY_RING_IDX4_SHFT) |
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_1_DLY_FUNC_DIR4_MASK |
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_DLY_IDX_CFG_1_DLY_FUNC_ENA4_MASK;
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

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG2_ADDR;
	u4Val = prWifiVar->u4DlyIntTime <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG2_PRI0_MAX_PTIME_SHFT |
		prWifiVar->u4DlyIntCnt <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG2_PRI0_MAX_PINT_SHFT |
		prWifiVar->fgEnDlyInt <<
		WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG2_PRI0_DLY_INT_EN_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	DBGLOG(HAL, INFO, "dly int[%u]: %uus, cnt=%u",
	       prWifiVar->fgEnDlyInt,
	       prWifiVar->u4DlyIntTime * 20,
	       prWifiVar->u4DlyIntCnt);
#endif /* CFG_SUPPORT_WFDMA_RX_DELAY_INT */

	mt6653UpdateWfdmaPrdcInt(prGlueInfo, TRUE);
}

static void mt6653WpdmaConfigExt0(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t u4Addr = 0, u4Val = 0, u4DefVal = 0;

	/* enable SDO */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_ADDR;
	/* default settings */
	u4DefVal = 0x28C004DF;
	u4Val = u4DefVal;

	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableSdo)) {
		u4Val |=
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_SDO_DISP_MODE_MASK;
	}

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro)) {
		u4Val |=
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_RX_WB_RXD_MASK;
	}
#endif

	if (u4Val != u4DefVal)
		HAL_MCR_WR(prAdapter, u4Addr, u4Val);
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
}

static void mt6653WpdmaConfigExt1(struct ADAPTER *prAdapter)
{
	uint32_t u4Addr = 0, u4Val = 0;

	/* packet based TX flow control */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT1_ADDR;
	/* default settings */
	u4Val = 0x8C800404 |
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT1_CSR_TX_FCTRL_MODE_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

static void mt6653WpdmaConfigExt2(struct ADAPTER *prAdapter)
{
	uint32_t u4Addr = 0, u4Val = 0;

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT2_ADDR;
	/* default settings */
	u4Val = 0x00001404;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);
}

static void mt6653WfdmaControl(struct ADAPTER *prAdapter, u_int8_t fgEn)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	union WPDMA_GLO_CFG_STRUCT *prGloCfg = &prHifInfo->GloCfg;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct BUS_INFO *prBusInfo = prChipInfo->bus_info;
	uint32_t u4Addr = 0;

	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR;
	/* default settings */
	prGloCfg->word = 0x5030b850;

	if (fgEn) {
		prGloCfg->word |=
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_RX_DMA_EN_MASK;
	}

	if (prBusInfo->u4DmaMask > 32) {
		prGloCfg->word |=
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_PDMA_ADDR_EXT_EN_MASK;
	}

	HAL_MCR_WR(prAdapter, u4Addr, prGloCfg->word);
	prHifInfo->GloCfg.word = prGloCfg->word;
}

static void mt6653WpdmaConfig(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable, bool fgResetHif)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

	mt6653WfdmaControl(prAdapter, enable);

	if (!enable)
		return;

#if CFG_MTK_WIFI_WFDMA_WB
	if (prAdapter->chip_info->is_support_wfdma_write_back)
		mt6653ConfigEmiIntMask(prGlueInfo, TRUE);
#endif /* CFG_MTK_WIFI_WFDMA_WB */

#if defined(_HIF_PCIE)
	mt6653WpdmaMsiConfig(prAdapter);
#endif
	mt6653ConfigWfdmaRxRingThreshold(prAdapter, 0, FALSE);

	mt6653WpdmaConfigExt0(prAdapter);
	mt6653WpdmaConfigExt1(prAdapter);
	mt6653WpdmaConfigExt2(prAdapter);

	mt6653WpdmaDlyInt(prGlueInfo);
}

#if CFG_MTK_WIFI_WFDMA_WB
static void mt6653WfdmaTxRingWbExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *prTxRing,
	uint32_t u4Idx)
{
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct WFDMA_EMI_RING_IDX_0 *prRingDidx;
	struct WFDMA_EMI_RING_IDX_1 *prRingMdDidx;
	int i4EmiRingIdx = -1;
#if CFG_ENABLE_MAWD_MD_RING
	u_int8_t fgIsSet1 = FALSE;
#endif

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prRingDidx = (struct WFDMA_EMI_RING_IDX_0 *)
		prHifInfo->rRingDidx.AllocVa;
	prRingMdDidx = (struct WFDMA_EMI_RING_IDX_1 *)
		prHifInfo->rRingMdDidx.AllocVa;

	if (!prChipInfo->is_support_wfdma_write_back)
		return;

	switch (u4Idx) {
	case TX_RING_DATA0:
		i4EmiRingIdx = 0;
		break;
	case TX_RING_DATA1:
		i4EmiRingIdx = 1;
		break;
	case TX_RING_DATA2:
		i4EmiRingIdx = 2;
		break;
	case TX_RING_DATA3:
#if CFG_ENABLE_MAWD_MD_RING
		fgIsSet1 = TRUE;
		i4EmiRingIdx = 0;
#else
		i4EmiRingIdx = 3;
#endif
		break;
	case TX_RING_DATA_PRIO:
#if CFG_ENABLE_MAWD_MD_RING
		fgIsSet1 = TRUE;
		i4EmiRingIdx = 1;
#else
		i4EmiRingIdx = 4;
#endif
		break;
	case TX_RING_DATA_ALTX:
#if CFG_ENABLE_MAWD_MD_RING
		fgIsSet1 = TRUE;
		i4EmiRingIdx = 2;
#else
		i4EmiRingIdx = 5;
#endif
		break;
	case TX_RING_CMD:
		i4EmiRingIdx = 7;
		break;
	case TX_RING_FWDL:
		i4EmiRingIdx = 8;
		break;
	default:
		return;
	}

#if CFG_ENABLE_MAWD_MD_RING
	if (fgIsSet1)
		prTxRing->pu2EmiDidx = &prRingMdDidx->u2TxRing[i4EmiRingIdx];
	else
#endif
		prTxRing->pu2EmiDidx = &prRingDidx->u2TxRing[i4EmiRingIdx];

	prTxRing->fgEnEmiDidx = TRUE;
	*prTxRing->pu2EmiDidx = 0;
}

static void mt6653WfdmaRxRingWbExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *prRxRing,
	uint32_t u4Idx)
{
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct WFDMA_EMI_RING_IDX_0 *prRingDidx;
	struct WFDMA_EMI_RING_IDX_1 *prRingMdDidx;
	int i4EmiRingIdx = -1;
#if CFG_ENABLE_MAWD_MD_RING
	u_int8_t fgIsSet1 = FALSE;
#endif

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prRingDidx = (struct WFDMA_EMI_RING_IDX_0 *)
		prHifInfo->rRingDidx.AllocVa;
	prRingMdDidx = (struct WFDMA_EMI_RING_IDX_1 *)
		prHifInfo->rRingMdDidx.AllocVa;

	if (!prChipInfo->is_support_wfdma_write_back)
		return;

	switch (u4Idx) {
	case RX_RING_EVT:
		i4EmiRingIdx = 3;
		break;
	case RX_RING_DATA0:
		i4EmiRingIdx = 0;
		break;
	case RX_RING_DATA1:
		i4EmiRingIdx = 1;
		break;
	case RX_RING_DATA2:
		i4EmiRingIdx = 2;
		break;
	case RX_RING_TXDONE0:
		i4EmiRingIdx = 4;
		break;
#if CFG_ENABLE_MAWD_MD_RING
	case RX_RING_DATA3:
		fgIsSet1 = TRUE;
		i4EmiRingIdx = 0;
		break;
	case RX_RING_DATA4:
		fgIsSet1 = TRUE;
		i4EmiRingIdx = 1;
		break;
	case RX_RING_DATA5:
		fgIsSet1 = TRUE;
		i4EmiRingIdx = 2;
		break;
	case RX_RING_TXDONE1:
		fgIsSet1 = TRUE;
		i4EmiRingIdx = 3;
		break;
	case RX_RING_TXDONE2:
		fgIsSet1 = TRUE;
		i4EmiRingIdx = 4;
		break;
#endif /* CFG_ENABLE_MAWD_MD_RING */
	default:
		return;
	}

#if CFG_ENABLE_MAWD_MD_RING
	if (fgIsSet1)
		prRxRing->pu2EmiDidx = &prRingMdDidx->u2RxRing[i4EmiRingIdx];
	else
#endif
		prRxRing->pu2EmiDidx = &prRingDidx->u2RxRing[i4EmiRingIdx];

	prRxRing->fgEnEmiDidx = TRUE;
	*prRxRing->pu2EmiDidx = 0;
}
#endif /* CFG_MTK_WIFI_WFDMA_WB */

static void mt6653WfdmaTxRingExtCtrl(
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
	mt6653WfdmaTxRingWbExtCtrl(prGlueInfo, prTxRing, index);
#endif /* CFG_MTK_WIFI_WFDMA_WB */
}

static void mt6653WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *prRxRing,
	u_int32_t index)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	uint32_t u4Offset = 0, u4RingIdx = 0, u4Val = 0;
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	struct WIFI_VAR *prWifiVar;
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	prWifiVar = &prAdapter->rWifiVar;
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

	u4RingIdx = mt6653RxRingSwIdx2HwIdx(index);
	if (u4RingIdx >= RX_RING_MAX)
		return;

	u4Offset = u4RingIdx * 4;

	prRxRing->hw_desc_base_ext =
		prBusInfo->host_rx_ring_ext_ctrl_base + u4Offset;
	HAL_MCR_WR(prAdapter, prRxRing->hw_desc_base_ext,
		   CONNAC3X_RX_RING_DISP_MAX_CNT);

	u4Val = prRxRing->u4RingSize;
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	/* enable wfdma magic cnt */
	if (IS_FEATURE_ENABLED(prWifiVar->fgEnableRro) &&
	    halIsDataRing(RX_RING, index))
		u4Val |= WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL1_MGC_ENA_MASK;
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

	asicConnac3xWfdmaRxRingBasePtrExtCtrl(
		prGlueInfo, prRxRing, index, u4Val);

#if CFG_MTK_WIFI_WFDMA_WB
	mt6653WfdmaRxRingWbExtCtrl(prGlueInfo, prRxRing, index);
#endif /* CFG_MTK_WIFI_WFDMA_WB */
}

#if defined(_HIF_PCIE)
static void mt6653PcieMsiDebugDump(struct ADAPTER *prAdapter)
{
	uint32_t u4ReadVal = 0;

	HAL_RMCR_RD(HIF_DBG, prAdapter, 0x74030188, &u4ReadVal);
	DBGLOG(HAL, INFO, "074030188: 0x%08X\n", u4ReadVal);
	HAL_RMCR_RD(HIF_DBG, prAdapter, 0x70025018, &u4ReadVal);
	DBGLOG(HAL, INFO, "0x70025018: 0x%08X\n", u4ReadVal);
	HAL_RMCR_RD(HIF_DBG, prAdapter, 0x740310E0, &u4ReadVal);
	DBGLOG(HAL, INFO, "0x740310E0: 0x%08X\n", u4ReadVal);
	HAL_RMCR_RD(HIF_DBG, prAdapter, 0x740310F0, &u4ReadVal);
	DBGLOG(HAL, INFO, "0x740310F0: 0x%08X\n", u4ReadVal);

	if (pcie_vir_addr) {
		u4ReadVal = readl(pcie_vir_addr + 0xc14);
		DBGLOG(HAL, INFO, "0x16910c14: 0x%08X\n", u4ReadVal);
		u4ReadVal = readl(pcie_vir_addr + 0xc18);
		DBGLOG(HAL, INFO, "0x16910c18: 0x%08X\n", u4ReadVal);
		u4ReadVal = readl(pcie_vir_addr + 0xc1c);
		DBGLOG(HAL, INFO, "0x16910c1c: 0x%08X\n", u4ReadVal);
	} else
		DBGLOG(HAL, ERROR, "0x16910000: ioremap fail\n");

}

static void mt6653RecoveryMsiStatus(struct ADAPTER *prAdapter, u_int8_t fgForce)
{
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct pcie_msi_info *prMsiInfo = &prBusInfo->pcie_msi_info;
	uint32_t u4Val = 0, u4Cnt = 0, u4RxCnt, i;
#if (WFDMA_AP_MSI_NUM == 1)
	uint32_t u4IntMask = BIT(0);
#else
	uint32_t u4IntMask = BITS(0, 7);
#endif
#if CFG_MTK_WIFI_WFDMA_WB
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	struct RTMP_DMABUF *prRingIntSta = &prHifInfo->rRingIntSta;
	uint32_t u4Addr = 0, u4IntSta = 0, u4AfterVal;
	u_int8_t fgRet = FALSE;
#endif /* CFG_MTK_WIFI_WFDMA_WB */

	if (fgForce)
		goto recovery;

	/* check wfdma bits(0-7) */
	if (prMsiInfo->ulEnBits & 0xff)
		return;

	if (time_before(jiffies, prBusInfo->ulRecoveryMsiCheckTime))
		return;

	prBusInfo->ulRecoveryMsiCheckTime = jiffies +
		prAdapter->rWifiVar.u4RecoveryMsiShortTime * HZ / 1000;

	u4Cnt = halGetWfdmaRxCnt(prAdapter);
	if (u4Cnt < prWifiVar->u4RecoveryMsiRxCnt)
		return;

	for (i = 0; i < NUM_OF_RX_RING; i++) {
		u4RxCnt = halWpdmaGetRxDmaDoneCnt(prAdapter->prGlueInfo, i);
		if (u4RxCnt == 0)
			continue;

		if (halIsWfdmaRxCidxChanged(prAdapter, i))
			fgRet = TRUE;
	}
	if (fgRet)
		return;

recovery:
	/* read PCIe EP MSI status */
	u4Val = mtk_pci_read_msi_mask(prAdapter->prGlueInfo);
	if (u4Val & u4IntMask) {
		mtk_pci_msi_unmask_all_irq(prAdapter->prGlueInfo);
		DBGLOG(HAL, WARN,
		       "unmask msi Rx[%u] MSI_MASK=[0x%08x] EP=[0x%08x]",
		       u4Cnt, u4IntMask, u4Val);
	}

#if CFG_MTK_WIFI_WFDMA_WB
	if (prRingIntSta->AllocVa)
		u4IntSta = *((uint32_t *)prRingIntSta->AllocVa);
	/* enable wfdma writeback interrupt */
	u4Addr = WF_WFDMA_HOST_DMA0_HOST_TX_INT_WB_EN_ADDR;
	HAL_MCR_EMI_RD(prAdapter, u4Addr, &u4Val, &fgRet);
	HAL_MCR_WR(prAdapter, u4Addr, 0xF800);
	HAL_MCR_EMI_RD(prAdapter, u4Addr, &u4AfterVal, &fgRet);

	DBGLOG(HAL, WARN,
		"Emi=[0x%08x] WbIntSta=[0x%08x] [0x%08x]=[0x%08x]->[0x%08x]",
	       u4IntSta, prHifInfo->u4WbIntSta, u4Addr, u4Val, u4AfterVal);
#endif /* CFG_MTK_WIFI_WFDMA_WB */
}

static void mt6653RecoverSerStatus(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct HIF_MEM *prMem = NULL;
	uint32_t u4Idx;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prChipInfo = prAdapter->chip_info;

	if (prMemOps->getWifiMiscRsvEmi) {
		prMem = prMemOps->getWifiMiscRsvEmi(
			prChipInfo, WIFI_MISC_MEM_BLOCK_SER_STATUS);
	}

	if (prMem && prMem->va) {
		struct SER_EMI_STATUS *prEmiSta =
			(struct SER_EMI_STATUS *)prMem->va;

		for (u4Idx = 0; u4Idx < HIF_EMI_SER_STATUS_SIZE; u4Idx++) {
			if (prEmiSta->ucStatus[u4Idx]) {
				nicProcessSoftwareInterruptEx(prAdapter);
				break;
			}
		}
	}
}

static void mt6653CheckFwOwnMsiStatus(struct ADAPTER *prAdapter)
{
	mt6653RecoveryMsiStatus(prAdapter, FALSE);
}

#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
static void mt6653PcieMsiMaskIrq(uint32_t u4Irq, uint32_t u4Bit)
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

static void mt6653PcieMsiUnmaskIrq(uint32_t u4Irq, uint32_t u4Bit)
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

static void mt6653InitPcieInt(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4WrVal = 0x08021000, u4Val = 0;

#if CFG_SUPPORT_PCIE_ASPM_EP
	HAL_MCR_WR(prGlueInfo->prAdapter, 0x74030074, u4WrVal);
#endif
#if defined(_HIF_PCIE)
	mt6653_notify_fw_disable_SR(prGlueInfo->prAdapter);
#endif
	if (!pcie_vir_addr) {
		DBGLOG(HAL, INFO, "pcie_vir_addr is null\n");
		return;
	}

	writel(u4WrVal, (pcie_vir_addr + 0x74));
	u4Val = readl(pcie_vir_addr + 0x74);
	DBGLOG(HAL, INFO,
	       "pcie_addr=0x%llx, write 0x74=[0x%08x], read 0x74=[0x%08x]\n",
	       (uint64_t)pcie_vir_addr, u4WrVal, u4Val);
}

static void mt6653PcieHwControlVote(
	struct ADAPTER *prAdapter,
	uint8_t enable,
	uint32_t u4WifiUser)
{
	halPcieHwControlVote(prAdapter, enable, u4WifiUser);
}

#if CFG_SUPPORT_PCIE_ASPM
static u_int8_t mt6653SetL1ssEnable(struct ADAPTER *prAdapter,
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
	else if (role == WIFI_RST_ROLE)
		prChipInfo->bus_info->fgWifiRstEnL1_2 = fgEn;

	DBGLOG(HAL, TRACE,
		"fgWifiEnL1_2 = %d, fgMDEnL1_2=%d, fgWifiRstEnL1_2=%d\n",
		prChipInfo->bus_info->fgWifiEnL1_2,
		prChipInfo->bus_info->fgMDEnL1_2,
		prChipInfo->bus_info->fgWifiRstEnL1_2);

	if (prChipInfo->bus_info->fgWifiEnL1_2 &&
	    prChipInfo->bus_info->fgMDEnL1_2 &&
	    prChipInfo->bus_info->fgWifiRstEnL1_2)
		return TRUE;
	else
		return FALSE;
}
static uint32_t mt6653ConfigPcieAspm(struct GLUE_INFO *prGlueInfo,
				u_int8_t fgEn, u_int enable_role)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t value = 0, delay = 0, value1 = 0, u4PollTimeout = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	u_int8_t enableL1ss = FALSE;
	u_int8_t isL0Status = FALSE;

	if (pcie_vir_addr == NULL) {
		DBGLOG(HAL, ERROR, "get pcie_vir_addr null\n");
		return WLAN_STATUS_FAILURE;
	}

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	spin_lock_bh(&rPCIELock);
	enableL1ss =
		mt6653SetL1ssEnable(prGlueInfo->prAdapter, enable_role, fgEn);

	u4PollTimeout = (prGlueInfo->prAdapter->fgIsFwOwn) ?
		POLLING_TIMEOUT_IN_UDS : POLLING_TIMEOUT;

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

				if (delay >= u4PollTimeout) {
					DBGLOG(HAL, INFO,
						"Enable L1.2 POLLING_TIMEOUT\n");
					rStatus = WLAN_STATUS_FAILURE;
					goto exit;
				}

				delay += 10;
				udelay(10);
			}
#if CFG_SUPPORT_PCIE_ASPM_EP
			HAL_MCR_WR(prGlueInfo->prAdapter,
				0x74030194, 0xf);
			HAL_RMCR_RD(PCIEASPM_READ, prGlueInfo->prAdapter,
				0x74030194, &value);
#endif
			writel(0xf, (pcie_vir_addr + 0x194));


			DBGLOG(HAL, LOUD, "Enable aspm L1.1/L1.2..\n");
		} else {
			DBGLOG(HAL, LOUD, "Not to enable aspm L1.1/L1.2..\n");
		}
	} else {
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

			if (delay >= u4PollTimeout) {
				DBGLOG(HAL, INFO,
					"Disable L1.2 POLLING_TIMEOUT\n");
				rStatus = WLAN_STATUS_FAILURE;
				goto exit;
			}

			delay += 10;
			udelay(10);
		}

		if (prHifInfo->eCurPcieState == PCIE_STATE_L0)
			value1 = 0xe0f;
		else
			value1 = 0xc0f;
#if CFG_SUPPORT_PCIE_ASPM_EP
		HAL_MCR_WR(prGlueInfo->prAdapter, 0x74030194, value1);
		HAL_RMCR_RD(PCIEASPM_READ, prGlueInfo->prAdapter,
			    0x74030194, &value);
#endif
		writel(value1, (pcie_vir_addr + 0x194));

		if (prHifInfo->eCurPcieState == PCIE_STATE_L0)
			DBGLOG(HAL, LOUD, "Disable aspm L1..\n");
		else
			DBGLOG(HAL, LOUD, "Disable aspm L1.1/L1.2..\n");
	}

exit:
	spin_unlock_bh(&rPCIELock);
	return rStatus;
}

static void mt6653UpdatePcieAspm(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn)
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
			mt6653ConfigPcieAspm(prGlueInfo, TRUE, 1);
		else
			mt6653ConfigPcieAspm(prGlueInfo, FALSE, 1);
		prHifInfo->eCurPcieState = prHifInfo->eNextPcieState;
	}
}

static void mt6653KeepPcieWakeup(struct GLUE_INFO *prGlueInfo,
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
#endif //CFG_SUPPORT_PCIE_ASPM

static void mt6653ShowPcieDebugInfo(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint32_t u4Addr, u4Val = 0, u4Idx;
	uint32_t u4BufSize = 512, pos = 0;
	char *buf;
	uint32_t au4PcieEpReg[] = {
		0x74030188, 0x7403018C, 0x740310f0, 0x740310f4, 0x70025018
	};

	buf = (char *)kalMemAlloc(u4BufSize, PHY_MEM_TYPE);
	if (!buf) {
		DBGLOG(HAL, WARN, "buffer alloc fail%s\n", buf);
		return;
	}

#if CFG_MTK_WIFI_PCIE_SUPPORT
	u4Val = mtk_pcie_dump_link_info(0);
	pos += kalSnprintf(buf + pos, u4BufSize - pos,
			   "link_info:0x%x ", u4Val);
#endif

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4PcieEpReg); u4Idx++) {
		u4Addr = au4PcieEpReg[u4Idx];
		HAL_RMCR_RD(HIF_DBG, prAdapter, u4Addr, &u4Val);
		pos += kalSnprintf(buf + pos, u4BufSize - pos,
				   "[0x%08x]=[0x%08x] ", u4Addr, u4Val);
	}

	DBGLOG(HAL, INFO, "%s\n", buf);
	kalMemFree(buf, PHY_MEM_TYPE, u4BufSize);
}

#if CFG_SUPPORT_PCIE_ASPM
static u_int8_t mt6653DumpPcieDateFlowStatus(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4RegVal[3] = {0};
#if CFG_MTK_WIFI_PCIE_SUPPORT
	uint32_t link_info = mtk_pcie_dump_link_info(0);
#endif

#if CFG_MTK_WIFI_PCIE_SUPPORT
	DBGLOG(HAL, INFO, "link_info %u\n", link_info);
	if (!(link_info & BIT(5))) {
		if ((link_info & BIT(10))) {
			/* SDES try recover link */
			kalMdelay(48);
		} else
			return FALSE;
	}
#endif

	glReadPcieCfgSpace(0x0, &u4RegVal[0]);
	if (u4RegVal[0] == 0 || u4RegVal[0] == 0xffffffff) {
		DBGLOG(HAL, INFO,
		       "PCIE link down 0x0=0x%08x\n", u4RegVal[0]);
		/* block pcie to prevent access */
#if CFG_MTK_WIFI_PCIE_SUPPORT
		mtk_pcie_disable_data_trans(0);
#endif
		return FALSE;
	}

	/*1. read pcie cfg.space 0x488 // Readable check*/
	glReadPcieCfgSpace(0x488, &u4RegVal[1]);
	if ((u4RegVal[1] & 0x3811) != 0x3811 ||
	    u4RegVal[1] == 0xffffffff) {
		glReadPcieCfgSpace(0x48C, &u4RegVal[2]);
		DBGLOG(HAL, INFO,
		       "Cb_infra bus fatal error and un-readble 0x488=0x%08x 0x48C=0x%08x\n",
		       u4RegVal[1], u4RegVal[2]);
		return FALSE;
	}

#if CFG_MTK_WIFI_PCIE_SR
	if (!fgIsL2Finished) {
		DBGLOG(HAL, INFO, "L2 not finished\n");
		return FALSE;
	}
#endif

#if CFG_MTK_WIFI_PCIE_SUPPORT
	/* MalfTLP */
	if (link_info & BIT(8)) {
		wlanUpdateBusAccessStatus(TRUE);
#ifdef CFG_MTK_WIFI_CONNV3_SUPPORT
		fgTriggerDebugSop = TRUE;
#endif
		return FALSE;
	}
#endif

#if CFG_MTK_WIFI_PCIE_SUPPORT
	if (link_info & BIT(10)) {
		link_info = mtk_pcie_dump_link_info(0);
		if (link_info & BIT(5)) {
			if (pcie_restore_config_space_settings(
				prGlueInfo->prAdapter) != 0)
				return FALSE;
			wlanUpdateBusAccessStatus(FALSE);
#ifdef CFG_MTK_WIFI_CONNV3_SUPPORT
			fgTriggerDebugSop = FALSE;
#endif
		}
	}
#endif

	return TRUE;
}
#endif /* CFG_SUPPORT_PCIE_ASPM */
static void mt6653SetupMcuEmiAddr(struct ADAPTER *prAdapter)
{
#define SETUP_MCU_EMI2_BASE_ADDRESS (0x7C05B2EC)
#define SETUP_MCU_EMI2_SIZE_ADDRESS (0x7C05B2E4)

	phys_addr_t base = emi_mem_get_phy_base(prAdapter->chip_info);
	uint32_t size = emi_mem_get_size(prAdapter->chip_info);
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct HIF_MEM_OPS *prMemOps = &prHifInfo->rMemOps;
	struct HIF_MEM *prMem = NULL;
	uint8_t uIdx = 0;
#endif

	if (!base)
		return;

	DBGLOG(HAL, INFO, "base: 0x%llx, size: 0x%x\n", base, size);

	HAL_MCR_WR(prAdapter,
		   CONNAC3X_CONN_CFG_ON_CONN_ON_EMI_ADDR,
		   ((uint32_t)(base >> 16)));

	HAL_MCR_WR(prAdapter,
		   MT6653_EMI_SIZE_ADDR,
		   size);

	/* Get EMI2 Info */
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	size = 0;
	if (prMemOps->getWifiMiscRsvEmi &&
		prAdapter->chip_info->rsvMemWiFiMisc) {
		prMem = prMemOps->getWifiMiscRsvEmi(prAdapter->chip_info, 0);
		if (prMem == NULL) {
			DBGLOG(NIC, INFO, "not support EMI2\n");
			return;
		}
		base = prMem->pa;

		for (uIdx = 0; uIdx <
			prAdapter->chip_info->rsvMemWiFiMiscSize; uIdx++)
			size += prAdapter->chip_info->rsvMemWiFiMisc[uIdx].size;
	}
	DBGLOG(HAL, INFO, "emi2 base: 0x%llx, size: 0x%x\n", base, size);

	HAL_MCR_WR(prAdapter, SETUP_MCU_EMI2_BASE_ADDRESS,
		   ((uint32_t)(base >> 16)));
	HAL_MCR_WR(prAdapter, SETUP_MCU_EMI2_SIZE_ADDRESS,
		   size);
#endif
}

static u_int8_t mt6653_get_sw_interrupt_status(struct ADAPTER *prAdapter,
	uint32_t *pu4Status)
{
	*pu4Status = ccif_get_interrupt_status(prAdapter);
	return TRUE;
}

static uint32_t mt6653_ccif_get_interrupt_status(struct ADAPTER *ad)
{
	uint32_t u4Status = 0;

	HAL_RMCR_RD(CCIF_READ, ad,
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_RCHNUM_ADDR,
		&u4Status);
	HAL_MCR_WR(ad,
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_ACK_ADDR,
		u4Status);

	return u4Status;
}

static void mt6653_ccif_notify_utc_time_to_fw(struct ADAPTER *ad,
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

static void mt6653_ccif_set_fw_log_read_pointer(struct ADAPTER *ad,
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

static uint32_t mt6653_ccif_get_fw_log_read_pointer(struct ADAPTER *ad,
	enum ENUM_FW_LOG_CTRL_TYPE type)
{
	uint32_t u4Addr = 0, u4Value = 0;

	if (type == ENUM_FW_LOG_CTRL_TYPE_MCU)
		u4Addr = WF2AP_CONN_INFRA_ON_CCIF4_WF2AP_PCCIF_DUMMY2_ADDR;
	else
		u4Addr = WF2AP_CONN_INFRA_ON_CCIF4_WF2AP_PCCIF_DUMMY1_ADDR;

	HAL_RMCR_RD(CCIF_READ, ad, u4Addr, &u4Value);

	return u4Value;
}

static void mt6653_force_conn_infra_on(struct ADAPTER *ad,
	u_int8_t fgForceOn)
{
	uint32_t u4WriteValue = (fgForceOn == TRUE) ? 1 : 0;

	HAL_MCR_WR(ad,
		CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_WF_ADDR,
		u4WriteValue);
}

static int32_t mt6653_ccif_trigger_fw_assert(struct ADAPTER *ad)
{
	mt6653_force_conn_infra_on(ad, TRUE);
	mdelay(5);

	HAL_MCR_WR(ad,
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_TCHNUM_ADDR,
		SW_INT_SUBSYS_RESET);

	return 0;
}

u_int8_t mt6653_is_ap2conn_off_readable(struct ADAPTER *ad)
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
	if (value != MT6653_CONNINFRA_VERSION_ID) {
		DBGLOG(HAL, ERROR,
			"Conninfra ver id: 0x%08x\n",
			value);
		return FALSE;
	}
	return TRUE;
}

u_int8_t mt6653_is_conninfra_bus_timeout(struct ADAPTER *ad)
{
	uint32_t value = 0;

	HAL_RMCR_RD(PLAT_DBG, ad,
		   CONN_DBG_CTL_CONN_INFRA_BUS_DBG_CR_00_ADDR,
		   &value);
	if ((value & BITS(0, 9)) != 0) {
		DBGLOG(HAL, ERROR,
			"Conninfra bus timeout irq status: 0x%08x\n",
			value);
		return TRUE;
	}

	return FALSE;
}

/* Please do ap2conn_off_readable check before execute
 * conn2wf_readable check
 */
u_int8_t mt6653_is_conn2wf_readable(struct ADAPTER *ad)
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
	if (value != MT6653_WF_VERSION_ID) {
		DBGLOG(HAL, ERROR,
			"WF ver id: 0x%08x\n",
			value);
		return FALSE;
	}

	return TRUE;
}

u_int8_t mt6653_is_mcusys_bus_timeout(struct ADAPTER *ad)
{
	uint32_t value = 0;

	HAL_RMCR_RD(PLAT_DBG, ad,
		   CONN_DBG_CTL_WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_BUS_TIMEOUT_IRQ_ADDR,
		   &value);
	if ((value & BIT(0)) != 0x0) {
		DBGLOG(HAL, WARN,
			"WF mcusys bus hang irq status: 0x%08x\n",
			value);
		return TRUE;
	}

	return FALSE;
}

static u_int8_t mt6653_check_recovery_needed(struct ADAPTER *ad)
{
	uint32_t u4Value = 0;
	u_int8_t fgResult = FALSE;

	/*
	 * if (0x81021604[31:16]==0xdead &&
	 *     (0x70005350[30:28]!=0x0 || 0x70005360[6:4]!=0x0)) == 0x1
	 * do recovery flow
	 */

	HAL_RMCR_RD(ONOFF_READ, ad, WF_TOP_CFG_ON_ROMCODE_INDEX_ADDR,
		&u4Value);
	DBGLOG(INIT, INFO, "0x%08x=0x%08x\n",
		WF_TOP_CFG_ON_ROMCODE_INDEX_ADDR, u4Value);
	if ((u4Value & 0xFFFF0000) != 0xDEAD0000) {
		fgResult = FALSE;
		goto exit;
	}

	HAL_RMCR_RD(ONOFF_READ, ad, CBTOP_GPIO_MODE5_ADDR,
		&u4Value);
	DBGLOG(INIT, INFO, "0x%08x=0x%08x\n",
		CBTOP_GPIO_MODE5_ADDR, u4Value);
	if (((u4Value & CBTOP_GPIO_MODE5_GPIO47_MASK) >>
	    CBTOP_GPIO_MODE5_GPIO47_SHFT) != 0x0) {
		fgResult = TRUE;
		goto exit;
	}

	HAL_RMCR_RD(ONOFF_READ, ad, CBTOP_GPIO_MODE6_ADDR,
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

static uint32_t mt6653_mcu_reinit(struct ADAPTER *ad)
{
#define CONNINFRA_ID_MAX_POLLING_COUNT		10

	uint32_t u4Value = 0, u4PollingCnt = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	/* Check recovery needed */
	if (mt6653_check_recovery_needed(ad) == FALSE)
		goto exit;

	DBGLOG(INIT, INFO, "mt6653_mcu_reinit.\n");

	/* Force on conninfra */
	HAL_MCR_WR(ad,
		CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR,
		0x1);

	/* Wait conninfra wakeup */
	while (TRUE) {
		HAL_RMCR_RD(ONOFF_READ, ad,
			       CONN_CFG_IP_VERSION_IP_VERSION_ADDR,
			       &u4Value);

		if (u4Value == MT6653_CONNINFRA_VERSION_ID)
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

	HAL_RMCR_RD(ONOFF_READ, ad, CBTOP_GPIO_MODE5_ADDR, &u4Value);
	DBGLOG(INIT, INFO, "0x%08x=0x%08x\n",
		CBTOP_GPIO_MODE5_ADDR, u4Value);

	HAL_RMCR_RD(ONOFF_READ, ad, CBTOP_GPIO_MODE6_ADDR, &u4Value);
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
static uint32_t mt6653_mcu_reset(struct ADAPTER *ad)
{
	uint32_t u4Value = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO, "mt6653_mcu_reset..\n");

	/* set driver own */
	HAL_MCR_WR(ad,
		CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR, PCIE_LPCR_HOST_CLR_OWN);
	kalMdelay(10);

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

static uint32_t mt6653_mcu_check_idle(struct ADAPTER *ad)
{
#define MCU_IDLE		0x1D1E
#define MCU_ON_RDY		0xDEAD1234

	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint32_t u4Value = 0, u4PollingCnt = 0;

#if (CFG_MTK_WIFI_ON_READ_BY_CFG_SPACE == 1) && defined(_HIF_PCIE)
	/* check sram */
	u4PollingCnt = 0;
	while (TRUE) {
		if (u4PollingCnt >= 1000) {
			DBGLOG(INIT, ERROR, "read sram timeout: 0x%08x\n",
				u4Value);
			break;
		}

		HAL_RMCR_RD(ONOFF_READ, ad, 0x7c05b160, &u4Value);

		if ((u4Value == MCU_IDLE)
#if (CFG_MTK_ANDROID_WMT == 0)
			|| (u4Value == MCU_ON_RDY)
#endif
		) {
			DBGLOG(INIT, TRACE, "read 0x%08x by sram\n", u4Value);
			rStatus = WLAN_STATUS_SUCCESS;
			goto exit;
		}
		u4PollingCnt++;
		kalUdelay(1000);
	}
#endif

	/* check CR */
	u4PollingCnt = 0;
	while (TRUE) {
		if (u4PollingCnt >= 1000) {
			DBGLOG(INIT, ERROR, "read timeout: 0x%08x\n", u4Value);
			break;
		}

		HAL_RMCR_RD(ONOFF_READ, ad, WF_TOP_CFG_ON_ROMCODE_INDEX_ADDR,
			&u4Value);

		if ((u4Value == MCU_IDLE)
#if (CFG_MTK_ANDROID_WMT == 0)
			|| (u4Value == MCU_ON_RDY)
#endif
		) {
			DBGLOG(INIT, TRACE, "read 0x%08x by CR\n", u4Value);
			rStatus = WLAN_STATUS_SUCCESS;
			goto exit;
		}
		u4PollingCnt++;
		kalUdelay(1000);
	}

exit:
	return rStatus;
}

static uint32_t mt6653_mcu_init(struct ADAPTER *ad)
{
	uint32_t u4Value = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct CHIP_DBG_OPS *prDbgOps = NULL;

	if (!ad) {
		DBGLOG(INIT, ERROR, "NULL ADAPTER.\n");
		rStatus = WLAN_STATUS_FAILURE;
		goto exit;
	}

	prChipInfo = ad->chip_info;
	if (prChipInfo == NULL) {
		DBGLOG(INIT, ERROR, "NULL prChipInfo.\n");
		rStatus = WLAN_STATUS_FAILURE;
		goto exit;
	}

	HAL_MCR_WR(ad,
		CB_INFRA_SLP_CTRL_CB_INFRA_SLP_PROT_SW_CTRL_ADDR, 0x0);
#if (CFG_MTK_ANDROID_WMT == 0) && (CFG_MTK_FPGA_PLATFORM == 0)
	rStatus = mt6653_mcu_reset(ad);
	if (rStatus != WLAN_STATUS_SUCCESS)
		goto dump;
#endif

	HAL_MCR_WR(ad,
		CONN_HOST_CSR_TOP_CSR_AP2CONN_ACCESS_DETECT_EN_ADDR, 0x0);
	rStatus = mt6653_mcu_check_idle(ad);
	if (rStatus != WLAN_STATUS_SUCCESS)
		goto dump;

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (connv3_ext_32k_on()) {
		DBGLOG(INIT, ERROR, "connv3_ext_32k_on failed.\n");
		rStatus = WLAN_STATUS_FAILURE;
		goto dump;
	}
#endif

	if (ad->chip_info->coexpccifon)
		ad->chip_info->coexpccifon(ad);

#if CFG_SUPPORT_PCIE_ASPM
#if (CFG_PCIE_MT6989 == 1 || CFG_PCIE_MT6989_6653 == 1)
	pcie_vir_addr = ioremap(0x112f0000, 0x2000);
#else
	pcie_vir_addr = ioremap(0x16910000, 0x2000);
#endif
	spin_lock_init(&rPCIELock);
#endif

dump:
	if (rStatus != WLAN_STATUS_SUCCESS) {
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
		HAL_MCR_WR(ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_1_ADDR,
			   0x011f0000);
		kalUdelay(1);
		HAL_RMCR_RD(ONOFF_DBG, ad,
			   CB_CKGEN_TOP_CBTOP_ULPOSC_2_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_CKGEN_TOP_CBTOP_ULPOSC_2_ADDR,
			u4Value);
		HAL_RMCR_RD(ONOFF_DBG, ad,
			   CB_INFRA_SLP_CTRL_CB_INFRA_CRYPTO_TOP_MCU_OWN_ADDR,
			   &u4Value);
		DBGLOG(INIT, INFO,
			"0x%08x=0x%08x\n",
			CB_INFRA_SLP_CTRL_CB_INFRA_CRYPTO_TOP_MCU_OWN_ADDR,
			u4Value);
	}
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
	if (prChipInfo->sw_sync_emi_info)
		kalMemSet(prChipInfo->sw_sync_emi_info, 0x0,
		sizeof(struct sw_sync_emi_info) * SW_SYNC_TAG_NUM);
#endif /* CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI */

#if (CFG_SUPPORT_CONNFEM == 1)
	prChipInfo->isAaDbdcEnable = mt6653_is_AA_DBDC_enable();
	prChipInfo->isSupportBand2 = mt6653_is_support_band2();
#endif

exit:
	return rStatus;
}

static void mt6653_mcu_deinit(struct ADAPTER *ad)
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
		}
		kalMsleep(100);
		retry++;
	}

	wifi_coredump_set_enable(FALSE);

	mt6653_force_conn_infra_on(ad, FALSE);

	if (ad->chip_info->coexpccifoff)
		ad->chip_info->coexpccifoff(ad);

#if CFG_SUPPORT_PCIE_ASPM
	if (pcie_vir_addr)
		iounmap(pcie_vir_addr);
#endif
}

static int32_t mt6653_trigger_fw_assert(struct ADAPTER *prAdapter)
{
	int32_t ret = 0;

	ccif_trigger_fw_assert(prAdapter);

#if CFG_WMT_RESET_API_SUPPORT
	ret = reset_wait_for_trigger_completion();
#endif

	return ret;
}

#define MCIF_EMI_MEMORY_SIZE 128
#define MCIF_EMI_COEX_SWMSG_OFFSET 0xF8518000
#define MCIF_EMI_BASE_OFFSET 0xE4
static int mt6653ConnacPccifOn(struct ADAPTER *prAdapter)
{
#if CFG_MTK_CCCI_SUPPORT && CFG_MTK_MDDP_SUPPORT
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	uint32_t ccif_base = 0x160000;
	uint32_t mcif_emi_base, u4Val = 0, u4WifiEmi = 0;
	void *vir_addr = NULL;
	int size = 0;

#if CFG_MTK_ANDROID_WMT
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (is_pwr_on_notify_processing())
		return -1;
#endif
#endif
	ccif_base += (uint32_t)(prChipInfo->u8CsrOffset);
	mcif_emi_base = get_smem_phy_start_addr(
		MD_SYS1, SMEM_USER_RAW_MD_CONSYS, &size);
	if (!mcif_emi_base) {
		DBGLOG(INIT, ERROR, "share memory is NULL.\n");
		return -1;
	}

	vir_addr = ioremap(mcif_emi_base, MCIF_EMI_MEMORY_SIZE);
	if (!vir_addr) {
		DBGLOG(INIT, ERROR, "ioremap fail.\n");
		return -1;
	}

	u4WifiEmi = (uint32_t)emi_mem_get_phy_base(prAdapter->chip_info) +
		emi_mem_offset_convert(0x235C01);

#if CFG_MTK_WIFI_WFDMA_WB
#if CFG_MTK_MDDP_SUPPORT
	if (size >= (WFDMA_WB_MEMORY_SIZE * 2)) {
		prAdapter->u8MdRingStaBase =
			((uint64_t)mcif_emi_base + size - WFDMA_WB_MEMORY_SIZE);
		prAdapter->u8MdRingIdxBase =
			prAdapter->u8MdRingStaBase - WFDMA_WB_MEMORY_SIZE;
	}
#endif /* CFG_MTK_MDDP_SUPPORT */
#endif /* CFG_MTK_WIFI_WFDMA_WB */

	kalMemSetIo(vir_addr, 0xFF, MCIF_EMI_MEMORY_SIZE);
	writel(0x4D4D434D, vir_addr);
	writel(0x4D4D434D, vir_addr + 0x4);
	writel(0x00000000, vir_addr + 0x8);
	writel(0x00000000, vir_addr + 0xC);
	writel(u4WifiEmi, vir_addr + 0x10);
	writel(0x04000080, vir_addr + 0x14);
	writel(ccif_base + 0xF00C, vir_addr + 0x18);
	writel(0x00000001, vir_addr + 0x1C);
	writel(0x00000000, vir_addr + 0x70);
	writel(0x00000000, vir_addr + 0x74);
	writel(0x4D434D4D, vir_addr + 0x78);
	writel(0x4D434D4D, vir_addr + 0x7C);

	u4Val = readl(vir_addr + MCIF_EMI_BASE_OFFSET);

	if (mddpIsSupportMcifWifi()) {
		HAL_MCR_WR(prAdapter,
			   MT6653_MCIF_MD_STATE_WHEN_WIFI_ON_ADDR,
			   u4Val);
	}

	DBGLOG(INIT, TRACE, "MCIF_EMI_BASE_OFFSET=[0x%08x]\n", u4Val);
	DBGLOG_MEM128(HAL, TRACE, vir_addr, MCIF_EMI_MEMORY_SIZE);

	iounmap(vir_addr);
#else
	DBGLOG(INIT, ERROR, "[%s] ECCCI Driver is not supported.\n", __func__);
#endif
	return 0;
}

static int mt6653ConnacPccifOff(struct ADAPTER *prAdapter)
{
#if CFG_MTK_CCCI_SUPPORT && CFG_MTK_MDDP_SUPPORT
	uint32_t mcif_emi_base;
	void *vir_addr = NULL;
	int ret = 0;

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

	writel(0, vir_addr + 0x10);
	writel(0, vir_addr + 0x14);
	writel(0, vir_addr + 0x18);
	writel(0, vir_addr + 0x1C);

	iounmap(vir_addr);
#else
	DBGLOG(INIT, ERROR, "[%s] ECCCI Driver is not supported.\n", __func__);
#endif
	return 0;
}

static int mt6653_CheckBusHang(void *priv, uint8_t rst_enable)
{
	struct ADAPTER *ad = priv;
	u_int8_t readable = TRUE;

	if (fgIsBusAccessFailed) {
		readable = FALSE;
		goto exit;
	}

	if (!mt6653_is_ap2conn_off_readable(ad) ||
	    !mt6653_is_conn2wf_readable(ad)) {
		readable = FALSE;
		goto exit;
	}

	if (mt6653_is_conninfra_bus_timeout(ad)) {
		readable = FALSE;
		goto exit;
	}

	if (mt6653_is_mcusys_bus_timeout(ad))
		readable = FALSE;

exit:
	return readable ? 0 : 1;
}

#if CFG_MTK_WIFI_PCIE_SR
static uint32_t mt6653_EnterL2(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct pci_dev *dev = NULL;
	uint32_t u4Status = WLAN_STATUS_FAILURE;
	int ret;

	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL\n");
		goto exit;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	dev = prGlueInfo->rHifInfo.pdev;
	ret = mtk_pcie_enter_L2(dev);
	if (ret)
		goto exit;

	u4Status = WLAN_STATUS_SUCCESS;
exit:
	return u4Status;
}

static uint32_t mt6653_ExitL2(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct pci_dev *dev = NULL;
	uint32_t u4Status = WLAN_STATUS_FAILURE;
	int ret;

	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL\n");
		goto exit;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	dev = prGlueInfo->rHifInfo.pdev;
	ret = mtk_pcie_exit_L2(dev);
	if (ret)
		goto exit;

	u4Status = WLAN_STATUS_SUCCESS;
exit:
	return u4Status;
}
#endif

static uint32_t mt6653_wlanDownloadPatch(struct ADAPTER *prAdapter)
{
	uint32_t status = wlanDownloadPatch(prAdapter);

	if (status == WLAN_STATUS_SUCCESS) {
		wifi_coredump_set_enable(TRUE);

		pcie_backup_config_space_settings(prAdapter);

#if CFG_MTK_WIFI_PCIE_SR
		/* enter -> keep 100ms -> exit L2 for enabling PCIE SR */
		if (kalIsSupportPcieL2()) {
			HAL_MCR_WR(prAdapter,
				CB_INFRA_SLP_CTRL_CB_INFRA_SLP_PROT_SW_CTRL_ADDR,
				0x800);
			kalUsleep(5000);
			mt6653_EnterL2(prAdapter);
			msleep(100);
			status = mt6653_ExitL2(prAdapter);
			if (status != WLAN_STATUS_SUCCESS)
				DBGLOG(INIT, ERROR, "Exit L2 failed\n");
			HAL_MCR_WR(prAdapter,
				CB_INFRA_SLP_CTRL_CB_INFRA_SLP_PROT_SW_CTRL_ADDR,
				0x0);
		}
		fgIsL2Finished = TRUE;
#endif
	}

	return status;
}
#endif /* _HIF_PCIE */

static uint32_t mt6653GetFlavorVer(uint8_t *flavor)
{
	uint32_t ret = WLAN_STATUS_FAILURE;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN] = {0};

	if (kalGetFwFlavor(&aucFlavor[0]) == 1) {
		kalScnprintf(flavor,
					CFG_FW_FLAVOR_MAX_LEN,
					"%s", aucFlavor);
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

static void mt6653WiFiNappingCtrl(
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

static u_int8_t mt6653_isUpgradeWholeChipReset(struct ADAPTER *prAdapter)
{
#if defined(_HIF_PCIE)
	uint32_t u4Val1, u4Val2;
#if CFG_SUPPORT_PCIE_ASPM
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	uint32_t u4Status = 0;

	/* Request keep L1/L0 */
	u4Status = mt6653ConfigPcieAspm(prGlueInfo, FALSE, WIFI_RST_ROLE);
	if (u4Status) {
		DBGLOG(INIT, ERROR, "Leave L1.2 failed\n");
		return TRUE;
	}
#endif

	glReadPcieCfgSpace(0x488, &u4Val1);
	glReadPcieCfgSpace(0x48c, &u4Val2);
	DBGLOG(INIT, INFO,
		"0x488=0x%08x, 0x48c=0x%08x\n",
		u4Val1, u4Val2);

	/* 1. Cfg_Rd[0x488] == 0 or 0xFFFFFFFF: PCIE is not at link up status */
	/* 2. Bit 0/4/11~13 of Cfg_Rd[0x488] is not set: cb_infra is abnormal */
	if (u4Val1 == 0x0 || u4Val1 == 0xFFFFFFFF ||
	   ((u4Val1 & 0x3811) != 0x3811)) {
		DBGLOG(INIT, ERROR,
			"Get abnormal PCIE or cb_infra bus status\n");
		return TRUE;
	}
#endif

	if (mt6653_is_conninfra_bus_timeout(prAdapter))
		return TRUE;

	if (mt6653_is_mcusys_bus_timeout(prAdapter))
		return TRUE;

	return FALSE;
}

#if (CFG_SUPPORT_APS == 1)
uint8_t mt6653_apsLinkPlanDecision(struct ADAPTER *prAdapter,
	struct AP_COLLECTION *prAp, enum ENUM_MLO_LINK_PLAN eLinkPlan,
	uint8_t ucBssIndex)
{
	uint32_t u4TmpLinkPlanBmap;
	uint32_t u4LinkPlanAGBmap =
		BIT(MLO_LINK_PLAN_2_5)
#if (CFG_SUPPORT_WIFI_6G == 1)
		| BIT(MLO_LINK_PLAN_2_6)
#endif
	;
	uint32_t u4LinkPlanAABmap =
		BIT(MLO_LINK_PLAN_2_5)
		| BIT(MLO_LINK_PLAN_5_5)
#if (CFG_SUPPORT_WIFI_6G == 1)
		| BIT(MLO_LINK_PLAN_2_6)
		| BIT(MLO_LINK_PLAN_5_6)
#endif
	;
#if (CONFIG_BAND_NUM == 3)
	uint32_t u4LinkPlan3Bmap =
		BIT(MLO_LINK_PLAN_2_5_5)
#if (CFG_SUPPORT_WIFI_6G == 1)
		| BIT(MLO_LINK_PLAN_2_5_6)
#endif
	;
#endif

	/* Eable A+A when support TBTC and EMLSR */
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucNonApMldEMLSupport) &&
	    BE_IS_EML_CAP_SUPPORT_EMLSR(prAdapter->rWifiVar.u2NonApMldEMLCap)) {
#if (CONFIG_BAND_NUM == 3)
		if (prAdapter->rWifiVar.ucStaMldLinkMax == 3)
			u4TmpLinkPlanBmap = u4LinkPlan3Bmap;
		else
#endif
			u4TmpLinkPlanBmap = u4LinkPlanAABmap;
	} else {
		u4TmpLinkPlanBmap = u4LinkPlanAGBmap;
	}

	return !!(u4TmpLinkPlanBmap & BIT(eLinkPlan));
}

static void mt6653_apsUpdateTotalScore(struct ADAPTER *prAdapter,
	struct BSS_DESC *arLinks[], uint8_t ucLinkNum,
	enum ENUM_MLO_LINK_PLAN eCurrPlan, struct AP_COLLECTION *prAp,
	uint8_t ucBssidx)
{
	uint32_t u4TotalScore = 0;
	uint32_t u4TotalTput = 0;
	struct BSS_DESC *best_bss = arLinks[0]; /* links is sorted by score */
	uint8_t ucEmlsrLinkWeight = prAdapter->rWifiVar.ucEmlsrLinkWeight;
	uint8_t i;
	uint8_t ucRfBandBmap = 0;
	enum ENUM_MLO_MODE eMloMode;
	uint8_t ucMaxSimuLinks = 0;

	for (i = 0; i < ucLinkNum; i++) {
		u4TotalScore += arLinks[i]->u2Score;
		u4TotalTput += arLinks[i]->u4Tput;
		ucRfBandBmap |= BIT(arLinks[i]->eBand);
	}

	switch (ucLinkNum) {
	case 2:
		/* STR: 2+5, 2+6
		 * EMLSR: 5+5, 5+6
		 */
		if (ucRfBandBmap & BIT(BAND_2G4)) {
			ucMaxSimuLinks = 1;
			eMloMode = MLO_MODE_STR;
		} else {
			if (BE_IS_EML_CAP_SUPPORT_EMLSR(
				best_bss->rMlInfo.u2EmlCap)) {
				u4TotalScore = best_bss->u2Score +
					(u4TotalScore - best_bss->u2Score) *
					 ucEmlsrLinkWeight / 100;
				u4TotalTput = best_bss->u4Tput +
					(u4TotalTput - best_bss->u4Tput) *
					 ucEmlsrLinkWeight / 100;
				ucMaxSimuLinks = 0;
				eMloMode = MLO_MODE_EMLSR;
			} else {
				/* fallback to single link if ap no emlsr */
				u4TotalScore = best_bss->u2Score;
				u4TotalTput = best_bss->u4Tput;
				ucMaxSimuLinks = 0;
				eMloMode = MLO_MODE_SLSR;
				ucLinkNum = 1;
			}
		}
		break;
	case 3:
		/* TODO: HYEMLSR: 2+5+5, 2+5+6 */
		DBGLOG(APS, WARN, "not full support 3 links yet\n");
		kal_fallthrough;
	default:
		eMloMode = MLO_MODE_STR;
		ucMaxSimuLinks = ucLinkNum - 1;
		break;
	}

	if (u4TotalScore > prAp->u4TotalScore) {
		kalMemCopy(prAp->aprTarget, arLinks, sizeof(prAp->aprTarget));
		prAp->ucLinkNum = ucLinkNum;
		prAp->u4TotalScore = u4TotalScore;
		prAp->u4TotalTput = u4TotalTput;
		prAp->eMloMode = eMloMode;
		prAp->ucMaxSimuLinks = ucMaxSimuLinks;

		DBGLOG(APS, TRACE,
			"CAND[%d] RfBandBmap[0x%x] num[%d] score[%d] tput[%d] mode[%d] simu[%d]\n",
			prAp->u4Index, ucRfBandBmap, prAp->ucLinkNum,
			prAp->u4TotalScore, prAp->u4TotalTput,
			prAp->eMloMode, prAp->ucMaxSimuLinks);
	}
}

static void mt6653_apsFillBssDescSet(struct ADAPTER *prAdapter,
		struct BSS_DESC_SET *prSet,
		uint8_t ucBssidx)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1) && (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t i, bss5G = MLD_LINK_MAX, bss6G = MLD_LINK_MAX;

	for (i = 0; i < prSet->ucLinkNum; i++) {
		if (prSet->aprBssDesc[i]->eBand == BAND_5G)
			bss5G = i;
		if (prSet->aprBssDesc[i]->eBand == BAND_6G &&
		    prSet->aprBssDesc[i]->eChannelWidth >= CW_320_1MHZ)
			bss6G = i;
	}

	if (bss5G != MLD_LINK_MAX && bss6G != MLD_LINK_MAX) {
		struct BSS_DESC *prBssDesc;

		prBssDesc = prSet->aprBssDesc[bss6G];
		prSet->aprBssDesc[bss6G] = prSet->aprBssDesc[0];
		prSet->aprBssDesc[0] = prBssDesc;

		DBGLOG(APS, INFO, MACSTR
			" link_id=%d max_links=%d Setup for 6G BW320\n",
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->rMlInfo.ucLinkIndex,
			prBssDesc->rMlInfo.ucMaxSimuLinks);
	}
#endif
}

#endif /* CFG_SUPPORT_APS */

static void mt6653LowPowerOwnInit(struct ADAPTER *prAdapter)
{
	HAL_MCR_WR(prAdapter,
		   CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_ADDR,
		   BIT(0));
}

static void mt6653LowPowerOwnRead(struct ADAPTER *prAdapter,
				  u_int8_t *pfgResult)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4RegValue = 0;

	prChipInfo = prAdapter->chip_info;
	if (prChipInfo->is_support_asic_lp == FALSE) {
		*pfgResult = TRUE;
		return;
	}

#if (CFG_MTK_WIFI_ON_READ_BY_CFG_SPACE == 1)
	/* read own status from pcie config space: 0x48C[14] */
	glReadPcieCfgSpace(PCIE_CFGSPACE_BASE_OFFSET, &u4RegValue);
	*pfgResult = (((u4RegValue >> PCIE_CFGSPACE_OWN_STATUS_SHIFT)
			& PCIE_CFGSPACE_OWN_STATUS_MASK)
			== 0) ? TRUE : FALSE;
#else
	HAL_RMCR_RD(LPOWN_READ, prAdapter,
		    CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		    &u4RegValue);
	*pfgResult = (u4RegValue &
		PCIE_LPCR_AP_HOST_OWNER_STATE_SYNC) == 0 ? TRUE : FALSE;
#endif
}

static void mt6653LowPowerOwnSet(struct ADAPTER *prAdapter,
				 u_int8_t *pfgResult)
{
	struct mt66xx_chip_info *prChipInfo;
#ifndef CFG_MTK_WIFI_DRV_OWN_INT_MODE
	uint32_t u4RegValue = 0;
#endif

	prChipInfo = prAdapter->chip_info;
	if (prChipInfo->is_support_asic_lp == FALSE) {
		*pfgResult = TRUE;
		return;
	}

	HAL_MCR_WR(prAdapter,
		   CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		   PCIE_LPCR_HOST_SET_OWN);

#ifndef CFG_MTK_WIFI_DRV_OWN_INT_MODE
	HAL_RMCR_RD(LPOWN_READ, prAdapter,
		    CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		    &u4RegValue);

	*pfgResult = (u4RegValue &
		PCIE_LPCR_AP_HOST_OWNER_STATE_SYNC) == 0x4;

#if defined(_HIF_PCIE)
	if (prChipInfo->bus_info->hwControlVote)
		prChipInfo->bus_info->hwControlVote(prAdapter,
			TRUE, PCIE_VOTE_USER_DRVOWN);
#endif

#else
	*pfgResult = TRUE;
#endif
}

static void mt6653LowPowerOwnClear(struct ADAPTER *prAdapter,
				   u_int8_t *pfgResult)
{
	struct mt66xx_chip_info *prChipInfo;
#ifndef CFG_MTK_WIFI_DRV_OWN_INT_MODE
	uint32_t u4RegValue = 0;
#endif

	prChipInfo = prAdapter->chip_info;
	if (prChipInfo->is_support_asic_lp == FALSE) {
		*pfgResult = TRUE;
		return;
	}

#if defined(_HIF_PCIE)
	if (prChipInfo->bus_info->hwControlVote)
		prChipInfo->bus_info->hwControlVote(prAdapter,
			FALSE, PCIE_VOTE_USER_DRVOWN);
#endif

#if CFG_MTK_WIFI_PCIE_SUPPORT
	mtk_pcie_dump_link_info(0);
#endif

#ifdef CFG_MTK_WIFI_DRV_OWN_INT_MODE
	clear_bit(GLUE_FLAG_DRV_OWN_INT_BIT,
		  &prAdapter->prGlueInfo->ulFlag);
#endif

	HAL_MCR_WR(prAdapter,
		   CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		   PCIE_LPCR_HOST_CLR_OWN);

#ifndef CFG_MTK_WIFI_DRV_OWN_INT_MODE
	HAL_RMCR_RD(LPOWN_READ, prAdapter,
		    CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		    &u4RegValue);

	*pfgResult = (u4RegValue &
		PCIE_LPCR_AP_HOST_OWNER_STATE_SYNC) == 0;
#else
	*pfgResult = TRUE;
#endif
}

#if CFG_SUPPORT_WIFI_SLEEP_COUNT
int mt6653PowerDumpStart(void *priv_data, unsigned int force_dump)
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

	if (glue->u4ReadyFlag)
#if CFG_MTK_WIFI_PCIE_SUPPORT
		u4Val = mtk_pcie_dump_link_info(0);
#else
		return 1;
#endif
	else
		return 1;

	if (force_dump == TRUE) {
		DBGLOG(REQ, INFO, "PowerDumpStart force_dump\n");
		ad->fgIsPowerDumpDrvOwn = TRUE;
		/* ACQUIRE_POWER_CONTROL_FROM_PM(ad); */
		halSetDriverOwn(ad, DRV_OWN_SRC_POWER_DUMP);
		if (ad->fgWiFiInSleepyState == TRUE)
			ad->fgWiFiInSleepyState = FALSE;
		ad->fgIsPowerDumpDrvOwn = FALSE;

		if (ad->fgIsFwOwn == TRUE) {
			DBGLOG(REQ, ERROR,
				"PowerDumpStart end: driver own fail!\n");
			return 1;
		}

		return 0;

	} else {
		u4Val = (u4Val & 0x0000001F);
		DBGLOG(REQ, INFO,
			"PowerDumpStart PCIE status: 0x%08x\n", u4Val);

		if (u4Val == 0x10) {
			ad->fgIsPowerDumpDrvOwn = TRUE;
			/* ACQUIRE_POWER_CONTROL_FROM_PM(ad); */
			halSetDriverOwn(ad, DRV_OWN_SRC_POWER_DUMP);
			if (ad->fgWiFiInSleepyState == TRUE)
				ad->fgWiFiInSleepyState = FALSE;
			ad->fgIsPowerDumpDrvOwn = FALSE;

			if (ad->fgIsFwOwn == TRUE) {
				DBGLOG(REQ, ERROR,
					"PowerDumpStart end: driver own fail!\n");
				return 1;
			}
			return 0;
		}

		return 1;
	}

}

int mt6653PowerDumpEnd(void *priv_data)
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
#if (CFG_SUPPORT_CONNFEM == 1)
/*----------------------------------------------------------------------------*/
/*!
 * Get sku Version from EFUSE
 * Band1 Need Switch to SISO Mode for Skyhawk Sku2 A+A DBDC Scenario
 * FE_SPDT_5 BIT(3) for WF Band2 Support
 * FE_SPDT_6 BIT(4) for WF Band1 2x2 when Band2 is support (sku1)
 */
/*----------------------------------------------------------------------------*/
u_int8_t mt6653_is_AA_DBDC_enable(void)
{
	uint8_t fe_bt_wf_usage;
	uint32_t rStarus;

	rStarus = connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-bt-wf-usage",
			    &fe_bt_wf_usage);
	if (rStarus != 0)
		return FALSE;

	return !!((fe_bt_wf_usage & BIT(3)) && (fe_bt_wf_usage & BIT(4)));
}

static u_int8_t mt6653_is_support_band2(void)
{
	uint8_t fe_bt_wf_usage = 0;
	uint32_t rStarus;

	rStarus = connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-bt-wf-usage",
			    &fe_bt_wf_usage);
	if (rStarus != 0)
		return FALSE;

	return (fe_bt_wf_usage & BIT(3)) ? TRUE : FALSE;
}
#endif /* CFG_SUPPORT_CONNFEM == 1 */

#if CFG_MTK_WIFI_MBU
static void mt6653MbuDumpDebugCr(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct BUS_INFO *prBusInfo;
	struct SW_EMI_RING_INFO *prMbuInfo;
	struct MBU_EMI_CTX *prEmi;
	char *aucBuf;
	uint32_t u4BufferSize = 1024, u4Pos = 0, u4Idx, u4Val = 0;
	const uint32_t au4DbgCr1[] = {
		/* cb_infra */
		0x7002500C, 0x70025014, 0x70025024, 0x7002502C,
		0x70028730, 0x70026100,
		/* mbu */
		0x74130200, 0x74130204, 0x7413A004, 0x74138018,
		0x7413B000, 0x70028800, 0x74130040, 0x74130044,
		0x74130048, 0x7413004C, 0x74130050, 0x74130054,
		0x74130058, 0x7413005C, 0x74138000, 0x74138004,
		0x74138008, 0x7413800C, 0x74138010, 0x74138014,
		0x74138018, 0x7413801C, 0x74138020, 0x74138060,
		0x74138064, 0x74138100, 0x74138104, 0x74138108
	};
	const uint32_t au4DbgCr2[] = {
		/* mbu */
		0x7413810C, 0x74138110, 0x74138114, 0x74138118,
		0x7413811C, 0x74138120, 0x74138160, 0x74138164,
		0x7413D008, 0x7413B008, 0x7413B00C,
		/* pcie mac */
		0x74030150, 0x74030154, 0x74030184, 0x74031010,
		0x74031204, 0x74031210, 0x740301A8, 0x740301D4,
		0x74030D40, 0x740310A8,
		/* pcie phy */
		0x740700B0, 0x740700C0
	};
	const struct wlan_dbg_command arMbuDbg[] = {
		/* write, w_addr, mask, value, read, r_addr*/
		/* pcie mac */
		{TRUE, 0x74030168, 0, 0xCCCC0100, FALSE, 0},
		{TRUE, 0x74030164, 0, 0x4C4D4E4F, TRUE, 0x7403002C},
		{TRUE, 0x74030164, 0, 0x50515253, TRUE, 0x7403002C},
		{TRUE, 0x74030164, 0, 0x54555657, TRUE, 0x7403002C},
		{TRUE, 0x74030168, 0, 0x88880100, FALSE, 0},
		{TRUE, 0x74030164, 0, 0x0001030C, TRUE, 0x7403002C},
		{TRUE, 0x74030168, 0, 0x99990100, FALSE, 0},
		{TRUE, 0x74030164, 0, 0x24252627, TRUE, 0x7403002C},
		{TRUE, 0x74030164, 0, 0x50515253, TRUE, 0x7403002C},
		{TRUE, 0x74030164, 0, 0x54555657, TRUE, 0x7403002C},
		{TRUE, 0x74030164, 0, 0xB0B1B2B3, TRUE, 0x7403002C},
		{TRUE, 0x74030164, 0, 0xB4B5B6B7, TRUE, 0x7403002C},
		{TRUE, 0x74030164, 0, 0x98999A9B, TRUE, 0x7403002C},
		{TRUE, 0x74030164, 0, 0x9C9D9E9F, TRUE, 0x7403002C},
		/* cb_infra */
		{TRUE, 0x70025300, 0, 0x00010E0F, TRUE, 0x70025304},
		{TRUE, 0x70025300, 0, 0x00011011, TRUE, 0x70025304},
	};

	prAdapter = prGlueInfo->prAdapter;
	prBusInfo = prAdapter->chip_info->bus_info;
	prMbuInfo = &prBusInfo->rSwEmiRingInfo;
	prEmi = prMbuInfo->prMbuEmiData;

	if (!prMbuInfo->fgIsSupport || !prMbuInfo->fgIsEnable || !prEmi)
		return;

	aucBuf = kalMemAlloc(u4BufferSize, PHY_MEM_TYPE);
	if (aucBuf == NULL) {
		DBGLOG(HAL, WARN, "buffer alloc fail\n");
		return;
	}

	kalMemZero(aucBuf, u4BufferSize);
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4DbgCr1); u4Idx++) {
		HAL_RMCR_RD(PLAT_DBG, prAdapter, au4DbgCr1[u4Idx], &u4Val);
		u4Pos += kalSnprintf(
			aucBuf + u4Pos,
			u4BufferSize - u4Pos,
			"[0x%08x]=[0x%08x] ",
			au4DbgCr1[u4Idx],
			u4Val);
	}
	DBGLOG(HAL, INFO, "%s\n", aucBuf);

	u4Pos = 0;
	kalMemZero(aucBuf, u4BufferSize);
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4DbgCr2); u4Idx++) {
		HAL_RMCR_RD(PLAT_DBG, prAdapter, au4DbgCr2[u4Idx], &u4Val);
		u4Pos += kalSnprintf(
			aucBuf + u4Pos,
			u4BufferSize - u4Pos,
			"[0x%08x]=[0x%08x] ",
			au4DbgCr2[u4Idx],
			u4Val);
	}

	kalMdelay(3);
	HAL_RMCR_RD(PLAT_DBG, prAdapter, 0x74130200, &u4Val);
		u4Pos += kalSnprintf(
			aucBuf + u4Pos, u4BufferSize - u4Pos,
			"delay[0x%08x]=[0x%08x]",
			0x74130200, u4Val);
	DBGLOG(HAL, INFO, "%s\n", aucBuf);

	u4Pos = 0;
	kalMemZero(aucBuf, u4BufferSize);
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(arMbuDbg); u4Idx++) {
		if (arMbuDbg[u4Idx].write) {
			HAL_MCR_WR(prAdapter,
				   arMbuDbg[u4Idx].w_addr,
				   arMbuDbg[u4Idx].value);
			u4Pos += kalSnprintf(
				aucBuf + u4Pos,
				u4BufferSize - u4Pos,
				"W[0x%08x]=[0x%08x] ",
				arMbuDbg[u4Idx].w_addr,
				arMbuDbg[u4Idx].value);
		}
		if (arMbuDbg[u4Idx].read) {
			HAL_RMCR_RD(PLAT_DBG, prAdapter,
				    arMbuDbg[u4Idx].r_addr, &u4Val);
			u4Pos += kalSnprintf(
				aucBuf + u4Pos,
				u4BufferSize - u4Pos,
				"R[0x%08x]=[0x%08x] ",
				arMbuDbg[u4Idx].r_addr,
				u4Val);
		}
	}
	DBGLOG(HAL, INFO, "%s\n", aucBuf);

	kalMemFree(aucBuf, PHY_MEM_TYPE, u4BufferSize);
}
#endif /* CFG_MTK_WIFI_MBU */

#if CFG_MTK_MDDP_SUPPORT
static void mt6653CheckMdRxHang(struct ADAPTER *prAdapter)
{
	char aucRsn[MDDP_EXP_RSN_SIZE];
	uint32_t u4Base = 0, u4Cnt = 0, u4Cidx = 0, u4Didx = 0, u4Addr;

	/* check md rx event ring */
	u4Addr = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING12_CTRL0_ADDR;
	HAL_RMCR_RD(HIF_DBG, prAdapter, u4Addr, &u4Base);
	HAL_RMCR_RD(HIF_DBG, prAdapter, u4Addr + 0x04, &u4Cnt);
	HAL_RMCR_RD(HIF_DBG, prAdapter, u4Addr + 0x08, &u4Cidx);
	HAL_RMCR_RD(HIF_DBG, prAdapter,	u4Addr + 0x0c, &u4Didx);

	u4Cnt &= MT_RING_CNT_MASK;
	u4Cidx &= MT_RING_CIDX_MASK;
	u4Didx &= MT_RING_DIDX_MASK;

	if (u4Base == 0 || u4Cnt == 0 || u4Cidx >= u4Cnt || u4Didx >= u4Cnt)
		return;

	if (u4Cidx != u4Didx)
		return;

	DBGLOG(HAL, ERROR, "md rx ring full, cidx[%u] didx[%u]\n",
	       u4Cidx, u4Didx);

	kalMemZero(aucRsn, MDDP_EXP_RSN_SIZE);
	kalScnprintf(aucRsn,
		     MDDP_EXP_RSN_SIZE,
		     MDDP_EXP_RST_STR,
		     MDDP_EXP_RX_HANG);
#if CFG_WMT_RESET_API_SUPPORT
	glSetRstReasonString(aucRsn);
	glResetWholeChipResetTrigger(aucRsn);
#endif /* CFG_WMT_RESET_API_SUPPORT */
}
#endif

#if defined(_HIF_PCIE)
static void mt6653_notify_fw_disable_SR(struct ADAPTER *prAdapter)
{
	/* notify fw to disable SR if platform not support L2 */
#if CFG_MTK_WIFI_PCIE_SR
	if (!kalIsSupportPcieL2())
#endif
		HAL_MCR_WR(prAdapter,
			   PCIE_MAC_IREG_PCIE_DEBUG_DUMMY_5_ADDR, 0x1);
}
#endif

#endif  /* MT6653 */
