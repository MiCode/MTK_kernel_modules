/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */


/* AUTO-GENERATED FILE.  DO NOT MODIFY.
 *
 * This file, mt6886_debug_gen.h was automatically generated
 * by the tool from the DEBUG data DE provided.
 * It should not be modified by hand.
 *
 * Reference debug file,
 * - [Cxxxxxs]connsys_power_debug.xlsx (Modified date: 2022-04-15)
 * - [MT6886]conn_infra_bus_debug_ctrl.xlsx (Modified date: 2021-10-14)
 */


#ifndef MT6886_DEBUG_GEN_H
#define MT6886_DEBUG_GEN_H

#define CONN_DEBUG_INFO_SIZE 256
#define DEBUG_TAG_SIZE 10

struct conn_debug_info_mt6886 {
	char tag[CONN_DEBUG_INFO_SIZE][DEBUG_TAG_SIZE];
	unsigned int wr_addr[CONN_DEBUG_INFO_SIZE];
	int wr_addr_lsb[CONN_DEBUG_INFO_SIZE];
	int wr_addr_msb[CONN_DEBUG_INFO_SIZE];
	unsigned int wr_data[CONN_DEBUG_INFO_SIZE];
	unsigned int rd_addr[CONN_DEBUG_INFO_SIZE];
	unsigned int rd_data[CONN_DEBUG_INFO_SIZE];
	int length;
};

void consys_debug_init_mt6886_debug_gen(void);
void consys_debug_deinit_mt6886_debug_gen(void);
void update_debug_read_info_mt6886_debug_gen(
		struct conn_debug_info_mt6886 *info,
		char *tag,
		unsigned int rd_addr,
		unsigned int rd_data);
void update_debug_write_info_mt6886_debug_gen(
		struct conn_debug_info_mt6886 *info,
		char *tag,
		unsigned int wr_addr,
		int wr_addr_lsb,
		int wr_addr_msb,
		unsigned int wr_data);
void consys_print_power_debug_dbg_level_0_mt6886_debug_gen(
		int level,
		struct conn_debug_info_mt6886 *pdbg_level_0_info);
void consys_print_power_debug_dbg_level_1_mt6886_debug_gen(
		int level,
		struct conn_debug_info_mt6886 *pdbg_level_1_info);
void consys_print_power_debug_dbg_level_2_mt6886_debug_gen(
		int level,
		struct conn_debug_info_mt6886 *pdbg_level_2_info);
void consys_print_bus_debug_dbg_level_1_mt6886_debug_gen(
		int level,
		struct conn_debug_info_mt6886 *pdbg_level_1_info);
void consys_print_bus_debug_dbg_level_2_mt6886_debug_gen(
		int level,
		struct conn_debug_info_mt6886 *pdbg_level_2_info);
void consys_print_bus_slpprot_debug_dbg_level_2_mt6886_debug_gen(
		int level,
		struct conn_debug_info_mt6886 *pdbg_level_2_info);
void consys_print_bus_slpprot_debug_dbg_level_0_mt6886_debug_gen(
		int level,
		struct conn_debug_info_mt6886 *pdbg_level_0_info);

/**********************************************************************************************************/
/* Base: SPM_REG_BASE (0x1C00_1000)                                                                       */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_SPM_REQ_STA_0_OFFSET_ADDR                                                  0x848
#define CONSYS_DBG_GEN_SPM_REQ_STA_1_OFFSET_ADDR                                                  0x84C
#define CONSYS_DBG_GEN_SPM_REQ_STA_2_OFFSET_ADDR                                                  0x850
#define CONSYS_DBG_GEN_SPM_REQ_STA_3_OFFSET_ADDR                                                  0x854
#define CONSYS_DBG_GEN_SPM_REQ_STA_4_OFFSET_ADDR                                                  0x858
#define CONSYS_DBG_GEN_CONN_PWR_CON_OFFSET_ADDR                                                   0xE04

/**********************************************************************************************************/
/* Base: CONSYS_DBG_GEN_TOPCKGEN_BASE_ADDR (0x1000_0000)                                                  */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_TOPCKGEN_BASE_ADDR                                                         0x10000000

/**********************************************************************************************************/
/* Base: CONN_HOST_CSR_TOP_BASE (0x1806_0000)                                                             */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR                               0x15C
#define CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR                                          0xa04
#define CONSYS_DBG_GEN_CONNSYS_PWR_STATES_OFFSET_ADDR                                             0xA10

/**********************************************************************************************************/
/* Base: CONSYS_DBG_GEN_CONN_DBG_CTL_BASE_ADDR (0x1802_3000)                                              */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_CONN_DBG_CTL_BASE_ADDR                                                     0x18023000
#define CONSYS_DBG_GEN_CONN_INFRA_MONFLAG_OUT_OFFSET_ADDR                                         0x200
#define CONSYS_DBG_GEN_CONN_INFRA_BUS_TIMEOUT_IRQ_OFFSET_ADDR                                     0x400
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR                                     0x404
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR                                     0x408
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR                                   0x40c
#define CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_INFO_OFFSET_ADDR                                  0x410
#define CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_ADDR_OFFSET_ADDR                                  0x414
#define CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_WDATA_OFFSET_ADDR                                 0x418
#define CONSYS_DBG_GEN_CONN_INFRA_VON_BUS_DEBUG_INFO_OFFSET_ADDR                                  0x41c

/**********************************************************************************************************/
/* Base: CONN_CFG_BASE (0x1801_1000)                                                                      */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_PLL_STATUS_OFFSET_ADDR                                                     0x30
#define CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR                                                      0x100
#define CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR                                                    0x134

/**********************************************************************************************************/
/* Base: CONN_CLKGEN_TOP_BASE (0x1801_2000)                                                               */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_CKGEN_BUS_OFFSET_ADDR                                                      0x50

/**********************************************************************************************************/
/* Base: CONN_CFG_ON_BASE (0x1800_1000)                                                                   */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_CONN_INFRA_CFG_RC_STATUS_OFFSET_ADDR                                       0x344
#define CONSYS_DBG_GEN_CONN_INFRA_CONN2AP_SLP_STATUS_OFFSET_ADDR                                  0x404
#define CONSYS_DBG_GEN_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_OFFSET_ADDR                              0x408
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_SLP_STATUS_OFFSET_ADDR                                  0x434
#define CONSYS_DBG_GEN_CONN_INFRA_WF_SLP_STATUS_OFFSET_ADDR                                       0x444
#define CONSYS_DBG_GEN_GALS_CONN2BT_SLP_STATUS_OFFSET_ADDR                                        0x454
#define CONSYS_DBG_GEN_GALS_BT2CONN_SLP_STATUS_OFFSET_ADDR                                        0x464
#define CONSYS_DBG_GEN_GALS_CONN2GPS_SLP_STATUS_OFFSET_ADDR                                       0x474
#define CONSYS_DBG_GEN_GALS_GPS2CONN_SLP_STATUS_OFFSET_ADDR                                       0x484

/**********************************************************************************************************/
/* Base: CONN_RGU_ON_BASE (0x1800_0000)                                                                   */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_WFSYS_ON_TOP_PWR_ST_OFFSET_ADDR                                            0x400
#define CONSYS_DBG_GEN_BGFSYS_ON_TOP_PWR_ST_OFFSET_ADDR                                           0x404

/**********************************************************************************************************/
/* Base: CONN_WT_SLP_CTL_REG_BASE (0x1800_3000)                                                           */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_WB_CK_STA_OFFSET_ADDR                                                      0xA8
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_0_OFFSET_ADDR                                                0x120
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_1_OFFSET_ADDR                                                0x124
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_2_OFFSET_ADDR                                                0x128
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_3_OFFSET_ADDR                                                0x12C
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_4_OFFSET_ADDR                                                0x130
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_5_OFFSET_ADDR                                                0x134

/**********************************************************************************************************/
/* Base: CONSYS_DBG_GEN_CONN_INFRA_SYSRAM_BASE_OFFSET_ADDR (0x1805_0000)                                  */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_CONN_INFRA_SYSRAM_BASE_OFFSET_ADDR                                         0x18050000

/**********************************************************************************************************/
/* Base: CONN_RF_SPI_MST_REG_BASE (0x1804_2000)                                                           */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_SPI_STA_OFFSET_ADDR                                                        0x0
#define CONSYS_DBG_GEN_SPI_CRTL_OFFSET_ADDR                                                       0x4
#define CONSYS_DBG_GEN_SPI_TOP_ADDR_OFFSET_ADDR                                                   0x50
#define CONSYS_DBG_GEN_SPI_TOP_WDAT_OFFSET_ADDR                                                   0x54
#define CONSYS_DBG_GEN_SPI_TOP_RDAT_OFFSET_ADDR                                                   0x58
#define CONSYS_DBG_GEN_SPI_HSCK_CTL_OFFSET_ADDR                                                   0x108

/**********************************************************************************************************/
/* Base: CONN_BUS_CR_ON_BASE (0x1800_e000)                                                                */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_0_OFFSET_ADDR                           0x3c
#define CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_1_OFFSET_ADDR                           0x40
#define CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_2_OFFSET_ADDR                           0x44

/**********************************************************************************************************/
/* Base: CONN_OFF_DEBUG_CTRL_AO_BASE (0x1804_d000)                                                        */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_OFFSET_ADDR       0x0
#define CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL1_OFFSET_ADDR       0x4
#define CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL2_OFFSET_ADDR       0x8
#define CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT0_OFFSET_ADDR     0x400
#define CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT1_OFFSET_ADDR     0x404
#define CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT2_OFFSET_ADDR     0x408
#define CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT3_OFFSET_ADDR     0x40c
#define CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT4_OFFSET_ADDR     0x410
#define CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT5_OFFSET_ADDR     0x414
#define CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT6_OFFSET_ADDR     0x418

/**********************************************************************************************************/
/* Base: INFRACFG_AO_REG_BASE (0x1000_1000)                                                               */
/**********************************************************************************************************/
#define CONSYS_DBG_GEN_INFRASYS_PROTECT_RDY_STA_1_OFFSET_ADDR                                     0xc5c
#define CONSYS_DBG_GEN_MCU_CONNSYS_PROTECT_RDY_STA_0_OFFSET_ADDR                                  0xc9c

#endif /* MT6886_DEBUG_GEN_H */
