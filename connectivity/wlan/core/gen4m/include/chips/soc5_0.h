/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  soc5_0.h
*    \brief This file contains the info of soc5_0
*/

#ifdef SOC5_0

#ifndef _SOC5_0_H
#define _SOC5_0_H

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
#define CONN_INFRA_CFG_BASE			0x830C0000
#define SOC5_0_TX_DESC_APPEND_LENGTH		32
#define SOC5_0_RX_DESC_LENGTH			24
#define SOC5_0_CHIP_ID				0x7961
#define CONNAC2X_TOP_HCR			0x88000000
#define CONNAC2X_TOP_HVR			0x88000000
#define CONNAC2X_TOP_FVR			0x88000004
#define SOC5_0_TOP_CFG_BASE			NIC_CONNAC_CFG_BASE
#define SOC5_0_PATCH_START_ADDR			0x00900000
#define SOC5_0_ARB_AC_MODE_ADDR			(0x820E315C)
#define MTK_CUSTOM_OID_INTERFACE_VERSION \
	0x00000200 /* for WPDWifi DLL */
#define MTK_EM_INTERFACE_VERSION		0x0001
#define RX_DATA_RING_BASE_IDX			2

#define CONN_HOST_CSR_TOP_BASE_ADDR 0x18060000
#define CONN_INFRA_CFG_BASE_ADDR 0x18001000
#define CONN_INFRA_RGU_BASE_ADDR 0x18000000
#define WF_TOP_SLPPROT_ON_BASE_ADDR  0x184C3000
#define WF_TOP_CFG_BASE_ADDR 0x184B0000
#define WF_MCU_CFG_LS_BASE_ADDR 0x184F0000
#define WF_MCU_BUS_CR_BASE_ADDR 0x18400000
#define WF_MCUSYS_INFRA_BUS_BASE_ADDR 0x18500000
#define CONN_INFRA_VDNR_AXI_LAYER_BASE_ADDR 0x1801D000
#define WF_TOP_CFG_ON_BASE_ADDR 0x184C1000
#define CONN_WT_SLP_CTL_REG_BASE_ADDR 0x18005000
#define AP2WF_CONN_INFRA_ON_CCIF4_BASE_ADDR 0x1803D000

#define CONN_INFRA_WAKEUP_WF_ADDR (CONN_HOST_CSR_TOP_BASE_ADDR + 0x01A4)
#define CONN_HW_VER_ADDR (CONN_INFRA_CFG_BASE_ADDR + 0x0000)
#define WFSYS_CPU_SW_RST_B_ADDR (CONN_INFRA_RGU_BASE_ADDR + 0x0120)
#define CONN_INFRA_WF_SLP_CTRL (CONN_INFRA_CFG_BASE_ADDR + 0x0540)
#define WFSYS_ON_TOP_PWR_CTL_ADDR (CONN_INFRA_RGU_BASE_ADDR + 0x0010)
#define TOP_DBG_DUMMY_3_CONNSYS_PWR_STATUS_ADDR \
	(CONN_HOST_CSR_TOP_BASE_ADDR + 0x02CC)
#define CONN_INFRA_WF_SLP_STATUS (CONN_INFRA_CFG_BASE_ADDR + 0x0544)
#define WF_TOP_SLPPROT_ON_STATUS (WF_TOP_SLPPROT_ON_BASE_ADDR + 0x000C)
#define WFSYS_VERSION_ID_ADDR (WF_TOP_CFG_BASE_ADDR + 0x10)
#define BUSHANGCR_BUS_HANG (WF_MCU_CFG_LS_BASE_ADDR + 0x0440)
#define WF_MCU_BUS_CR_AP2WF_REMAP_1 (WF_MCU_BUS_CR_BASE_ADDR + 0x0120)
#define DEBUG_CTRL_AO_WFMCU_PWA_CTRL0 (WF_MCUSYS_INFRA_BUS_BASE_ADDR + 0x0000)
#define DEBUG_CTRL_AO_WFMCU_PWA_CTRL3 (WF_MCUSYS_INFRA_BUS_BASE_ADDR + 0x000C)
#define DEBUG_CTRL_AO_CONN_INFRA_CTRL0 \
	(CONN_INFRA_VDNR_AXI_LAYER_BASE_ADDR + 0x0000)
#define WF_ROM_CODE_INDEX_ADDR (WF_TOP_CFG_ON_BASE_ADDR + 0x0604)
#define WFSYS_SW_RST_B_ADDR (CONN_INFRA_RGU_BASE_ADDR + 0x0140)
#define WB_SLP_TOP_CK_1 (CONN_WT_SLP_CTL_REG_BASE_ADDR + 0x0124)
#define CONN_INFRA_WFSYS_EMI_REQ_ADDR (CONN_INFRA_CFG_BASE_ADDR + 0x0414)
#define AP2WF_PCCIF_RCHNUM (AP2WF_CONN_INFRA_ON_CCIF4_BASE_ADDR + 0x0010)
#define AP2WF_PCCIF_ACK (AP2WF_CONN_INFRA_ON_CCIF4_BASE_ADDR + 0x0014)

#define CONNSYS_VERSION_ID  0x02060002
#define WFSYS_VERSION_ID  0x02050002
#define WF_MCUSYS_INFRA_BUS_FULL_U_DEBUG_CTRL_AO_BASE 0x810F0000
#define CONNSYS_ROM_DONE_CHECK  0x00001D1E

#define WF_TRIGGER_AP2CONN_EINT 0x10001F00

#define WMMCU_ROM_PATCH_DATE_ADDR 0xF04954D0
#define WMMCU_MCU_ROM_EMI_DATE_ADDR 0xF04954E0
#define WMMCU_WIFI_ROM_EMI_DATE_ADDR 0xF04954F0
#define DATE_CODE_SIZE 16

#define WF_CONN_INFA_BUS_CLOCK_RATE 0x18009A00

#define WF_PP_TOP_BASE             0x820CC000
#define WF_PP_TOP_DBG_CTRL_ADDR    (WF_PP_TOP_BASE + 0x00FC)
#define WF_PP_TOP_DBG_CS_0_ADDR    (WF_PP_TOP_BASE + 0x0104)
#define WF_PP_TOP_DBG_CS_1_ADDR    (WF_PP_TOP_BASE + 0x0108)
#define WF_PP_TOP_DBG_CS_2_ADDR    (WF_PP_TOP_BASE + 0x010C)

#define SOC5_0_PCIE2AP_REMAP_BASE_ADDR	0x50000
#define SOC5_0_REMAP_BASE_ADDR		0x7c500000

/*------------------------------------------------------------------------------
 * MACRO for SOC5_0 RXVECTOR0(GROUP3 NEW FORMAT WITH ENTIRE RATE) Parsing
 *------------------------------------------------------------------------------
 */
#define SOC5_0_RX_VT_RX_RATE_MASK         BITS(0, 6)
#define SOC5_0_RX_VT_RX_RATE_OFFSET       0
#define SOC5_0_RX_VT_NSTS_MASK            BITS(7, 9)
#define SOC5_0_RX_VT_NSTS_OFFSET          7
#define SOC5_0_RX_VT_BF_MASK              BIT(10)
#define SOC5_0_RX_VT_BF_OFFSET            10
#define SOC5_0_RX_VT_LDPC_MASK            BIT(11)
#define SOC5_0_RX_VT_LDPC_OFFSET          11
#define SOC5_0_RX_VT_FR_MODE_MASK         BITS(12, 14)
#define SOC5_0_RX_VT_FR_MODE_OFFSET       12
#define SOC5_0_RX_VT_GI_MASK              BITS(15, 16)
#define SOC5_0_RX_VT_GI_OFFSET            15
#define SOC5_0_RX_VT_DCM_MASK             BIT(17)
#define SOC5_0_RX_VT_DCM_OFFSET           17
#define SOC5_0_RX_VT_NUMRX_MASK           BITS(18, 20)
#define SOC5_0_RX_VT_NUMRX_OFFSET         18
#define SOC5_0_RX_VT_MUMIMO_MASK          BIT(21)
#define SOC5_0_RX_VT_MUMIMO_OFFSET        21
#define SOC5_0_RX_VT_STBC_MASK            BITS(22, 23)
#define SOC5_0_RX_VT_STBC_OFFSET          22
#define SOC5_0_RX_VT_TXMODE_MASK          BITS(24, 27)
#define SOC5_0_RX_VT_TXMODE_OFFSET        24

#define RXV_GET_RX_RATE(_prRxVector)				\
		(((_prRxVector) & SOC5_0_RX_VT_RX_RATE_MASK)	\
			 >> SOC5_0_RX_VT_RX_RATE_OFFSET)

#define RXV_GET_RX_NSTS(_prRxVector)				\
		(((_prRxVector) & SOC5_0_RX_VT_NSTS_MASK)	\
			 >> SOC5_0_RX_VT_NSTS_OFFSET)

#define RXV_GET_FR_MODE(_prRxVector)				\
		(((_prRxVector) & SOC5_0_RX_VT_FR_MODE_MASK)	\
			 >> SOC5_0_RX_VT_FR_MODE_OFFSET)

#define RXV_GET_GI(_prRxVector)					\
		(((_prRxVector) & SOC5_0_RX_VT_GI_MASK)	\
			 >> SOC5_0_RX_VT_GI_OFFSET)

#define RXV_GET_STBC(_prRxVector)				\
		(((_prRxVector) & SOC5_0_RX_VT_STBC_MASK)	\
			 >> SOC5_0_RX_VT_STBC_OFFSET)

#define RXV_GET_TXMODE(_prRxVector)				\
		(((_prRxVector) & SOC5_0_RX_VT_TXMODE_MASK)	\
			 >> SOC5_0_RX_VT_TXMODE_OFFSET)

#define RXV_GET_MUMIMO(_prRxVector)				\
		(((_prRxVector) & SOC5_0_RX_VT_MUMIMO_MASK)	\
			 >> SOC5_0_RX_VT_MUMIMO_OFFSET)

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

extern struct PLE_TOP_CR rSoc5_0_PleTopCr;
extern struct PSE_TOP_CR rSoc5_0_PseTopCr;
extern struct PP_TOP_CR rSoc5_0_PpTopCr;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void soc5_0_show_wfdma_info(struct ADAPTER *prAdapter);
void soc5_0_show_ple_info(struct ADAPTER *prAdapter, u_int8_t fgDumpTxd);
void soc5_0_show_pse_info(struct ADAPTER *prAdapter);
bool soc5_0_show_host_csr_info(struct ADAPTER *prAdapter);
void soc5_0_show_wfdma_dbg_probe_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
void soc5_0_show_wfdma_wrapper_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
void soc5_0_dump_mac_info(
	struct ADAPTER *prAdapter);
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
int soc5_0_get_rx_rate_info(const uint32_t *prRxV,
		struct RxRateInfo *prRxRateInfo);
#endif

#if CFG_SUPPORT_LLS
void soc5_0_get_rx_link_stats(struct ADAPTER *prAdapter,
	struct SW_RFB *prRetSwRfb, uint32_t *pu4RxV);
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
int soc5_0_Trigger_fw_assert(struct ADAPTER *prAdapter);
void wlanCoAntVFE28En(struct ADAPTER *prAdapter);
void wlanCoAntVFE28Dis(void);

#if (CFG_SUPPORT_CONNINFRA == 1)
int wlanConnacPccifon(struct ADAPTER *prAdapter);
int wlanConnacPccifoff(struct ADAPTER *prAdapter);
#endif

void soc5_0_icapRiseVcoreClockRate(void);
void soc5_0_icapDownVcoreClockRate(void);

#endif /* _SOC5_0_H */

#endif  /* soc5_0 */
