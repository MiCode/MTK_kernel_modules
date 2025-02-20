/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  soc3_0.h
*    \brief This file contains the info of soc3_0
*/

#ifdef SOC3_0

#ifndef _SOC3_0_H
#define _SOC3_0_H
#if (CFG_SUPPORT_CONNINFRA == 1)
#include "conninfra.h"
#endif
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define CONNAC2X_TOP_HCR 0x88000000  /*no use, set HCR = HVR*/
#define CONNAC2X_TOP_HVR 0x88000000
#define CONNAC2X_TOP_FVR 0x88000004

#define SOC3_0_CHIP_ID		(0x7915)
#define SOC3_0_SW_SYNC0		CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_ADDR
#define SOC3_0_SW_SYNC0_RDY_OFFSET \
	CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT
#define SOC3_0_PATCH_START_ADDR			(0x00100000)
#define SOC3_0_TOP_CFG_BASE			NIC_CONNAC_CFG_BASE
#define SOC3_0_TX_DESC_APPEND_LENGTH		32
#define SOC3_0_RX_DESC_LENGTH			24
#define SOC3_0_ARB_AC_MODE_ADDR			(0x820e3020)
#define MTK_CUSTOM_OID_INTERFACE_VERSION \
	0x00000200 /* for WPDWifi DLL */
#define MTK_EM_INTERFACE_VERSION		0x0001

#define CONN_HOST_CSR_TOP_BASE_ADDR 0x18060000
#define CONN_INFRA_CFG_BASE_ADDR 0x18001000
#define CONN_INFRA_RGU_BASE_ADDR 0x18000000
#define CONN_INFRA_BRCM_BASE_ADDR 0x1800E000
#define WFDMA_AXI0_R2A_CTRL_0	0x7c027500

#define WF_TOP_MISC_OFF_BASE_ADDR 0x184B0000
#define BID_CHK_BYP_EN_MASK 0x00000800

#define WF2CONN_SLPPROT_IDLE_ADDR 0x1800F004

#define CONN_INFRA_WAKEUP_WF_ADDR (CONN_HOST_CSR_TOP_BASE_ADDR + 0x01A4)
#define CONN_INFRA_ON2OFF_SLP_PROT_ACK_ADDR \
	(CONN_HOST_CSR_TOP_BASE_ADDR + 0x0184)
#define CONN_MCU_CONFG_ON_HOST_MAILBOX_WF_ADDR \
	(CONN_HOST_CSR_TOP_BASE_ADDR + 0x0260)
#define CONN_HW_VER_ADDR (CONN_INFRA_CFG_BASE_ADDR + 0x0000)
#define WFSYS_SW_RST_B_ADDR (CONN_INFRA_RGU_BASE_ADDR + 0x0018)
#define WFSYS_CPU_SW_RST_B_ADDR (CONN_INFRA_RGU_BASE_ADDR + 0x0010)
#define WFSYS_ON_TOP_PWR_CTL_ADDR (CONN_INFRA_RGU_BASE_ADDR + 0x0000)
#define TOP_DBG_DUMMY_3_CONNSYS_PWR_STATUS_ADDR \
	(CONN_HOST_CSR_TOP_BASE_ADDR + 0x02CC)
#define CONN_INFRA_WF_SLP_CTRL_R_ADDR (CONN_INFRA_CFG_BASE_ADDR + 0x0620)
#define CONN_INFRA_WFDMA_SLP_CTRL_R_ADDR (CONN_INFRA_CFG_BASE_ADDR + 0x0624)
#define CONN_INFRA_WFSYS_EMI_REQ_ADDR (CONN_INFRA_CFG_BASE_ADDR + 0x0c14)

#define WF_VDNR_EN_ADDR (CONN_INFRA_BRCM_BASE_ADDR + 0x6C)
#define WFSYS_VERSION_ID_ADDR (WF_TOP_MISC_OFF_BASE_ADDR + 0x10)
#define CONN_CFG_AP2WF_REMAP_1_ADDR (CONN_INFRA_CFG_BASE_ADDR + 0x0120)
#define WFSYS_VERSION_ID  0x20010000
#define WF_DYNAMIC_BASE 0x18500000
#define MCU_EMI_ENTRY_OFFSET 0x01DC
#define WF_EMI_ENTRY_OFFSET 0x01E0
#define CONN_AFE_CTL_RG_DIG_EN_ADDR (0x18003008)

#define CONNSYS_ROM_DONE_CHECK  0x00001D1E

#define WF_ROM_CODE_INDEX_ADDR 0x184C1604

#define WF_TRIGGER_AP2CONN_EINT 0x10001F00
#define WF_CONN_INFA_BUS_CLOCK_RATE 0x1000123C

#define WMMCU_ROM_PATCH_DATE_ADDR 0xF027F0D0
#define WMMCU_MCU_ROM_EMI_DATE_ADDR 0xF027F0E0
#define WMMCU_WIFI_ROM_EMI_DATE_ADDR 0xF027F0F0
#define DATE_CODE_SIZE 16

#define WF_PP_TOP_BASE             0x820CC000
#define WF_PP_TOP_DBG_CTRL_ADDR    (WF_PP_TOP_BASE + 0x00F0)
#define WF_PP_TOP_DBG_CS_0_ADDR    (WF_PP_TOP_BASE + 0x00F8)
#define WF_PP_TOP_DBG_CS_1_ADDR    (WF_PP_TOP_BASE + 0x00FC)
#define WF_PP_TOP_DBG_CS_2_ADDR    (WF_PP_TOP_BASE + 0x0100)

#define CONN_MCU_CONFG_HS_BASE 0x89040000
#define CONN_MCU_CONFG_WF2AP_SW_IRQ_CTRL_ADDR \
	(CONN_MCU_CONFG_HS_BASE + 0x00c0)
#define CONN_MCU_CONFG_WF2AP_SW_IRQ_SET_ADDR \
	(CONN_MCU_CONFG_HS_BASE + 0x00c4)
#define CONN_MCU_CONFG_WF2AP_SW_IRQ_CLEAR_ADDR \
	(CONN_MCU_CONFG_HS_BASE + 0x00c8)

#define SOC3_0_PCIE2AP_REMAP_BASE_ADDR		0x50000
#define SOC3_0_REMAP_BASE_ADDR			0x7c500000

union soc3_0_WPDMA_INT_MASK {

	struct {
		uint32_t wfdma1_rx_done_0:1;
		uint32_t wfdma1_rx_done_1:1;
		uint32_t wfdma1_rx_done_2:1;
		uint32_t wfdma1_rx_done_3:1;
		uint32_t wfdma1_tx_done_0:1;
		uint32_t wfdma1_tx_done_1:1;
		uint32_t wfdma1_tx_done_2:1;
		uint32_t wfdma1_tx_done_3:1;
		uint32_t wfdma1_tx_done_4:1;
		uint32_t wfdma1_tx_done_5:1;
		uint32_t wfdma1_tx_done_6:1;
		uint32_t wfdma1_tx_done_7:1;
		uint32_t wfdma1_tx_done_8:1;
		uint32_t wfdma1_tx_done_9:1;
		uint32_t wfdma1_tx_done_10:1;
		uint32_t wfdma1_tx_done_11:1;
		uint32_t wfdma1_tx_done_12:1;
		uint32_t wfdma1_tx_done_13:1;
		uint32_t wfdma1_tx_done_14:1;
		uint32_t reserved19:1;
		uint32_t wfdma1_rx_coherent:1;
		uint32_t wfdma1_tx_coherent:1;
		uint32_t reserved:2;
		uint32_t wpdma2host_err_int_en:1;
		uint32_t reserved25:1;
		uint32_t wfdma1_tx_done_16:1;
		uint32_t wfdma1_tx_done_17:1;
		uint32_t wfdma1_subsys_int_en:1;
		uint32_t wfdma1_mcu2host_sw_int_en:1;
		uint32_t wfdma1_tx_done_18:1;
		uint32_t reserved31:1;
	} field_wfdma1_ena;

	struct {
		uint32_t wfdma0_rx_done_0:1;
		uint32_t wfdma0_rx_done_1:1;
		uint32_t wfdma0_rx_done_2:1;
		uint32_t wfdma0_rx_done_3:1;
		uint32_t wfdma0_tx_done_0:1;
		uint32_t wfdma0_tx_done_1:1;
		uint32_t wfdma0_tx_done_2:1;
		uint32_t wfdma0_tx_done_3:1;
		uint32_t reserved8:11;
		uint32_t wfdma0_rx_done_6:1;
		uint32_t wfdma0_rx_coherent:1;
		uint32_t wfdma0_tx_coherent:1;
		uint32_t wfdma0_rx_done_4:1;
		uint32_t wfdma0_rx_done_5:1;
		uint32_t wpdma2host_err_int_en:1;
		uint32_t wfdma0_rx_done_7:1;
		uint32_t reserved26:2;
		uint32_t wfdma0_subsys_int_en:1;
		uint32_t wfdma0_mcu2host_sw_int_en:1;
		uint32_t reserved30:2;
	} field_wfdma0_ena;
	uint32_t word;
};
/*******************************************************************************
*                         D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
#if (CFG_SUPPORT_CONNINFRA == 1)
extern u_int8_t g_IsWfsysBusHang;
extern u_int8_t g_fgRstRecover;
#endif

#if (CFG_WIFI_COREDUMP_SUPPORT == 1)
extern u_int8_t g_IsNeedWaitCoredump;
#endif

extern struct PLE_TOP_CR rSoc3_0_PleTopCr;
extern struct PSE_TOP_CR rSoc3_0_PseTopCr;
extern struct PP_TOP_CR rSoc3_0_PpTopCr;
/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void soc3_0_show_ple_info(
	struct ADAPTER *prAdapter,
	u_int8_t fgDumpTxd);
void soc3_0_show_pse_info(
	struct ADAPTER *prAdapter);

void soc3_0_show_wfdma_info(
	struct ADAPTER *prAdapter);

void soc3_0_show_wfdma_info_by_type(
	struct ADAPTER *prAdapter,
	bool bShowWFDMA_type);

void soc3_0_show_wfdma_info_by_type_without_adapter(
	bool bIsHostDMA);

void soc3_0_show_wfdma_dbg_probe_info(
	struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);

void soc3_0_DumpWFDMACr(struct ADAPTER *prAdapter);

void soc3_0_show_dmashdl_info(
	struct ADAPTER *prAdapter);
void soc3_0_dump_mac_info(
	struct ADAPTER *prAdapter);
void soc3_0EnableInterrupt(
	struct ADAPTER *prAdapter);

void soc3_0EnableInterrupt(
	struct ADAPTER *prAdapter);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

void soc3_0_DumpWfsyscpupcr(struct ADAPTER *prAdapter);
void soc3_0_WfdmaAxiCtrl(struct ADAPTER *prAdapter);

int soc3_0_Trigger_fw_assert(struct ADAPTER *prAdapter);
int soc3_0_CheckBusHang(void *adapter,
	uint8_t ucWfResetEnable);
void soc3_0_DumpBusHangCr(struct ADAPTER *prAdapter);

void wlanCoAntWiFi(void);
void wlanCoAntMD(void);
void wlanCoAntVFE28En(struct ADAPTER *prAdapter);
void wlanCoAntVFE28Dis(void);

#if (CFG_SUPPORT_CONNINFRA == 1)
int wlanConnacPccifon(struct ADAPTER *prAdapter);
int wlanConnacPccifoff(struct ADAPTER *prAdapter);
#endif
void soc3_0_DumpWfsysdebugflag(void);

void soc3_0_icapRiseVcoreClockRate(void);
void soc3_0_icapDownVcoreClockRate(void);

#endif /* _SOC3_0_H */

#endif  /* soc3_0 */
