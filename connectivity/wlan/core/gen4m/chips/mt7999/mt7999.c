// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   mt7999.c
*    \brief  Internal driver stack will export
*    the required procedures here for GLUE Layer.
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifdef MT7999

#include "precomp.h"
#include "mt7999.h"
#include "coda/mt7999/cb_ckgen_top.h"
#include "coda/mt7999/cb_infra_misc0.h"
#include "coda/mt7999/cb_infra_rgu.h"
#include "coda/mt7999/cb_infra_slp_ctrl.h"
#include "coda/mt7999/cbtop_gpio_sw_def.h"
#include "coda/mt7999/conn_bus_cr.h"
#include "coda/mt7999/conn_cfg.h"
#include "coda/mt7999/conn_dbg_ctl.h"
#include "coda/mt7999/conn_host_csr_top.h"
#include "coda/mt7999/conn_semaphore.h"
#include "coda/mt7999/wf_cr_sw_def.h"
#include "coda/mt7999/wf_p0_wfdma.h"
#include "coda/mt7999/wf_m0_wfdma.h"
#include "coda/mt7999/wf_p0_wfdma_trinfo_top.h"
#include "coda/mt7999/wfdma_wrap_csr.h"
#include "coda/mt7999/wf_hif_dmashdl_top.h"
#include "coda/mt7999/wf_pse_top.h"
#include "coda/mt7999/pcie_mac_ireg.h"
#include "coda/mt7999/pcie_mac_conf_common.h"
#include "coda/mt7999/conn_mcu_bus_cr.h"
#include "coda/mt7999/conn_bus_cr_von.h"
#include "coda/mt7999/conn_host_csr_top.h"
#include "coda/mt7999/vlp_uds_ctrl.h"
#include "coda/mt7999/wf_rro_top.h"
#include "hal_dmashdl_mt7999.h"
#include "coda/mt7999/wf2ap_conn_infra_on_ccif4.h"
#include "coda/mt7999/ap2wf_conn_infra_on_ccif4.h"
#include "coda/mt7999/wf_wtblon_top.h"
#include "coda/mt7999/wf_uwtbl_top.h"
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
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
#include "coda/mt7999/wf_top_cfg_vlp.h"
#include "coda/mt7999/wf_top_cfg_von.h"
#include "coda/mt7999/wf_top_mcu_cfg.h"

#if (CFG_SUPPORT_UNI_FWDL == 1)
#include "uni_fw_dl_pcie.h"
#include "uni_fw_dl_shm_mt7999.h"
#endif /* CFG_SUPPORT_UNI_FWDL */

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
static uint32_t mt7999GetFlavorVer(struct GLUE_INFO *prGlueInfo,
	uint8_t *flavor);

static void mt7999_ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx);

static void mt7999_ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);

#if CFG_MTK_WIFI_SUPPORT_PHY_FWDL
static void mt7999_ConstructPhyName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);
#endif

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
static void mt7999_ConstructIdxLogBinName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName);
#endif

#if defined(_HIF_PCIE)
static uint8_t mt7999SetRxRingHwAddr(struct RTMP_RX_RING *prRxRing,
		struct BUS_INFO *prBusInfo, uint32_t u4SwRingIdx);

static bool mt7999WfdmaAllocRxRing(struct GLUE_INFO *prGlueInfo,
		bool fgAllocMem);

static void mt7999ProcessTxInterrupt(
		struct ADAPTER *prAdapter);

static void mt7999ProcessRxInterrupt(
	struct ADAPTER *prAdapter);

static void mt7999WfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo);

static void mt7999ReadIntStatus(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus);

#if (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
static void mt7999EnableInterruptViaPcie(struct ADAPTER *prAdapter);
#endif
static void mt7999EnableInterrupt(struct ADAPTER *prAdapter);
static void mt7999DisableInterrupt(struct ADAPTER *prAdapter);

static void mt7999ConfigIntMask(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable);

static void mt7999ConfigWfdmaRxRingThreshold(
	struct ADAPTER *prAdapter, uint32_t u4Num, u_int8_t fgIsData);

static void mt7999UpdateWfdmaPrdcInt(
	struct GLUE_INFO *prGlueInfo, u_int8_t fgForceEn);

static void mt7999WpdmaConfig(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable, bool fgResetHif);

#if CFG_MTK_WIFI_WFDMA_WB
static u_int8_t mt7999IsWfdmaRxReady(struct ADAPTER *prAdapter);
static void mt7999ProcessTxInterruptByEmi(struct ADAPTER *prAdapter);
static void mt7999ProcessRxInterruptByEmi(struct ADAPTER *prAdapter);
static void mt7999ProcessSoftwareInterruptByEmi(struct ADAPTER *prAdapter);
static void mt7999ReadIntStatusByEmi(struct ADAPTER *prAdapter,
				     uint32_t *pu4IntStatus);
static void mt7999ConfigEmiIntMask(struct GLUE_INFO *prGlueInfo,
				   u_int8_t enable);
static void mt7999EnableInterruptViaPcieByEmi(struct ADAPTER *prAdapter);
static void mt7999RunWfdmaCidxFetch(struct GLUE_INFO *prGlueInfo);
static void mt7999TriggerWfdmaTxCidx(struct GLUE_INFO *prGlueInfo,
				     struct RTMP_TX_RING *prTxRing);
static void mt7999TriggerWfdmaRxCidx(struct GLUE_INFO *prGlueInfo,
				     struct RTMP_RX_RING *prRxRing);
static void mt7999EnableWfdmaWb(struct GLUE_INFO *prGlueInfo);
#endif /* CFG_MTK_WIFI_WFDMA_WB */

static void mt7999SetupMcuEmiAddr(struct ADAPTER *prAdapter);

static void mt7999WfdmaTxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *prTxRing,
	u_int32_t index);
static void mt7999WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	u_int32_t index);

static void mt7999CheckFwOwnMsiStatus(struct ADAPTER *prAdapter);
static void mt7999RecoveryMsiStatus(struct ADAPTER *prAdapter,
				    u_int8_t fgForce);
static void mt7999RecoverSerStatus(struct ADAPTER *prAdapter);

static void mt7999InitPcieInt(struct GLUE_INFO *prGlueInfo);

static void mt7999PcieHwControlVote(
	struct ADAPTER *prAdapter,
	uint8_t enable,
	uint32_t u4WifiUser);

#if CFG_SUPPORT_PCIE_ASPM
static u_int8_t mt7999DumpPcieDateFlowStatus(struct GLUE_INFO *prGlueInfo);
#endif

#if CFG_SUPPORT_PCIE_ASPM
static u_int8_t mt7999SetL1ssEnable(struct ADAPTER *prAdapter, u_int role,
		u_int8_t fgEn);
static uint32_t mt7999ConfigPcieAspm(struct GLUE_INFO *prGlueInfo,
	u_int8_t fgEn, u_int enable_role);
static void mt7999UpdatePcieAspm(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn);
static void mt7999KeepPcieWakeup(struct GLUE_INFO *prGlueInfo,
	u_int8_t fgWakeup);
#endif

static void mt7999_clear_sw_interrupt_status(struct ADAPTER *prAdapter);
static u_int8_t mt7999_get_sw_interrupt_status(struct ADAPTER *prAdapter,
	uint32_t *pu4Status);

static void mt7999_ccif_notify_utc_time_to_fw(struct ADAPTER *ad,
	uint32_t sec,
	uint32_t usec);
static void mt7999_ccif_clear_interrupt_status(struct ADAPTER *ad);
static uint32_t mt7999_ccif_get_interrupt_status(struct ADAPTER *ad);
static void mt7999_ccif_set_fw_log_read_pointer(struct ADAPTER *ad,
	enum ENUM_FW_LOG_CTRL_TYPE type,
	uint32_t read_pointer);
static uint32_t mt7999_ccif_get_fw_log_read_pointer(struct ADAPTER *ad,
	enum ENUM_FW_LOG_CTRL_TYPE type);
static int32_t mt7999_ccif_trigger_fw_assert(struct ADAPTER *ad);

static int32_t mt7999_trigger_fw_assert(struct ADAPTER *prAdapter);
static uint32_t mt7999_mcu_init(struct ADAPTER *ad);
static void mt7999_mcu_deinit(struct ADAPTER *ad);
static int mt7999ConnacPccifOn(struct ADAPTER *prAdapter);
static int mt7999ConnacPccifOff(struct ADAPTER *prAdapter);
static int mt7999_CheckBusNoAck(void *priv, uint8_t rst_enable);
static uint32_t mt7999_wlanDownloadPatch(struct ADAPTER *prAdapter);
static void mt7999WiFiNappingCtrl(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn);
#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
static void mt7999PcieMsiMaskIrq(uint32_t u4Irq, uint32_t u4Bit);
static void mt7999PcieMsiUnmaskIrq(uint32_t u4Irq, uint32_t u4Bit);
#endif /* CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE */
#endif

static u_int8_t mt7999_isUpgradeWholeChipReset(struct ADAPTER *prAdapter);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
static uint8_t mt7999_apsLinkPlanDecision(struct ADAPTER *prAdapter,
	struct AP_COLLECTION *prAp, enum ENUM_MLO_LINK_PLAN eLinkPlan,
	uint8_t ucBssIndex);
static void mt7999_apsUpdateTotalScore(struct ADAPTER *prAdapter,
	struct BSS_DESC *arLinks[], uint8_t ucLinkNum,
	struct AP_SCORE_INFO *prScoreInfo, uint8_t ucBssidx);
static void mt7999_apsFillBssDescSet(struct ADAPTER *prAdapter,
		struct BSS_DESC_SET *prSet,
		uint8_t ucBssidx);
#endif

static void mt7999LowPowerOwnInit(struct ADAPTER *prAdapter);
static void mt7999LowPowerOwnRead(struct ADAPTER *prAdapter,
				  u_int8_t *pfgResult);
static void mt7999LowPowerOwnSet(struct ADAPTER *prAdapter,
				 u_int8_t *pfgResult);
static void mt7999LowPowerOwnClear(struct ADAPTER *prAdapter,
				   u_int8_t *pfgResult);
#if CFG_MTK_WIFI_MBU
static void mt7999MbuDumpDebugCr(struct GLUE_INFO *prGlueInfo);
#endif

#if CFG_MTK_MDDP_SUPPORT
static void mt7999CheckMdRxStall(struct ADAPTER *prAdapter);
#endif

#if (CFG_SUPPORT_CONNFEM == 1)
u_int8_t mt7999_is_AA_DBDC_enable(void);
#endif

#if (CFG_SUPPORT_UNI_FWDL == 1)
static void mt7999_trigger_wf_fwdl_doorbell(struct ADAPTER *ad);
#endif /* CFG_SUPPORT_UNI_FWDL */

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct ECO_INFO mt7999_eco_table[] = {
	/* HW version,  ROM version,    Factory version */
	{0x00, 0x00, 0xA, 0x1},	/* E1 */
	{0x00, 0x00, 0x0, 0x0}	/* End of table */
};

uint8_t *apucmt7999FwName[] = {
	(uint8_t *) CFG_FW_FILENAME "_7999",
	NULL
};

#if defined(_HIF_PCIE)
struct PCIE_CHIP_CR_MAPPING mt7999_bus2chip_cr_mapping[] = {
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
#if (CFG_SUPPORT_UNI_FWDL == 1)
	{0x200b0000, 0x1b0000, 0x10000}, /* CONN_INFRA sysram */
#endif /* CFG_SUPPORT_UNI_FWDL */
	{0x7c000000, 0xf0000, 0x10000}, /* CONN_INFRA (io_top bus_cr rgu_on cfg_on) */
	{0x7c010000, 0x100000, 0x10000}, /* CONN_INFRA (gpio  clkgen cfg) */
	{0x20090000, 0x150000, 0x10000}, /* CONN_INFRA VON (RO) */
	{0x20030000, 0x160000, 0x10000}, /* CONN_INFRA CCIF */
	{0x7c040000, 0x170000, 0x10000}, /* CONN_INFRA (bus, afe) */
	{0x7c070000, 0x180000, 0x10000}, /* CONN_INFRA Semaphore */
	{0x7c080000, 0x190000, 0x10000}, /* CONN_INFRA (coex, pta) */
	{0x7c050000, 0x1a0000, 0x10000}, /* CONN_INFRA SYSRAM */
	{0x70010000, 0x1c0000, 0x10000}, /* CB Infra1 */
	{0x74040000, 0x1d0000, 0x10000}, /* CB PCIe (cbtop remap) */
	{0x70000000, 0x1e0000, 0x10000}, /* CB TOP */
	{0x70020000, 0x1f0000, 0x10000}, /* CB Infra2 (RO) */
	{0x7c500000, MT7999_PCIE2AP_REMAP_BASE_ADDR, 0x200000}, /* remap */
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
struct pcie2ap_remap mt7999_pcie2ap_remap = {
	.reg_base = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_ADDR,
	.reg_mask = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_MASK,
	.reg_shift = CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_0_76_CR_PCIE2AP_PUBLIC_REMAPPING_WF_06_SHFT,
	.base_addr = MT7999_PCIE2AP_REMAP_BASE_ADDR
};

struct pcie2ap_remap mt7999_pcie2ap_remap_cbtop = {
	.reg_base = CB_INFRA_MISC0_CBTOP_PCIE_REMAP_WF_pcie_remap_wf_rg0_ADDR,
	.reg_mask = CB_INFRA_MISC0_CBTOP_PCIE_REMAP_WF_pcie_remap_wf_rg0_MASK,
	.reg_shift = CB_INFRA_MISC0_CBTOP_PCIE_REMAP_WF_pcie_remap_wf_rg0_SHFT,
	.base_addr = MT7999_PCIE2AP_REMAP_CBTOP_BASE_ADDR
};

struct ap2wf_remap mt7999_ap2wf_remap = {
	.reg_base = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_ADDR,
	.reg_mask = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_MASK,
	.reg_shift = CONN_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_SHFT,
	.base_addr = MT7999_REMAP_BASE_ADDR
};

struct remap_range mt7999_cbtop_remap_ranges[] = {
	{0x70000000, 0x70ffffff},
	{0x74000000, 0x74ffffff},
	{0x75010000, 0x7501ffff},
	{0, 0}, /* END */
};

struct PCIE_CHIP_CR_REMAPPING mt7999_bus2chip_cr_remapping = {
	.pcie2ap = &mt7999_pcie2ap_remap,
	.pcie2ap_cbtop = &mt7999_pcie2ap_remap_cbtop,
	.ap2wf = &mt7999_ap2wf_remap,
	.cbtop_ranges = mt7999_cbtop_remap_ranges,
};

struct wfdma_group_info mt7999_wfmda_host_tx_group[] = {
	{"P0T0:AP DATA0", WF_P0_WFDMA_WPDMA_TX_RING0_CTRL0_ADDR},
	{"P0T1:AP DATA1", WF_P0_WFDMA_WPDMA_TX_RING1_CTRL0_ADDR},
	{"P0T2:AP DATA2", WF_P0_WFDMA_WPDMA_TX_RING2_CTRL0_ADDR},
	{"P0T3:AP DATA3", WF_P0_WFDMA_WPDMA_TX_RING3_CTRL0_ADDR},
	{"P0T3:AP DATA4", WF_P0_WFDMA_WPDMA_TX_RING4_CTRL0_ADDR},
	{"P0T3:AP DATA5", WF_P0_WFDMA_WPDMA_TX_RING5_CTRL0_ADDR},
	{"P0T15:AP CMD", WF_P0_WFDMA_WPDMA_TX_RING15_CTRL0_ADDR, true},
	{"P0T16:FWDL", WF_P0_WFDMA_WPDMA_TX_RING16_CTRL0_ADDR},
};

struct wfdma_group_info mt7999_wfmda_host_rx_group[] = {
	{"P0R4:AP DATA0", WF_P0_WFDMA_WPDMA_RX_RING4_CTRL0_ADDR},
	{"P0R5:AP DATA1", WF_P0_WFDMA_WPDMA_RX_RING5_CTRL0_ADDR},
	{"P0R6:AP EVT", WF_P0_WFDMA_WPDMA_RX_RING6_CTRL0_ADDR, true},
	{"P0R7:AP TDONE", WF_P0_WFDMA_WPDMA_RX_RING7_CTRL0_ADDR},
	{"P0R8:AP ICS", WF_P0_WFDMA_WPDMA_RX_RING8_CTRL0_ADDR},
};

struct pse_group_info mt7999_pse_group[] = {
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
struct pcie_msi_layout mt7999_pcie_msi_layout[] = {
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
#if (CFG_SUPPORT_UNI_FWDL == 1)
	{"uni_fwdl_int", pcie_uni_fwdl_top_handler,
	 pcie_uni_fwdl_thread_handler, AP_MISC_INT, 0},
#else
	{"reserved", NULL, NULL, NONE_INT, 0},
#endif /* CFG_SUPPORT_UNI_FWDL */
	{"reserved", NULL, NULL, NONE_INT, 0},

#if (CFG_PCIE_GEN_SWITCH == 1)
	{"gen_switch_irq1", pcie_gen_switch_end_top_handler,
	pcie_gen_switch_end_thread_handler, PCIE_GEN_SWITCH_INT, 0},
#else
	{"reserved", NULL, NULL, NONE_INT, 0},
#endif

#if (CFG_MTK_WIFI_DRV_OWN_INT_MODE == 1)
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

struct BUS_INFO mt7999_bus_info = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.top_cfg_base = MT7999_TOP_CFG_BASE,

	/* host_dma0 for TXP */
	.host_dma0_base = WF_P0_WFDMA_BASE,
	.host_int_status_addr = WF_P0_WFDMA_HOST_INT_STA_ADDR,

	.host_int_txdone_bits =
	(
#if (CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0)
	 WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_0_MASK |
	 WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_1_MASK |
	 WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_2_MASK |
	 WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_3_MASK |
	 WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_4_MASK |
	 WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_5_MASK |
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0 */
#if (CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0)
	 WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_15_MASK |
#endif /* CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0 */
#if (WFDMA_AP_MSI_NUM == 1)
	 WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_16_MASK |
#endif
	 WF_P0_WFDMA_HOST_INT_STA_MCU2HOST_SW_INT_STS_MASK),
	.host_int_rxdone_bits =
	(WF_P0_WFDMA_HOST_INT_STA_RX_DONE_INT_STS_4_MASK |
	 WF_P0_WFDMA_HOST_INT_STA_RX_DONE_INT_STS_5_MASK |
	 WF_P0_WFDMA_HOST_INT_STA_RX_DONE_INT_STS_6_MASK |
	 WF_P0_WFDMA_HOST_INT_STA_RX_DONE_INT_STS_7_MASK |
	 WF_P0_WFDMA_HOST_INT_STA_RX_DONE_INT_STS_8_MASK),

	.host_tx_ring_base = WF_P0_WFDMA_WPDMA_TX_RING0_CTRL0_ADDR,
	.host_tx_ring_ext_ctrl_base =
		WF_P0_WFDMA_WPDMA_TX_RING0_EXT_CTRL_ADDR,
	.host_tx_ring_cidx_addr = WF_P0_WFDMA_WPDMA_TX_RING0_CTRL2_ADDR,
	.host_tx_ring_didx_addr = WF_P0_WFDMA_WPDMA_TX_RING0_CTRL3_ADDR,
	.host_tx_ring_cnt_addr = WF_P0_WFDMA_WPDMA_TX_RING0_CTRL1_ADDR,

	.host_rx_ring_base = WF_P0_WFDMA_WPDMA_RX_RING0_CTRL0_ADDR,
	.host_rx_ring_ext_ctrl_base =
		WF_P0_WFDMA_WPDMA_RX_RING0_EXT_CTRL_ADDR,
	.host_rx_ring_cidx_addr = WF_P0_WFDMA_WPDMA_RX_RING0_CTRL2_ADDR,
	.host_rx_ring_didx_addr = WF_P0_WFDMA_WPDMA_RX_RING0_CTRL3_ADDR,
	.host_rx_ring_cnt_addr = WF_P0_WFDMA_WPDMA_RX_RING0_CTRL1_ADDR,

	.bus2chip = mt7999_bus2chip_cr_mapping,
	.bus2chip_remap = &mt7999_bus2chip_cr_remapping,
	.max_static_map_addr = 0x00200000,

	.tx_ring_fwdl_idx = CONNAC5X_FWDL_TX_RING_IDX,
	.tx_ring_cmd_idx = 15,
	.tx_ring0_data_idx = 0,
	.tx_ring1_data_idx = 1,
	.tx_ring2_data_idx = 2,
	.tx_ring3_data_idx = 3,
	.tx_prio_data_idx = 4,
	.tx_altx_data_idx = 5,
	.rx_data_ring_num = 2,
	.rx_evt_ring_num = 3,
	.rx_data_ring_size = 3072,
	.rx_evt_ring_size = 128,
	.rx_data_ring_prealloc_size = 1024,
	.fw_own_clear_addr = CONN_HOST_CSR_TOP_WF_BAND0_IRQ_STAT_ADDR,
	.fw_own_clear_bit = CONN_HOST_CSR_TOP_WF_BAND0_IRQ_STAT_WF_B0_HOST_LPCR_FW_OWN_CLR_STAT_MASK,
#if (CFG_MTK_WIFI_DRV_OWN_INT_MODE == 1)
	.fgCheckDriverOwnInt = TRUE,
#else
	.fgCheckDriverOwnInt = FALSE,
#endif /* CFG_MTK_WIFI_DRV_OWN_INT_MODE */
#if CFG_MTK_WIFI_PCIE_SUPPORT
	.checkFwOwnMsiStatus = mt7999CheckFwOwnMsiStatus,
	.recoveryMsiStatus = mt7999RecoveryMsiStatus,
	.recoverSerStatus = mt7999RecoverSerStatus,
#endif
#if (CFG_MTK_ANDROID_WMT == 1)
	.u4DmaMask = 36,
#else /* !CFG_MTK_ANDROID_WMT */
	.u4DmaMask = 34,
#endif /* !CFG_MTK_ANDROID_WMT */
	.wfmda_host_tx_group = mt7999_wfmda_host_tx_group,
	.wfmda_host_tx_group_len = ARRAY_SIZE(mt7999_wfmda_host_tx_group),
	.wfmda_host_rx_group = mt7999_wfmda_host_rx_group,
	.wfmda_host_rx_group_len = ARRAY_SIZE(mt7999_wfmda_host_rx_group),
	.prDmashdlCfg = &rMt7999DmashdlCfg,
#if (DBG_DISABLE_ALL_INFO == 0)
	.prPleTopCr = &rMt7999PleTopCr,
	.prPseTopCr = &rMt7999PseTopCr,
	.prPpTopCr = &rMt7999PpTopCr,
#endif
#if CFG_SUPPORT_WIFI_SLEEP_COUNT
	.wf_power_dump_start = mt7999PowerDumpStart,
	.wf_power_dump_end = mt7999PowerDumpEnd,
#endif
	.prPseGroup = mt7999_pse_group,
	.u4PseGroupLen = ARRAY_SIZE(mt7999_pse_group),
	.pdmaSetup = mt7999WpdmaConfig,
#if defined(_HIF_PCIE)
#if CFG_MTK_WIFI_WFDMA_WB
	.enableInterrupt = mt7999EnableInterruptViaPcieByEmi,
	.disableInterrupt = asicConnac5xDisablePlatformIRQ,
#elif (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
	.enableInterrupt = mt7999EnableInterruptViaPcie,
	.disableInterrupt = asicConnac5xDisablePlatformIRQ,
#else
	.enableInterrupt = mt7999EnableInterrupt,
	.disableInterrupt = mt7999DisableInterrupt,
#endif /* CFG_SUPPORT_PCIE_PLAT_INT_FLOW */
#else /* !_HIF_PCIE */
	.enableInterrupt = mt7999EnableInterrupt,
	.disableInterrupt = mt7999DisableInterrupt,
#endif /* _HIF_PCIE */
#if CFG_MTK_WIFI_WFDMA_WB
	.configWfdmaIntMask = mt7999ConfigEmiIntMask,
#else
	.configWfdmaIntMask = mt7999ConfigIntMask,
#endif /* CFG_MTK_WIFI_WFDMA_WB */
	.configWfdmaRxRingTh = mt7999ConfigWfdmaRxRingThreshold,
#if defined(_HIF_PCIE)
	.initPcieInt = mt7999InitPcieInt,
	.hwControlVote = mt7999PcieHwControlVote,
	.pdmaStop = asicConnac5xWfdmaStop,
	.pdmaPollingIdle = asicConnac5xWfdmaPollingAllIdle,
	.pcie_msi_info = {
		.prMsiLayout = mt7999_pcie_msi_layout,
		.u4MaxMsiNum = ARRAY_SIZE(mt7999_pcie_msi_layout),
	},

#if CFG_SUPPORT_PCIE_ASPM
	.configPcieAspm = mt7999ConfigPcieAspm,
	.updatePcieAspm = mt7999UpdatePcieAspm,
	.keepPcieWakeup = mt7999KeepPcieWakeup,
	.fgWifiEnL1_2 = TRUE,
	.fgMDEnL1_2 = TRUE,
	.fgWifiRstEnL1_2 = TRUE,
#endif

#if CFG_MTK_WIFI_PCIE_SUPPORT
	.is_en_drv_ctrl_pci_msi_irq = TRUE,
#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
	.pcieMsiMaskIrq = mt7999PcieMsiMaskIrq,
	.pcieMsiUnmaskIrq = mt7999PcieMsiUnmaskIrq,
#endif /* CFG_MTK_ANDROID_WMT */
#endif
	.showDebugInfo = mt7999ShowDebugInfo,
#endif /* _HIF_PCIE */
#if CFG_MTK_WIFI_WFDMA_WB
	.processTxInterrupt = mt7999ProcessTxInterruptByEmi,
	.processRxInterrupt = mt7999ProcessRxInterruptByEmi,
	.processSoftwareInterrupt = mt7999ProcessSoftwareInterruptByEmi,
#else
	.processTxInterrupt = mt7999ProcessTxInterrupt,
	.processRxInterrupt = mt7999ProcessRxInterrupt,
	.processSoftwareInterrupt = asicConnac5xProcessSoftwareInterrupt,
#endif /* CFG_MTK_WIFI_WFDMA_WB */
	.tx_ring_ext_ctrl = mt7999WfdmaTxRingExtCtrl,
	.rx_ring_ext_ctrl = mt7999WfdmaRxRingExtCtrl,
	/* null wfdmaManualPrefetch if want to disable manual mode */
	.wfdmaManualPrefetch = mt7999WfdmaManualPrefetch,
	.lowPowerOwnInit = mt7999LowPowerOwnInit,
	.lowPowerOwnRead = mt7999LowPowerOwnRead,
	.lowPowerOwnSet = mt7999LowPowerOwnSet,
	.lowPowerOwnClear = mt7999LowPowerOwnClear,
	.wakeUpWiFi = asicWakeUpWiFi,
	.softwareInterruptMcu = asicConnac5xSoftwareInterruptMcu,
	.hifRst = asicConnac5xHifRst,
#if CFG_MTK_WIFI_WFDMA_WB
	.devReadIntStatus = mt7999ReadIntStatusByEmi,
#else
	.devReadIntStatus = mt7999ReadIntStatus,
#endif /* CFG_MTK_WIFI_WFDMA_WB */
	.setRxRingHwAddr = mt7999SetRxRingHwAddr,
	.wfdmaAllocRxRing = mt7999WfdmaAllocRxRing,
	.clearEvtRingTillCmdRingEmpty = connac5xClearEvtRingTillCmdRingEmpty,
#if (CFG_MTK_WIFI_SW_EMI_RING == 1) && (CFG_MTK_WIFI_MBU == 1)
	.rSwEmiRingInfo = {
		.rOps = {
			.init = halMbuInit,
			.uninit = halMbuUninit,
			.read = halMbuRead,
			.debug = halMbuDebug,
			.dumpDebugCr = mt7999MbuDumpDebugCr,
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
	.DmaShdlInit = mt7999DmashdlInit,
#endif

#if defined(_HIF_NONE)
	/* for compiler need one entry */
	.DmaShdlInit = NULL
#endif
#if (CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1)
	.updateTxRingMaxQuota = asicConnac5xUpdateDynamicDmashdlQuota,
#endif
};

#if CFG_ENABLE_FW_DOWNLOAD
struct FWDL_OPS_T mt7999_fw_dl_ops = {
	.constructFirmwarePrio = mt7999_ConstructFirmwarePrio,
#if (CFG_SUPPORT_UNI_FWDL == 0)
	.constructPatchName = mt7999_ConstructPatchName,
#if CFG_SUPPORT_SINGLE_FW_BINARY
	.parseSingleBinaryFile = wlanParseSingleBinaryFile,
#endif
#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
	.constrcutIdxLogBin = mt7999_ConstructIdxLogBinName,
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */
#if defined(_HIF_PCIE)
	.downloadPatch = mt7999_wlanDownloadPatch,
#endif
	.downloadFirmware = wlanConnacFormatDownload,
	.downloadByDynMemMap = NULL,
	.getFwInfo = wlanGetConnacFwInfo,
	.getFwDlInfo = asicGetFwDlInfo,
	.downloadEMI = wlanDownloadEMISectionViaDma,
#if CFG_MTK_WIFI_SUPPORT_PHY_FWDL
	.constructPhyName = mt7999_ConstructPhyName,
	.downloadPhyFw = wlanDownloadPhyFw,
#endif
	.getFwVerInfo = wlanParseRamCodeReleaseManifest,
#else
	.getFwVerInfo = uniFwdlGetReleaseManifest,
#endif /* CFG_SUPPORT_UNI_FWDL */
#if (CFG_SUPPORT_PRE_ON_PHY_ACTION == 1)
	.phyAction = wlanPhyAction,
#else
	.phyAction = NULL,
#endif
#if defined(_HIF_PCIE)
	.mcu_init = mt7999_mcu_init,
	.mcu_deinit = mt7999_mcu_deinit,
#endif
#if CFG_SUPPORT_WIFI_DL_BT_PATCH
	.constructBtPatchName = asicConnac5xConstructBtPatchName,
	.downloadBtPatch = asicConnac5xDownloadBtPatch,
#if (CFG_SUPPORT_CONNAC5X == 1)
	.configBtImageSection = asicConnac5xConfigBtImageSection,
#endif
#endif
};

#if (CFG_SUPPORT_UNI_FWDL == 1)
struct UNI_FWDL_INFO mt7999_uni_fwdl_info = {
	.u4SyncInfo = BIT(UNI_FWDL_SYNC_INFO_FW_TIMEOUT)
#if (CFG_SUPPORT_UNI_FWDL_MSG_NOTIF == 1)
	| BIT(UNI_FWDL_SYNC_INFO_SUPPORT_MSG_NOTIF)
#endif /* CFG_SUPPORT_UNI_FWDL_MSG_NOTIF */
#if (CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM == 1)
	| BIT(UNI_FWDL_SYNC_INFO_SHM_EMI)
#endif /* CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM */
#if (CFG_SUPPORT_UNI_FWDL_SLOT_DL == 1)
	| BIT(UNI_FWDL_SYNC_INFO_SUPPORT_SLOT_DL)
#endif /* CFG_SUPPORT_UNI_FWDL_SLOT_DL */
#if (CFG_SUPPORT_UNI_FWDL_BROM_PATCH_NOTIF == 1)
	| BIT(UNI_FWDL_SYNC_INFO_BROM_NOTIF)
#endif /* CFG_SUPPORT_UNI_FWDL_BROM_PATCH_NOTIF */
	,
	.u4FwTimeoutDuration = 2000, /* ms */
#if defined(_HIF_PCIE)
	.rHifOps = {
		.init = uniFwdlPcieInit,
		.deinit = uniFwdlPcieDeInit,
		.dump = uniFwdlPcieDump,
		.query_hw_info = uniFwdlPcieQueryHwInfo,
		.start_dl = uniFwdlPcieStartDl,
		.dl_block = uniFwdlPcieDlBlock,
		.send_msg = uniFwdlPcieSendMsg,
		.rcv_irq = uniFwdlPcieRcvInterrupt,
	},
	.rShmOps = {
		.init = mt7999_shm_init,
		.deinit = mt7999_shm_deinit,
		.dump = mt7999_shm_dump,
		.write_sync_info = mt7999_write_sync_info,
		.write_timeout_duration = mt7999_write_timeout_duration,
		.write_msg_idx = mt7999_write_msg_idx,
		.write_msg_id = mt7999_write_msg_id,
		.write_calibration_action = mt7999_write_calibration_action,
		.write_radio_type = mt7999_write_radio_type,
		.write_dl_addr = mt7999_write_dl_addr,
		.write_dl_size = mt7999_write_dl_size,
		.write_slot_tbl_addr = mt7999_write_slot_tbl_addr,
		.write_slot_tbl_size = mt7999_write_slot_tbl_size,
		.write_slot_num = mt7999_write_slot_num,
		.write_slot_size = mt7999_write_slot_size,
		.write_slot_rdy_idx = mt7999_write_slot_rdy_idx,
		.write_notif_done_idx = mt7999_write_notif_done_idx,
		.write_ctx_info_A_addr = mt7999_write_ctx_info_A_addr,
		.write_ctx_info_A_size = mt7999_write_ctx_info_A_size,
		.write_ctx_info_B_addr = mt7999_write_ctx_info_B_addr,
		.write_ctx_info_B_size = mt7999_write_ctx_info_B_size,
		.write_co_dl_radio = mt7999_write_co_dl_radio,
		.write_co_dl_sec_map_size_wf =
			mt7999_write_co_dl_sec_map_size_wf,
		.write_co_dl_sec_map_size_bt =
			mt7999_write_co_dl_sec_map_size_bt,
		.write_co_dl_sec_map_size_zb =
			mt7999_write_co_dl_sec_map_size_zb,
		.read_chip_info = mt7999_read_chip_info,
		.read_chip_id = mt7999_read_chip_id,
		.read_hw_ver = mt7999_read_hw_ver,
		.read_fw_ver = mt7999_read_fw_ver,
		.read_efuse_info = mt7999_read_efuse_info,
		.read_boot_stage = mt7999_read_boot_stage,
		.read_slot_done_idx = mt7999_read_slot_done_idx,
		.read_msg_done_idx = mt7999_read_msg_done_idx,
		.read_notif_idx = mt7999_read_notif_idx,
		.read_notif_id = mt7999_read_notif_id,
		.read_dl_resp = mt7999_read_dl_resp,
		.read_block_radio = mt7999_read_block_radio,
		.read_block_id = mt7999_read_block_id,
		.read_error_code = mt7999_read_error_code,
		.read_sub_error_code = mt7999_read_sub_error_code,
		.read_comm_state = mt7999_read_comm_state,
	},
#endif
	.trigger_wf_fwdl_doorbell = mt7999_trigger_wf_fwdl_doorbell,
	.u4MinDoorbellTime = 3, /* ms */
};
#endif /* CFG_SUPPORT_UNI_FWDL */
#endif /* CFG_ENABLE_FW_DOWNLOAD */

struct TX_DESC_OPS_T mt7999_TxDescOps = {
	.fillNicAppend = fillNicTxDescAppend,
	.fillHifAppend = fillTxDescAppendByHostV2,
	.fillTxByteCount = fillConnac5xTxDescTxByteCount,
};

struct RX_DESC_OPS_T mt7999_RxDescOps = {0};

#if (DBG_DISABLE_ALL_INFO == 0)
struct CHIP_DBG_OPS mt7999_DebugOps = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.showPdmaInfo = connac5x_show_wfdma_info,
#endif
	.showPseInfo = connac5x_show_pse_info,
	.showPleInfo = connac5x_show_ple_info,
	.showTxdInfo = connac5x_show_txd_Info,
	.showWtblInfo = connac5x_show_wtbl_info,
	.showUmacWtblInfo = connac5x_show_umac_wtbl_info,
	.showCsrInfo = NULL,
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.showDmaschInfo = connac5x_show_dmashdl_lite_info,
#endif
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.getFwDebug = NULL,
	.setFwDebug = connac5x_set_ple_int_no_read,
#endif
	.showHifInfo = NULL,
	.printHifDbgInfo = NULL,
	.show_rx_rate_info = connac5x_show_rx_rate_info,
	.show_rx_rssi_info = connac5x_show_rx_rssi_info,
	.show_stat_info = connac5x_show_stat_info,
	.get_tx_info_from_txv = connac5x_get_tx_info_from_txv,
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	.show_mld_info = connac5x_show_mld_info,
#endif
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.show_wfdma_dbg_probe_info = mt7999_show_wfdma_dbg_probe_info,
	.show_wfdma_wrapper_info = mt7999_show_wfdma_wrapper_info,
	.dumpwfsyscpupcr = mt7999_dumpWfsyscpupcr,
	.dumpBusStatus = mt7999_DumpBusStatus,
#if CFG_SUPPORT_PCIE_ASPM
	.dumpPcieStatus = mt7999DumpPcieDateFlowStatus,
#endif /* CFG_SUPPORT_PCIE_ASPM */
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	.dumpPcieCr = mt7999_dumpPcieReg,
	.checkDumpViaBt = mt7999_CheckDumpViaBt,
#endif
#if CFG_MTK_WIFI_MBU
	.getMbuTimeoutStatus = mt7999_get_mbu_timeout_status,
#endif
#endif
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	.get_rx_rate_info = mt7999_get_rx_rate_info,
#endif
#if CFG_SUPPORT_LLS
	.get_rx_link_stats = mt7999_get_rx_link_stats,
#endif
	.dumpTxdInfo = connac5x_dump_tmac_info,
};
#endif /* DBG_DISABLE_ALL_INFO */

#if CFG_SUPPORT_QA_TOOL
#if (CONFIG_WLAN_SERVICE == 1)
struct test_capability mt7999_toolCapability = {
	/* u_int32 version; */
	0xD,
	/* u_int32 tag_num; */
	3,
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
		0x00000000,	/* u_int32 channel_band_dbdc_ext */

		/* BIT[7:0]: Support phy 0xF (bitwise),
		 *           phy0, phy1, phy2, phy3(little)
		 */
		/* BIT[15:8]: Support Adie 0x1 (bitwise) */
		0x0103,	/* u_int32 phy_adie_index; CFG_SUPPORT_CONNAC5X */

		/* BIT[7:0]: Band0 TX path 2 */
		/* BIT[15:8]: Band0 RX path 2 */
		/* BIT[23:16]: Band1 TX path 2 */
		/* BIT[31:24]: Band1 RX path 2 */
		0x02020202,	/* u_int32 band_0_1_wf_path_num; */

		/* BIT[7:0]: Band2 TX path 1 */
		/* BIT[15:8]: Band2 RX path 1*/
		/* BIT[23:16]: Band3 TX path 0 */
		/* BIT[31:24]: Band3 RX path 1 */
		0x00000000,	/* u_int32 band_2_3_wf_path_num; */

		/* BIT[7:0]: Band0 BW20, 0x1 */
		/* BIT[15:8]: Band1 BW320, 0x2F */
		/* BIT[23:16]: Band2 BW160, 0xF */
		/* BIT[31:24]: Band3 BW20, 0x1 */
		0x00002F01,	/* u_int32 band_bandwidth; */

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
		/* BIT12: TX time 1 */
		/* BIT13: keep full power switch 1*/
		0x312C,	/*u_int32 feature1; */

		/* u_int32 reserved[15]; */
		{ 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0 }
	},

	{
		/* GET_CAPABILITY_TAG_RX_INFO */
		3,	/* u_int32 tag; */
		/* GET_CAPABILITY_TAG_RX_INFO_LEN */
		32,	/* u_int32 tag_len; */
		/* u_int32 reserved[64]; */
		{0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
		}
	}
};
#endif

struct ATE_OPS_T mt7999_AteOps = {
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
	.icapRiseVcoreClockRate = mt7999_icapRiseVcoreClockRate,
	.icapDownVcoreClockRate = mt7999_icapDownVcoreClockRate,
#if (CONFIG_WLAN_SERVICE == 1)
	.tool_capability = &mt7999_toolCapability,
#endif
};
#endif /* CFG_SUPPORT_QA_TOOL */

#if defined(_HIF_PCIE)
#if (CFG_MTK_FPGA_PLATFORM == 0)
static struct CCIF_OPS mt7999_ccif_ops = {
	.clear_interrupt_status = mt7999_ccif_clear_interrupt_status,
	.get_interrupt_status = mt7999_ccif_get_interrupt_status,
	.notify_utc_time_to_fw = mt7999_ccif_notify_utc_time_to_fw,
	.set_fw_log_read_pointer = mt7999_ccif_set_fw_log_read_pointer,
	.get_fw_log_read_pointer = mt7999_ccif_get_fw_log_read_pointer,
	.trigger_fw_assert = mt7999_ccif_trigger_fw_assert,
};
#endif
#if CFG_MTK_WIFI_FW_LOG_MMIO
static struct FW_LOG_OPS mt7999_fw_log_mmio_ops = {
	.init = fwLogMmioInitMcu,
	.deinit = fwLogMmioDeInitMcu,
	.start = fwLogMmioStart,
	.stop = fwLogMmioStop,
	.handler = fwLogMmioHandler,
};
#endif

#if CFG_MTK_WIFI_FW_LOG_EMI
static struct FW_LOG_OPS mt7999_fw_log_emi_ops = {
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
struct thermal_sensor_info mt7999_thermal_sensor_info[] = {
	{"wifi_adie_0", THERMAL_TEMP_TYPE_ADIE, 0},
	{"wifi_ddie_0", THERMAL_TEMP_TYPE_DDIE, 0},
	{"wifi_ddie_1", THERMAL_TEMP_TYPE_DDIE, 1},
	{"wifi_ddie_2", THERMAL_TEMP_TYPE_DDIE, 2},
	{"wifi_ddie_3", THERMAL_TEMP_TYPE_DDIE, 3},
};
#endif

#if CFG_NEW_HIF_DEV_REG_IF
enum HIF_DEV_REG_REASON mt7999ValidMmioReadReason[] = {
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
#if (CFG_MTK_FPGA_PLATFORM != 0)
	HIF_DEV_REG_HIF_RING,
#endif
#if (CFG_MTK_WIFI_WFDMA_WB == 0)
	HIF_DEV_REG_HIF_READ,
	HIF_DEV_REG_HIF_RING,
#endif
};

#if (CFG_MTK_WIFI_SW_EMI_RING == 1) && (CFG_MTK_WIFI_MBU == 1)
enum HIF_DEV_REG_REASON mt7999NoMmioReadReason[] = {
	HIF_DEV_REG_HIF_DBG,
	HIF_DEV_REG_NOMMIO_DBG,
};
#endif
#endif /* CFG_NEW_HIF_DEV_REG_IF */

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
struct EMI_WIFI_MISC_RSV_MEM_INFO mt7999_wifi_misc_rsv_mem_info[] = {
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
static struct sw_sync_emi_info mt7999_sw_sync_emi_info[SW_SYNC_TAG_NUM];

struct mt66xx_chip_info mt66xx_chip_info_mt7999 = {
	.bus_info = &mt7999_bus_info,
#if CFG_ENABLE_FW_DOWNLOAD
	.fw_dl_ops = &mt7999_fw_dl_ops,
#if (CFG_SUPPORT_UNI_FWDL == 1)
	.uni_fwdl_info = &mt7999_uni_fwdl_info,
#endif /* CFG_SUPPORT_UNI_FWDL */
#endif /* CFG_ENABLE_FW_DOWNLOAD */
#if CFG_SUPPORT_QA_TOOL
	.prAteOps = &mt7999_AteOps,
#endif /* CFG_SUPPORT_QA_TOOL */
	.prTxDescOps = &mt7999_TxDescOps,
	.prRxDescOps = &mt7999_RxDescOps,
#if (DBG_DISABLE_ALL_INFO == 0)
	.prDebugOps = &mt7999_DebugOps,
#endif
	.chip_id = MT7999_CHIP_ID,
	.should_verify_chip_id = FALSE,
	.sw_sync0 = CONNAC5X_CONN_CFG_ON_CONN_ON_MISC_ADDR,
	.sw_ready_bits = WIFI_FUNC_NO_CR4_READY_BITS,
	.sw_ready_bit_offset =
		Connac5x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT,
	.patch_addr = MT7999_PATCH_START_ADDR,
	.is_support_cr4 = FALSE,
	.is_support_wacpu = FALSE,
	.is_support_dmashdl_lite = TRUE,
	.is_support_dmashdl_lite_wlanid_dec = TRUE,
	.sw_sync_emi_info = mt7999_sw_sync_emi_info,
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
	.wifi_off_magic_num = MT7999_WIFI_OFF_MAGIC_NUM,
#endif /* CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI */
#if defined(_HIF_PCIE)
	.is_en_wfdma_no_mmio_read = FALSE,
#if CFG_MTK_WIFI_SW_EMI_RING
	.is_en_sw_emi_read = TRUE,
#endif
#endif /* _HIF_PCIE */
#if CFG_MTK_WIFI_WFDMA_WB
	.is_support_wfdma_write_back = TRUE,
	.is_support_wfdma_cidx_fetch = TRUE,
	.wb_didx_size = sizeof(struct WFDMA_EMI_RING_DIDX),
	.wb_cidx_size = sizeof(struct WFDMA_EMI_RING_CIDX),
	.wb_hw_done_flag_size = sizeof(struct WFDMA_EMI_DONE_FLAG),
	.wb_sw_done_flag_size = sizeof(struct WFDMA_EMI_DONE_FLAG),
	.allocWfdmaWbBuffer = asicConnac5xAllocWfdmaWbBuffer,
	.freeWfdmaWbBuffer = asicConnac5xFreeWfdmaWbBuffer,
	.enableWfdmaWb = mt7999EnableWfdmaWb,
	.runWfdmaCidxFetch = mt7999RunWfdmaCidxFetch,
#if defined(_HIF_PCIE)
	.isWfdmaRxReady = mt7999IsWfdmaRxReady,
#endif /* _HIF_PCIE */
#endif
	.txd_append_size = MT7999_TX_DESC_APPEND_LENGTH,
	.hif_txd_append_size = MT7999_HIF_TX_DESC_APPEND_LENGTH,
	.rxd_size = MT7999_RX_DESC_LENGTH,
	.init_evt_rxd_size = MT7999_RX_INIT_DESC_LENGTH,
	.pse_header_length = CONNAC5X_NIC_TX_PSE_HEADER_LENGTH,
	.init_event_size = CONNAC5X_RX_INIT_EVENT_LENGTH,
	.eco_info = mt7999_eco_table,
	.isNicCapV1 = FALSE,
	.is_support_efuse = TRUE,
	.top_hcr = CONNAC5X_TOP_HCR,
	.top_hvr = CONNAC5X_TOP_HVR,
	.top_fvr = CONNAC5X_TOP_FVR,
#if (CFG_SUPPORT_802_11AX == 1)
	.arb_ac_mode_addr = MT7999_ARB_AC_MODE_ADDR,
#endif
	.asicCapInit = asicConnac5xCapInit,
#if CFG_ENABLE_FW_DOWNLOAD
	.asicEnableFWDownload = NULL,
#endif /* CFG_ENABLE_FW_DOWNLOAD */

	.downloadBufferBin = NULL,
	.is_support_hw_amsdu = TRUE,
	.is_support_nvram_fragment = TRUE,
	.is_support_asic_lp = TRUE,
	.asicWfdmaReInit = asicConnac5xWfdmaReInit,
	.asicWfdmaReInit_handshakeInit = asicConnac5xWfdmaDummyCrWrite,
	.group5_size = sizeof(struct HW_MAC_RX_STS_GROUP_5),
	.u4LmacWtblDUAddr = CONNAC5X_WIFI_LWTBL_BASE,
	.u4UmacWtblDUAddr = CONNAC5X_WIFI_UWTBL_BASE,
	.coexpccifon = mt7999ConnacPccifOn,
	.coexpccifoff = mt7999ConnacPccifOff,
#if CFG_MTK_MDDP_SUPPORT
	.isSupportMddpAOR = false,
	.isSupportMddpSHM = true,
	.u4MdLpctlAddr = CONN_HOST_CSR_TOP_WF_MD_LPCTL_ADDR,
	.u4MdDrvOwnTimeoutTime = 2000,
	.checkMdRxStall = mt7999CheckMdRxStall,
#else
	.isSupportMddpAOR = false,
	.isSupportMddpSHM = false,
#endif
	.u4HostWfdmaBaseAddr = WF_P0_WFDMA_BASE,
	.u4HostWfdmaWrapBaseAddr = WFDMA_WRAP_CSR_BASE,
	.u4McuWfdmaBaseAddr = WF_M0_WFDMA_BASE,
	.u4DmaShdlBaseAddr = WF_HIF_DMASHDL_TOP_BASE,
	.cmd_max_pkt_size = CFG_TX_MAX_PKT_SIZE, /* size 1600 */
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	.apsLinkPlanDecision = mt7999_apsLinkPlanDecision,
	.apsUpdateTotalScore = mt7999_apsUpdateTotalScore,
	.apsFillBssDescSet = mt7999_apsFillBssDescSet,
#endif
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
	.queryDFDInfo = asicConnac5xQueryDFDInfo,
#endif
	.isUpgradeWholeChipReset = mt7999_isUpgradeWholeChipReset,

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
	.checkbusNoAck = mt7999_CheckBusNoAck,
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
		.sensor_num = ARRAY_SIZE(mt7999_thermal_sensor_info),
		.sensor_info = mt7999_thermal_sensor_info,
	},
#endif
	.trigger_fw_assert = mt7999_trigger_fw_assert,
	.fw_log_info = {
#if CFG_MTK_WIFI_FW_LOG_MMIO
		.ops = &mt7999_fw_log_mmio_ops,
#endif
#if CFG_MTK_WIFI_FW_LOG_EMI
		.base = 0x1A8000,
		.ops = &mt7999_fw_log_emi_ops,
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
	.ccif_ops = &mt7999_ccif_ops,
#endif
	.clear_sw_interrupt_status = mt7999_clear_sw_interrupt_status,
	.get_sw_interrupt_status = mt7999_get_sw_interrupt_status,
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
	.isValidMmioReadReason = connac5xIsValidMmioReadReason,
	.prValidMmioReadReason = mt7999ValidMmioReadReason,
	.u4ValidMmioReadReasonSize = ARRAY_SIZE(mt7999ValidMmioReadReason),
#if (CFG_MTK_WIFI_SW_EMI_RING == 1) && (CFG_MTK_WIFI_MBU == 1)
	.isNoMmioReadReason = connac5xIsNoMmioReadReason,
	.prNoMmioReadReason = mt7999NoMmioReadReason,
	.u4NoMmioReadReasonSize = ARRAY_SIZE(mt7999NoMmioReadReason),
#endif
#endif /* CFG_NEW_HIF_DEV_REG_IF */
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.rsvMemWiFiMisc = mt7999_wifi_misc_rsv_mem_info,
	.rsvMemWiFiMiscSize = ARRAY_SIZE(mt7999_wifi_misc_rsv_mem_info),
#endif
#if (CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1)
	.dmashdlQuotaDecision = asicConnac5xDynamicDmashdlQuotaDecision,
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
	.au4DmaMaxQuotaRfBand = {0x100, 0x2d0},
#endif /* CFG_SUPPORT_WIFI_6G */
#endif
#if CFG_SUPPORT_XONVRAM
	/* Platform custom config for conninfra */
	.rPlatcfgInfraSysram = {
		.addr = CONNAC5X_PLAT_CFG_ADDR,
		.size = CONNAC5X_PLAT_CFG_SIZE,
	}
#endif
};

struct mt66xx_hif_driver_data mt66xx_driver_data_mt7999 = {
	.chip_info = &mt66xx_chip_info_mt7999,
};

void mt7999_icapRiseVcoreClockRate(void)
{
	DBGLOG(HAL, STATE, "icapRiseVcoreClockRate skip\n");
}

void mt7999_icapDownVcoreClockRate(void)
{
	DBGLOG(HAL, STATE, "icapDownVcoreClockRate skip\n");
}

static void mt7999_ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx)
{
#if (CFG_SUPPORT_UNI_FWDL == 0)
	int ret = 0;
	uint8_t ucIdx = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];
	uint8_t aucTestmode[CFG_FW_FLAVOR_MAX_LEN] = {0};

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7999GetFlavorVer(prGlueInfo, &aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt7999_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7999_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt7999_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7999_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2. WIFI_RAM_CODE_MT7999_1_1.bin */
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
	if (get_wifi_test_mode_fwdl() == 1)
		kalScnprintf(aucTestmode,
			CFG_FW_FLAVOR_MAX_LEN,
			"TESTMODE_");
#endif
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"WIFI_RAM_CODE_MT%x_%s%s_%u.bin",
			MT7999_CHIP_ID,
			aucTestmode,
			aucFlavor,
			MT7999_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	for (ucIdx = 0; apucmt7999FwName[ucIdx]; ucIdx++) {
		if ((*pucNameIdx + 3) >= ucMaxNameIdx) {
			/* the table is not large enough */
			DBGLOG(INIT, ERROR,
				"kalFirmwareImageMapping >> file name array is not enough.\n");
			ASSERT(0);
			continue;
		}

		/* Type 3. WIFI_RAM_CODE_7999.bin */
		ret = kalSnprintf(*(apucName + (*pucNameIdx)),
				CFG_FW_NAME_MAX_LEN, "%s.bin",
				apucmt7999FwName[ucIdx]);
		if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
			(*pucNameIdx) += 1;
		else
			DBGLOG(INIT, ERROR,
				"[%u] kalSnprintf failed, ret: %d\n",
				__LINE__, ret);
	}
#else
	int ret = 0;

	if (apucName && pucNameIdx && ((*pucNameIdx) < ucMaxNameIdx)) {
		ret = kalSnprintf(apucName[(*pucNameIdx)],
				  CFG_FW_NAME_MAX_LEN,
				  "WIFI_UNI_CODE_MT7999_1_1.bin");
		if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
			(*pucNameIdx) += 1;
		else
			DBGLOG(INIT, ERROR,
				"[%u] kalSnprintf failed, ret: %d\n",
				__LINE__, ret);
	}
#endif
}

static void mt7999_ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];
	uint8_t aucTestmode[CFG_FW_FLAVOR_MAX_LEN] = {0};

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7999GetFlavorVer(prGlueInfo, &aucFlavor[0]);

#if CFG_SUPPORT_SINGLE_FW_BINARY
	/* Type 0. mt7999_wifi.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7999_wifi.bin");
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 1. mt7999_wifi_flavor.bin */
	ret = kalSnprintf(*(apucName + (*pucNameIdx)),
			CFG_FW_NAME_MAX_LEN,
			"mt7999_wifi_%s.bin",
			aucFlavor);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
#endif

	/* Type 2. WIFI_MT7999_PATCH_MCU_1_1_hdr.bin */
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
	if (get_wifi_test_mode_fwdl() == 1)
		kalScnprintf(aucTestmode,
			CFG_FW_FLAVOR_MAX_LEN,
			"TESTMODE_");
#endif
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			CFG_FW_NAME_MAX_LEN,
			"WIFI_MT%x_PATCH_MCU_%s%s_%u_hdr.bin",
			MT7999_CHIP_ID,
			aucTestmode,
			aucFlavor,
			MT7999_ROM_VERSION);
	if (ret >= 0 && ret < CFG_FW_NAME_MAX_LEN)
		(*pucNameIdx) += 1;
	else
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);

	/* Type 3. mt7999_patch_e1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "mt7999_patch_e1_hdr.bin");
	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
}

#if CFG_MTK_WIFI_SUPPORT_PHY_FWDL
static void mt7999_ConstructPhyName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	kalMemZero(aucFlavor, sizeof(aucFlavor));
	mt7999GetFlavorVer(prGlueInfo, &aucFlavor[0]);

	/* Type 1. WIFI_MT7999_PHY_RAM_CODE_1_1_hdr.bin */
	ret = kalSnprintf(apucName[(*pucNameIdx)],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_MT%x_PHY_RAM_CODE_%s_%u.bin",
			  MT7999_CHIP_ID,
			  aucFlavor,
			  MT7999_ROM_VERSION);
	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
	else
		(*pucNameIdx) += 1;
}
#endif

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
static void mt7999_ConstructIdxLogBinName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName)
{
	int ret = 0;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN];

	mt7999GetFlavorVer(prGlueInfo, &aucFlavor[0]);

	/* ex: WIFI_RAM_CODE_MT7999_2_1_idxlog.bin */
	ret = kalSnprintf(apucName[0],
			  CFG_FW_NAME_MAX_LEN,
			  "WIFI_RAM_CODE_MT%x_%s_%u_idxlog.bin",
			  MT7999_CHIP_ID,
			  aucFlavor,
			  MT7999_ROM_VERSION);

	if (ret < 0 || ret >= CFG_FW_NAME_MAX_LEN)
		DBGLOG(INIT, ERROR,
			"[%u] kalSnprintf failed, ret: %d\n",
			__LINE__, ret);
}
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */

#if defined(_HIF_PCIE)
static uint32_t mt7999RxRingSwIdx2HwIdx(uint32_t u4SwRingIdx)
{
	uint32_t u4HwRingIdx = 0;

	/*
	 * RX_RING_DATA0   (RX_Ring4) - Band0/1 Rx Data
	 * RX_RING_DATA1   (RX_Ring5) - Band2 Rx Data
	 * RX_RING_EVT     (RX_Ring6) - Event
	 * RX_RING_TXDONE0 (RX_Ring7) - Tx Free Done
	 * RX_RING_TXDONE1 (RX_Ring8) - ICS report
	*/
	switch (u4SwRingIdx) {
	case RX_RING_DATA0:
		u4HwRingIdx = 4;
		break;
	case RX_RING_DATA1:
		u4HwRingIdx = 5;
		break;
	case RX_RING_EVT:
		u4HwRingIdx = 6;
		break;
	case RX_RING_TXDONE0:
		u4HwRingIdx = 7;
		break;
	case RX_RING_TXDONE1:
		u4HwRingIdx = 8;
		break;
	default:
		return RX_RING_MAX;
	}

	return u4HwRingIdx;
}

static uint8_t mt7999SetRxRingHwAddr(struct RTMP_RX_RING *prRxRing,
		struct BUS_INFO *prBusInfo, uint32_t u4SwRingIdx)
{
	uint32_t u4RingIdx = mt7999RxRingSwIdx2HwIdx(u4SwRingIdx);

	if (u4RingIdx >= RX_RING_MAX)
		return FALSE;

	halSetRxRingHwAddr(prRxRing, prBusInfo, u4RingIdx);

	return TRUE;
}

static bool mt7999WfdmaAllocRxRing(struct GLUE_INFO *prGlueInfo,
		bool fgAllocMem)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;

	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_DATA1, prHifInfo->u4RxDataRingSize,
			RXD_SIZE, CFG_RX_MAX_PKT_SIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[%d] fail\n", RX_RING_DATA1);
		return false;
	}

	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_TXDONE0, prHifInfo->u4RxEvtRingSize,
			RXD_SIZE, RX_BUFFER_AGGRESIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[%d] fail\n", RX_RING_TXDONE0);
		return false;
	}

	if (!halWpdmaAllocRxRing(prGlueInfo,
			RX_RING_TXDONE1, prHifInfo->u4RxEvtRingSize,
			RXD_SIZE, RX_BUFFER_AGGRESIZE, fgAllocMem)) {
		DBGLOG(HAL, ERROR, "AllocRxRing[%d] fail\n", RX_RING_TXDONE1);
		return false;
	}
	return true;
}

static void mt7999ProcessTxInterrupt(
		struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if (u4Sta & WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_16_MASK)
		halWpdmaProcessCmdDmaDone(
			prAdapter->prGlueInfo, TX_RING_FWDL);

#if (CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0)
	if (u4Sta & WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_15_MASK)
		halWpdmaProcessCmdDmaDone(
			prAdapter->prGlueInfo, TX_RING_CMD);
#endif /* CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0 */

#if (CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0)
	if (u4Sta & WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_0_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA0);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}

	if (u4Sta & WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_1_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA1);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}

	if (u4Sta & WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_2_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA2);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}
	if (u4Sta & WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_3_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA3);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}

	if (u4Sta & WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_4_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA_PRIO);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}

	if (u4Sta & WF_P0_WFDMA_HOST_INT_STA_TX_DONE_INT_STS_5_MASK) {
		halWpdmaProcessDataDmaDone(
			prAdapter->prGlueInfo, TX_RING_DATA_ALTX);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0 */
}

static void mt7999ProcessRxDataInterrupt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if ((u4Sta &
	     WF_P0_WFDMA_HOST_INT_STA_RX_DONE_INT_STS_4_MASK) ||
	    (KAL_TEST_BIT(RX_RING_DATA0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_DATA0, TRUE);

	if ((u4Sta &
	     WF_P0_WFDMA_HOST_INT_STA_RX_DONE_INT_STS_5_MASK) ||
	    (KAL_TEST_BIT(RX_RING_DATA1, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_DATA1, TRUE);
}

static void mt7999ProcessRxInterrupt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->u4IntStatus;

	if ((u4Sta & WF_P0_WFDMA_HOST_INT_STA_RX_DONE_INT_STS_6_MASK) ||
	    (KAL_TEST_BIT(RX_RING_EVT, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_EVT, FALSE);

	if ((u4Sta & WF_P0_WFDMA_HOST_INT_STA_RX_DONE_INT_STS_7_MASK) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE0, FALSE);

	if ((u4Sta & WF_P0_WFDMA_HOST_INT_STA_RX_DONE_INT_STS_8_MASK) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE1, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE1, FALSE);

	mt7999ProcessRxDataInterrupt(prAdapter);
}

static void mt7999SetTRXRingPriorityInterrupt(struct ADAPTER *prAdapter)
{
#if (WFDMA_AP_MSI_NUM == 8)
	uint32_t u4Val = 0;

	u4Val = 0xFF8E;
	HAL_MCR_WR(prAdapter, WF_P0_WFDMA_WPDMA_INT_RX_PRI_SEL_ADDR, u4Val);

	u4Val = 0xFFF1FFFF;
	HAL_MCR_WR(prAdapter, WF_P0_WFDMA_WPDMA_INT_TX_PRI_SEL_ADDR, u4Val);
#endif
}

static void mt7999WfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint32_t u4WrVal = 0, u4Addr = 0;
	uint32_t u4PrefetchCnt = 0x4, u4TxDataPrefetchCnt = 0x10;
	uint32_t u4PrefetchBase = 0x00400000, u4TxDataPrefetchBase = 0x01000000;
	uint32_t u4RxDataPrefetchCnt = 0x8;
	uint32_t u4RxDataPrefetchBase = 0x00800000;
#if CFG_MTK_MDDP_SUPPORT
	uint32_t u4MdTxDataPrefetchCnt = 0x8;
	uint32_t u4MdTxDataPrefetchBase = 0x00800000;
#endif

	/* Rx ring */
	for (u4Addr = WF_P0_WFDMA_WPDMA_RX_RING4_EXT_CTRL_ADDR;
	     u4Addr <= WF_P0_WFDMA_WPDMA_RX_RING5_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4RxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4RxDataPrefetchBase;
	}
	for (u4Addr = WF_P0_WFDMA_WPDMA_RX_RING6_EXT_CTRL_ADDR;
	     u4Addr <= WF_P0_WFDMA_WPDMA_RX_RING8_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4PrefetchBase;
	}

	/* Tx ring */
	/* fw download reuse tx data ring */
	u4Addr = WF_P0_WFDMA_WPDMA_TX_RING16_EXT_CTRL_ADDR;
	u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	for (u4Addr = WF_P0_WFDMA_WPDMA_TX_RING0_EXT_CTRL_ADDR;
		 u4Addr <= WF_P0_WFDMA_WPDMA_TX_RING3_EXT_CTRL_ADDR;
	     u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4TxDataPrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4TxDataPrefetchBase;
	}
	for (u4Addr = WF_P0_WFDMA_WPDMA_TX_RING4_EXT_CTRL_ADDR;
		 u4Addr <=
			WF_P0_WFDMA_WPDMA_TX_RING5_EXT_CTRL_ADDR;
		 u4Addr += 0x4) {
		u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
		u4WrVal += u4PrefetchBase;
	}

	u4Addr = WF_P0_WFDMA_WPDMA_TX_RING15_EXT_CTRL_ADDR;
	u4WrVal = (u4WrVal & 0xFFFF0000) | u4PrefetchCnt;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
	u4WrVal += u4PrefetchBase;

	mt7999SetTRXRingPriorityInterrupt(prAdapter);
}

static void mt7999ReadIntStatus(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct BUS_INFO *prBusInfo = prChipInfo->bus_info;
	uint32_t u4RegValue = 0, u4WrValue = 0, u4Addr;

	*pu4IntStatus = 0;

	u4Addr = WF_P0_WFDMA_HOST_INT_STA_ADDR;
	HAL_RMCR_RD(HIF_READ, prAdapter, u4Addr, &u4RegValue);

#if defined(_HIF_PCIE) || defined(_HIF_AXI)

	if (HAL_IS_CONNAC5X_EXT_RX_DONE_INTR(
		    u4RegValue, prBusInfo->host_int_rxdone_bits)) {
		*pu4IntStatus |= WHISR_RX0_DONE_INT;
		u4WrValue |= (u4RegValue & prBusInfo->host_int_rxdone_bits);
	}

	if (HAL_IS_CONNAC5X_EXT_TX_DONE_INTR(
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

static void mt7999ConfigIntMask(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct WIFI_VAR *prWifiVar;
	uint32_t u4Addr = 0, u4WrVal = 0;

	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;

	u4Addr = enable ? WF_P0_WFDMA_HOST_INT_ENA_SET_ADDR :
		WF_P0_WFDMA_HOST_INT_ENA_CLR_ADDR;
	u4WrVal =
		WF_P0_WFDMA_HOST_INT_ENA_HOST_RX_DONE_INT_ENA4_MASK |
		WF_P0_WFDMA_HOST_INT_ENA_HOST_RX_DONE_INT_ENA5_MASK |
		WF_P0_WFDMA_HOST_INT_ENA_HOST_RX_DONE_INT_ENA6_MASK |
		WF_P0_WFDMA_HOST_INT_ENA_HOST_RX_DONE_INT_ENA7_MASK |
		WF_P0_WFDMA_HOST_INT_ENA_HOST_RX_DONE_INT_ENA8_MASK |
#if (CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0)
		WF_P0_WFDMA_HOST_INT_ENA_HOST_TX_DONE_INT_ENA0_MASK |
		WF_P0_WFDMA_HOST_INT_ENA_HOST_TX_DONE_INT_ENA1_MASK |
		WF_P0_WFDMA_HOST_INT_ENA_HOST_TX_DONE_INT_ENA2_MASK |
		WF_P0_WFDMA_HOST_INT_ENA_HOST_TX_DONE_INT_ENA3_MASK |
		WF_P0_WFDMA_HOST_INT_ENA_HOST_TX_DONE_INT_ENA4_MASK |
		WF_P0_WFDMA_HOST_INT_ENA_HOST_TX_DONE_INT_ENA5_MASK |
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR == 0 */
#if (CFG_SUPPORT_DISABLE_CMD_DDONE_INTR == 0)
		WF_P0_WFDMA_HOST_INT_ENA_HOST_TX_DONE_INT_ENA15_MASK |
#endif /* CFG_SUPPORT_DISABLE_CMD_DDONE_INTR */
#if (WFDMA_AP_MSI_NUM == 1)
		WF_P0_WFDMA_HOST_INT_ENA_HOST_TX_DONE_INT_ENA16_MASK |
#endif
		WF_P0_WFDMA_HOST_INT_ENA_MCU2HOST_SW_INT_ENA_MASK;

	HAL_MCR_WR(prGlueInfo->prAdapter, u4Addr, u4WrVal);
}

#if CFG_MTK_WIFI_WFDMA_WB
static u_int8_t mt7999IsWfdmaRxReady(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prHwDoneFlagBuf, *prSwDoneFlagBuf;
	struct WFDMA_EMI_DONE_FLAG *prHwDoneFlag, *prSwDoneFlag, rIntFlag = {0};

	prChipInfo = prAdapter->chip_info;
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	if (!prChipInfo->is_support_wfdma_write_back)
		return TRUE;

	prHwDoneFlagBuf = &prHifInfo->rHwDoneFlag;
	prSwDoneFlagBuf = &prHifInfo->rSwDoneFlag;
	prHwDoneFlag = (struct WFDMA_EMI_DONE_FLAG *)prHwDoneFlagBuf->AllocVa;
	prSwDoneFlag = (struct WFDMA_EMI_DONE_FLAG *)prSwDoneFlagBuf->AllocVa;

	rIntFlag.rx_int0 = prHwDoneFlag->rx_int0 ^ prSwDoneFlag->rx_int0;
	rIntFlag.sw_int = prHwDoneFlag->sw_int ^ prSwDoneFlag->sw_int;

	if (rIntFlag.rx_int0 | rIntFlag.sw_int)
		return TRUE;

	return FALSE;
}

static void mt7999ReadIntStatusByEmi(struct ADAPTER *prAdapter,
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

static void mt7999ProcessTxInterruptByEmi(struct ADAPTER *prAdapter)
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

static void mt7999ProcessRxInterruptByEmi(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Sta = prHifInfo->rIntFlag.rx_int0;

	if ((u4Sta & BIT(6)) ||
	    (KAL_TEST_BIT(RX_RING_EVT, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_EVT, FALSE);

	if ((u4Sta & BIT(7)) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE0, FALSE);

	if ((u4Sta & BIT(8)) ||
	    (KAL_TEST_BIT(RX_RING_TXDONE1, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_TXDONE1, FALSE);

	if ((u4Sta & BIT(4)) ||
	    (KAL_TEST_BIT(RX_RING_DATA0, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_DATA0, TRUE);

	if ((u4Sta & BIT(5)) ||
	    (KAL_TEST_BIT(RX_RING_DATA1, prAdapter->ulNoMoreRfb)))
		halRxReceiveRFBs(prAdapter, RX_RING_DATA1, TRUE);
}

static void mt7999ProcessSoftwareInterruptByEmi(struct ADAPTER *prAdapter)
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

	DBGLOG(HAL, TRACE, "sw_int[0x%x]hwdone[0x%x]swdone[0x%x]\n",
	       u4Sta, prHwDoneFlag->sw_int, prSwDoneFlag->sw_int);

	prSwDoneFlag->sw_int = prHwDoneFlag->sw_int;
	prErrRecoveryCtrl->u4BackupStatus = u4Sta;
	if (u4Sta & ERROR_DETECT_SUBSYS_BUS_TIMEOUT) {
		DBGLOG(INIT, ERROR, "[SER][L0.5] wfsys timeout!!\n");
		GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_SUBSYS_BUS_TIMEOUT);
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

	u4Addr = WF_P0_WFDMA_MCU2HOST_SW_INT_STA_ADDR;
	HAL_MCR_WR(prAdapter, u4Addr, u4Sta);
}

static void mt7999WfdmaConfigWriteBack(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_DMABUF *prRingDmyRd, *prRingDmyWr, *prRingDidx;
	struct RTMP_DMABUF *prHwDoneFlag, *prSwDoneFlag;
	struct WFDMA_EMI_DONE_FLAG *prSwDone;
	uint32_t u4Addr = 0, u4WrVal = 0;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;

	prRingDmyWr = &prHifInfo->rRingDmyWr;
	prRingDmyRd = &prHifInfo->rRingDmyRd;
	prRingDidx = &prHifInfo->rRingDidx;
	prHwDoneFlag = &prHifInfo->rHwDoneFlag;
	prSwDoneFlag = &prHifInfo->rSwDoneFlag;
	prSwDone = (struct WFDMA_EMI_DONE_FLAG *)prSwDoneFlag->AllocVa;

	/* set err_int enable */
	u4Addr = WF_P0_WFDMA_WPDMA2HOST_ERR_INT_ENA_ADDR;
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set sw_int enable */
	u4Addr = WF_P0_WFDMA_MCU2HOST_SW_INT_ENA_ADDR;
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set subsys_int enable */
	u4Addr = WF_P0_WFDMA_SUBSYS2HOST_INT_ENA_ADDR;
	u4WrVal = 0x200;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set trinfo wb enable */
	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_CTRL_0_ADDR;
	u4WrVal = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_CTRL_0_AP_ONLY_MASK |
		WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_CTRL_0_EN_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set periodic int */
	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_CTRL_1_ADDR;
	u4WrVal = (50 <<
	WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_CTRL_1_AP_PER_RD_TIME_SHFT) &
	WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_CTRL_1_AP_PER_RD_TIME_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set dmy AXI */
	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DMY_CTRL_ADDR;
	u4WrVal = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DMY_CTRL_DMY_AXI_EN_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set dmy AXI write address */
	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DMY_WR_ADDR_31_00_ADDR;
	u4WrVal = ((uint64_t)prRingDmyRd->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DMY_WR_ADDR_63_32_ADDR;
	u4WrVal = ((uint64_t)prRingDmyRd->AllocPa) >> DMA_BITS_OFFSET;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set dmy AXI read address */
	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DMY_RD_ADDR_31_00_ADDR;
	u4WrVal = ((uint64_t)prRingDmyRd->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DMY_RD_ADDR_63_32_ADDR;
	u4WrVal = ((uint64_t)prRingDmyRd->AllocPa) >> DMA_BITS_OFFSET;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set HW_DONE_FLAG_WB_BASE_PTR */
	u4Addr = WF_P0_WFDMA_TRINFO_TOP_AP_HW_DONE_FLAG_ADDR_31_00_ADDR;
	u4WrVal = ((uint64_t)prHwDoneFlag->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_AP_HW_DONE_FLAG_ADDR_63_32_ADDR;
	u4WrVal = ((uint64_t)prHwDoneFlag->AllocPa) >> DMA_BITS_OFFSET;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set SW_DONE_FLAG_BASE_PTR */
	u4Addr = WF_P0_WFDMA_TRINFO_TOP_AP_SW_DONE_FLAG_ADDR_31_00_ADDR;
	u4WrVal = ((uint64_t)prSwDoneFlag->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_AP_SW_DONE_FLAG_ADDR_63_32_ADDR;
	u4WrVal = ((uint64_t)prSwDoneFlag->AllocPa) >> DMA_BITS_OFFSET;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	kalMemZero(prSwDone, sizeof(struct WFDMA_EMI_DONE_FLAG));

	/* set DIDX_WB_BASE_PTR */
	u4Addr = WF_P0_WFDMA_TRINFO_TOP_AP_TRX_DIDX_ADDR_31_00_ADDR;
	u4WrVal = ((uint64_t)prRingDidx->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_AP_TRX_DIDX_ADDR_63_32_ADDR;
	u4WrVal = ((uint64_t)prRingDidx->AllocPa) >> DMA_BITS_OFFSET;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DLY_INT_CTRL_AP_ADDR;
	u4WrVal =
		(32 <<
		WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DLY_INT_CTRL_AP_DLY_CNT_SHFT) |
		(20 <<
		WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DLY_INT_CTRL_AP_DLY_TIME_SHFT);
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_PER_INT_CTRL_AP_ADDR;
	u4WrVal = 100 <<
		WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_PER_INT_CTRL_AP_PER_TIME_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

#if (CFG_SUPPORT_DISABLE_TX_DDONE_INTR == 1)
	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_INT_TX_EN_31_00_ADDR;
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_INT_TX_EN_63_32_ADDR;
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DLY_INT_TX_EN_31_00_SET_ADDR;
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DLY_INT_TX_EN_63_32_SET_ADDR;
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_PER_INT_TX_EN_31_00_SET_ADDR
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_PER_INT_TX_EN_63_32_SET_ADDR
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
#endif /* CFG_SUPPORT_DISABLE_TX_DDONE_INTR == 1 */

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_INT_RX_EN_31_00_ADDR;
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_DLY_INT_RX_EN_31_00_SET_ADDR;
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_TRINFO_WB_PER_INT_RX_EN_31_00_SET_ADDR;
	u4WrVal = 0xffffffff;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
}

static void mt7999WfdmaConfigCidxFetch(struct GLUE_INFO *prGlueInfo)
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
	for (u4Addr = WF_P0_WFDMA_TRINFO_TOP_RX_CFET_TH_0001_ADDR;
	     u4Addr <= WF_P0_WFDMA_TRINFO_TOP_RX_CFET_TH_0607_ADDR;
	     u4Addr += 4) {
		u4WrVal = u4RxPps;
		u4WrVal |= u4RxPps <<
			WF_P0_WFDMA_TRINFO_TOP_RX_CFET_TH_0001_TH_01_SHFT;
		HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
	}

	/* enable pcie txp first qos priority */
	u4Addr = WF_P0_WFDMA_WPDMA_TX_QOS_QTM_CFG1_ADDR;
	u4WrVal =
	WF_P0_WFDMA_WPDMA_TX_QOS_QTM_CFG1_CSR_QOS_DYNAMIC_SET_MASK |
	WF_P0_WFDMA_WPDMA_TX_QOS_QTM_CFG1_CSR_QOS_PRI_SEL_MASK |
	(3 << WF_P0_WFDMA_WPDMA_TX_QOS_QTM_CFG1_CSR_TXD_RSVD_QTM_SHFT) |
	(13 << WF_P0_WFDMA_WPDMA_TX_QOS_QTM_CFG1_CSR_TXD_FFA_QTM_SHFT) |
	(1 << WF_P0_WFDMA_WPDMA_TX_QOS_QTM_CFG1_CSR_DMAD_RSVD_QTM_SHFT) |
	(7 << WF_P0_WFDMA_WPDMA_TX_QOS_QTM_CFG1_CSR_DMAD_FFA_QTM_SHFT);
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set tx periodic fetch mode */
	u4Addr = WF_P0_WFDMA_WPDMA_TX_QOS_FSM_ADDR;
	u4WrVal = 0x32; /* 50 * 20us */
	u4WrVal |= WF_P0_WFDMA_WPDMA_TX_QOS_FSM_CSR_PCIE_LP_QOS_EN_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* set SW_DONE_FLAG_BASE_PTR */
	u4Addr = WF_P0_WFDMA_TRINFO_TOP_AP_TRX_CIDX_ADDR_31_00_ADDR;
	u4WrVal = ((uint64_t)prRingCidx->AllocPa) & DMA_LOWER_32BITS_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	u4Addr = WF_P0_WFDMA_TRINFO_TOP_AP_TRX_CIDX_ADDR_63_32_ADDR;
	u4WrVal = ((uint64_t)prRingCidx->AllocPa) >> DMA_BITS_OFFSET;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	/* enable TRX CIDX fetch */
	u4Addr = WF_P0_WFDMA_TRINFO_TOP_CIDX_FET_CTRL_ADDR;
	u4WrVal = WF_P0_WFDMA_TRINFO_TOP_CIDX_FET_CTRL_RX_EN_MASK |
		WF_P0_WFDMA_TRINFO_TOP_CIDX_FET_CTRL_TX_EN_MASK |
		WF_P0_WFDMA_TRINFO_TOP_CIDX_FET_CTRL_AP_EN_MASK;
	u4WrVal |= 0x5 <<
		WF_P0_WFDMA_TRINFO_TOP_CIDX_FET_CTRL_DLY_TIME_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);
}

static void mt7999ConfigEmiIntMask(struct GLUE_INFO *prGlueInfo,
				   u_int8_t enable)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	uint32_t u4Addr = 0, u4WrVal = 0;

	if (!prChipInfo->is_support_wfdma_write_back ||
	    !prChipInfo->is_enable_wfdma_write_back)
		return;

	/* Clr HOST interrupt enable */
	u4Addr = WF_P0_WFDMA_HOST_INT_ENA_ADDR;
	u4WrVal = 0;
	HAL_MCR_WR(prAdapter, u4Addr, u4WrVal);

	mt7999WfdmaConfigWriteBack(prGlueInfo);
	if (prChipInfo->is_support_wfdma_cidx_fetch)
		mt7999WfdmaConfigCidxFetch(prGlueInfo);
}

static void mt7999TriggerWfdmaCidxFetch(struct GLUE_INFO *prGlueInfo)
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

static u_int8_t mt7999CheckWfdmaCidxFetchTimeout(struct GLUE_INFO *prGlueInfo)
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

static void mt7999RunWfdmaCidxFetch(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct HIF_STATS *prHifStats = &prAdapter->rHifStats;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (prHifInfo->fgIsNeedCidxFetchFlag) {
		GLUE_INC_REF_CNT(prHifStats->u4CidxFetchByNewTx);
		if (IS_FEATURE_ENABLED(prWifiVar->fgWfdmaCidxFetchDbg))
			DBGLOG(HAL, DEBUG, "Trigger cidx fetch by new tx");
		goto fetch;
	}

	if (prHifInfo->fgIsCidxFetchNewTx &&
	    mt7999CheckWfdmaCidxFetchTimeout(prGlueInfo)) {
		GLUE_INC_REF_CNT(prHifStats->u4CidxFetchByTimeout);
		if (IS_FEATURE_ENABLED(prWifiVar->fgWfdmaCidxFetchDbg))
			DBGLOG(HAL, DEBUG, "Trigger cidx fetch by timeout");
		goto fetch;
	}

	return;
fetch:
	prHifInfo->fgIsNeedCidxFetchFlag = FALSE;
	mt7999TriggerWfdmaCidxFetch(prGlueInfo);
}

static void mt7999TriggerWfdmaTxCidx(struct GLUE_INFO *prGlueInfo,
				     struct RTMP_TX_RING *prTxRing)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct HIF_STATS *prHifStats = &prAdapter->rHifStats;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t u4CurDidx = 0;

	if (!prChipInfo->is_support_wfdma_cidx_fetch)
		return;

	if (!halIsDataRing(TX_RING, prTxRing->u4RingIdx)) {
		mt7999TriggerWfdmaCidxFetch(prGlueInfo);
		GLUE_INC_REF_CNT(prHifStats->u4CidxFetchByCmd);
		if (IS_FEATURE_ENABLED(prWifiVar->fgWfdmaCidxFetchDbg))
			DBGLOG(HAL, DEBUG, "Trigger cidx fetch by cmd");
		goto exit;
	}

	if (prHifInfo->fgIsUrgentCidxFetch) {
		prHifInfo->fgIsUrgentCidxFetch = FALSE;
		prHifInfo->fgIsNeedCidxFetchFlag = TRUE;
	}

	HAL_GET_RING_DIDX(HIF_RING, prAdapter, prTxRing, &u4CurDidx);
	if ((u4CurDidx == prTxRing->u4LastDidx) &&
	    (u4CurDidx == prTxRing->u4LastCidx))
		prHifInfo->fgIsNeedCidxFetchFlag = TRUE;

	prHifInfo->fgIsCidxFetchNewTx = TRUE;

exit:
	if (IS_FEATURE_ENABLED(prWifiVar->fgWfdmaCidxFetchDbg)) {
		DBGLOG(HAL, DEBUG,
		       "Ring[%u]: old cidx[%u]didx[%u]",
		       prTxRing->u4RingIdx,
		       prTxRing->u4LastCidx,
		       prTxRing->u4LastDidx);
	}
}

static void mt7999TriggerWfdmaRxCidx(struct GLUE_INFO *prGlueInfo,
				     struct RTMP_RX_RING *prRxRing)
{
}

static void mt7999EnableWfdmaWb(struct GLUE_INFO *prGlueInfo)
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
		prTxRing->triggerCidx = mt7999TriggerWfdmaTxCidx;
	}

	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		prRxRing = &prHifInfo->RxRing[u4Idx];
		if (!prRxRing->pu2EmiCidx)
			continue;

		prRxRing->fgEnEmiCidx = TRUE;
		*prRxRing->pu2EmiCidx = prRxRing->RxCpuIdx;
		prRxRing->triggerCidx = mt7999TriggerWfdmaRxCidx;
	}

enable:
	prChipInfo->is_enable_wfdma_write_back = TRUE;
	if (prBusInfo->pdmaSetup)
		prBusInfo->pdmaSetup(prGlueInfo, TRUE, FALSE);
}
#endif /* CFG_MTK_WIFI_WFDMA_WB */

#if CFG_MTK_WIFI_WFDMA_WB
static void mt7999EnableInterruptViaPcieByEmi(struct ADAPTER *prAdapter)
{
	asicConnac5xEnablePlatformIRQ(prAdapter);
}
#endif /* CFG_MTK_WIFI_WFDMA_WB */

#if defined(_HIF_PCIE) && (CFG_SUPPORT_PCIE_PLAT_INT_FLOW == 1)
static void mt7999EnableInterruptViaPcie(struct ADAPTER *prAdapter)
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

	asicConnac5xEnablePlatformIRQ(prAdapter);
}
#endif

static void mt7999EnableInterrupt(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->configWfdmaIntMask)
		prBusInfo->configWfdmaIntMask(prAdapter->prGlueInfo, TRUE);
	asicConnac5xEnablePlatformIRQ(prAdapter);
}

static void mt7999DisableInterrupt(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->configWfdmaIntMask)
		prBusInfo->configWfdmaIntMask(prAdapter->prGlueInfo, FALSE);
	asicConnac5xDisablePlatformIRQ(prAdapter);
}

static void mt7999WpdmaMsiConfig(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct pcie_msi_info *prMsiInfo = NULL;
	uint32_t u4Val = 0;

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	if (!prMsiInfo->fgMsiEnabled)
		return;

	/* MSI only need to read int status once */
	prAdapter->rWifiVar.u4HifIstLoopCount = 1;

#if (WFDMA_AP_MSI_NUM == 8)
	/* Enable MSI 0-6 auto clear */
	u4Val = 0x7F;
	HAL_MCR_WR(prAdapter, WFDMA_WRAP_CSR_WFDMA_MSI_CONFIG0_ADDR, u4Val);

	u4Val = WFDMA_WRAP_CSR_WFDMA_MSI_CONFIG1_MSI_DEASSERT_TMR_EN_MASK |
		(0x40 <<
		 WFDMA_WRAP_CSR_WFDMA_MSI_CONFIG1_MSI_DEASSERT_TMR_TICKS_SHFT);
	HAL_MCR_WR(prAdapter, WFDMA_WRAP_CSR_WFDMA_MSI_CONFIG1_ADDR, u4Val);

	u4Val = 0x44461312;
	HAL_MCR_WR(prAdapter, WFDMA_WRAP_CSR_WFDMA_P0_MSI_CFG_0_ADDR, u4Val);
	u4Val = 0x55114045;
	HAL_MCR_WR(prAdapter, WFDMA_WRAP_CSR_WFDMA_P0_MSI_CFG_1_ADDR, u4Val);
#else
	/* Enable PCIE0 MSI 0 trinfo auto clear */
	u4Val = 1;
	HAL_MCR_WR(prAdapter, WFDMA_WRAP_CSR_WFDMA_MSI_CONFIG0_ADDR, u4Val);
#endif
}

static void mt7999ConfigWfdmaRxRingThreshold(
	struct ADAPTER *prAdapter, uint32_t u4Th, u_int8_t fgIsData)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	struct RTMP_RX_RING *prRxRing;
	uint32_t u4Addr, u4Val, u4Idx, u4RingIdx, u4SwRingIdx;
	uint32_t au4Vals[RX_RING_MAX] = {0};

	for (u4SwRingIdx = 0; u4SwRingIdx < NUM_OF_RX_RING; u4SwRingIdx++) {
		u4RingIdx = mt7999RxRingSwIdx2HwIdx(u4SwRingIdx);
		if (u4RingIdx >= RX_RING_MAX)
			continue;

		prRxRing = &prHifInfo->RxRing[u4SwRingIdx];
		au4Vals[u4RingIdx] = prRxRing->u4BufSize;
	}

	for (u4Idx = 4; u4Idx < 12; u4Idx += 2) {
		if (u4Idx >= RX_RING_MAX)
			break;

		u4Addr = WF_P0_WFDMA_WPDMA_RX_DMAD_SDL_45_ADDR +
			((u4Idx - 4) << 1);
		u4Val = au4Vals[u4Idx];
		if ((u4Idx + 1) < RX_RING_MAX)
			u4Val |= au4Vals[u4Idx + 1] << 16;
		HAL_MCR_WR(prAdapter, u4Addr, u4Val);
	}
}

static void mt7999WpdmaDlyInt(struct GLUE_INFO *prGlueInfo)
{
#if CFG_SUPPORT_WFDMA_RX_DELAY_INT
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t u4Addr, u4Val;

	/* setup ring 4-8 delay interrupt */
	u4Addr = WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_ADDR;
	u4Val = (4 <<
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_RING_IDX0_SHFT) |
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_DIR0_MASK |
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_ENA0_MASK |
		(5 <<
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_RING_IDX1_SHFT) |
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_DIR1_MASK |
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_ENA1_MASK |
		(6 <<
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_RING_IDX2_SHFT) |
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_DIR2_MASK |
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_ENA2_MASK |
		(7 <<
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_RING_IDX3_SHFT) |
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_DIR3_MASK |
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_0_DLY_FUNC_ENA3_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_1_ADDR;
	u4Val = (8 <<
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_1_DLY_RING_IDX4_SHFT) |
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_1_DLY_FUNC_DIR4_MASK |
		WFDMA_WRAP_CSR_WFDMA_DLY_IDX_CFG_1_DLY_FUNC_ENA4_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG0_ADDR;
	u4Val = prWifiVar->u4DlyIntTime <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG0_PRI0_MAX_PTIME_SHFT |
		prWifiVar->u4DlyIntCnt <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG0_PRI0_MAX_PINT_SHFT |
		prWifiVar->fgEnDlyInt <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG0_PRI0_DLY_INT_EN_SHFT |
		prWifiVar->u4DlyIntTime <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG0_PRI1_MAX_PTIME_SHFT |
		prWifiVar->u4DlyIntCnt <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG0_PRI1_MAX_PINT_SHFT |
		prWifiVar->fgEnDlyInt <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG0_PRI1_DLY_INT_EN_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG1_ADDR;
	u4Val = prWifiVar->u4DlyIntTime <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG1_PRI0_MAX_PTIME_SHFT |
		prWifiVar->u4DlyIntCnt <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG1_PRI0_MAX_PINT_SHFT |
		prWifiVar->fgEnDlyInt <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG1_PRI0_DLY_INT_EN_SHFT |
		prWifiVar->u4DlyIntTime <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG1_PRI1_MAX_PTIME_SHFT |
		prWifiVar->u4DlyIntCnt <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG1_PRI1_MAX_PINT_SHFT |
		prWifiVar->fgEnDlyInt <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG1_PRI1_DLY_INT_EN_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG2_ADDR;
	u4Val = prWifiVar->u4DlyIntTime <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG2_PRI0_MAX_PTIME_SHFT |
		prWifiVar->u4DlyIntCnt <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG2_PRI0_MAX_PINT_SHFT |
		prWifiVar->fgEnDlyInt <<
		WF_P0_WFDMA_WPDMA_PRI_DLY_INT_CFG2_PRI0_DLY_INT_EN_SHFT;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	DBGLOG(HAL, DEBUG, "dly int[%u]: %uus, cnt=%u",
	       prWifiVar->fgEnDlyInt,
	       prWifiVar->u4DlyIntTime * 20,
	       prWifiVar->u4DlyIntCnt);
#endif /* CFG_SUPPORT_WFDMA_RX_DELAY_INT */
}

static void mt7999WfdmaControl(struct ADAPTER *prAdapter, u_int8_t fgEn)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	union WPDMA_GLO_CFG_STRUCT *prGloCfg = &prHifInfo->GloCfg;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	struct BUS_INFO *prBusInfo = prChipInfo->bus_info;
	uint32_t u4Addr = 0, u4Val = 0;

	u4Addr = WFDMA_WRAP_CSR_WFDMA_HOST_CONFIG_ADDR;
	if (prBusInfo->u4DmaMask > 32)
		u4Val |=
		WFDMA_WRAP_CSR_WFDMA_HOST_CONFIG_TXD_FORMAT_EXT_36BIT_MASK;
	else if (prBusInfo->u4DmaMask > 36)
		u4Val |=
		WFDMA_WRAP_CSR_WFDMA_HOST_CONFIG_TXD_FORMAT_EXT_48BIT_MASK;
	HAL_MCR_WR(prAdapter, u4Addr, u4Val);

	u4Addr = WF_P0_WFDMA_WPDMA_GLO_CFG_ADDR;
	prGloCfg->word =
		(0x8 << WF_P0_WFDMA_WPDMA_GLO_CFG_CSR_WR_OUTSTAND_NUM_SHFT) |
		(0x50 << WF_P0_WFDMA_WPDMA_GLO_CFG_CSR_RD_OUTSTAND_NUM_SHFT) |
		WF_P0_WFDMA_WPDMA_GLO_CFG_CSR_TX_DMASHDL_EN_MASK |
		WF_P0_WFDMA_WPDMA_GLO_CFG_CSR_TX_WB_DDONE_MASK |
		(1 << WF_P0_WFDMA_WPDMA_GLO_CFG_CSR_BURST_SIZE_SHFT);
	if (fgEn) {
		prGloCfg->word |=
			WF_P0_WFDMA_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
			WF_P0_WFDMA_WPDMA_GLO_CFG_RX_DMA_EN_MASK;
	}
	HAL_MCR_WR(prAdapter, u4Addr, prGloCfg->word);
	prHifInfo->GloCfg.word = prGloCfg->word;
}

static void mt7999WpdmaConfig(struct GLUE_INFO *prGlueInfo,
		u_int8_t enable, bool fgResetHif)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

	mt7999WfdmaControl(prAdapter, enable);

	if (!enable)
		return;

#if CFG_MTK_WIFI_WFDMA_WB
	if (prAdapter->chip_info->is_support_wfdma_write_back)
		mt7999ConfigEmiIntMask(prGlueInfo, TRUE);
#endif /* CFG_MTK_WIFI_WFDMA_WB */

#if defined(_HIF_PCIE)
	mt7999WpdmaMsiConfig(prAdapter);
#endif
	mt7999ConfigWfdmaRxRingThreshold(prAdapter, 0, FALSE);

	mt7999WpdmaDlyInt(prGlueInfo);
}

#if CFG_MTK_WIFI_WFDMA_WB
static void mt7999WfdmaTxRingWbExtCtrl(
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
	*prTxRing->pu2EmiDidx = 0;

	if (!prChipInfo->is_support_wfdma_cidx_fetch)
		return;

	prTxRing->pu2EmiCidx = &prRingCidx->tx_ring[u4Idx];
	*prTxRing->pu2EmiCidx = 0;
}

static void mt7999WfdmaRxRingWbExtCtrl(
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

static void mt7999WfdmaTxRingExtCtrl(
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
		   CONNAC5X_TX_RING_DISP_MAX_CNT);

	asicConnac5xWfdmaTxRingBasePtrExtCtrl(
		prGlueInfo, prTxRing, index, prTxRing->u4RingSize);

#if CFG_MTK_WIFI_WFDMA_WB
	mt7999WfdmaTxRingWbExtCtrl(prGlueInfo, prTxRing, u4RingIdx);
#endif /* CFG_MTK_WIFI_WFDMA_WB */
}

static void mt7999WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *prRxRing,
	u_int32_t index)
{
	struct ADAPTER *prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	uint32_t u4Offset = 0, u4RingIdx = 0, u4Val = 0;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	u4RingIdx = mt7999RxRingSwIdx2HwIdx(index);
	if (u4RingIdx >= RX_RING_MAX)
		return;

	u4Offset = u4RingIdx * 4;

	prRxRing->hw_desc_base_ext =
		prBusInfo->host_rx_ring_ext_ctrl_base + u4Offset;
	HAL_MCR_WR(prAdapter, prRxRing->hw_desc_base_ext,
		   CONNAC5X_RX_RING_DISP_MAX_CNT);

	u4Val = prRxRing->u4RingSize;

	asicConnac5xWfdmaRxRingBasePtrExtCtrl(
		prGlueInfo, prRxRing, index, u4Val);

#if CFG_MTK_WIFI_WFDMA_WB
	mt7999WfdmaRxRingWbExtCtrl(prGlueInfo, prRxRing, u4RingIdx);
#endif /* CFG_MTK_WIFI_WFDMA_WB */
}

#if defined(_HIF_PCIE)
static void mt7999RecoveryMsiStatus(struct ADAPTER *prAdapter, u_int8_t fgForce)
{
	struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct pcie_msi_info *prMsiInfo = &prBusInfo->pcie_msi_info;
	uint32_t u4Val = 0, u4Cnt = 0, u4RxCnt, i;
	u_int8_t fgRet = FALSE;
#if (WFDMA_AP_MSI_NUM == 1)
	uint32_t u4IntMask = BIT(0);
#else
	uint32_t u4IntMask = BITS(0, 7);
#endif

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
}

static void mt7999RecoverSerStatus(struct ADAPTER *prAdapter)
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

static void mt7999CheckFwOwnMsiStatus(struct ADAPTER *prAdapter)
{
	mt7999RecoveryMsiStatus(prAdapter, FALSE);
}

#if (CFG_MTK_WIFI_PCIE_MSI_MASK_BY_MMIO_WRITE == 1)
static void mt7999PcieMsiMaskIrq(uint32_t u4Irq, uint32_t u4Bit)
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

static void mt7999PcieMsiUnmaskIrq(uint32_t u4Irq, uint32_t u4Bit)
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

static void mt7999InitPcieInt(struct GLUE_INFO *prGlueInfo)
{
#if CFG_SUPPORT_PCIE_ASPM_EP
	uint32_t u4WrVal = 0x08021000;
#if CFG_SUPPORT_PCIE_ASPM
	uint32_t u4Val = 0;
#endif /* CFG_SUPPORT_PCIE_ASPM */

#if CFG_SUPPORT_PCIE_ASPM
	if (pcie_vir_addr == NULL) {
#if CFG_PCIE_MT6989
		pcie_vir_addr = ioremap(0x112f0000, 0x2000);
#else
		pcie_vir_addr = ioremap(0x16910000, 0x2000);
#endif
		spin_lock_init(&rPCIELock);
	}
#endif /* CFG_SUPPORT_PCIE_ASPM */

	HAL_MCR_WR(prGlueInfo->prAdapter, 0x74030074, u4WrVal);

#if CFG_SUPPORT_PCIE_ASPM
	if (!pcie_vir_addr) {
		DBGLOG(HAL, DEBUG, "pcie_vir_addr is null\n");
		return;
	}

	writel(u4WrVal, (pcie_vir_addr + 0x74));
	u4Val = readl(pcie_vir_addr + 0x74);
	DBGLOG(HAL, DEBUG,
	       "pcie_addr=0x%llx, write 0x74=[0x%08x], read 0x74=[0x%08x]\n",
	       (uint64_t)pcie_vir_addr, u4WrVal, u4Val);
#endif /* CFG_SUPPORT_PCIE_ASPM */
#endif /* CFG_SUPPORT_PCIE_ASPM_EP */
}

static void mt7999PcieHwControlVote(
	struct ADAPTER *prAdapter,
	uint8_t enable,
	uint32_t u4WifiUser)
{
	halPcieHwControlVote(prAdapter, enable, u4WifiUser);
}

#if CFG_SUPPORT_PCIE_ASPM
static u_int8_t mt7999SetL1ssEnable(struct ADAPTER *prAdapter,
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

static uint32_t mt7999ConfigPcieAspm(struct GLUE_INFO *prGlueInfo,
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
		mt7999SetL1ssEnable(prGlueInfo->prAdapter, enable_role, fgEn);

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
				DBGLOG(HAL, DEBUG,
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
					DBGLOG(HAL, DEBUG,
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
			DBGLOG(HAL, DEBUG,
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
				DBGLOG(HAL, DEBUG,
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

static void mt7999UpdatePcieAspm(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Status;

	if (fgEn) {
		prHifInfo->eNextPcieState = PCIE_STATE_L1_2;
	} else {
		if (prHifInfo->eNextPcieState != PCIE_STATE_L0)
			prHifInfo->eNextPcieState = PCIE_STATE_L1;
	}

	if (prHifInfo->eCurPcieState != prHifInfo->eNextPcieState) {
		if (prHifInfo->eNextPcieState == PCIE_STATE_L1_2)
			u4Status = mt7999ConfigPcieAspm(prGlueInfo,
							TRUE, 1);
		else
			u4Status = mt7999ConfigPcieAspm(prGlueInfo,
							FALSE, 1);

		if (u4Status == WLAN_STATUS_SUCCESS)
			prHifInfo->eCurPcieState = prHifInfo->eNextPcieState;
	}
}

static void mt7999KeepPcieWakeup(struct GLUE_INFO *prGlueInfo,
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

#if CFG_SUPPORT_PCIE_ASPM
static u_int8_t mt7999DumpPcieDateFlowStatus(struct GLUE_INFO *prGlueInfo)
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
		if (u4RegVal[0] == 0 || u4RegVal[0] == 0xffffffff) {
			DBGLOG(HAL, DEBUG,
				"PCIE link down 0x0=0x%08x\n", u4RegVal[0]);
			/* block pcie to prevent access */
#if CFG_MTK_WIFI_PCIE_SUPPORT
			mtk_pcie_disable_data_trans(0);
#endif
			return FALSE;
		}

		/*1. read pcie cfg.space 0x488 // Readable check*/
		pci_read_config_dword(pci_dev, 0x488, &u4RegVal[1]);
		if ((u4RegVal[1] & 0x3811) != 0x3811 ||
			u4RegVal[1] == 0xffffffff) {
			pci_read_config_dword(pci_dev, 0x48C, &u4RegVal[2]);
			DBGLOG(HAL, DEBUG,
				"Cb_infra bus fatal error and un-readble 0x488=0x%08x 0x48C=0x%08x\n",
				u4RegVal[1], u4RegVal[2]);
			return FALSE;
		}
	}

#if CFG_MTK_WIFI_PCIE_SUPPORT
	/* MalfTLP */
	if (link_info & BIT(8)) {
		fgIsBusAccessFailed = TRUE;
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
		fgTriggerDebugSop = TRUE;
#endif
		return FALSE;
	}
#endif

	return TRUE;
}
#endif /* CFG_SUPPORT_PCIE_ASPM */
static void mt7999SetupMcuEmiAddr(struct ADAPTER *prAdapter)
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

	DBGLOG(HAL, DEBUG, "base: 0x%llx, size: 0x%x\n", base, size);

	HAL_MCR_WR(prAdapter,
		   CONNAC5X_CONN_CFG_ON_CONN_ON_EMI_ADDR,
		   ((uint32_t)(base >> 16)));

	HAL_MCR_WR(prAdapter,
		   MT7999_EMI_SIZE_ADDR,
		   size);

	/* Get EMI2 Info */
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	size = 0;
	if (prMemOps->getWifiMiscRsvEmi &&
		prAdapter->chip_info->rsvMemWiFiMisc) {
		prMem = prMemOps->getWifiMiscRsvEmi(prAdapter->chip_info, 0);
		if (prMem == NULL) {
			DBGLOG(NIC, DEBUG, "not support EMI2\n");
			return;
		}
		base = prMem->pa;

		for (uIdx = 0; uIdx <
			prAdapter->chip_info->rsvMemWiFiMiscSize; uIdx++)
			size += prAdapter->chip_info->rsvMemWiFiMisc[uIdx].size;
	}
	DBGLOG(HAL, DEBUG, "emi2 base: 0x%llx, size: 0x%x\n", base, size);

	HAL_MCR_WR(prAdapter, SETUP_MCU_EMI2_BASE_ADDRESS,
		   ((uint32_t)(base >> 16)));
	HAL_MCR_WR(prAdapter, SETUP_MCU_EMI2_SIZE_ADDRESS,
		   size);
#endif
}

static void mt7999_clear_sw_interrupt_status(struct ADAPTER *prAdapter)
{
	ccif_clear_interrupt_status(prAdapter);
}

static u_int8_t mt7999_get_sw_interrupt_status(struct ADAPTER *prAdapter,
	uint32_t *pu4Status)
{
	*pu4Status = ccif_get_interrupt_status(prAdapter);
	return TRUE;
}

static void mt7999_ccif_clear_interrupt_status(struct ADAPTER *ad)
{
	HAL_MCR_WR(ad,
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_ACK_ADDR,
		0xff);
}

static uint32_t mt7999_ccif_get_interrupt_status(struct ADAPTER *ad)
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

static void mt7999_ccif_notify_utc_time_to_fw(struct ADAPTER *ad,
	uint32_t sec,
	uint32_t usec)
{
	ACQUIRE_POWER_CONTROL_FROM_PM(ad,
		DRV_OWN_SRC_CCIF_NOTIFY_UTC_TIME_TO_FW);
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
	RECLAIM_POWER_CONTROL_TO_PM(ad, FALSE,
		DRV_OWN_SRC_CCIF_NOTIFY_UTC_TIME_TO_FW);
}

static void mt7999_ccif_set_fw_log_read_pointer(struct ADAPTER *ad,
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

static uint32_t mt7999_ccif_get_fw_log_read_pointer(struct ADAPTER *ad,
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

static void mt7999_force_conn_infra_on(struct ADAPTER *ad,
	u_int8_t fgForceOn)
{
	uint32_t u4WriteValue = (fgForceOn == TRUE) ? 1 : 0;

	HAL_MCR_WR(ad,
		CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_WF_ADDR,
		u4WriteValue);
}

static int32_t mt7999_ccif_trigger_fw_assert(struct ADAPTER *ad)
{
	mt7999_force_conn_infra_on(ad, TRUE);
	mdelay(5);

	HAL_MCR_WR(ad,
		AP2WF_CONN_INFRA_ON_CCIF4_AP2WF_PCCIF_TCHNUM_ADDR,
		SW_INT_SUBSYS_RESET);

	return 0;
}

u_int8_t mt7999_is_ap2conn_off_readable(struct ADAPTER *ad)
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
	if (value != MT7999_CONNINFRA_VERSION_ID) {
		DBGLOG(HAL, ERROR,
			"Conninfra ver id: 0x%08x\n",
			value);
		return FALSE;
	}

	HAL_RMCR_RD(PLAT_DBG, ad,
		   CONN_DBG_CTL_CONN_INFRA_BUS_DBG_CR_00_ADDR,
		   &value);
	if ((value & BITS(0, 9)) != 0)
		DBGLOG(HAL, ERROR,
			"Conninfra bus hang irq status: 0x%08x\n",
			value);

	return TRUE;
}

u_int8_t mt7999_is_conn2wf_readable(struct ADAPTER *ad)
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
		   WF_TOP_MCU_CFG_IP_VERSION_ADDR,
		   &value);
	if (value != MT7999_WF_VERSION_ID) {
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
		return FALSE;
	}

	return TRUE;
}

static u_int8_t mt7999_check_recovery_needed(struct ADAPTER *ad)
{
	uint32_t u4Value = 0;
	u_int8_t fgResult = FALSE;

	/*
	 * if (0x81021078[31:16]==0xdead &&
	 *     (0x70005350[30:28]!=0x0 || 0x70005360[6:4]!=0x0)) == 0x1
	 * do recovery flow
	 */

	HAL_RMCR_RD(ONOFF_READ, ad,
		    WF_TOP_CFG_VLP_WFSYS_MCU_ROMCODE_INDEX_SW_FLAG_ADDR,
		    &u4Value);
	DBGLOG(INIT, DEBUG, "0x%08x=0x%08x\n",
		WF_TOP_CFG_VLP_WFSYS_MCU_ROMCODE_INDEX_SW_FLAG_ADDR,
		u4Value);
	if ((u4Value & 0xFFFF0000) != 0xDEAD0000) {
		fgResult = FALSE;
		goto exit;
	}

	HAL_RMCR_RD(ONOFF_READ, ad, CBTOP_GPIO_MODE5_ADDR,
		&u4Value);
	DBGLOG(INIT, DEBUG, "0x%08x=0x%08x\n",
		CBTOP_GPIO_MODE5_ADDR, u4Value);
	if (((u4Value & CBTOP_GPIO_MODE5_GPIO47_MASK) >>
	    CBTOP_GPIO_MODE5_GPIO47_SHFT) != 0x0) {
		fgResult = TRUE;
		goto exit;
	}

	HAL_RMCR_RD(ONOFF_READ, ad, CBTOP_GPIO_MODE6_ADDR,
		&u4Value);
	DBGLOG(INIT, DEBUG, "0x%08x=0x%08x\n",
		CBTOP_GPIO_MODE6_ADDR, u4Value);
	if (((u4Value & CBTOP_GPIO_MODE6_GPIO49_MASK) >>
	    CBTOP_GPIO_MODE6_GPIO49_SHFT) != 0x0) {
		fgResult = TRUE;
		goto exit;
	}

exit:
	return fgResult;
}

static uint32_t mt7999_mcu_reinit(struct ADAPTER *ad)
{
#define CONNINFRA_ID_MAX_POLLING_COUNT		10

	uint32_t u4Value = 0, u4PollingCnt = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	/* Check recovery needed */
	if (mt7999_check_recovery_needed(ad) == FALSE)
		goto exit;

	TRACE_FUNC(INIT, DEBUG, "%s.\n");

	/* Force on conninfra */
	HAL_MCR_WR(ad,
		CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR,
		0x1);

	/* Wait conninfra wakeup */
	while (TRUE) {
		HAL_RMCR_RD(ONOFF_READ, ad,
			       CONN_CFG_IP_VERSION_IP_VERSION_ADDR,
			       &u4Value);

		if (u4Value == MT7999_CONNINFRA_VERSION_ID)
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
	DBGLOG(INIT, DEBUG, "0x%08x=0x%08x\n",
		CBTOP_GPIO_MODE5_ADDR, u4Value);

	HAL_RMCR_RD(ONOFF_READ, ad, CBTOP_GPIO_MODE6_ADDR, &u4Value);
	DBGLOG(INIT, DEBUG, "0x%08x=0x%08x\n",
		CBTOP_GPIO_MODE6_ADDR, u4Value);

	/* Clean force on conninfra */
	HAL_MCR_WR(ad,
		CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR,
		0x0);

exit:
	return rStatus;
}

#if (CFG_MTK_ANDROID_WMT == 0)
static uint32_t mt7999_mcu_reset(struct ADAPTER *ad)
{
	uint32_t u4Value = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	HAL_RMCR_RD(RESET_READ, ad,
		    CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		    &u4Value);
	u4Value |= CB_INFRA_RGU_WF_SUBSYS_RST_BYPASS_WFDMA_SLP_PROT_MASK;
	u4Value |= CB_INFRA_RGU_WF_SUBSYS_RST_BYPASS_WFDMA_2_SLP_PROT_MASK;
	u4Value |= CB_INFRA_RGU_WF_SUBSYS_RST_WF_WHOLE_PATH_RST_REVERT_EN_MASK;
	u4Value |= CB_INFRA_RGU_WF_SUBSYS_RST_WF_WHOLE_PATH_RST_MASK;
	HAL_MCR_WR(ad,
		   CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		   u4Value);

	kalMdelay(1);

	HAL_RMCR_RD(RESET_READ, ad,
		    CB_INFRA_RGU_WF_SUBSYS_RST_ADDR,
		    &u4Value);

	DBGLOG(INIT, DEBUG,
		"CB_INFRA_RGU_WF_SUBSYS_RST_ADDR u4Value=0x%08x\n",
		u4Value);

	return rStatus;
}
#endif

static uint32_t mt7999_mcu_check_idle(struct ADAPTER *ad)
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

		HAL_RMCR_RD(ONOFF_READ, ad,
			WF_TOP_CFG_VLP_WFSYS_MCU_ROMCODE_INDEX_SW_FLAG_ADDR,
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

static uint32_t mt7999_mcu_init(struct ADAPTER *ad)
{
#if (CFG_MTK_FPGA_PLATFORM != 1)
	uint32_t u4Value = 0;
#endif /* CFG_MTK_FPGA_PLATFORM */
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

#if (CFG_MTK_FPGA_PLATFORM != 1)
	HAL_MCR_WR(ad,
		CB_INFRA_SLP_CTRL_CB_INFRA_SLP_PROT_SW_CTRL_ADDR, 0x0);
#endif /* CFG_MTK_FPGA_PLATFORM */
#if (CFG_MTK_ANDROID_WMT == 0) && (CFG_MTK_FPGA_PLATFORM != 1)
	rStatus = mt7999_mcu_reset(ad);
	if (rStatus != WLAN_STATUS_SUCCESS)
		goto dump;
#endif

	HAL_MCR_WR(ad,
		CONN_HOST_CSR_TOP_CSR_AP2CONN_ACCESS_DETECT_EN_ADDR, 0x0);
	rStatus = mt7999_mcu_check_idle(ad);
	if (rStatus != WLAN_STATUS_SUCCESS)
		goto dump;

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
	if (connv3_ext_32k_on()) {
		DBGLOG(INIT, ERROR, "connv3_ext_32k_on failed.\n");
		rStatus = WLAN_STATUS_FAILURE;
		goto dump;
	}
#endif

	mt7999SetupMcuEmiAddr(ad);

	if (ad->chip_info->coexpccifon)
		ad->chip_info->coexpccifon(ad);

#if (CFG_SUPPORT_UNI_FWDL == 1)
	HAL_MCR_WR(ad,
		   CONN_BUS_CR_VON_CONN_INFRA_PCIE2AP_REMAP_WF_1_BA_ADDR,
		   0x180b1805);
	rStatus = uniFwdlDownloadFW(ad);
	if (rStatus != WLAN_STATUS_SUCCESS)
		goto dump;
#endif /* CFG_SUPPORT_UNI_FWDL */

dump:
	if (rStatus != WLAN_STATUS_SUCCESS) {
		WARN_ON_ONCE(TRUE);

		prChipInfo = ad->chip_info;
		prDbgOps = prChipInfo->prDebugOps;
		if (prDbgOps && prDbgOps->dumpBusStatus)
			prDbgOps->dumpBusStatus(ad);

#if (CFG_MTK_FPGA_PLATFORM != 1)
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
		DBGLOG(INIT, DEBUG,
			"0x%08x=0x%08x\n",
			CB_CKGEN_TOP_CBTOP_ULPOSC_2_ADDR,
			u4Value);
		HAL_RMCR_RD(ONOFF_DBG, ad,
			   CB_INFRA_SLP_CTRL_CB_INFRA_CRYPTO_TOP_MCU_OWN_ADDR,
			   &u4Value);
		DBGLOG(INIT, DEBUG,
			"0x%08x=0x%08x\n",
			CB_INFRA_SLP_CTRL_CB_INFRA_CRYPTO_TOP_MCU_OWN_ADDR,
			u4Value);
#endif /* CFG_MTK_FPGA_PLATFORM */
	}
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
	if (prChipInfo->sw_sync_emi_info)
		kalMemSet(prChipInfo->sw_sync_emi_info, 0x0,
		sizeof(struct sw_sync_emi_info) * SW_SYNC_TAG_NUM);
#endif /* CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI */

#if (CFG_SUPPORT_CONNFEM == 1)
	prChipInfo->isAaDbdcEnable = mt7999_is_AA_DBDC_enable();
#endif

exit:
	return rStatus;
}

static void mt7999_mcu_deinit(struct ADAPTER *ad)
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

	mt7999_force_conn_infra_on(ad, FALSE);

	if (ad->chip_info->coexpccifoff)
		ad->chip_info->coexpccifoff(ad);

#if CFG_SUPPORT_PCIE_ASPM
	if (pcie_vir_addr) {
		iounmap(pcie_vir_addr);
		pcie_vir_addr = NULL;
	}
#endif
}

static int32_t mt7999_trigger_fw_assert(struct ADAPTER *prAdapter)
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
static int mt7999ConnacPccifOn(struct ADAPTER *prAdapter)
{
#if CFG_MTK_CCCI_SUPPORT && CFG_MTK_MDDP_SUPPORT
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	uint32_t ccif_base = 0x160000;
	uint32_t mcif_emi_base, u4Val = 0, u4WifiEmi = 0;
	void *vir_addr = NULL;
	int size = 0;

#if CFG_MTK_ANDROID_WMT
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
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
			   MT7999_MCIF_MD_STATE_WHEN_WIFI_ON_ADDR,
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

static int mt7999ConnacPccifOff(struct ADAPTER *prAdapter)
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

static int mt7999_CheckBusNoAck(void *priv, uint8_t rst_enable)
{
	struct ADAPTER *ad = priv;
	u_int8_t readable = FALSE;

	if (fgIsBusAccessFailed) {
		readable = FALSE;
		goto exit;
	}

	if (mt7999_is_ap2conn_off_readable(ad) &&
	    mt7999_is_conn2wf_readable(ad))
		readable = TRUE;
	else
		readable = FALSE;

exit:
	return readable ? 0 : 1;
}

static uint32_t mt7999_wlanDownloadPatch(struct ADAPTER *prAdapter)
{
	uint32_t status = wlanDownloadPatch(prAdapter);

	if (status == WLAN_STATUS_SUCCESS)
		wifi_coredump_set_enable(TRUE);

	return status;
}
#endif /* _HIF_PCIE */

static uint32_t mt7999GetFlavorVer(struct GLUE_INFO *prGlueInfo,
	uint8_t *flavor)
{
	uint32_t ret = WLAN_STATUS_FAILURE;
	uint8_t aucFlavor[CFG_FW_FLAVOR_MAX_LEN] = {0};

	if (kalGetFwFlavorByGlue(prGlueInfo, &aucFlavor[0]) == 1) {
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

static void mt7999WiFiNappingCtrl(
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

static u_int8_t mt7999_isUpgradeWholeChipReset(struct ADAPTER *prAdapter)
{
#if defined(_HIF_PCIE)
	uint32_t u4Val1, u4Val2;
#if CFG_SUPPORT_PCIE_ASPM
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	uint32_t u4Status = 0;

	/* Request keep L1/L0 */
	u4Status = mt7999ConfigPcieAspm(prGlueInfo, FALSE, WIFI_RST_ROLE);
	if (u4Status) {
		DBGLOG(INIT, ERROR, "Leave L1.2 failed\n");
		return TRUE;
	}
#endif

	glReadPcieCfgSpace(0x488, &u4Val1);
	glReadPcieCfgSpace(0x48c, &u4Val2);
	DBGLOG(INIT, DEBUG,
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

	HAL_RMCR_RD(PLAT_DBG, prAdapter,
		CONN_DBG_CTL_CONN_INFRA_BUS_DBG_CR_00_ADDR, &u4Val1);
	if ((u4Val1 & BITS(0, 9)) != 0x0) {
		DBGLOG(HAL, WARN,
			"Conninfra timeout status: 0x%08x\n",
			u4Val1);
		return TRUE;
	}

	HAL_RMCR_RD(PLAT_DBG, prAdapter,
		CONN_DBG_CTL_WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_BUS_TIMEOUT_IRQ_ADDR,
		&u4Val1);
	if ((u4Val1 & BIT(0)) != 0x0) {
		DBGLOG(HAL, WARN,
			"WF mcusys bus status: 0x%08x\n",
			u4Val1);
		return TRUE;
	}

	return FALSE;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint8_t mt7999_apsLinkPlanDecision(struct ADAPTER *prAdapter,
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
		u4LinkPlanAGBmap |
		BIT(MLO_LINK_PLAN_5_5)
#if (CFG_SUPPORT_WIFI_6G == 1)

		| BIT(MLO_LINK_PLAN_5_6)
#endif
	;

#if (CFG_SUPPORT_MLC == 1)
	uint32_t u4LinkPlan3Bmap =
		BIT(MLO_LINK_PLAN_2_5_5)
#if (CFG_SUPPORT_WIFI_6G == 1)
		| BIT(MLO_LINK_PLAN_2_5_6)
#endif
	;
#endif /* CFG_SUPPORT_MLC */

	/* Eable A+A when support EMLSR */
	if (IS_NON_AP_EML_ENABLED(prAdapter))
		u4TmpLinkPlanBmap = u4LinkPlanAABmap;
	else
		u4TmpLinkPlanBmap = u4LinkPlanAGBmap;

#if (CFG_SUPPORT_MLC == 1)
	if (IS_MLC_ENABLED(prAdapter))
		u4TmpLinkPlanBmap = u4LinkPlan3Bmap;
#endif

	return !!(u4TmpLinkPlanBmap & BIT(eLinkPlan));
}

static void mt7999_apsUpdateTotalScore(struct ADAPTER *prAdapter,
	struct BSS_DESC *arLinks[], uint8_t ucLinkNum,
	struct AP_SCORE_INFO *prScoreInfo, uint8_t ucBssidx)
{
	uint8_t ucAbandCount = 0;
	uint32_t u4TotalScore = 0, u4TotalTput = 0;
	struct BSS_DESC *best_bss = arLinks[0]; /* links is sorted by score */
	uint8_t ucEmlsrLinkWeight = prAdapter->rWifiVar.ucEmlsrLinkWeight;
	enum ENUM_MLO_MODE eMloMode = MLO_MODE_SLSR;
	enum ENUM_MLO_LINK_PLAN eLinkPlan;
	uint8_t ucMaxSimuLinks = 0;
	uint8_t i;

	for (i = 0; i < ucLinkNum; i++) {
		if (arLinks[i]->eBand == BAND_2G4) {
			u4TotalScore += arLinks[i]->u2Score;
			u4TotalTput += arLinks[i]->u4Tput;
		} else {
			if (ucAbandCount == 0) {
				u4TotalScore += arLinks[i]->u2Score;
				u4TotalTput += arLinks[i]->u4Tput;
			} else {
				u4TotalScore += arLinks[i]->u2Score *
					 ucEmlsrLinkWeight / 100;
				u4TotalTput += arLinks[i]->u4Tput *
					 ucEmlsrLinkWeight / 100;
			}

			ucAbandCount++;
		}
	}

	eLinkPlan = apsLinksToLinkPlan(arLinks, ucLinkNum);

	switch (eLinkPlan) {
	/* A/G */
	case MLO_LINK_PLAN_2:
	case MLO_LINK_PLAN_5:
#if (CFG_SUPPORT_WIFI_6G == 1)
	case MLO_LINK_PLAN_6:
#endif
		ucMaxSimuLinks = 0;
		eMloMode = MLO_MODE_SLSR;
		break;

	/* A+G */
	case MLO_LINK_PLAN_2_5:
#if (CFG_SUPPORT_WIFI_6G == 1)
	case MLO_LINK_PLAN_2_6:
#endif
		ucMaxSimuLinks = 1;
		eMloMode = MLO_MODE_STR;
		break;

	/* A+A */
	case MLO_LINK_PLAN_5_5:
#if (CFG_SUPPORT_WIFI_6G == 1)
	case MLO_LINK_PLAN_5_6:
#endif
		if (IS_NON_AP_EML_ENABLED(prAdapter) &&
		    BE_IS_EML_CAP_SUPPORT_EMLSR(best_bss->rMlInfo.u2EmlCap)) {
			ucMaxSimuLinks = 0;
			eMloMode = MLO_MODE_EMLSR;
#if (CFG_SUPPORT_MLC == 1)
		} else if (IS_MLC_ENABLED(prAdapter)) {
			ucMaxSimuLinks = 0;
			eMloMode = MLO_MODE_MLSR;
#endif
		} else {
			/* fallback to single link if ap no emlsr */
			u4TotalScore = best_bss->u2Score;
			u4TotalTput = best_bss->u4Tput;
			ucLinkNum = 1;
			ucMaxSimuLinks = 0;
			eMloMode = MLO_MODE_SLSR;
		}

		break;

	/* G+A+A */
	case MLO_LINK_PLAN_2_5_5:
#if (CFG_SUPPORT_WIFI_6G == 1)
	case MLO_LINK_PLAN_2_5_6:
#endif

#if (CFG_SUPPORT_MLC == 1)
		if (IS_MLC_ENABLED(prAdapter)) {
			ucMaxSimuLinks = 1;
			eMloMode = MLO_MODE_HYMLSR;
			break;
		}
#endif

		DBGLOG(APS, INFO, "unsupported link plan=%d\n", eLinkPlan);
		kal_fallthrough;

	default:
		u4TotalScore = best_bss->u2Score;
		u4TotalTput = best_bss->u4Tput;
		ucMaxSimuLinks = 0;
		ucLinkNum = 1;
		eMloMode = MLO_MODE_SLSR;
		break;
	}

	kalMemCopy(prScoreInfo->aprTarget, arLinks,
		sizeof(prScoreInfo->aprTarget));
	prScoreInfo->ucLinkNum = ucLinkNum;
	prScoreInfo->u4TotalScore = u4TotalScore;
	prScoreInfo->u4TotalTput = u4TotalTput;
	prScoreInfo->eMloMode = eMloMode;
	prScoreInfo->ucMaxSimuLinks = ucMaxSimuLinks;
}

static void mt7999_apsFillBssDescSet(struct ADAPTER *prAdapter,
		struct BSS_DESC_SET *prSet,
		uint8_t ucBssidx)
{
#if (CFG_SUPPORT_MLC == 1)
	if (!IS_MLC_ENABLED(prAdapter))
		return;

	/* A + G + A -> A + A + G */
	if (prSet->ucLinkNum == 3 &&
	    prSet->aprBssDescW[0]->eBand != BAND_2G4 &&
	    prSet->aprBssDescW[1]->eBand == BAND_2G4) {
		struct BSS_DESC_W *w = prSet->aprBssDescW[1];

		prSet->aprBssDescW[1] = prSet->aprBssDescW[2];
		prSet->aprBssDescW[2] = w;
	}

	/* 2 or 3 link: om = 0 + 1 + 0 */
	if (prSet->ucLinkNum > 1)
		prSet->aprBssDescW[1]->fgUnSyncOm = TRUE;
#endif
}

#endif /* CFG_SUPPORT_802_11BE_MLO */

static void mt7999LowPowerOwnInit(struct ADAPTER *prAdapter)
{
	HAL_MCR_WR(prAdapter,
		   CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_ADDR,
		   BIT(0));
}

static void mt7999LowPowerOwnRead(struct ADAPTER *prAdapter,
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

static void mt7999LowPowerOwnSet(struct ADAPTER *prAdapter,
				 u_int8_t *pfgResult)
{
	struct mt66xx_chip_info *prChipInfo;
#if (CFG_MTK_WIFI_DRV_OWN_INT_MODE == 0)
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

#if (CFG_MTK_WIFI_DRV_OWN_INT_MODE == 0)
	HAL_RMCR_RD(LPOWN_READ, prAdapter,
		    CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		    &u4RegValue);

	*pfgResult = (u4RegValue &
		PCIE_LPCR_AP_HOST_OWNER_STATE_SYNC) == 0x4;
#else
	*pfgResult = TRUE;
#endif
#if defined(_HIF_PCIE)
	if (prChipInfo->bus_info->hwControlVote)
		prChipInfo->bus_info->hwControlVote(prAdapter,
			TRUE, PCIE_VOTE_USER_DRVOWN);
#endif
}

static void mt7999LowPowerOwnClear(struct ADAPTER *prAdapter,
				   u_int8_t *pfgResult)
{
	struct mt66xx_chip_info *prChipInfo;
#if (CFG_MTK_WIFI_DRV_OWN_INT_MODE == 0)
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

#if (CFG_MTK_WIFI_DRV_OWN_INT_MODE == 1)
	clear_bit(GLUE_FLAG_DRV_OWN_INT_BIT,
		  &prAdapter->prGlueInfo->ulFlag);
#endif

	HAL_MCR_WR(prAdapter,
		   CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR,
		   PCIE_LPCR_HOST_CLR_OWN);

#if (CFG_MTK_WIFI_DRV_OWN_INT_MODE == 0)
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
int mt7999PowerDumpStart(void *priv_data, unsigned int force_dump)
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
		DBGLOG(REQ, DEBUG, "PowerDumpStart force_dump\n");
		ad->fgIsPowerDumpDrvOwn = TRUE;
		ACQUIRE_POWER_CONTROL_FROM_PM(ad,
			DRV_OWN_SRC_POWER_DUMP_START);
		ad->fgIsPowerDumpDrvOwn = FALSE;

		if (ad->fgIsFwOwn == TRUE) {
			DBGLOG(REQ, ERROR,
				"PowerDumpStart end: driver own fail!\n");
			return 1;
		}

		return 0;

	} else {
		u4Val = (u4Val & 0x0000001F);
		DBGLOG(REQ, DEBUG,
			"PowerDumpStart PCIE status: 0x%08x\n", u4Val);

		if (u4Val == 0x10) {
			ad->fgIsPowerDumpDrvOwn = TRUE;
			ACQUIRE_POWER_CONTROL_FROM_PM(ad,
				DRV_OWN_SRC_POWER_DUMP_START);
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

int mt7999PowerDumpEnd(void *priv_data)
{
	struct GLUE_INFO *glue = priv_data;
	struct ADAPTER *ad = glue->prAdapter;

	if (ad == NULL) {
		DBGLOG(REQ, ERROR, "prAdapter is NULL!\n");
		return 0;
	}

	if (ad->fgIsFwOwn == FALSE && glue->u4ReadyFlag)
		RECLAIM_POWER_CONTROL_TO_PM(ad, FALSE,
			DRV_OWN_SRC_POWER_DUMP_START);

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
u_int8_t mt7999_is_AA_DBDC_enable(void)
{
	uint8_t fe_bt_wf_usage;
	uint32_t rStarus;

	rStarus = connfem_sku_flag_u8(CONNFEM_SUBSYS_NONE, "fe-bt-wf-usage",
			    &fe_bt_wf_usage);
	if (rStarus != 0)
		return FALSE;

	return !!((fe_bt_wf_usage & BIT(3)) && (fe_bt_wf_usage & BIT(4)));
}
#endif /* CFG_SUPPORT_CONNFEM == 1 */

#if CFG_MTK_WIFI_MBU
static void mt7999MbuDumpDebugCr(struct GLUE_INFO *prGlueInfo)
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
	DBGLOG(HAL, DEBUG, "%s\n", aucBuf);

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
	DBGLOG(HAL, DEBUG, "%s\n", aucBuf);

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
	DBGLOG(HAL, DEBUG, "%s\n", aucBuf);

	kalMemFree(aucBuf, PHY_MEM_TYPE, u4BufferSize);
}
#endif /* CFG_MTK_WIFI_MBU */

#if CFG_MTK_MDDP_SUPPORT
static void mt7999CheckMdRxStall(struct ADAPTER *prAdapter)
{
	char aucRsn[MDDP_EXP_RSN_SIZE];
	uint32_t u4Base = 0, u4Cnt = 0, u4Cidx = 0, u4Didx = 0, u4Addr;

	/* check md rx event ring */
	u4Addr = WF_P0_WFDMA_WPDMA_RX_RING12_CTRL0_ADDR;
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
		     MDDP_EXP_RX_STALL);
#if CFG_WMT_RESET_API_SUPPORT
	glSetRstReasonString(aucRsn);
	glResetWholeChipResetTrigger(aucRsn);
#endif /* CFG_WMT_RESET_API_SUPPORT */
}
#endif

#if (CFG_SUPPORT_UNI_FWDL == 1)
static void mt7999_trigger_wf_fwdl_doorbell(struct ADAPTER *ad)
{
#if defined(_HIF_PCIE)
#define IMAGE_DOORBELL_BIT		(24)

	glWritePcieCfgSpace((PCIE_MAC_CONF_COMMON_PCIE_VENDOR_EINT_SET_0_ADDR -
			    PCIE_MAC_CONF_COMMON_BASE),
			    BIT(IMAGE_DOORBELL_BIT));
#endif
}
#endif /* CFG_SUPPORT_UNI_FWDL */

#endif  /* MT7999 */
