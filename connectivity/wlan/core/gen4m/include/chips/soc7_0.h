/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  soc7_0.h
*    \brief This file contains the info of soc7_0
*/

#ifdef SOC7_0

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
#define CONNAC2x_CONN_CFG_ON_BASE			0x7C060000
#define SOC7_0_TX_DESC_APPEND_LENGTH			32
#define SOC7_0_RX_DESC_LENGTH				24
#define SOC7_0_CHIP_ID					0x7961
#define CONNAC2X_TOP_HCR				0x88000000
#define CONNAC2X_TOP_HVR				0x88000000
#define CONNAC2X_TOP_FVR				0x88000004
#define SOC7_0_TOP_CFG_BASE				NIC_CONNAC_CFG_BASE
#define SOC7_0_PATCH_START_ADDR				0x00900000
#define SOC7_0_ARB_AC_MODE_ADDR				(0x820E315C)
#define RX_DATA_RING_BASE_IDX				2
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT	0
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_ADDR \
		(CONNAC2x_CONN_CFG_ON_BASE + 0xF0)

#define CONN_DBG_CTL_BASE (0x7C023000)
#define CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR (CONN_DBG_CTL_BASE + 0x608)
#define CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR         (CONN_DBG_CTL_BASE + 0x610)

#define SOC7_WF_MCUSYS_INFRA_BUS_FULL_U_DEBUG_CTRL_AO_BASE	0x810F0000
#define SOC7_REMAP0_BASE					0x18500000
#define DEBUG_CTRL_AO_WFMCU_PWA_CTRL0				(SOC7_REMAP0_BASE + \
	WF_MCUSYS_INFRA_BUS_FULL_U_DEBUG_CTRL_AO_WFMCU_PWA_DEBUG_CTRL_AO_WF_MCUSYS_INFRA_BUS_FULL_U_DEBUG_CTRL_AO_WFMCU_PWA_CTRL0_ADDR - \
	WF_MCUSYS_INFRA_BUS_FULL_U_DEBUG_CTRL_AO_WFMCU_PWA_DEBUG_CTRL_AO_BASE)
#define DEBUG_CTRL_AO_WFMCU_PWA_CTRL3				(SOC7_REMAP0_BASE + \
	WF_MCUSYS_INFRA_BUS_FULL_U_DEBUG_CTRL_AO_WFMCU_PWA_DEBUG_CTRL_AO_WF_MCUSYS_INFRA_BUS_FULL_U_DEBUG_CTRL_AO_WFMCU_PWA_CTRL3_ADDR - \
	WF_MCUSYS_INFRA_BUS_FULL_U_DEBUG_CTRL_AO_WFMCU_PWA_DEBUG_CTRL_AO_BASE)
#define WFSYS_ON_TOP_WRITE_KEY					(0x5746 << CONN_INFRA_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_WRITE_KEY_SHFT)

#define MTK_CUSTOM_OID_INTERFACE_VERSION	0x00000200	/* for WPDWifi DLL */
#define MTK_EM_INTERFACE_VERSION		0x0001
#define WF_TRIGGER_AP2CONN_EINT			0x10001F00
#define CONNSYS_ROM_DONE_CHECK			0x00001D1E

#define WMMCU_ROM_PATCH_DATE_ADDR		0xF04954D0
#define WMMCU_MCU_ROM_EMI_DATE_ADDR		0xF04954E0
#define WMMCU_WIFI_ROM_EMI_DATE_ADDR		0xF04954F0
#define DATE_CODE_SIZE 16

#define WF_CONN_INFA_BUS_CLOCK_RATE 0x18012050
#define WF_MCU_BUS_CR_BASE_ADDR 0x18400000
#define WF_MCU_BUS_CR_AP2WF_REMAP_1 (WF_MCU_BUS_CR_BASE_ADDR + 0x0120)
#define WF_MCUSYS_INFRA_BUS_FULL_U_DEBUG_CTRL_AO_BASE 0x810F0000

#define SOC7_0_PCIE2AP_REMAP_BASE_ADDR	0x50000
#define SOC7_0_REMAP_BASE_ADDR		0x7c500000

/*------------------------------------------------------------------------------
 * MACRO for SOC7_0 RXVECTOR0(Same as SOC5_0) Parsing
 *------------------------------------------------------------------------------
 */
#define SOC7_0_RX_VT_RX_RATE_MASK         BITS(0, 6)
#define SOC7_0_RX_VT_RX_RATE_OFFSET       0
#define SOC7_0_RX_VT_NSTS_MASK            BITS(7, 9)
#define SOC7_0_RX_VT_NSTS_OFFSET          7
#define SOC7_0_RX_VT_BF_MASK              BIT(10)
#define SOC7_0_RX_VT_BF_OFFSET            10
#define SOC7_0_RX_VT_LDPC_MASK            BIT(11)
#define SOC7_0_RX_VT_LDPC_OFFSET          11
#define SOC7_0_RX_VT_FR_MODE_MASK         BITS(12, 14)
#define SOC7_0_RX_VT_FR_MODE_OFFSET       12
#define SOC7_0_RX_VT_GI_MASK              BITS(15, 16)
#define SOC7_0_RX_VT_GI_OFFSET            15
#define SOC7_0_RX_VT_DCM_MASK             BIT(17)
#define SOC7_0_RX_VT_DCM_OFFSET           17
#define SOC7_0_RX_VT_NUMRX_MASK           BITS(18, 20)
#define SOC7_0_RX_VT_NUMRX_OFFSET         18
#define SOC7_0_RX_VT_MUMIMO_MASK          BIT(21)
#define SOC7_0_RX_VT_MUMIMO_OFFSET        21
#define SOC7_0_RX_VT_STBC_MASK            BITS(22, 23)
#define SOC7_0_RX_VT_STBC_OFFSET          22
#define SOC7_0_RX_VT_TXMODE_MASK          BITS(24, 27)
#define SOC7_0_RX_VT_TXMODE_OFFSET        24

#define RXV_GET_RX_RATE(_prRxVector)				\
		(((_prRxVector) & SOC7_0_RX_VT_RX_RATE_MASK)	\
			 >> SOC7_0_RX_VT_RX_RATE_OFFSET)

#define RXV_GET_RX_NSTS(_prRxVector)				\
		(((_prRxVector) & SOC7_0_RX_VT_NSTS_MASK)	\
			 >> SOC7_0_RX_VT_NSTS_OFFSET)

#define RXV_GET_FR_MODE(_prRxVector)				\
		(((_prRxVector) & SOC7_0_RX_VT_FR_MODE_MASK)	\
			 >> SOC7_0_RX_VT_FR_MODE_OFFSET)

#define RXV_GET_GI(_prRxVector)					\
		(((_prRxVector) & SOC7_0_RX_VT_GI_MASK)	\
			 >> SOC7_0_RX_VT_GI_OFFSET)

#define RXV_GET_STBC(_prRxVector)				\
		(((_prRxVector) & SOC7_0_RX_VT_STBC_MASK)	\
			 >> SOC7_0_RX_VT_STBC_OFFSET)

#define RXV_GET_TXMODE(_prRxVector)				\
		(((_prRxVector) & SOC7_0_RX_VT_TXMODE_MASK)	\
			 >> SOC7_0_RX_VT_TXMODE_OFFSET)

#define RXV_GET_RX_MUMIMO(_prRxVector)				\
		(((_prRxVector) & SOC7_0_RX_VT_MUMIMO_MASK)	\
			 >> SOC7_0_RX_VT_MUMIMO_OFFSET)

extern struct PLE_TOP_CR rSoc7_0_PleTopCr;
extern struct PSE_TOP_CR rSoc7_0_PseTopCr;
extern struct PP_TOP_CR rSoc7_0_PpTopCr;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void soc7_0_show_wfdma_info(struct ADAPTER *prAdapter);
void soc7_0_show_ple_info(struct ADAPTER *prAdapter, u_int8_t fgDumpTxd);
void soc7_0_show_pse_info(struct ADAPTER *prAdapter);
bool soc7_0_show_host_csr_info(struct ADAPTER *prAdapter);
void soc7_0_show_wfdma_dbg_probe_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
void soc7_0_show_wfdma_wrapper_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
int soc7_0_Trigger_fw_assert(struct ADAPTER *prAdapter);
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
int soc7_0_get_rx_rate_info(const uint32_t *prRxV,
		struct RxRateInfo *prRxRateInfo);
#endif

#if CFG_SUPPORT_LLS
void soc7_0_get_rx_link_stats(struct ADAPTER *prAdapter,
	struct SW_RFB *prRetSwRfb, uint32_t *pu4RxV);
#endif

void soc7_0_icapRiseVcoreClockRate(void);
void soc7_0_icapDownVcoreClockRate(void);

#endif  /* soc7_0 */
