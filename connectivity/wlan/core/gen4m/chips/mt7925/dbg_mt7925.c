// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/******************************************************************************
 *[File]             dbg_mt7925.c
 *[Version]          v1.0
 *[Revision Date]    2020-05-22
 *[Author]
 *[Description]
 *    The program provides WIFI FALCON MAC Debug APIs
 *[Copyright]
 *    Copyright (C) 2015 MediaTek Incorporation. All Rights Reserved.
 ******************************************************************************/
#ifdef MT7925

#include "precomp.h"
#if (DBG_DISABLE_ALL_INFO == 0)
#include "mt7925.h"
#include "dbg_mt7925.h"
#if defined(_HIF_PCIE)
#include "hif_pdma.h"
#endif
#include "coda/mt7925/wf_ple_top.h"
#include "coda/mt7925/wf_pse_top.h"
#include "coda/mt7925/wf_wfdma_host_dma0.h"
#include "coda/mt7925/wf_hif_dmashdl_top.h"
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
#include "connv3.h"
#include "connectivity_build_in_adapter.h"
#endif

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
#if defined(_HIF_PCIE)
static void mt7925_dumpPcGprLog(struct ADAPTER *ad);
static void mt7925_dumpWfTopReg(struct ADAPTER *ad);
static void mt7925_dumpWfBusReg(struct ADAPTER *ad);
#endif

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#define MAP_DRIVER_SIDE(ADDR) (ADDR - (CONN_INFRA_REMAPPING_OFFSET))
#define PC_LOG_NUM_CE   32
#define GPR_LOG_NUM_CE  32
#define CURRENT_PC_IDX  0x22
#define PC_LOG_CTRL_IDX 0x20

struct dump_cr_set conninfra_bus_off_domain_dump_bf_list[] = {
	{
		TRUE,
		CONN_DBG_CTL_CONN_VON_VDNR_GLUE_BUS_P_D_N54_TIMEOUT_INFO_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_VON_VDNR_GLUE_BUS_P_D_N54_TIMEOUT_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_VON_VDNR_GLUE_BUS_P_D_N54_TIMEOUT_WDATA_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_VON_VDNR_GLUE_BUS_P_D_N57_TIMEOUT_INFO_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_VON_VDNR_GLUE_BUS_P_D_N57_TIMEOUT_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_VON_VDNR_GLUE_BUS_P_D_N57_TIMEOUT_WDATA_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ON_CONN_INFRA_ON_P_D_N7_TIMEOUT_INFO_0_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ON_CONN_INFRA_ON_P_D_N7_TIMEOUT_INFO_1_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ON_CONN_INFRA_ON_P_D_N7_TIMEOUT_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ON_CONN_INFRA_ON_P_D_N7_TIMEOUT_WDATA_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ON_CONN_INFRA_ON_P_D_N9_TIMEOUT_INFO_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ON_CONN_INFRA_ON_P_D_N9_TIMEOUT_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ON_CONN_INFRA_ON_P_D_N9_TIMEOUT_WDATA_ADDR,
		0xFFFFFFFF,
		0
	},
};

struct dump_cr_set conninfra_pwr_on_domain_dump_list[] = {
	{
		TRUE,
		0x7C060A10,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C060014,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C060054,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C060010,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C060050,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C060018,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C060058,
		0xFFFFFFFF,
		0
	}
};

struct dump_cr_set conninfra_pwr_off_domain_dump_list[] = {
	{
		TRUE,
		CONN_CFG_PLL_STATUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_CLKGEN_TOP_CKGEN_BUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_CFG_EMI_CTL_0_ADDR,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_MASK,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_SHFT,
		0
	},
	{
		TRUE,
		CONN_CFG_EMI_PROBE_1_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_CFG_EMI_CTL_0_ADDR,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_MASK,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_SHFT,
		0x1
	},
	{
		TRUE,
		CONN_CFG_EMI_PROBE_1_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_CFG_EMI_CTL_0_ADDR,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_MASK,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_SHFT,
		0x2
	},
	{
		TRUE,
		CONN_CFG_EMI_PROBE_1_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_CFG_EMI_CTL_0_ADDR,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_MASK,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_SHFT,
		0x3
	},
	{
		TRUE,
		CONN_CFG_EMI_PROBE_1_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C050C50,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C050C54,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C050C58,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C050C5C,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C050C60,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C050C64,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_RF_SPI_MST_REG_SPI_STA_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_RF_SPI_MST_REG_SPI_TOP_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_RF_SPI_MST_REG_SPI_TOP_WDAT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_RF_SPI_MST_REG_SPI_TOP_RDAT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C001620,
		0xFFFFFFFF,
		0
	}
	,
	{
		TRUE,
		0x7C001610,
		0xFFFFFFFF,
		0
	}
	,
	{
		TRUE,
		0x7C001600,
		0xFFFFFFFF,
		0
	}
};

struct dump_cr_set conninfra_pwr_adie_common_dump_list[] = {
	{
		TRUE,
		0xA10,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x090,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x08C,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x0A0,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x09C,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x094,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x5B4,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x2CC,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0xA70,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0xA20,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0xC08,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0xAA0,
		0xFFFFFFFF,
		0
	}
};

struct dump_cr_set conninfra_pwr_adie_7971_dump_list[] = {
	{
		TRUE,
		0x02C,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x000,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x750,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0xC50,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0xB08,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x0B4,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x580,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x30C,
		0xFFFFFFFF,
		0
	}
};

struct dump_cr_set conninfra_clk_off_domain_dump_list[] = {
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_MONFLAG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_CFG_EMI_PROBE_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_CFG_EMI_CTL_0_ADDR,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_MASK,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_SHFT,
		0
	},
	{
		TRUE,
		CONN_CFG_EMI_PROBE_1_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_CFG_EMI_CTL_0_ADDR,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_MASK,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_SHFT,
		1
	},
	{
		TRUE,
		CONN_CFG_EMI_PROBE_1_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_CFG_EMI_CTL_0_ADDR,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_MASK,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_SHFT,
		2
	},
	{
		TRUE,
		CONN_CFG_EMI_PROBE_1_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_CFG_EMI_CTL_0_ADDR,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_MASK,
		CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_SHFT,
		3
	},
	{
		TRUE,
		CONN_CFG_EMI_PROBE_1_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL9_ADDR,
		CONN_GPIO_MON_SEL9_MON_SYS_SEL_0_MASK,
		CONN_GPIO_MON_SEL9_MON_SYS_SEL_0_SHFT,
		0x14
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL9_ADDR,
		CONN_GPIO_MON_SEL9_MON_SYS_SEL_1_MASK,
		CONN_GPIO_MON_SEL9_MON_SYS_SEL_1_SHFT,
		0x17
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL1_ADDR,
		0xFFFFFFFF,
		0,
		0x03020100
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL2_ADDR,
		0xFFFFFFFF,
		0,
		0x07060504
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL3_ADDR,
		0xFFFFFFFF,
		0,
		0x0B0A0908
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL4_ADDR,
		0xFFFFFFFF,
		0,
		0x0F0E0D0C
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL5_ADDR,
		0xFFFFFFFF,
		0,
		0x13121110
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL6_ADDR,
		0xFFFFFFFF,
		0,
		0x17161514
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL7_ADDR,
		0xFFFFFFFF,
		0,
		0x1B1A1918
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL8_ADDR,
		0xFFFFFFFF,
		0,
		0x1F1E1D1C
	},
	{
		FALSE,
		CONN_GPIO_GPIO_MISC_CTRL_ADDR,
		CONN_GPIO_GPIO_MISC_CTRL_MON_FLAG_EN_MASK,
		CONN_GPIO_GPIO_MISC_CTRL_MON_FLAG_EN_SHFT,
		0x1
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_MONFLAG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL9_ADDR,
		CONN_GPIO_MON_SEL9_MON_SYS_SEL_0_MASK,
		CONN_GPIO_MON_SEL9_MON_SYS_SEL_0_SHFT,
		0x24
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL1_ADDR,
		0xFFFFFFFF,
		0,
		0x03020100
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL2_ADDR,
		0xFFFFFFFF,
		0,
		0x07060504
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL3_ADDR,
		0xFFFFFFFF,
		0,
		0x0B0A0908
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL4_ADDR,
		0xFFFFFFFF,
		0,
		0x0F0E0D0C
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL5_ADDR,
		0xFFFFFFFF,
		0,
		0x13121110
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL6_ADDR,
		0xFFFFFFFF,
		0,
		0x17161514
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL7_ADDR,
		0xFFFFFFFF,
		0,
		0x1B1A1918
	},
	{
		FALSE,
		CONN_GPIO_MON_SEL8_ADDR,
		0xFFFFFFFF,
		0,
		0x1F1E1D1C
	},
	{
		FALSE,
		CONN_GPIO_GPIO_MISC_CTRL_ADDR,
		CONN_GPIO_GPIO_MISC_CTRL_MON_FLAG_EN_MASK,
		CONN_GPIO_GPIO_MISC_CTRL_MON_FLAG_EN_SHFT,
		0x1
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_MONFLAG_OUT_ADDR,
		0xFFFFFFFF,
		0
	}
};

struct dump_cr_set conninfra_clk_on_domain_dump_list[] = {
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_SYSSTRAP_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_CLKGEN_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_ADDR,
		0x7,
		0,
		0
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_ADDR,
		0x7,
		0,
		1
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_ADDR,
		0x7,
		0,
		2
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_ADDR,
		0x7,
		0,
		3
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_ADDR,
		0x7,
		0,
		4
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_ADDR,
		0x7,
		0,
		5
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_ADDR,
		0x7,
		0,
		6
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_ADDR,
		0x7,
		0,
		7
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_ADDR,
		0xF,
		0,
		0
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_ADDR,
		0xF,
		0,
		1
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_ADDR,
		0xF,
		0,
		2
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_ADDR,
		0xF,
		0,
		3
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_ADDR,
		0xF,
		0,
		4
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_ADDR,
		0xF,
		0,
		5
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_ADDR,
		0xF,
		0,
		7
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_ADDR,
		0xF,
		0,
		8
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_ADDR,
		0xFFFFFFFF,
		0
	}
};

struct dump_cr_set conninfra_bus_off_domain_dump_af_list[] = {
	{
		TRUE,
		CONN_BUS_CR_ADDR_SUBSYS_0_ARADDR_LATCH_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ADDR_SUBSYS_0_AWADDR_LATCH_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ADDR_SUBSYS_1_HADDR_LATCH_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ADDR_SUBSYS_2_HADDR_LATCH_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ADDR_ACCESS_PROTECTOR_IRQ_STATUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C04F294,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C04F298,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C04F29C,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C04F2A0,
		0xFFFFFFFF,
		0
	},

/* Reference conn_vdnr_gen_u_debug_ctrl_ao_off_pwr_debug_ctrl_ao.h +. */
	{
		TRUE,
		0x7C049408,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C04940C,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C049410,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C049414,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C049418,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C04941C,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C049420,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C049424,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C049428,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C04942C,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C049430,
		0xFFFFFFFF,
		0
	},
/* Reference conn_vdnr_gen_u_debug_ctrl_ao_off_pwr_debug_ctrl_ao.h -. */
	{
		TRUE,
		CONN_BUS_CR_ADDR_CONN2AP_REMAP_EN_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_CONN_INFRA_LOW_POWER_LAYER_CTRL_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_ADDR_MCU_0_EMI_BASE_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_ADDR_MD_SHARED_BASE_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_HOST_CSR_TOP_ADDR_GPS_EMI_BASE_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ADDR_SCPSYS_SRAM_BASE_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ADDR_PERI_WF_BASE_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ADDR_PERI_BT_BASE_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ADDR_PERI_GPS_BASE_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		0x7C023408,
		0xFFFFFFFF,
		0,
		0X0000002E
	},
	{
		TRUE,
		0x7C023404,
		0xFFFFFFFF,
		0
	}
};

struct dump_cr_set conninfra_bus_off_domain_dump_af_list_c[] = {
	/* Section C */
	{
		TRUE,
		CONN_CFG_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_CFG_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_CFG_CONN_INFRA_OFF_BUS_SLP_STATUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_CFG_CONN_INFRA_WF_SLP_STATUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_CFG_GALS_CONN2BT_SLP_STATUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_CFG_GALS_BT2CONN_SLP_STATUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_CFG_GALS_CONN2GPS_SLP_STATUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_CFG_GALS_GPS2CONN_SLP_STATUS_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		0x7C01148C,
		0xFFFFFFFF,
		0
	}
	,
	{
		TRUE,
		0x7C01149C,
		0xFFFFFFFF,
		0
	}
	,
	{
		TRUE,
		0x7C0114AC,
		0xFFFFFFFF,
		0
	}
};

struct PLE_TOP_CR rMt7925PleTopCr = {
	.rAc0QueueEmpty0 = {
		WF_PLE_TOP_AC0_QUEUE_EMPTY0_ADDR,
		0,
		0
	},
	.rAc1QueueEmpty0 = {
		WF_PLE_TOP_AC1_QUEUE_EMPTY0_ADDR,
		0,
		0
	},
	.rAc2QueueEmpty0 = {
		WF_PLE_TOP_AC2_QUEUE_EMPTY0_ADDR,
		0,
		0
	},
	.rAc3QueueEmpty0 = {
		WF_PLE_TOP_AC3_QUEUE_EMPTY0_ADDR,
		0,
		0
	},
	.rCpuPgInfo = {
		WF_PLE_TOP_CPU_PG_INFO_ADDR,
		0,
		0
	},
	.rCpuPgInfoCpuRsvCnt = {
		0,
		WF_PLE_TOP_CPU_PG_INFO_CPU_RSV_CNT_MASK,
		WF_PLE_TOP_CPU_PG_INFO_CPU_RSV_CNT_SHFT
	},
	.rCpuPgInfoCpuSrcCnt = {
		0,
		WF_PLE_TOP_CPU_PG_INFO_CPU_SRC_CNT_MASK,
		WF_PLE_TOP_CPU_PG_INFO_CPU_SRC_CNT_SHFT
	},
	.rDisStaMap0 = {
		WF_PLE_TOP_DIS_STA_MAP0_ADDR,
		0,
		0
	},
	.rFlQueCtrl0 = {
		WF_PLE_TOP_FL_QUE_CTRL_0_ADDR,
		0,
		0
	},
	.rFlQueCtrl0Execute = {
		0,
		WF_PLE_TOP_FL_QUE_CTRL_0_EXECUTE_MASK,
		0
	},
	.rFlQueCtrl0QBufWlanid = {
		0,
		0,
		WF_PLE_TOP_FL_QUE_CTRL_0_Q_BUF_WLANID_SHFT
	},
	.rFlQueCtrl2 = {
		WF_PLE_TOP_FL_QUE_CTRL_2_ADDR,
		0,
		0
	},
	.rFlQueCtrl2QueueHeadFid = {
		0,
		WF_PLE_TOP_FL_QUE_CTRL_2_QUEUE_HEAD_FID_MASK,
		WF_PLE_TOP_FL_QUE_CTRL_2_QUEUE_HEAD_FID_SHFT
	},
	.rFlQueCtrl2QueueTailFid = {
		0,
		WF_PLE_TOP_FL_QUE_CTRL_2_QUEUE_TAIL_FID_MASK,
		WF_PLE_TOP_FL_QUE_CTRL_2_QUEUE_TAIL_FID_SHFT
	},
	.rFlQueCtrl3 = {
		WF_PLE_TOP_FL_QUE_CTRL_3_ADDR,
		0,
		0
	},
	.rFlQueCtrl3QueuePktNum = {
		0,
		WF_PLE_TOP_FL_QUE_CTRL_3_QUEUE_PKT_NUM_MASK,
		WF_PLE_TOP_FL_QUE_CTRL_3_QUEUE_PKT_NUM_SHFT
	},
	.rFreepgCnt = {
		WF_PLE_TOP_FREEPG_CNT_ADDR,
		0,
		0
	},
	.rFreepgCntFfaCnt = {
		0,
		WF_PLE_TOP_FREEPG_CNT_FFA_CNT_MASK,
		WF_PLE_TOP_FREEPG_CNT_FFA_CNT_SHFT
	},
	.rFreepgCntFreepgCnt = {
		0,
		WF_PLE_TOP_FREEPG_CNT_FREEPG_CNT_MASK,
		WF_PLE_TOP_FREEPG_CNT_FREEPG_CNT_SHFT
	},
	.rFreepgHeadTail = {
		WF_PLE_TOP_FREEPG_HEAD_TAIL_ADDR,
		0,
		0
	},
	.rFreepgHeadTailFreepgHead = {
		0,
		WF_PLE_TOP_FREEPG_HEAD_TAIL_FREEPG_HEAD_MASK,
		WF_PLE_TOP_FREEPG_HEAD_TAIL_FREEPG_HEAD_SHFT
	},
	.rFreepgHeadTailFreepgTail = {
		0,
		WF_PLE_TOP_FREEPG_HEAD_TAIL_FREEPG_TAIL_MASK,
		WF_PLE_TOP_FREEPG_HEAD_TAIL_FREEPG_TAIL_SHFT
	},
	.rFsmPeekCr00 = {
		WF_PLE_TOP_PEEK_CR_00_ADDR,
		0,
		0
	},
	.rFsmPeekCr01 = {
		WF_PLE_TOP_PEEK_CR_01_ADDR,
		0,
		0
	},
	.rFsmPeekCr02 = {
		WF_PLE_TOP_PEEK_CR_02_ADDR,
		0,
		0
	},
	.rFsmPeekCr03 = {
		WF_PLE_TOP_PEEK_CR_03_ADDR,
		0,
		0
	},
	.rFsmPeekCr04 = {
		WF_PLE_TOP_PEEK_CR_04_ADDR,
		0,
		0
	},
	.rFsmPeekCr05 = {
		WF_PLE_TOP_PEEK_CR_05_ADDR,
		0,
		0
	},
	.rFsmPeekCr06 = {
		WF_PLE_TOP_PEEK_CR_06_ADDR,
		0,
		0
	},
	.rFsmPeekCr07 = {
		WF_PLE_TOP_PEEK_CR_07_ADDR,
		0,
		0
	},
	.rFsmPeekCr08 = {
		WF_PLE_TOP_PEEK_CR_08_ADDR,
		0,
		0
	},
	.rFsmPeekCr09 = {
		WF_PLE_TOP_PEEK_CR_09_ADDR,
		0,
		0
	},
	.rFsmPeekCr10 = {
		WF_PLE_TOP_PEEK_CR_10_ADDR,
		0,
		0
	},
	.rFsmPeekCr11 = {
		WF_PLE_TOP_PEEK_CR_11_ADDR,
		0,
		0
	},
	.rHifPgInfo = {
		WF_PLE_TOP_HIF_PG_INFO_ADDR,
		0,
		0
	},
	.rHifPgInfoHifRsvCnt = {
		0,
		WF_PLE_TOP_HIF_PG_INFO_HIF_RSV_CNT_MASK,
		WF_PLE_TOP_HIF_PG_INFO_HIF_RSV_CNT_SHFT
	},
	.rHifPgInfoHifSrcCnt = {
		0,
		WF_PLE_TOP_HIF_PG_INFO_HIF_SRC_CNT_MASK,
		WF_PLE_TOP_HIF_PG_INFO_HIF_SRC_CNT_SHFT
	},
	.rHifTxcmdPgInfo = {
		WF_PLE_TOP_HIF_TXCMD_PG_INFO_ADDR,
		0,
		0
	},
	.rHifTxcmdPgInfoHifTxcmdRsvCnt = {
		0,
		WF_PLE_TOP_HIF_TXCMD_PG_INFO_HIF_TXCMD_RSV_CNT_MASK,
		WF_PLE_TOP_HIF_TXCMD_PG_INFO_HIF_TXCMD_RSV_CNT_SHFT
	},
	.rHifTxcmdPgInfoHifTxcmdSrcCnt = {
		0,
		WF_PLE_TOP_HIF_TXCMD_PG_INFO_HIF_TXCMD_SRC_CNT_MASK,
		WF_PLE_TOP_HIF_TXCMD_PG_INFO_HIF_TXCMD_SRC_CNT_SHFT
	},
	.rHifWmtxdPgInfo = {
		WF_PLE_TOP_HIF_WMTXD_PG_INFO_ADDR,
		0,
		0
	},
	.rHifWmtxdPgInfoHifWmtxdRsvCnt = {
		0,
		WF_PLE_TOP_HIF_WMTXD_PG_INFO_HIF_WMTXD_RSV_CNT_MASK,
		WF_PLE_TOP_HIF_WMTXD_PG_INFO_HIF_WMTXD_RSV_CNT_SHFT
	},
	.rHifWmtxdPgInfoHifWmtxdSrcCnt = {
		0,
		WF_PLE_TOP_HIF_WMTXD_PG_INFO_HIF_WMTXD_SRC_CNT_MASK,
		WF_PLE_TOP_HIF_WMTXD_PG_INFO_HIF_WMTXD_SRC_CNT_SHFT
	},
	.rIntN9ErrSts = {
		WF_PLE_TOP_INT_N9_ERR_STS_ADDR,
		0,
		0
	},
	.rIntN9ErrSts1 = {
		WF_PLE_TOP_INT_N9_ERR_STS_1_ADDR,
		0,
		0
	},
	.rPbufCtrl = {
		WF_PLE_TOP_PBUF_CTRL_ADDR,
		0,
		0
	},
	.rPbufCtrlPageSizeCfg = {
		0,
		WF_PLE_TOP_PBUF_CTRL_PAGE_SIZE_CFG_MASK,
		WF_PLE_TOP_PBUF_CTRL_PAGE_SIZE_CFG_SHFT
	},
	.rPbufCtrlPbufOffset = {
		0,
		WF_PLE_TOP_PBUF_CTRL_PBUF_OFFSET_MASK,
		WF_PLE_TOP_PBUF_CTRL_PBUF_OFFSET_SHFT
	},
	.rPbufCtrlTotalPageNum = {
		0,
		WF_PLE_TOP_PBUF_CTRL_TOTAL_PAGE_NUM_MASK,
		WF_PLE_TOP_PBUF_CTRL_TOTAL_PAGE_NUM_SHFT
	},
	.rPgCpuGroup = {
		WF_PLE_TOP_PG_CPU_GROUP_ADDR,
		0,
		0
	},
	.rPgCpuGroupCpuMaxQuota = {
		0,
		WF_PLE_TOP_PG_CPU_GROUP_CPU_MAX_QUOTA_MASK,
		WF_PLE_TOP_PG_CPU_GROUP_CPU_MAX_QUOTA_SHFT
	},
	.rPgCpuGroupCpuMinQuota = {
		0,
		WF_PLE_TOP_PG_CPU_GROUP_CPU_MIN_QUOTA_MASK,
		WF_PLE_TOP_PG_CPU_GROUP_CPU_MIN_QUOTA_SHFT
	},
	.rPgHifGroup = {
		WF_PLE_TOP_PG_HIF_GROUP_ADDR,
		0,
		0
	},
	.rPgHifGroupHifMaxQuota = {
		0,
		WF_PLE_TOP_PG_HIF_GROUP_HIF_MAX_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_GROUP_HIF_MAX_QUOTA_SHFT
	},
	.rPgHifGroupHifMinQuota = {
		0,
		WF_PLE_TOP_PG_HIF_GROUP_HIF_MIN_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_GROUP_HIF_MIN_QUOTA_SHFT
	},
	.rPgHifTxcmdGroup = {
		WF_PLE_TOP_PG_HIF_TXCMD_GROUP_ADDR,
		0,
		0
	},
	.rPgHifTxcmdGroupHifTxcmdMaxQuota = {
		0,
		WF_PLE_TOP_PG_HIF_TXCMD_GROUP_HIF_TXCMD_MAX_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_TXCMD_GROUP_HIF_TXCMD_MAX_QUOTA_SHFT
	},
	.rPgHifTxcmdGroupHifTxcmdMinQuota = {
		0,
		WF_PLE_TOP_PG_HIF_TXCMD_GROUP_HIF_TXCMD_MIN_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_TXCMD_GROUP_HIF_TXCMD_MIN_QUOTA_SHFT
	},
	.rPgHifWmtxdGroup = {
		WF_PLE_TOP_PG_HIF_WMTXD_GROUP_ADDR,
		0,
		0
	},
	.rPgHifWmtxdGroupHifWmtxdMaxQuota = {
		0,
		WF_PLE_TOP_PG_HIF_WMTXD_GROUP_HIF_WMTXD_MAX_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_WMTXD_GROUP_HIF_WMTXD_MAX_QUOTA_SHFT
	},
	.rPgHifWmtxdGroupHifWmtxdMinQuota = {
		0,
		WF_PLE_TOP_PG_HIF_WMTXD_GROUP_HIF_WMTXD_MIN_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_WMTXD_GROUP_HIF_WMTXD_MIN_QUOTA_SHFT
	},
	.rQueueEmpty = {
		WF_PLE_TOP_QUEUE_EMPTY_ADDR,
		0,
		0
	},
	.rQueueEmptyAllAcEmpty = {
		0,
		WF_PLE_TOP_QUEUE_EMPTY_ALL_AC_EMPTY_MASK,
		0
	},
#if 0
	.rStationPause0 = {
		WF_PLE_TOP_STATION_PAUSE0_ADDR,
		0,
		0
	},
#endif
	.rTxdQueueEmpty = {
		WF_PLE_TOP_TXD_QUEUE_EMPTY_ADDR,
		0,
		0
	},
	.rToN9IntToggle = {
		WF_PLE_TOP_TO_N9_INT_TOGGLE_ADDR,
		WF_PLE_TOP_TO_N9_INT_TOGGLE_MASK,
		WF_PLE_TOP_TO_N9_INT_TOGGLE_SHFT
	},
};

struct PSE_TOP_CR rMt7925PseTopCr = {
	.rFlQueCtrl0 = {
		WF_PSE_TOP_FL_QUE_CTRL_0_ADDR,
		0,
		0
	},
	.rFlQueCtrl0Execute = {
		0,
		WF_PSE_TOP_FL_QUE_CTRL_0_EXECUTE_MASK,
		0
	},
	.rFlQueCtrl0QBufQid = {
		0,
		0,
		WF_PSE_TOP_FL_QUE_CTRL_0_Q_BUF_QID_SHFT
	},
	.rFlQueCtrl2 = {
		WF_PSE_TOP_FL_QUE_CTRL_2_ADDR,
		0,
		0
	},
	.rFlQueCtrl2QueueHeadFid = {
		0,
		WF_PSE_TOP_FL_QUE_CTRL_2_QUEUE_HEAD_FID_MASK,
		WF_PSE_TOP_FL_QUE_CTRL_2_QUEUE_HEAD_FID_SHFT
	},
	.rFlQueCtrl2QueueTailFid = {
		0,
		WF_PSE_TOP_FL_QUE_CTRL_2_QUEUE_TAIL_FID_MASK,
		WF_PSE_TOP_FL_QUE_CTRL_2_QUEUE_TAIL_FID_SHFT
	},
	.rFlQueCtrl3 = {
		WF_PSE_TOP_FL_QUE_CTRL_3_ADDR,
		0,
		0
	},
	.rFlQueCtrl3QueuePktNum = {
		0,
		WF_PSE_TOP_FL_QUE_CTRL_3_QUEUE_PKT_NUM_MASK,
		WF_PSE_TOP_FL_QUE_CTRL_3_QUEUE_PKT_NUM_SHFT
	},
	.rFreepgCnt = {
		WF_PSE_TOP_FREEPG_CNT_ADDR,
		0,
		0
	},
	.rFreepgCntFfaCnt = {
		0,
		WF_PSE_TOP_FREEPG_CNT_FFA_CNT_MASK,
		WF_PSE_TOP_FREEPG_CNT_FFA_CNT_SHFT
	},
	.rFreepgCntFreepgCnt = {
		0,
		WF_PSE_TOP_FREEPG_CNT_FREEPG_CNT_MASK,
		WF_PSE_TOP_FREEPG_CNT_FREEPG_CNT_SHFT
	},
	.rFreepgHeadTail = {
		WF_PSE_TOP_FREEPG_HEAD_TAIL_ADDR,
		0,
		0
	},
	.rFreepgHeadTailFreepgHead = {
		0,
		WF_PSE_TOP_FREEPG_HEAD_TAIL_FREEPG_HEAD_MASK,
		WF_PSE_TOP_FREEPG_HEAD_TAIL_FREEPG_HEAD_SHFT
	},
	.rFreepgHeadTailFreepgTail = {
		0,
		WF_PSE_TOP_FREEPG_HEAD_TAIL_FREEPG_TAIL_MASK,
		WF_PSE_TOP_FREEPG_HEAD_TAIL_FREEPG_TAIL_SHFT
	},
	.rFsmPeekCr00 = {
		WF_PSE_TOP_PSE_SEEK_CR_00_ADDR,
		0,
		0
	},
	.rFsmPeekCr01 = {
		WF_PSE_TOP_PSE_SEEK_CR_01_ADDR,
		0,
		0
	},
	.rFsmPeekCr02 = {
		WF_PSE_TOP_PSE_SEEK_CR_02_ADDR,
		0,
		0
	},
	.rFsmPeekCr03 = {
		WF_PSE_TOP_PSE_SEEK_CR_03_ADDR,
		0,
		0
	},
	.rFsmPeekCr04 = {
		WF_PSE_TOP_PSE_SEEK_CR_04_ADDR,
		0,
		0
	},
	.rFsmPeekCr05 = {
		WF_PSE_TOP_PSE_SEEK_CR_05_ADDR,
		0,
		0
	},
	.rFsmPeekCr06 = {
		WF_PSE_TOP_PSE_SEEK_CR_06_ADDR,
		0,
		0
	},
	.rFsmPeekCr07 = {
		WF_PSE_TOP_PSE_SEEK_CR_07_ADDR,
		0,
		0
	},
	.rFsmPeekCr08 = {
		WF_PSE_TOP_PSE_SEEK_CR_08_ADDR,
		0,
		0
	},
	.rFsmPeekCr09 = {
		WF_PSE_TOP_PSE_SEEK_CR_09_ADDR,
		0,
		0
	},
	.rHif0PgInfoHif0RsvCnt = {
		0,
		WF_PSE_TOP_HIF0_PG_INFO_HIF0_RSV_CNT_MASK,
		WF_PSE_TOP_HIF0_PG_INFO_HIF0_RSV_CNT_SHFT
	},
	.rHif0PgInfoHif0SrcCnt = {
		0,
		WF_PSE_TOP_HIF0_PG_INFO_HIF0_SRC_CNT_MASK,
		WF_PSE_TOP_HIF0_PG_INFO_HIF0_SRC_CNT_SHFT
	},
	.rIntN9Err1Sts = {
		WF_PSE_TOP_INT_N9_ERR1_STS_ADDR,
		0,
		0
	},
	.rIntN9ErrSts = {
		WF_PSE_TOP_INT_N9_ERR_STS_ADDR,
		0,
		0
	},
	.rPbufCtrl = {
		WF_PSE_TOP_PBUF_CTRL_ADDR,
		0,
		0
	},
	.rPbufCtrlPageSizeCfg = {
		0,
		WF_PSE_TOP_PBUF_CTRL_PAGE_SIZE_CFG_MASK,
		WF_PSE_TOP_PBUF_CTRL_PAGE_SIZE_CFG_SHFT
	},
	.rPbufCtrlPbufOffset = {
		0,
		WF_PSE_TOP_PBUF_CTRL_PBUF_OFFSET_MASK,
		WF_PSE_TOP_PBUF_CTRL_PBUF_OFFSET_SHFT
	},
	.rPbufCtrlTotalPageNum = {
		0,
		WF_PSE_TOP_PBUF_CTRL_TOTAL_PAGE_NUM_MASK,
		WF_PSE_TOP_PBUF_CTRL_TOTAL_PAGE_NUM_SHFT
	},
	.rPgHif0GroupHif0MaxQuota = {
		0,
		WF_PSE_TOP_PG_HIF0_GROUP_HIF0_MAX_QUOTA_MASK,
		WF_PSE_TOP_PG_HIF0_GROUP_HIF0_MAX_QUOTA_SHFT
	},
	.rPgHif0GroupHif0MinQuota = {
		0,
		WF_PSE_TOP_PG_HIF0_GROUP_HIF0_MIN_QUOTA_MASK,
		WF_PSE_TOP_PG_HIF0_GROUP_HIF0_MIN_QUOTA_SHFT
	},
	.rQueueEmpty = {
		WF_PSE_TOP_QUEUE_EMPTY_ADDR,
		WF_PSE_TOP_QUEUE_EMPTY_MASK_ADDR,
		0
	},
	.rQueueEmptyCpuQ0Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q0_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q0_EMPTY_SHFT
	},
	.rQueueEmptyCpuQ1Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q1_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q1_EMPTY_SHFT
	},
	.rQueueEmptyCpuQ2Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q2_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q2_EMPTY_SHFT
	},
	.rQueueEmptyCpuQ3Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q3_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q3_EMPTY_SHFT
	},
	.rQueueEmptyHif0Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_0_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_0_EMPTY_SHFT
	},
	.rQueueEmptyHif10Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_10_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_10_EMPTY_SHFT
	},
	.rQueueEmptyHif11Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_11_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_11_EMPTY_SHFT
	},
	.rQueueEmptyHif1Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_1_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_1_EMPTY_SHFT
	},
	.rQueueEmptyHif2Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_2_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_2_EMPTY_SHFT
	},
	.rQueueEmptyHif3Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_3_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_3_EMPTY_SHFT
	},
	.rQueueEmptyHif4Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_4_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_4_EMPTY_SHFT
	},
	.rQueueEmptyHif5Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_5_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_5_EMPTY_SHFT
	},
	.rQueueEmptyHif6Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_6_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_6_EMPTY_SHFT
	},
	.rQueueEmptyHif7Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_7_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_7_EMPTY_SHFT
	},
	.rQueueEmptyHif8Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_8_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_8_EMPTY_SHFT
	},
	.rQueueEmptyHif9Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_9_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_9_EMPTY_SHFT
	},
	.rQueueEmptyLmacTxQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_LMAC_TX_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_LMAC_TX_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpRxQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RX_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RX_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpRxioc1QueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RXIOC1_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RXIOC1_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpRxiocQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RXIOC_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RXIOC_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpTx1QueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TX1_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TX1_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpTxQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TX_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TX_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpTxioc1QueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TXIOC1_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TXIOC1_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpTxiocQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TXIOC_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TXIOC_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyRlsQEmtpy = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_RLS_Q_EMTPY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_RLS_Q_EMTPY_SHFT
	},
	.rQueueEmptySecRxQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_RX_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_RX_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptySecTx1QueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_TX1_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_TX1_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptySecTxQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_TX_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_TX_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptySfdParkQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_SFD_PARK_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_SFD_PARK_QUEUE_EMPTY_SHFT
	},
};

struct PP_TOP_CR rMt7925PpTopCr = {
	.rDbgCtrl = {
		WF_PP_TOP_DBG_CTRL_ADDR,
		0,
		0
	},
	.rDbgCs0 = {
		WF_PP_TOP_DBG_CS_0_ADDR,
		0,
		0
	},
	.rDbgCs1 = {
		WF_PP_TOP_DBG_CS_1_ADDR,
		0,
		0
	},
	.rDbgCs2 = {
		WF_PP_TOP_DBG_CS_2_ADDR,
		0,
		0
	},
};

struct dump_cr_set conninfra_bus_on_domain_dump_list[] = {
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_SEL_ADDR,
		0xFFFFFFFF,
		0,
		0
	},
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_DEBUGSYS_CTRL_ADDR,
		0xFFFFFFFF,
		0,
		0x00010001
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_DEBUGSYS_CTRL_ADDR,
		0xFFFFFFFF,
		0,
		0x00020001
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_DEBUGSYS_CTRL_ADDR,
		0xFFFFFFFF,
		0,
		0x00030001
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_DEBUGSYS_CTRL_ADDR,
		0xFFFFFFFF,
		0,
		0x00010002
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_DEBUGSYS_CTRL_ADDR,
		0xFFFFFFFF,
		0,
		0x00000003
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_DEBUGSYS_CTRL_ADDR,
		0xFFFFFFFF,
		0,
		0x00010003
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_DEBUGSYS_CTRL_ADDR,
		0xFFFFFFFF,
		0,
		0x00020003
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_DEBUGSYS_CTRL_ADDR,
		0xFFFFFFFF,
		0,
		0x00030003
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_DEBUGSYS_CTRL_ADDR,
		0xFFFFFFFF,
		0,
		0x00010004
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_DEBUGSYS_CTRL_ADDR,
		0xFFFFFFFF,
		0,
		0x00020004
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		FALSE,
		CONN_DBG_CTL_CONN_INFRA_OFF_DEBUGSYS_CTRL_ADDR,
		0xFFFFFFFF,
		0,
		0x00030004
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_INFRA_BUS_TIMEOUT_IRQ_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_VON_VDNR_GLUE_BUS_S2P_N6_TIMEOUT_INFO_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_VON_VDNR_GLUE_BUS_S2P_N6_TIMEOUT_ADDR_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_VON_VDNR_GLUE_BUS_S2P_N6_TIMEOUT_WDATA_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_DBG_CTL_CONN_DEADFEED_PROT_EN_ADDR,
		0xFFFFFFFF,
		0
	},
	{
		TRUE,
		CONN_BUS_CR_ON_ADDR_AP2CONN_AHB_GALS_DBG_ADDR,
		0xFFFFFFFF,
		0
	},
};

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
void mt7925_show_wfdma_dbg_probe_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type)
{
	uint32_t dbg_cr_idx[] = {0x0, 0x1, 0x2, 0x3, 0x30, 0x5, 0x7, 0xA, 0xB,
		0xC};
	uint32_t i = 0, u4DbgIdxAddr = 0, u4DbgProbeAddr = 0, u4DbgIdxValue = 0,
		u4DbgProbeValue = 0;

	if (!prAdapter)
		return;

	if (enum_wfdma_type != WFDMA_TYPE_HOST)
		return;

	u4DbgIdxAddr = WF_WFDMA_HOST_DMA0_WPDMA_DBG_IDX_ADDR;
	u4DbgProbeAddr = WF_WFDMA_HOST_DMA0_WPDMA_DBG_PROBE_ADDR;

	for (i = 0; i < ARRAY_SIZE(dbg_cr_idx); i++) {
		u4DbgIdxValue = 0x100 + dbg_cr_idx[i];
		HAL_MCR_WR(prAdapter, u4DbgIdxAddr, u4DbgIdxValue);
		HAL_MCR_RD(prAdapter, u4DbgProbeAddr, &u4DbgProbeValue);
		DBGLOG(HAL, DEBUG, "\t Write(0x%2x) DBG_PROBE[0x%X]=0x%08X\n",
			u4DbgIdxValue, u4DbgProbeAddr, u4DbgProbeValue);
	}
}

void mt7925_show_wfdma_wrapper_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type)
{
	uint32_t u4DmaCfgCr = 0;
	uint32_t u4RegValue = 0;

	if (!prAdapter)
		return;

	if (enum_wfdma_type == WFDMA_TYPE_HOST) {
		u4DmaCfgCr = 0x7c027044;
		HAL_MCR_RD(prAdapter, u4DmaCfgCr, &u4RegValue);
		DBGLOG(INIT, DEBUG, "WFDMA_HIF_BUSY(0x%08x): 0x%08x\n",
				u4DmaCfgCr,
				u4RegValue);

		u4DmaCfgCr = 0x7c027050;
		HAL_MCR_RD(prAdapter, u4DmaCfgCr, &u4RegValue);
		DBGLOG(INIT, DEBUG, "WFDMA_AXI_SLPPROT_CTRL(0x%08x): 0x%08x\n",
				u4DmaCfgCr,
				u4RegValue);

		u4DmaCfgCr = 0x7c027078;
		HAL_MCR_RD(prAdapter, u4DmaCfgCr, &u4RegValue);
		DBGLOG(INIT, DEBUG, "WFDMA_AXI_SLPPROT0_CTRL(0x%08x): 0x%08x\n",
				u4DmaCfgCr,
				u4RegValue);

		u4DmaCfgCr = 0x7c02707C;
		HAL_MCR_RD(prAdapter, u4DmaCfgCr, &u4RegValue);
		DBGLOG(INIT, DEBUG, "WFDMA_AXI_SLPPROT1_CTRL(0x%08x): 0x%08x\n",
				u4DmaCfgCr,
				u4RegValue);
	}
}

#if defined(_HIF_PCIE)
void mt7925_dumpCbtopReg(struct ADAPTER *ad)
{
	uint32_t u4Value = 0;

	DBGLOG(HAL, DEBUG, "Start mt6639_dumpCbtopReg.\n");

	/* 1. dump 0x18023C00[31:0] -> 0x7c023c00 */
	HAL_MCR_RD(ad, 0x7c023c00, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x7c023c00] value[0x%08x]\n", u4Value);

	/* 2. dump 0x70007204, read patch version */
	HAL_MCR_RD(ad, 0x70007204, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x70007204] value[0x%08x]\n", u4Value);

	/* 3. dump 0x7002500C, cb_infra_slp_status */
	HAL_MCR_RD(ad, 0x7002500C, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x7002500C] value[0x%08x]\n", u4Value);

	/* 4. dump slp_ctrl setting: 0x70025004/0x7002500C/0x70025014 */
	HAL_MCR_RD(ad, 0x70025004, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x70025004] value[0x%08x]\n", u4Value);
	HAL_MCR_RD(ad, 0x7002500C, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x7002500C] value[0x%08x]\n", u4Value);
	HAL_MCR_RD(ad, 0x70025014, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x70025014] value[0x%08x]\n", u4Value);

	/* 5. dump slp_ctrl cnt: 0x70025400/0x70025404 */
	HAL_MCR_RD(ad, 0x70025400, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x70025400] value[0x%08x]\n", u4Value);
	HAL_MCR_RD(ad, 0x70025404, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x70025404] value[0x%08x]\n", u4Value);

	/* 6. dump ap2conn gals dbg : 0x70026008 */
	HAL_MCR_RD(ad, 0x70026008, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x70026008] value[0x%08x]\n", u4Value);

	/* 7. dump conn2ap gals dbg : 0x70026000 */
	HAL_MCR_RD(ad, 0x70026000, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x70026000] value[0x%08x]\n", u4Value);

	/* 8. dump dma2ap gals dbg : 0x70026100 */
	HAL_MCR_RD(ad, 0x70026100, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x70026100] value[0x%08x]\n", u4Value);

	/* 9. dump 0x70025300  // debug index */
	HAL_MCR_RD(ad, 0x70025300, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x70025300] value[0x%08x]\n", u4Value);

	/* 10. dump 0x70026550 // debug index */
	HAL_MCR_RD(ad, 0x70026550, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x70026550] value[0x%08x]\n", u4Value);

	/* 11. dump 0x7002801C // debug index */
	HAL_MCR_RD(ad, 0x7002801C, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x7002801C] value[0x%08x]\n", u4Value);

	/* 12. write 0x70003020 = 0x0 // set vlp_uds_ctrl probe 00 */
	HAL_MCR_WR(ad, 0x70003020, 0);

	/* 13. write 0x70007150 = 0x2 */
	HAL_MCR_WR(ad, 0x70007150, 0x2);

	/* 14. CBTOP REGs dump 0x70007154 */
	HAL_MCR_RD(ad, 0x70007154, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x70007154] value[0x%08x]\n", u4Value);

	/* 15.PCIE LTSSM dump: 0x74030150 */
	HAL_MCR_RD(ad, 0x74030150, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x74030150] value[0x%08x]\n", u4Value);

	/* 16. PCIE LP state dump: 0x74030154 */
	HAL_MCR_RD(ad, 0x74030154, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x74030154] value[0x%08x]\n", u4Value);

	/* 17. PCIE INT status dump: 0x74030184 */
	HAL_MCR_RD(ad, 0x74030184, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x74030184] value[0x%08x]\n", u4Value);

	/* 18. PCIE Cfg BAR check: 0x74031010 */
	HAL_MCR_RD(ad, 0x74031010, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x74031010] value[0x%08x]\n", u4Value);

	/* 19. write PCIE debug partition: 0x74030168 = 0x22cc0100 */
	HAL_MCR_WR(ad, 0x74030168, 0x22cc0100);

	/* 20. write PCIE debug bus: 0x74030164 = 0x81804845 */
	HAL_MCR_WR(ad, 0x74030164, 0x81804845);

	/* 21. PCIE debug monitor dump: 0x7403002C */
	HAL_MCR_RD(ad, 0x7403002C, &u4Value);
	DBGLOG(HAL, DEBUG, "CR[0x7403002C] value[0x%08x]\n", u4Value);
}
void mt7925_dumpWfsyscpupcr(struct ADAPTER *ad)
{
#define CPUPCR_LOG_NUM	5
#define CPUPCR_BUF_SZ	50

	uint32_t i = 0;
	uint32_t var_pc = 0;
	uint32_t var_lp = 0;
	uint64_t log_sec = 0;
	uint64_t log_nsec = 0;
	char log_buf_pc[CPUPCR_LOG_NUM][CPUPCR_BUF_SZ];
	char log_buf_lp[CPUPCR_LOG_NUM][CPUPCR_BUF_SZ];

	HAL_MCR_WR_FIELD(ad,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR,
		0x3F,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_WF_MCU_DBG_PC_LOG_SHFT,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_WF_MCU_DBG_PC_LOG_MASK);
	HAL_MCR_WR_FIELD(ad,
		CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_ADDR,
		0x3F,
		CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_WF_MCU_DBG_GPR_LOG_SEL_SHFT,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_WF_MCU_DBG_PC_LOG_MASK);
	HAL_MCR_WR_FIELD(ad,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_ADDR,
		0x0,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_SHFT,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_MASK);

	for (i = 0; i < CPUPCR_LOG_NUM; i++) {
		log_sec = kalGetTimeTickNs();
		log_nsec = do_div(log_sec, 1000000000)/1000;
		HAL_MCR_RD(ad,
			   CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR,
			   &var_pc);
		HAL_MCR_RD(ad,
			   CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR,
			   &var_lp);

		kalSnprintf(log_buf_pc[i],
			    CPUPCR_BUF_SZ,
			    "%llu.%06llu/0x%08x;",
			    log_sec,
			    log_nsec,
			    var_pc);

		kalSnprintf(log_buf_lp[i],
			    CPUPCR_BUF_SZ,
			    "%llu.%06llu/0x%08x;",
			    log_sec,
			    log_nsec,
			    var_lp);
	}

	DBGLOG(HAL, DEBUG, "wm pc=%s%s%s%s%s\n",
		log_buf_pc[0],
		log_buf_pc[1],
		log_buf_pc[2],
		log_buf_pc[3],
		log_buf_pc[4]);

	DBGLOG(HAL, DEBUG, "wm lp=%s%s%s%s%s\n",
		log_buf_lp[0],
		log_buf_lp[1],
		log_buf_lp[2],
		log_buf_lp[3],
		log_buf_lp[4]);
}

static void mt7925_dumpPcGprLog(struct ADAPTER *ad)
{
#define PC_LOG_NUM			35
#define GPR_LOG_NUM			35

	uint32_t i = 0;
	uint32_t pc_dump[PC_LOG_NUM];
	uint32_t gpr_dump[GPR_LOG_NUM];

	DBGLOG(HAL, DEBUG, "Dump PC log / GPR log\n");

	HAL_MCR_WR_FIELD(ad,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_ADDR,
		0x0,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_SHFT,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_MASK);

	kalMemZero(pc_dump, sizeof(pc_dump));
	for (i = 0; i < PC_LOG_NUM; i++) {
		HAL_MCR_WR_FIELD(ad,
			CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_ADDR,
			i,
			0,
			0x0000003F);
		HAL_MCR_RD(ad,
			   CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR,
			   &pc_dump[i]);
	}
	connac3x_dump_format_memory32(pc_dump, PC_LOG_NUM, "PC log");

	kalMemZero(gpr_dump, sizeof(gpr_dump));
	for (i = 0; i < GPR_LOG_NUM; i++) {
		HAL_MCR_WR_FIELD(ad,
			CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_ADDR,
			i,
			0,
			0x0000003F);
		HAL_MCR_RD(ad,
			   CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR,
			   &gpr_dump[i]);
	}
	connac3x_dump_format_memory32(gpr_dump, GPR_LOG_NUM, "GPR log");
}

static void mt7925_dumpWfTopMiscOn(struct ADAPTER *ad)
{
	uint32_t u4WrVal = 0, u4Val = 0, u4Idx, u4RdAddr, u4WrAddr;
	uint32_t indexA = 1;
	uint32_t au4List[] = {
		0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000004,
		0x00000010, 0x00000012, 0x00000017, 0x00000018, 0x00000019,
		0x0000001A, 0x0000001B, 0x0000001D
	};

	u4WrAddr = CONN_HOST_CSR_TOP_WF_ON_MONFLG_EN_FR_HIF_ADDR;
	u4WrVal = 0x00000001;
	HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
	DBGLOG(HAL, DEBUG,
		"=PSOP_3_1_A%d=0x%08X=0x%08X\n",
		indexA,
		u4Val,
		u4WrAddr);
	indexA++;

	u4WrAddr = CONN_HOST_CSR_TOP_WF_ON_MONFLG_SEL_FR_HIF_ADDR;
	u4RdAddr = CONN_HOST_CSR_TOP_WF_ON_MONFLG_OUT_ADDR;
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4WrVal = au4List[u4Idx];
		HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	DBGLOG(HAL, DEBUG,
		"=PSOP_3_1_A%d=0x%08X=0x%08X\n",
		indexA,
		u4Val,
		u4RdAddr);
	indexA++;
	}
}

static void mt7925_dumpWfTopMiscVon(struct ADAPTER *ad)
{
	uint32_t u4WrVal = 0, u4Val = 0, u4Idx, u4RdAddr, u4WrAddr;
	uint32_t indexB = 1;
	uint32_t au4List[] = {
		0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000004,
		0x00000008
	};

	u4WrAddr = CONN_HOST_CSR_TOP_ADDR_WF_VON_MONFLG_EN_FR_HIF_ADDR;
	u4WrVal = 0x00000001;
	HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
	DBGLOG(HAL, DEBUG,
	       "\tW 0x%08x=[0x%08x]\n",
	       u4WrAddr, u4WrVal);

	u4WrAddr = CONN_HOST_CSR_TOP_ADDR_WF_VON_MONFLG_SEL_FR_HIF_ADDR;
	u4RdAddr = CONN_DBG_CTL_WF_VON_DEBUG_OUT_ADDR;
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4WrVal = au4List[u4Idx];
		HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
		DBGLOG(HAL, DEBUG,
			"=PSOP_3_1_B%d=0x%08X=0x%08X\n",
			indexB,
			u4Val,
			u4RdAddr);
		indexB++;
	}
}

static void mt7925_dumpWfTopCfgon(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0, u4Idx;
	uint32_t indexC = 1;
	uint32_t au4List[] = {
		WF_TOP_CFG_ON_DEBUG_FLAG0_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG1_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG2_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG3_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG4_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG5_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG6_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG7_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG8_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG9_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG10_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG11_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG12_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG13_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG14_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG15_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG16_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG17_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG18_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG19_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG20_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG21_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG22_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG23_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG24_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG25_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG26_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG27_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG28_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG29_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG30_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG31_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG32_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG33_ADDR,
	};

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4RdAddr = au4List[u4Idx];
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
		DBGLOG(HAL, DEBUG,
			"=PSOP_3_1_C%d=0x%08X=0x%08X\n",
			indexC,
			u4Val,
			u4RdAddr);
		indexC++;
	}
}

static void mt7925_dumpWfTopReg(struct ADAPTER *ad)
{
	/* Section A: Dump wf_top_misc_on monflag */
	mt7925_dumpWfTopMiscOn(ad);

	/* Section B: Dump wf_top_misc_von monflag */
	mt7925_dumpWfTopMiscVon(ad);

	/* Section C: Dump wf_top_cfg_on debug CR */
	mt7925_dumpWfTopCfgon(ad);
}

static void mt7925_dumpHostVdnrTimeoutInfo(struct ADAPTER *ad)
{
	uint32_t u4WrVal = 0, u4Val = 0, u4Idx, u4RdAddr, u4WrAddr;
	uint32_t indexA = 1;
	uint32_t au4List[] = {
		0x00010001, 0x00020001, 0x00030001, 0x00040001, 0x00050001,
		0x00060001, 0x00070001, 0x00080001, 0x00090001, 0x00010002,
		0x00020002, 0x00030002, 0x00040002
	};

	u4RdAddr = WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_BUS_TIMEOUT_IRQ_ADDR;
	u4WrAddr = CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_ADDR;
	u4WrVal = 0x4;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
	DBGLOG(HAL, DEBUG,
		"=PSOP_4_1_A%d=0x%08X=0x%08X\n",
		indexA,
		u4Val,
		u4RdAddr);
	indexA++;

	u4WrAddr = WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_DEBUGSYS_CTRL_ADDR;
	u4RdAddr = CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR;
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4WrVal = au4List[u4Idx];
		HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
		DBGLOG(HAL, DEBUG,
			"=PSOP_4_1_A%d=0x%08X=0x%08X\n",
			indexA,
			u4Val,
			u4RdAddr);
		indexA++;
	}
}

static void mt7925_dumpWfVdnrTimeoutInfo(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0, u4Idx;
	uint32_t indexB = 0, u4WrVal, u4WrAddr;
	uint32_t au4List[] = {
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT2_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT3_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT4_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT5_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT6_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT7_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT8_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT9_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT10_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT11_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT12_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT13_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT14_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT15_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_CTRL0_ADDR,
	};

	u4RdAddr = 0x1802362C;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	DBGLOG(HAL, DEBUG,
		"=PSOP_4_1_B%d=0x%08X=0x%08X\n",
		indexB,
		u4Val,
		u4RdAddr);

	u4WrAddr = 0x18023604;
	u4WrVal = 0x4;
	HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
	indexB++;

	u4WrAddr = 0x18400120;
	u4WrVal = 0x810F0000;
	HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
	indexB++;

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4RdAddr = au4List[u4Idx];
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
		DBGLOG(HAL, DEBUG,
			"=PSOP_4_1_B%d=0x%08X=0x%08X\n",
			indexB,
			u4Val,
			u4RdAddr);
		indexB++;
	}
}

static void mt7925_dumpAhbApbTimeoutInfo(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0, u4Idx, u4WrAddr, u4WrVal;
	uint32_t indexC = 0;
	uint32_t au4List[] = {
		CONN_MCU_BUS_CR_AHB_APB_TIMEOUT_ADDR_ADDR,
		CONN_MCU_BUS_CR_AHB_APB_TIMEOUT_INFO_ADDR,
		CONN_MCU_BUS_CR_AHB_APB_TIMEOUT_ID_ADDR,
		CONN_MCU_BUS_CR_AHB_APB_TIMEOUT_LYR_ADDR,
		CONN_MCU_BUS_CR_AHB_APB_TIMEOUT_CTRL_ADDR
	};

	u4WrAddr = 0x18400120;
	u4WrVal = 0x830C0000;
	HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
	indexC++;

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4RdAddr = au4List[u4Idx];
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
		DBGLOG(HAL, DEBUG,
			"=PSOP_4_1_C%d=0x%08X=0x%08X\n",
			indexC,
			u4Val,
			u4RdAddr);
		indexC++;
	}
}

static void mt7925_dumpWfBusReg(struct ADAPTER *ad)
{
	/* Section A: Dump VDNR timeout host side info */
	mt7925_dumpHostVdnrTimeoutInfo(ad);

	/* Section B: Dump VDNR timeout wf side info */
	mt7925_dumpWfVdnrTimeoutInfo(ad);

	/* Section C: Dump AHB APB timeout info */
	mt7925_dumpAhbApbTimeoutInfo(ad);
}

void mt7925_DumpBusStatus(struct ADAPTER *ad)
{
	if (!ad) {
		DBGLOG(HAL, ERROR, "NULL ADAPTER.\n");
		return;
	}
	mt7925_dumpCbtopReg(ad);
	mt7925_dumpWfsyscpupcr(ad);
	mt7925_dumpPcGprLog(ad);
	mt7925_dumpWfTopReg(ad);
	mt7925_dumpWfBusReg(ad);
}
#endif
#ifdef CFG_SUPPORT_LINK_QUALITY_MONITOR
int mt7925_get_rx_rate_info(const uint32_t *prRxV,
	struct RxRateInfo *prRxRateInfo)
{
	uint32_t rxmode = 0, rate = 0, frmode = 0, sgi = 0, nsts = 0;
	uint32_t stbc = 0, nss = 0;

	if (!prRxRateInfo || !prRxV)
		return -1;

	if (prRxV[0] == 0) {
		DBGLOG(SW4, WARN, "u4RxV0 is 0\n");
		return -1;
	}

	rate = RXV_GET_RX_RATE(prRxV[0]);
	nsts = RXV_GET_RX_NSTS(prRxV[0]);
	rxmode = RXV_GET_TXMODE(prRxV[2]);
	frmode = RXV_GET_FR_MODE(prRxV[2]);
	sgi = RXV_GET_GI(prRxV[2]);
	stbc = RXV_GET_STBC(prRxV[2]);

	nsts += 1;
	if (nsts == 1)
		nss = nsts;
	else
		nss = stbc ? (nsts >> 1) : nsts;

	if (frmode > 5) { /* 320M */
		DBGLOG(SW4, ERROR, "frmode error: %u\n", frmode);
		return -1;
	}

	prRxRateInfo->u4Rate = rate;
	prRxRateInfo->u4Nss = nss;
	prRxRateInfo->u4Mode = rxmode;
	prRxRateInfo->u4Bw = frmode;
	prRxRateInfo->u4Gi = sgi;

	DBGLOG(SW4, TRACE,
		   "rxvec0=[0x%x] rxmode=[%u], rate=[%u], bw=[%u], sgi=[%u], nss=[%u]\n",
		   prRxV[0], rxmode, rate, frmode, sgi, nss
	);

	return 0;
}
#endif

void mt7925_get_rx_link_stats(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb, uint32_t *pu4RxV)
{
#if CFG_SUPPORT_LLS
	static const uint8_t TX_MODE_2_LLS_MODE[] = {
		LLS_MODE_CCK,
		LLS_MODE_OFDM,	/* LG OFDM */
		LLS_MODE_HT,	/* MM */
		LLS_MODE_HT,	/* GF */
		LLS_MODE_VHT,
		LLS_MODE_RESERVED,
		LLS_MODE_RESERVED,
		LLS_MODE_RESERVED,
		LLS_MODE_HE,	/* HE-SU */
		LLS_MODE_HE,	/* HE-ER */
		LLS_MODE_HE,	/* HE-TB */
		LLS_MODE_HE,	/* HE-MU */
		LLS_MODE_RESERVED,
		LLS_MODE_EHT,	/* EHT_EXT */
		LLS_MODE_EHT,	/* EHT_TRIG */
		LLS_MODE_EHT,	/* EHT_MU */
	};
	static const uint8_t OFDM_RATE[STATS_LLS_OFDM_NUM] = {
						6, 4, 2, 0, 7, 5, 3, 1};
		/* report &= 7:  3  7  2   6   1   5   0    4 */
		/* Mbps       : 6M 9M 12M 18M 24M 36M 48M 54M */
		/* in 0.5 Mbps: 12 18 24  36  48  72  96  108 */
		/* Save format:  0  1  2   3   4   5   6    7 */
	struct STATS_LLS_WIFI_RATE rate = {0};
	struct STA_RECORD *prStaRec;
	uint32_t mcsIdx;

	if (prAdapter->rWifiVar.fgLinkStatsDump)
		DBGLOG(RX, DEBUG,
			"RXV: pmbl=%lu nsts=%lu stbc=%lu bw=%lu mcs=%lu\n",
			RXV_GET_TXMODE(pu4RxV[2]),
			RXV_GET_RX_NSTS(pu4RxV[0]),
			RXV_GET_STBC(pu4RxV[2]),
			RXV_GET_FR_MODE(pu4RxV[2]),
			RXV_GET_RX_RATE(pu4RxV[0]));

	if (!IS_RX_MPDU_BEGIN(prSwRfb->ucPayloadFormat))
		return;

	rate.preamble = TX_MODE_2_LLS_MODE[RXV_GET_TXMODE(pu4RxV[2])];

	if (rate.preamble == LLS_MODE_RESERVED)
		return;

	rate.bw = RXV_GET_FR_MODE(pu4RxV[2]);
	rate.nss = RXV_GET_RX_NSTS(pu4RxV[0]);
	if (rate.preamble >= LLS_MODE_VHT) {
		if (RXV_GET_STBC(pu4RxV[2]))
			rate.nss /= 2;
	}

	rate.rateMcsIdx = RXV_GET_RX_RATE(pu4RxV[0]) & 0xF; /* 0 ~ 15 */
	if (rate.preamble == LLS_MODE_CCK)
		rate.rateMcsIdx &= 0x3; /* 0: 1M; 1: 2M; 2: 5.5M; 3: 11M  */
	else if (rate.preamble == LLS_MODE_OFDM)
		rate.rateMcsIdx = OFDM_RATE[rate.rateMcsIdx & 0x7];
	mcsIdx = rate.rateMcsIdx;

	if (rate.nss >= STATS_LLS_MAX_NSS_NUM)
		goto wrong_rate;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec) {
		DBGLOG(RX, WARN, "StaRec %u not found", prSwRfb->ucStaRecIdx);
		goto wrong_rate;
	}

	if (rate.preamble == LLS_MODE_OFDM) {
		if (mcsIdx >= STATS_LLS_OFDM_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduOFDM[0][0][mcsIdx]++;
	} else if (rate.preamble == LLS_MODE_CCK) {
		if (mcsIdx >= STATS_LLS_CCK_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduCCK[0][0][mcsIdx]++;
	} else if (rate.preamble == LLS_MODE_HT) {
		if (rate.bw >= STATS_LLS_MAX_HT_BW_NUM ||
				mcsIdx >= STATS_LLS_HT_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduHT[0][rate.bw][mcsIdx]++;
	} else if (rate.preamble == LLS_MODE_VHT) {
		if (rate.bw >= STATS_LLS_MAX_VHT_BW_NUM ||
				mcsIdx >= STATS_LLS_VHT_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduVHT[rate.nss][rate.bw][mcsIdx]++;
	} else if (rate.preamble == LLS_MODE_HE) {
		if (rate.bw >= STATS_LLS_MAX_HE_BW_NUM ||
				mcsIdx >= STATS_LLS_HE_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduHE[rate.nss][rate.bw][mcsIdx]++;
	} else if (rate.preamble == LLS_MODE_EHT) {
		if (rate.bw >= STATS_LLS_MAX_EHT_BW_NUM ||
				mcsIdx >= STATS_LLS_EHT_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduEHT[rate.nss][rate.bw][mcsIdx]++;
	}

	if (prAdapter->rWifiVar.fgLinkStatsDump)
		DBGLOG(RX, DEBUG, "rate preamble=%u, nss=%u, bw=%u, mcsIdx=%u",
			rate.preamble, rate.nss, rate.bw, mcsIdx);
	return;

wrong_rate:
	DBGLOG(RX, WARN, "Invalid rate preamble=%u, nss=%u, bw=%u, mcsIdx=%u",
			rate.preamble, rate.nss, rate.bw, mcsIdx);
#endif
}

#if defined(_HIF_USB)
static u_int8_t usb_mt7925_ConninfraAp2WfRdableChk(struct ADAPTER *ad)
{
#define WIFI_IP_VER 0x03010001

	uint32_t u4RdAddr, u4Val = 0;
	u_int8_t fgStatus = FALSE;

/* CONN_DBG_CTL_WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_BUS_TIMEOUT_IRQ_ADDR */
	u4RdAddr = 0x1802362C;
	HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
	DBGLOG(HAL, DEBUG,
	   "\tR 0x%08x=[0x%08x]\n",
	   u4RdAddr, u4Val);

	u4RdAddr =
	  MAP_DRIVER_SIDE(CONN_BUS_CR_ADDR_CONN2SUBSYS_0_AHB_GALS_DBG_ADDR);
	HAL_UHW_RD_FIELD(ad, u4RdAddr, 26, BIT(26), &u4Val);
	if (u4Val != 0) {
		DBGLOG(HAL, DEBUG,
		  "conn2wf sleep protect not release, skip further sop dump!\n");

		return FALSE;
	}

	u4RdAddr = 0x184B0010;
	HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
	DBGLOG(HAL, DEBUG,
	   "\tR 0x%08x=[0x%08x]\n",
	   u4RdAddr, u4Val);

	if (u4Val != WIFI_IP_VER) {
		DBGLOG(HAL, DEBUG,
		  "wifi IP ver not match, skip further sop dump!\n");

		return FALSE;
	}

	return TRUE;
}
static u_int8_t usb_mt7925_ConninfraOffRdableChk(struct ADAPTER *ad)
{
#define CONNINFRA_OFF_POLLING_TIME 4
#define CONNINFRA_IP_VER 0x03010001
	uint32_t u4RdAddr = 0, u4Addr, u4Val = 0;
	uint32_t i = 0;
	u_int8_t fgStatus = FALSE;

	u4Addr =
	  MAP_DRIVER_SIDE(CONN_DBG_CTL_CONN_INFRA_BUS_CLK_DETECT_ADDR);
	HAL_UHW_WR_FIELD(ad, u4Addr, 1, 0, 0x1);

	DBGLOG(HAL, DEBUG,
	   "\tW 0x%08x[0]=[0x1]\n", u4Addr);

	for (i = 0; i <= CONNINFRA_OFF_POLLING_TIME; i++) {
		HAL_UHW_RD_FIELD(ad, u4Addr, 1, (BIT(1) | BIT(2)), &u4Val);

		if (u4Val == 0x3)
			break;
		else if (i == CONNINFRA_OFF_POLLING_TIME) {
			DBGLOG(HAL, DEBUG,
			  "clock not exist, skip further sop dump!\n");

			return FALSE;
		}
		kalUsleep_range(900, 1000);
	}
	u4RdAddr = MAP_DRIVER_SIDE(CONN_CFG_IP_VERSION_ADDR);
	HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
	DBGLOG(HAL, DEBUG,
	   "\tR 0x%08x=[0x%08x]\n",
	   u4RdAddr, u4Val);

	if (u4Val != CONNINFRA_IP_VER) {
		DBGLOG(HAL, DEBUG,
		  "IP ver not match, skip further sop dump!\n");

		return FALSE;
	}
	u4RdAddr =
	  MAP_DRIVER_SIDE(CONN_DBG_CTL_CONN_INFRA_BUS_TIMEOUT_IRQ_ADDR);
	HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
	DBGLOG(HAL, DEBUG,
	   "\tR 0x%08x=[0x%08x]\n",
	   u4RdAddr, u4Val);

	if (u4Val != 0) {
		DBGLOG(HAL, DEBUG,
		  "conn_infra off bus maybe hang, skip further sop dump!\n");

		return FALSE;
	}

	return TRUE;
}
static u_int8_t usb_mt7925_ConninfraOnRdableChk(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0;
	u_int8_t fgStatus = FALSE;

	u4RdAddr = CB_INFRA_RGU_SLP_PROT_RDY_STAT_ADDR;
	HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
	HAL_UHW_RD_FIELD(ad, u4RdAddr, 6, (BIT(6) | BIT(7)), &u4Val);

	DBGLOG(HAL, DEBUG,
	   "\tR 0x%08x[7:6]=[0x%08x]\n",
	   u4RdAddr, u4Val);

	if (u4Val != 0) {
		DBGLOG(HAL, DEBUG,
		  "sleep protect not release, skip further sop dump!\n");
		return FALSE;
	}

	return TRUE;
}
uint32_t mtk_sys_usb_spi_read(struct ADAPTER *ad, uint32_t addr)
{
	uint32_t u4Val = 0;
	u_int8_t fgStatus = FALSE;

	HAL_UHW_RD(ad, 0x18098000, &u4Val, &fgStatus);
	if ((u4Val & BIT(5)) != BIT(5)) {
		u4Val = 0x0000B000 | addr;
		HAL_UHW_WR_FIELD(ad, 0x18098050, u4Val, 0, 0xFFFFFFFF);
		HAL_UHW_WR_FIELD(ad, 0x18098054, 0, 0, 0xFFFFFFFF);
		HAL_UHW_RD(ad, 0x18098000, &u4Val, &fgStatus);
		if ((u4Val & BIT(5)) != BIT(5))
			HAL_UHW_RD(ad, 0x18098058, &u4Val, &fgStatus);
	}
	return u4Val;
}
static void usb_mt7925_dumpConninfraPwrDbg(struct ADAPTER *ad,
u_int8_t fgOnDomain)
{
	uint32_t u4Val = 0, u4Addr;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexB = 1;
	uint32_t indexC = 1;
	uint32_t indexE = 1;
	uint32_t indexD[12] = {1, 2, 3, 4, 5, 9, 10, 11, 15, 16, 19, 20};
	u_int8_t fgStatus = FALSE;

	if (fgOnDomain) {
		dump = conninfra_pwr_on_domain_dump_list;
		size = ARRAY_SIZE(conninfra_pwr_on_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (!IS_CONN_INFRA_PHY_ADDR(dump[i].addr))
				u4Addr = MAP_DRIVER_SIDE(dump[i].addr);
			else
				u4Addr = dump[i].addr;
			if (dump[i].read) {
				HAL_UHW_RD(ad,
					u4Addr,
					&u4Val,
					&fgStatus);
				DBGLOG(HAL, DEBUG,
					"=PSOP_2_1_B%d=0x%08X=0x%08X\n",
					indexB,
					u4Val,
					u4Addr);
				indexB++;
			} else {
				HAL_UHW_WR_FIELD(ad,
					u4Addr,
					dump[i].value,
					dump[i].shift,
					dump[i].mask);
			}
		}
	} else {
		dump = conninfra_pwr_off_domain_dump_list;
		size = ARRAY_SIZE(conninfra_pwr_off_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (!IS_CONN_INFRA_PHY_ADDR(dump[i].addr))
				u4Addr = MAP_DRIVER_SIDE(dump[i].addr);
			else
				u4Addr = dump[i].addr;
			if (dump[i].read) {
				HAL_UHW_RD(ad,
					u4Addr,
					&u4Val,
					&fgStatus);
				DBGLOG(HAL, DEBUG,
					"=PSOP_2_1_C%d=0x%08X=0x%08X\n",
					indexC,
					u4Val,
					u4Addr);
				indexC++;
			} else {
				HAL_UHW_WR_FIELD(ad,
					u4Addr,
					dump[i].value,
					dump[i].shift,
					dump[i].mask);
			}
		}

		dump = conninfra_pwr_adie_common_dump_list;
		size = ARRAY_SIZE(conninfra_pwr_adie_common_dump_list);
		for (i = 0; i < size; i++) {
			u4Addr = dump[i].addr;
			u4Val = mtk_sys_usb_spi_read(ad, u4Addr);
			DBGLOG(HAL, DEBUG,
				"=PSOP_2_1_D%d=0x%08X=0x%08X\n",
				indexD[i],
				u4Val,
				u4Addr);
		}

		dump = conninfra_pwr_adie_7971_dump_list;
		size = ARRAY_SIZE(conninfra_pwr_adie_7971_dump_list);
		for (i = 0; i < size; i++) {
			u4Addr = dump[i].addr;
			u4Val = mtk_sys_usb_spi_read(ad, u4Addr);
			DBGLOG(HAL, DEBUG,
				"=PSOP_2_1_E%d=0x%08X=0x%08X\n",
				indexE,
				u4Val,
				u4Addr);
			indexE++;
		}
	}
}

static void usb_mt7925_dumpConninfraBusDbg(struct ADAPTER *ad,
	u_int8_t fgOnDomain)
{
	uint32_t u4RdAddr, u4WrAddr, u4Addr, u4Val = 0, u4Idx;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	u_int8_t fgStatus = FALSE;
	uint32_t indexA = 1;
	uint32_t indexB = 1;
	uint32_t indexC = 1;

	if (fgOnDomain) {
		dump = conninfra_bus_on_domain_dump_list;
		size = ARRAY_SIZE(conninfra_bus_on_domain_dump_list);

		for (i = 0; i < size; i++) {
			if (!IS_CONN_INFRA_PHY_ADDR(dump[i].addr))
				u4Addr = MAP_DRIVER_SIDE(dump[i].addr);
			else
				u4Addr = dump[i].addr;

			if (dump[i].read) {
				HAL_UHW_RD(ad,
					u4Addr,
					&u4Val,
					&fgStatus);
				DBGLOG(HAL, DEBUG,
					"=PSOP_1_1_A%d=0x%08X=0x%08X\n",
					indexA,
					u4Val,
					u4Addr);
				indexA++;
			} else {
				HAL_UHW_WR_FIELD(ad,
					u4Addr,
					dump[i].value,
					dump[i].shift,
					dump[i].mask);
			}
		}
	} else {
		dump = conninfra_bus_off_domain_dump_bf_list;
		size = ARRAY_SIZE(conninfra_bus_off_domain_dump_bf_list);

		for (i = 0; i < size; i++) {
			if (!IS_CONN_INFRA_PHY_ADDR(dump[i].addr))
				u4Addr = MAP_DRIVER_SIDE(dump[i].addr);
			else
				u4Addr = dump[i].addr;

			if (dump[i].read) {
				HAL_UHW_RD(ad,
					u4Addr,
					&u4Val,
					&fgStatus);
				DBGLOG(HAL, DEBUG,
					"=PSOP_1_1_B%d=0x%08X=0x%08X\n",
					indexB,
					u4Val,
					u4Addr);
				indexB++;
			} else {
				HAL_UHW_WR_FIELD(ad,
					u4Addr,
					dump[i].value,
					dump[i].shift,
					dump[i].mask);
			}
		}
		u4WrAddr =
		  MAP_DRIVER_SIDE(CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_SEL_ADDR);
		u4RdAddr =
		  MAP_DRIVER_SIDE(CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR);
		for (u4Idx = 0x13; u4Idx <= 0x2d; u4Idx++) {
			HAL_UHW_WR(ad, u4WrAddr, u4Idx, &fgStatus);

			HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
			DBGLOG(HAL, DEBUG,
				"=PSOP_1_1_B%d=0x%08X=0x%08X\n",
				indexB,
				u4Val,
				u4RdAddr);
			indexB++;
		}

		for (u4Idx = 0x01; u4Idx <= 0x12; u4Idx++) {
			HAL_UHW_WR(ad, u4WrAddr, u4Idx, &fgStatus);

			HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
			DBGLOG(HAL, DEBUG,
				"=PSOP_1_1_B%d=0x%08X=0x%08X\n",
				indexB,
				u4Val,
				u4RdAddr);
			indexB++;
		}

		dump = conninfra_bus_off_domain_dump_af_list;
		size = ARRAY_SIZE(conninfra_bus_off_domain_dump_af_list);

		for (i = 0; i < size; i++) {
			if (!IS_CONN_INFRA_PHY_ADDR(dump[i].addr))
				u4Addr = MAP_DRIVER_SIDE(dump[i].addr);
			else
				u4Addr = dump[i].addr;

			if (dump[i].read) {
				HAL_UHW_RD(ad,
					u4Addr,
					&u4Val,
					&fgStatus);
				DBGLOG(HAL, DEBUG,
					"=PSOP_1_1_B%d=0x%08X=0x%08X\n",
					indexB,
					u4Val,
					u4Addr);
				indexB++;
			} else {
				HAL_UHW_WR_FIELD(ad,
					u4Addr,
					dump[i].value,
					dump[i].shift,
					dump[i].mask);
			}
		}
		dump = conninfra_bus_off_domain_dump_af_list_c;
		size = ARRAY_SIZE(conninfra_bus_off_domain_dump_af_list_c);

		for (i = 0; i < size; i++) {
			if (!IS_CONN_INFRA_PHY_ADDR(dump[i].addr))
				u4Addr = MAP_DRIVER_SIDE(dump[i].addr);
			else
				u4Addr = dump[i].addr;

			if (dump[i].read) {
				HAL_UHW_RD(ad,
					u4Addr,
					&u4Val,
					&fgStatus);
				DBGLOG(HAL, DEBUG,
					"=PSOP_1_1_C%d=0x%08X=0x%08X\n",
					indexC,
					u4Val,
					u4Addr);
				indexC++;
			} else {
				HAL_UHW_WR_FIELD(ad,
					u4Addr,
					dump[i].value,
					dump[i].shift,
					dump[i].mask);
			}
		}
		u4Addr = 0x70028730;
		HAL_UHW_RD(ad, u4Addr, &u4Val, &fgStatus);
		DBGLOG(HAL, DEBUG,
			"=PSOP_1_1_C%d=0x%08X=0x%08X\n",
			indexC, u4Val, u4Addr);
	}
}

static void usb_mt7925_dumpConninfraClkDbg(struct ADAPTER *ad,
	u_int8_t fgOnDomain)
{
	uint32_t u4Addr, u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;
	uint32_t indexB = 1;
	u_int8_t fgStatus = FALSE;

	if (fgOnDomain) {
		dump = conninfra_clk_on_domain_dump_list;
		size = ARRAY_SIZE(conninfra_clk_on_domain_dump_list);

		for (i = 0; i < size; i++) {
			if (!IS_CONN_INFRA_PHY_ADDR(dump[i].addr))
				u4Addr = MAP_DRIVER_SIDE(dump[i].addr);
			else
				u4Addr = dump[i].addr;

			if (dump[i].read) {
				HAL_UHW_RD(ad,
					u4Addr,
					&u4Val,
					&fgStatus);
				DBGLOG(HAL, DEBUG,
					"=PSOP_8_1_A%d=0x%08X=0x%08X\n",
					indexA,
					u4Val,
					u4Addr);
				indexA++;
			} else {
				HAL_UHW_WR_FIELD(ad,
					u4Addr,
					dump[i].value,
					dump[i].shift,
					dump[i].mask);
				DBGLOG(HAL, DEBUG,
					"WR 0x%08x=0x%08x\n",
					u4Addr,
					dump[i].value);
			}
		}
	} else {
		dump = conninfra_clk_off_domain_dump_list;
		size = ARRAY_SIZE(conninfra_clk_off_domain_dump_list);

		for (i = 0; i < size; i++) {
			if (!IS_CONN_INFRA_PHY_ADDR(dump[i].addr))
				u4Addr = MAP_DRIVER_SIDE(dump[i].addr);
			else
				u4Addr = dump[i].addr;

			if (dump[i].read) {
				HAL_UHW_RD(ad,
					u4Addr,
					&u4Val,
					&fgStatus);
				DBGLOG(HAL, DEBUG,
					"=PSOP_8_1_B%d=0x%08X=0x%08X\n",
					indexB,
					u4Val,
					u4Addr);
				indexB++;
			} else {
				HAL_UHW_WR_FIELD(ad,
					u4Addr,
					dump[i].value,
					dump[i].shift,
					dump[i].mask);
				DBGLOG(HAL, DEBUG,
					"WR 0x%08x=0x%08x\n",
					u4Addr,
					dump[i].value);
			}
		}
	}
}

static void usb_mt7925_dumpConninfraReg(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0;
	u_int8_t fgStatus = FALSE;

	if (!usb_mt7925_ConninfraOnRdableChk(ad))
		return;

	u4RdAddr = MAP_DRIVER_SIDE(CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_ADDR);
	HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
	DBGLOG(HAL, DEBUG,
	       "\tR 0x%08x=[0x%08x]\n",
	       u4RdAddr, u4Val);

	/* Dump on domain */
	usb_mt7925_dumpConninfraPwrDbg(ad, TRUE);
	usb_mt7925_dumpConninfraBusDbg(ad, TRUE);
	usb_mt7925_dumpConninfraClkDbg(ad, TRUE);

	if (!usb_mt7925_ConninfraOffRdableChk(ad))
		return;

	/* Dump off domain */
	usb_mt7925_dumpConninfraPwrDbg(ad, FALSE);
	usb_mt7925_dumpConninfraBusDbg(ad, FALSE);
	usb_mt7925_dumpConninfraClkDbg(ad, FALSE);
}

static void usb_mt7925_dumpWfTopMiscOn(struct ADAPTER *ad)
{
	uint32_t u4WrVal = 0, u4Val = 0, u4Idx, u4RdAddr, u4WrAddr;
	u_int8_t fgStatus = FALSE;
	uint32_t indexA = 1;
	uint32_t au4List[] = {
		0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000004,
		0x00000010, 0x00000012, 0x00000017, 0x00000018, 0x00000019,
		0x0000001A, 0x0000001B, 0x0000001D
	};

	u4WrAddr =
	  MAP_DRIVER_SIDE(CONN_HOST_CSR_TOP_WF_ON_MONFLG_EN_FR_HIF_ADDR);
	u4WrVal = 0x00000001;
	HAL_UHW_WR(ad, u4WrAddr, u4WrVal, &fgStatus);
	DBGLOG(HAL, DEBUG,
	       "\tW 0x%08x=[0x%08x]\n",
	       u4WrAddr, u4WrVal);

	u4WrAddr =
	  MAP_DRIVER_SIDE(CONN_HOST_CSR_TOP_WF_ON_MONFLG_SEL_FR_HIF_ADDR);
	u4RdAddr =
	  MAP_DRIVER_SIDE(CONN_HOST_CSR_TOP_WF_ON_MONFLG_OUT_ADDR);
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4WrVal = au4List[u4Idx];
		HAL_UHW_WR(ad, u4WrAddr, u4WrVal, &fgStatus);
		HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
		DBGLOG(HAL, DEBUG,
			"=PSOP_3_1_A%d=0x%08X=0x%08X\n",
			indexA,
			u4Val,
			u4RdAddr);
		indexA++;
	}
}

static void usb_mt7925_dumpWfTopMiscVon(struct ADAPTER *ad)
{
	uint32_t u4WrVal = 0, u4Val = 0, u4Idx, u4RdAddr, u4WrAddr;
	u_int8_t fgStatus = FALSE;
	uint32_t indexB = 1;
	uint32_t au4List[] = {
		0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000004,
		0x00000008
	};

	u4WrAddr =
	  MAP_DRIVER_SIDE(CONN_HOST_CSR_TOP_ADDR_WF_VON_MONFLG_EN_FR_HIF_ADDR);
	u4WrVal = 0x00000001;
	HAL_UHW_WR(ad, u4WrAddr, u4WrVal, &fgStatus);
	DBGLOG(HAL, DEBUG,
	       "\tW 0x%08x=[0x%08x]\n",
	       u4WrAddr, u4WrVal);

	u4WrAddr =
	  MAP_DRIVER_SIDE(CONN_HOST_CSR_TOP_ADDR_WF_VON_MONFLG_SEL_FR_HIF_ADDR);
	u4RdAddr =
	  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_VON_DEBUG_OUT_ADDR);
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4WrVal = au4List[u4Idx];
		HAL_UHW_WR(ad, u4WrAddr, u4WrVal, &fgStatus);
		HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
		DBGLOG(HAL, DEBUG,
			"=PSOP_3_1_B%d=0x%08X=0x%08X\n",
			indexB,
			u4Val,
			u4RdAddr);
		indexB++;
	}
}

static void usb_mt7925_dumpWfTopCfgon(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0, u4Idx;
	u_int8_t fgStatus = FALSE;
	uint32_t indexC = 1;
	uint32_t au4List[] = {
	/* Reference WF_top_cfg_on.h 0x184C_1XXX <=> 0x8102_1XXX */
		0x184C1B00, 0x184C1B04, 0x184C1B08, 0x184C1B0C,
		0x184C1B10, 0x184C1B14, 0x184C1B18, 0x184C1B1C,
		0x184C1B20, 0x184C1B24, 0x184C1B28, 0x184C1B2C,
		0x184C1B30, 0x184C1B34, 0x184C1B38, 0x184C1B3C,
		0x184C1B40, 0x184C1B50, 0x184C1B54, 0x184C1B58,
		0x184C1B5C, 0x184C1B60, 0x184C1B64, 0x184C1B68,
		0x184C1B6C, 0x184C1B70, 0x184C1B74, 0x184C1B78,
		0x184C1B7C, 0x184C1B80, 0x184C1B84, 0x184C1B88,
		0x184C1B8C, 0x184C1B90
	};

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4RdAddr = au4List[u4Idx];
		HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
		DBGLOG(HAL, DEBUG,
			"=PSOP_3_1_C%d=0x%08X=0x%08X\n",
			indexC,
			u4Val,
			u4RdAddr);
		indexC++;
	}
}

static void usb_mt7925_dumpHostVdnrTimeoutInfo(struct ADAPTER *ad)
{
	uint32_t u4WrVal = 0, u4Val = 0, u4Idx, u4RdAddr, u4WrAddr;
	u_int8_t fgStatus = FALSE;
	u_int8_t indexA = 0;
	uint32_t au4List[] = {
		0x00010001, 0x00020001, 0x00030001, 0x00040001, 0x00050001,
		0x00060001, 0x00070001, 0x00080001, 0x00090001, 0x00010002,
		0x00020002, 0x00030002, 0x00040002
	};

/* CONN_DBG_CTL_WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_BUS_TIMEOUT_IRQ_ADDR */
	u4RdAddr = 0x1802362C;
	u4WrAddr = MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_ADDR);
	u4WrVal = 0x4;

	HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
	DBGLOG(HAL, DEBUG,
		"=PSOP_4_1_A%d=0x%08X=0x%08X\n",
		indexA,
		u4Val,
		u4RdAddr);
	indexA++;
	HAL_UHW_WR(ad, u4WrAddr, u4WrVal, &fgStatus);
	DBGLOG(HAL, DEBUG,
	       "\tR 0x%08x=[0x%08x], W 0x%08x=[0x%08x]\n",
	       u4RdAddr, u4Val, u4WrAddr, u4WrVal);

/* CONN_DBG_CTL_WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_DEBUGSYS_CTRL_ADDR */
	u4WrAddr = 0x18023628;
	u4RdAddr =
	  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR);
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4WrVal = au4List[u4Idx];
		HAL_UHW_WR(ad, u4WrAddr, u4WrVal, &fgStatus);
		HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
		DBGLOG(HAL, DEBUG,
			"=PSOP_4_1_A%d=0x%08X=0x%08X\n",
			indexA,
			u4Val,
			u4RdAddr);
		indexA++;
	}
}

static void usb_mt7925_dumpWfVdnrTimeoutInfo(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4WrAddr, u4Val = 0, u4WrVal = 0;
	uint32_t u4Idx;
	u_int8_t fgStatus = FALSE;
	uint32_t indexB = 0;
	uint32_t au4List[] = {
/* Ref Wf_mcusys_vdnr_gen_bus_u_debug_ctrl_ao.h 0x1850_XXXX <=> 0x810F_XXXX */
		0x18500408, 0x1850040C, 0x18500410, 0x18500414,
		0x18500418, 0x1850041C, 0x18500420, 0x18500424,
		0x18500428, 0x1850042C, 0x18500430, 0x18500434,
		0x18500438, 0x18500000
	};

	u4RdAddr = 0x1802362C;
	HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
	DBGLOG(HAL, DEBUG,
		"=PSOP_4_1_B%d=0x%08X=0x%08X\n",
		indexB,
		u4Val,
		u4RdAddr);
	indexB++;

	u4WrAddr = 0x18023604;
	u4WrVal = 0X4;
	HAL_UHW_WR(ad, u4WrAddr, u4WrVal, &fgStatus);

	/* switch Ap2WF to 0x810F0000. */
	u4WrAddr = 0x18400120;
	u4WrVal = 0x810F0000;
	HAL_UHW_WR(ad, u4WrAddr, u4WrVal, &fgStatus);
	DBGLOG(HAL, DEBUG,
	       "\tW 0x%08x=[0x%08x]\n",
	       u4WrAddr, u4WrVal);
	indexB++;

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4RdAddr = au4List[u4Idx];
		HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
		DBGLOG(HAL, DEBUG,
			"=PSOP_4_1_B%d=0x%08X=0x%08X\n",
			indexB,
			u4Val,
			u4RdAddr);
		indexB++;
	}
}

static void usb_mt7925_dumpAhbApbTimeoutInfo(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4WrAddr, u4Val = 0, u4WrVal = 0;
	uint32_t u4Idx;
	u_int8_t fgStatus = FALSE;
	uint32_t indexC = 1;
	uint32_t au4List[] = {
/* Reference Conn_mcu_bus_cr.h 0x1850_XXXX <=> 0x830C_XXXX */
		0x18501004, 0x18501010, 0x18501008,
		0x1850100C, 0x18501000
	};

	/* switch Ap2WF to 0x830C0000. */
	u4WrAddr = 0x18400120;
	u4WrVal = 0x830C0000;
	HAL_UHW_WR(ad, u4WrAddr, u4WrVal, &fgStatus);
	DBGLOG(HAL, DEBUG,
	       "\tW 0x%08x=[0x%08x]\n",
	       u4WrAddr, u4WrVal);

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4RdAddr = au4List[u4Idx];
		HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);
		DBGLOG(HAL, DEBUG,
			"=PSOP_4_1_C%d=0x%08X=0x%08X\n",
			indexC,
			u4Val,
			u4RdAddr);
		indexC++;
	}
}

static void usb_mt7925_dumpWfsysReg(struct ADAPTER *ad)
{
	/* Section A: Dump wf_top_misc_on monflag */
	usb_mt7925_dumpWfTopMiscOn(ad);

	/* Section B: Dump wf_top_misc_von monflag */
	usb_mt7925_dumpWfTopMiscVon(ad);

	/* Section A: Dump VDNR timeout host side info */
	usb_mt7925_dumpHostVdnrTimeoutInfo(ad);

	if (!usb_mt7925_ConninfraAp2WfRdableChk(ad))
		return;

	/* Section C: Dump wf_top_cfg_on debug CR */
	usb_mt7925_dumpWfTopCfgon(ad);

	/* Section B: Dump VDNR timeout wf side info */
	usb_mt7925_dumpWfVdnrTimeoutInfo(ad);

	/* Section C: Dump AHB APB timeout info */
	usb_mt7925_dumpAhbApbTimeoutInfo(ad);
}

u_int8_t mt7925_usb_show_mcu_debug_info(struct ADAPTER *ad,
	uint8_t *pucBuf, uint32_t u4Max, uint8_t ucFlag,
	uint32_t *pu4Length)
{
	uint32_t u4Val = 0, u4Addr = 0, u4RdAddr = 0;
	uint32_t u4Shft = 0, u4Mask = 0;
	uint8_t  i = 0;
	u_int8_t fgStatus = FALSE;

	if (pucBuf) {
		LOGBUF(pucBuf, u4Max, *pu4Length, "\n");
		LOGBUF(pucBuf, u4Max, *pu4Length,
			"----<Dump MCU Debug Information>----\n");
	}
	u4Addr =
	  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_ADDR);
	/* Enable mcu debug function. */
	HAL_UHW_WR_FIELD(ad,
		u4Addr,
		0x0,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_SHFT,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_MASK);

	u4Addr =
	  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_ADDR);
	/* Get currenct PC log. */
	HAL_UHW_WR_FIELD(ad,
		u4Addr,
		CURRENT_PC_IDX,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_WF_MCU_DBG_PC_LOG_SEL_SHFT,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_WF_MCU_DBG_PC_LOG_SEL_MASK);

	u4RdAddr =
	  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR);

	HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);

	DBGLOG(INIT, DEBUG, "Current PC LOG: 0x%08x\n", u4Val);
	if (pucBuf)
		LOGBUF(pucBuf, u4Max, *pu4Length,
		"Current PC LOG: 0x%08x\n", u4Val);

	if (ucFlag != DBG_MCU_DBG_CURRENT_PC) {
		u4Shft =
		  CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_WF_MCU_DBG_PC_LOG_SEL_SHFT;
		u4Mask =
		  CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_WF_MCU_DBG_PC_LOG_SEL_MASK;
		u4Addr =
		  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_ADDR);

		HAL_UHW_WR_FIELD(ad, u4Addr, PC_LOG_CTRL_IDX,
		  u4Shft, u4Mask);

		u4RdAddr =
		  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR);

		HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);

		DBGLOG(INIT, DEBUG, "PC log control=0x%08x\n", u4Val);
		if (pucBuf)
			LOGBUF(pucBuf, u4Max, *pu4Length,
			"PC log control=0x%08x\n", u4Val);

		u4Addr =
		  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_ADDR);

		u4RdAddr =
		  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR);

		for (i = 0; i < PC_LOG_NUM_CE; i++) {
			HAL_UHW_WR_FIELD(ad,
			u4Addr,
			i,
			u4Shft,
			u4Mask);

		HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);

		DBGLOG(INIT, DEBUG, "PC log(%d)=0x%08x\n", i, u4Val);
		if (pucBuf)
			LOGBUF(pucBuf, u4Max, *pu4Length,
			"PC log(%d)=0x%08x\n", i, u4Val);
		}
		/* Read LR log. */
		u4Shft =
		CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_WF_MCU_DBG_GPR_LOG_SEL_SHFT;

		u4Mask =
		CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_WF_MCU_DBG_GPR_LOG_SEL_MASK;

		u4Addr =
		  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_ADDR);

		HAL_UHW_WR_FIELD(ad,
		  u4Addr,
		  PC_LOG_CTRL_IDX,
		  u4Shft,
		  u4Mask);

		u4RdAddr =
		  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR);

		HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);

		DBGLOG(INIT, DEBUG, "LR log control=0x%08x\n", u4Val);

		u4Addr =
		  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_ADDR);

		u4RdAddr =
		  MAP_DRIVER_SIDE(CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR);

		if (pucBuf)
			LOGBUF(pucBuf, u4Max, *pu4Length,
			"LR log control=0x%08x\n", u4Val);

		for (i = 0; i < GPR_LOG_NUM_CE; i++) {
			HAL_UHW_WR_FIELD(ad,
			u4Addr,
			i,
			u4Shft,
			u4Mask);

			HAL_UHW_RD(ad, u4RdAddr, &u4Val, &fgStatus);

			DBGLOG(INIT, DEBUG, "LR log(%d)=0x%08x\n", i, u4Val);
			if (pucBuf)
				LOGBUF(pucBuf, u4Max, *pu4Length,
				"LR log(%d)=0x%08x\n", i, u4Val);
		}
	}
	return TRUE;
}
#endif

#if defined(_HIF_PCIE)
uint32_t mtk_sys_pci_spi_read(struct ADAPTER *ad, uint32_t addr)
{
	uint32_t u4Val = 0;

	HAL_MCR_RD(ad, 0x7C098000, &u4Val);
	if ((u4Val & BIT(5)) != BIT(5)) {
		u4Val = 0x0000B000 | addr;
		HAL_MCR_WR_FIELD(ad, 0x7C098050, u4Val,
			0, 0xFFFFFFFF);
		HAL_MCR_WR_FIELD(ad, 0x7C098054, 0,
			0, 0xFFFFFFFF);
		HAL_MCR_RD(ad, 0x7C098000, &u4Val);
		if ((u4Val & BIT(5)) != BIT(5))
			HAL_MCR_RD(ad, 0x7C098058, &u4Val);
	}
	return u4Val;
}

static u_int8_t mt7925_ConninfraOnRdableChk(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0;

	u4RdAddr = CB_INFRA_RGU_SLP_PROT_RDY_STAT_ADDR;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	HAL_MCR_RD_FIELD(ad, u4RdAddr, 6, (BIT(6) | BIT(7)), &u4Val);

	DBGLOG(HAL, DEBUG,
	   "\tR 0x%08x[7:6]=[0x%08x]\n",
	   u4RdAddr, u4Val);

	if (u4Val != 0) {
		DBGLOG(HAL, DEBUG,
		  "sleep protect not release, skip further sop dump!\n");
		return FALSE;
	}

	return TRUE;
}

static u_int8_t mt7925_ConninfraOffRdableChk(struct ADAPTER *ad)
{
#define CONNINFRA_OFF_POLLING_TIME 4
#define CONNINFRA_IP_VER 0x03010001
	uint32_t u4RdAddr = 0, u4Addr, u4Val = 0;
	uint32_t i = 0;

	u4Addr = CONN_DBG_CTL_CONN_INFRA_BUS_CLK_DETECT_ADDR;
	HAL_MCR_WR_FIELD(ad, u4Addr, 1, 0, 0x1);

	DBGLOG(HAL, DEBUG,
	   "\tW 0x%08x[0]=[0x1]\n", u4Addr);

	for (i = 0; i <= CONNINFRA_OFF_POLLING_TIME; i++) {
		HAL_MCR_RD_FIELD(ad, u4Addr, 1, (BIT(1) | BIT(2)), &u4Val);

		if (u4Val == 0x3)
			break;
		else if (i == CONNINFRA_OFF_POLLING_TIME) {
			DBGLOG(HAL, DEBUG,
			  "clock not exist, skip further sop dump!\n");

			return FALSE;
		}
		kalUsleep_range(900, 1000);
	}
	u4RdAddr = CONN_CFG_IP_VERSION_ADDR;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	DBGLOG(HAL, DEBUG,
	   "\tR 0x%08x=[0x%08x]\n",
	   u4RdAddr, u4Val);

	if (u4Val != CONNINFRA_IP_VER) {
		DBGLOG(HAL, DEBUG,
		  "IP ver not match, skip further sop dump!\n");

		return FALSE;
	}
	u4RdAddr = CONN_DBG_CTL_CONN_INFRA_BUS_TIMEOUT_IRQ_ADDR;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	DBGLOG(HAL, DEBUG,
	   "\tR 0x%08x=[0x%08x]\n",
	   u4RdAddr, u4Val);

	if (u4Val != 0) {
		DBGLOG(HAL, DEBUG,
		  "conn_infra off bus maybe hang, skip further sop dump!\n");

		return FALSE;
	}

	return TRUE;
}

static u_int8_t mt7925_ConninfraAp2WfRdableChk(struct ADAPTER *ad)
{
#define WIFI_IP_VER 0x03010001

	uint32_t u4RdAddr, u4Val = 0;

/* CONN_DBG_CTL_WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_BUS_TIMEOUT_IRQ_ADDR */
	u4RdAddr = 0x7C02362C;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	DBGLOG(HAL, DEBUG,
	   "\tR 0x%08x=[0x%08x]\n",
	   u4RdAddr, u4Val);

	u4RdAddr = CONN_BUS_CR_ADDR_CONN2SUBSYS_0_AHB_GALS_DBG_ADDR;
	HAL_MCR_RD_FIELD(ad, u4RdAddr, 26, BIT(26), &u4Val);
	if (u4Val != 0) {
		DBGLOG(HAL, DEBUG,
		  "conn2wf sleep protect not release, skip further sop dump!\n");

		return FALSE;
	}

	u4RdAddr = 0x184B0010;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	DBGLOG(HAL, DEBUG,
	   "\tR 0x%08x=[0x%08x]\n",
	   u4RdAddr, u4Val);

	if (u4Val != WIFI_IP_VER) {
		DBGLOG(HAL, DEBUG,
		  "wifi IP ver not match, skip further sop dump!\n");

		return FALSE;
	}

	return TRUE;
}

static void mt7925_dumpConninfraPwrDbg(struct ADAPTER *ad, u_int8_t fgOnDomain)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexB = 1;
	uint32_t indexC = 1;
	uint32_t indexD[12] = {1, 2, 3, 4, 5, 9, 10, 11, 15, 16, 19, 20};
	uint32_t indexE = 1;

	if (fgOnDomain) {
		dump = conninfra_pwr_on_domain_dump_list;
		size = ARRAY_SIZE(conninfra_pwr_on_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad,
					dump[i].addr,
					&u4Val);
				DBGLOG(HAL, DEBUG,
					"=PSOP_2_1_B%d=0x%08X=0x%08X\n",
					indexB,
					u4Val,
					dump[i].addr);
				indexB++;
			} else {
				HAL_MCR_WR_FIELD(ad,
				dump[i].addr,
				dump[i].value,
				dump[i].shift,
				dump[i].mask);
			}
		}
	} else {
		dump = conninfra_pwr_off_domain_dump_list;
		size = ARRAY_SIZE(conninfra_pwr_off_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad,
					dump[i].addr,
					&u4Val);
				DBGLOG(HAL, DEBUG,
					"=PSOP_2_1_C%d=0x%08X=0x%08X\n",
					indexC,
					u4Val,
					dump[i].addr);
				indexC++;
			} else {
				HAL_MCR_WR_FIELD(ad,
				dump[i].addr,
				dump[i].value,
				dump[i].shift,
				dump[i].mask);
			}
		}
		dump = conninfra_pwr_adie_common_dump_list;
		size = ARRAY_SIZE(conninfra_pwr_adie_common_dump_list);
		for (i = 0; i < size; i++) {
			u4Val = mtk_sys_pci_spi_read(ad, dump[i].addr);
			DBGLOG(HAL, DEBUG,
				"=PSOP_2_1_D%d=0x%08X=0x%08X\n",
				indexD[i],
				u4Val,
				dump[i].addr);
		}

		dump = conninfra_pwr_adie_7971_dump_list;
		size = ARRAY_SIZE(conninfra_pwr_adie_7971_dump_list);
		for (i = 0; i < size; i++) {
			u4Val = mtk_sys_pci_spi_read(ad, dump[i].addr);
			DBGLOG(HAL, DEBUG,
				"=PSOP_2_1_E%d=0x%08X=0x%08X\n",
				indexE,
				u4Val,
				dump[i].addr);
			indexE++;
		}
	}
}

static void mt7925_dumpConninfraBusDbg(struct ADAPTER *ad,
	u_int8_t fgOnDomain)
{
	uint32_t u4RdAddr, u4WrAddr, u4Val = 0, u4Idx;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;
	uint32_t indexB = 1;
	uint32_t indexC = 1;

	if (fgOnDomain) {
		dump = conninfra_bus_on_domain_dump_list;
		size = ARRAY_SIZE(conninfra_bus_on_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad,
					   dump[i].addr,
					   &u4Val);
				DBGLOG(HAL, DEBUG,
					"=PSOP_1_1_A%d=0x%08X=0x%08X\n",
					indexA,
					u4Val,
					dump[i].addr);
				indexA++;
			} else {
				HAL_MCR_WR_FIELD(ad,
				dump[i].addr,
				dump[i].value,
				dump[i].shift,
				dump[i].mask);
			}
		}
	} else {
		dump = conninfra_bus_off_domain_dump_bf_list;
		size = ARRAY_SIZE(conninfra_bus_off_domain_dump_bf_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad,
					   dump[i].addr,
					   &u4Val);
				DBGLOG(HAL, DEBUG,
					"=PSOP_1_1_B%d=0x%08X=0x%08X\n",
					indexB,
					u4Val,
					dump[i].addr);
				indexB++;
			} else {
				HAL_MCR_WR_FIELD(ad,
				dump[i].addr,
				dump[i].value,
				dump[i].shift,
				dump[i].mask);
			}
		}
		u4WrAddr = CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_SEL_ADDR;
		u4RdAddr = CONN_DBG_CTL_CONN_INFRA_OFF_BUS_DBG_OUT_ADDR;
		for (u4Idx = 0x13; u4Idx <= 0x2d; u4Idx++) {
			HAL_MCR_WR(ad, u4WrAddr, u4Idx);

			HAL_MCR_RD(ad, u4RdAddr, &u4Val);
			DBGLOG(HAL, DEBUG,
				"=PSOP_1_1_B%d=0x%08X=0x%08X\n",
				indexB,
				u4Val,
				u4RdAddr);
			indexB++;
		}

		dump = conninfra_bus_off_domain_dump_af_list;
		size = ARRAY_SIZE(conninfra_bus_off_domain_dump_af_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad,
					   dump[i].addr,
					   &u4Val);
				DBGLOG(HAL, DEBUG,
				"=PSOP_1_1_B%d=0x%08X=0x%08X\n",
				indexB,
				u4Val,
				dump[i].addr);
			indexB++;
			} else {
				HAL_MCR_WR_FIELD(ad,
				dump[i].addr,
				dump[i].value,
				dump[i].shift,
				dump[i].mask);
			}
		}
		dump = conninfra_bus_off_domain_dump_af_list_c;
		size = ARRAY_SIZE(conninfra_bus_off_domain_dump_af_list_c);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad,
					   dump[i].addr,
					   &u4Val);
				DBGLOG(HAL, DEBUG,
				"=PSOP_1_1_C%d=0x%08X=0x%08X\n",
				indexC,
				u4Val,
				dump[i].addr);
			indexC++;
			} else {
				HAL_MCR_WR_FIELD(ad,
				dump[i].addr,
				dump[i].value,
				dump[i].shift,
				dump[i].mask);
			}
		}
	}
}

static void mt7925_dumpConninfraClkDbg(struct ADAPTER *ad,
	u_int8_t fgOnDomain)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;
	uint32_t indexB = 1;

	if (fgOnDomain) {
		dump = conninfra_clk_on_domain_dump_list;
		size = ARRAY_SIZE(conninfra_clk_on_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad,
					   dump[i].addr,
					   &u4Val);
				DBGLOG(HAL, DEBUG,
					"=PSOP_8_1_A%d=0x%08X=0x%08X\n",
					indexA,
					u4Val,
					dump[i].addr);
				indexA++;
			} else {
				HAL_MCR_WR_FIELD(ad,
				dump[i].addr,
				dump[i].value,
				dump[i].shift,
				dump[i].mask);
			}
		}
	} else {
		dump = conninfra_clk_off_domain_dump_list;
		size = ARRAY_SIZE(conninfra_clk_off_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad,
					   dump[i].addr,
					   &u4Val);
				DBGLOG(HAL, DEBUG,
					"=PSOP_8_1_B%d=0x%08X=0x%08X\n",
					indexB,
					u4Val,
					dump[i].addr);
				indexB++;
			} else {
				HAL_MCR_WR_FIELD(ad,
				dump[i].addr,
				dump[i].value,
				dump[i].shift,
				dump[i].mask);
			}
		}
	}
}


static void mt7925_dumpConninfraReg(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0;

	if (!mt7925_ConninfraOnRdableChk(ad))
		return;

	u4RdAddr = CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_ADDR;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	DBGLOG(HAL, DEBUG,
	       "\tR 0x%08x=[0x%08x]\n",
	       u4RdAddr, u4Val);

	/* Dump on domain */
	mt7925_dumpConninfraPwrDbg(ad, TRUE);
	mt7925_dumpConninfraBusDbg(ad, TRUE);
	mt7925_dumpConninfraClkDbg(ad, TRUE);

	if (!mt7925_ConninfraOffRdableChk(ad))
		return;

	/* Dump off domain */
	mt7925_dumpConninfraPwrDbg(ad, FALSE);
	mt7925_dumpConninfraBusDbg(ad, FALSE);
	mt7925_dumpConninfraClkDbg(ad, FALSE);
}

static void mt7925_dumpWfsysReg(struct ADAPTER *ad)
{
	/* Section A: Dump wf_top_misc_on monflag */
	mt7925_dumpWfTopMiscOn(ad);

	/* Section B: Dump wf_top_misc_von monflag */
	mt7925_dumpWfTopMiscVon(ad);

	/* Section A: Dump VDNR timeout host side info */
	mt7925_dumpHostVdnrTimeoutInfo(ad);

	if (!mt7925_ConninfraAp2WfRdableChk(ad))
		return;

	/* Section C: Dump wf_top_cfg_on debug CR */
	mt7925_dumpWfTopCfgon(ad);

	/* Section B: Dump VDNR timeout wf side info */
	mt7925_dumpWfVdnrTimeoutInfo(ad);

	/* Section C: Dump AHB APB timeout info */
	mt7925_dumpAhbApbTimeoutInfo(ad);
}

u_int8_t mt7925_pcie_show_mcu_debug_info(struct ADAPTER *ad,
	uint8_t *pucBuf, uint32_t u4Max, uint8_t ucFlag,
	uint32_t *pu4Length)
{
	uint32_t u4Val = 0;
	uint32_t u4Shft = 0, u4Mask = 0;
	uint8_t  i = 0;

	if (pucBuf) {
		LOGBUF(pucBuf, u4Max, *pu4Length, "\n");
		LOGBUF(pucBuf, u4Max, *pu4Length,
			"----<Dump MCU Debug Information>----\n");
	}
	/* Enable mcu debug function. */
	HAL_MCR_WR_FIELD(ad,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_ADDR,
		0x0,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_SHFT,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_MASK);

	/* Get currenct PC log. */
	HAL_MCR_WR_FIELD(ad,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_ADDR,
		CURRENT_PC_IDX,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_WF_MCU_DBG_PC_LOG_SEL_SHFT,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_WF_MCU_DBG_PC_LOG_SEL_MASK);

	HAL_MCR_RD(ad,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR,
		&u4Val);

	DBGLOG(INIT, DEBUG, "Current PC LOG: 0x%08x\n", u4Val);
	if (pucBuf)
		LOGBUF(pucBuf, u4Max, *pu4Length,
		"Current PC LOG: 0x%08x\n", u4Val);

	if (ucFlag != DBG_MCU_DBG_CURRENT_PC) {
		u4Shft =
		  CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_WF_MCU_DBG_PC_LOG_SEL_SHFT;
		u4Mask =
		  CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_WF_MCU_DBG_PC_LOG_SEL_MASK;

		HAL_MCR_WR_FIELD(ad,
		  CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_ADDR,
		  PC_LOG_CTRL_IDX,
		  u4Shft,
		  u4Mask);

		HAL_MCR_RD(ad,
			CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR,
			&u4Val);

		DBGLOG(INIT, DEBUG, "PC log control=0x%08x\n", u4Val);
		if (pucBuf)
			LOGBUF(pucBuf, u4Max, *pu4Length,
			"PC log control=0x%08x\n", u4Val);

		for (i = 0; i < PC_LOG_NUM_CE; i++) {
			HAL_MCR_WR_FIELD(ad,
			  CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_ADDR,
			  i,
			  u4Shft,
			  u4Mask);
			HAL_MCR_RD(ad,
			  CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR,
			  &u4Val);

			DBGLOG(INIT, DEBUG, "PC log(%d)=0x%08x\n", i, u4Val);
			if (pucBuf)
				LOGBUF(pucBuf, u4Max, *pu4Length,
				"PC log(%d)=0x%08x\n", i, u4Val);
		}
		/* Read LR log. */
		u4Shft =
		CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_WF_MCU_DBG_GPR_LOG_SEL_SHFT;
		u4Mask =
		CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_WF_MCU_DBG_GPR_LOG_SEL_MASK;
		HAL_MCR_WR_FIELD(ad,
			CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_ADDR,
			PC_LOG_CTRL_IDX,
			u4Shft,
			u4Mask);
		HAL_MCR_RD(ad,
			CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR,
			&u4Val);
		DBGLOG(INIT, DEBUG, "LR log control=0x%08x\n", u4Val);
		if (pucBuf)
			LOGBUF(pucBuf, u4Max, *pu4Length,
			"LR log control=0x%08x\n", u4Val);

		for (i = 0; i < GPR_LOG_NUM_CE; i++) {
			HAL_MCR_WR_FIELD(ad,
			  CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_ADDR,
			  i,
			  u4Shft,
			  u4Mask);
			HAL_MCR_RD(ad,
			  CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR,
			  &u4Val);
			DBGLOG(INIT, DEBUG, "LR log(%d)=0x%08x\n", i, u4Val);
			if (pucBuf)
				LOGBUF(pucBuf, u4Max, *pu4Length,
				"LR log(%d)=0x%08x\n", i, u4Val);
		}
	}

	return TRUE;

}
#endif

u_int8_t mt7925_show_debug_sop_info(struct ADAPTER *ad,
	uint8_t ucCase)
{
	switch (ucCase) {
	case SLAVENORESP:
		DBGLOG(HAL, ERROR, "Slave no response!\n");
#if defined(_HIF_USB)
		usb_mt7925_dumpConninfraReg(ad);
		usb_mt7925_dumpWfsysReg(ad);
#elif defined(_HIF_PCIE)
		mt7925_dumpCbtopReg(ad);
		mt7925_dumpConninfraReg(ad);
		mt7925_dumpWfsysReg(ad);
#endif
		break;
	default:
		break;
	}

	return TRUE;
}
#endif /* DBG_DISABLE_ALL_INFO */
#endif /* MT7925 */
