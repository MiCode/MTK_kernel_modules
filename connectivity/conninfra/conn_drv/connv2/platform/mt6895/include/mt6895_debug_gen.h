/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


/* AUTO-GENERATED FILE.  DO NOT MODIFY.
 *
 * This file, mt6895_debug_gen.h was automatically generated
 * by the tool from the DEBUG data DE provided.
 * It should not be modified by hand.
 *
 * Reference debug file,
 * - [Lxxxn]connsys_power_debug.xlsx (Modified date: 2021-12-28)
 * - [Lxxxn]conn_infra_bus_debug_ctrl.xlsx (Modified date: 2021-10-14)
 */


#ifndef _PLATFORM_MT6895_DEBUG_GEN_H_
#define _PLATFORM_MT6895_DEBUG_GEN_H_

#define CONN_DEBUG_INFO_SIZE 256
#define DEBUG_TAG_SIZE 10

struct conn_debug_info_mt6895 {
	char tag[CONN_DEBUG_INFO_SIZE][DEBUG_TAG_SIZE];
	unsigned int wr_addr[CONN_DEBUG_INFO_SIZE];
	int wr_addr_lsb[CONN_DEBUG_INFO_SIZE];
	int wr_addr_msb[CONN_DEBUG_INFO_SIZE];
	unsigned int wr_data[CONN_DEBUG_INFO_SIZE];
	unsigned int rd_addr[CONN_DEBUG_INFO_SIZE];
	unsigned int rd_data[CONN_DEBUG_INFO_SIZE];
	int length;
};

void consys_debug_init_mt6895_debug_gen(void);
void consys_debug_deinit_mt6895_debug_gen(void);
void update_debug_read_info_mt6895_debug_gen(
		struct conn_debug_info_mt6895 *info,
		char *tag,
		unsigned int rd_addr,
		unsigned int rd_data);
void update_debug_write_info_mt6895_debug_gen(
		struct conn_debug_info_mt6895 *info,
		char *tag,
		unsigned int wr_addr,
		int wr_addr_lsb,
		int wr_addr_msb,
		unsigned int wr_data);
void consys_print_power_debug_dbg_level_0_mt6895_debug_gen(
		int level,
		struct conn_debug_info_mt6895 *pdbg_level_0_info);
void consys_print_power_debug_dbg_level_1_mt6895_debug_gen(
		int level,
		struct conn_debug_info_mt6895 *pdbg_level_1_info);
void consys_print_power_debug_dbg_level_2_mt6895_debug_gen(
		int level,
		struct conn_debug_info_mt6895 *pdbg_level_2_info);
void consys_print_bus_debug_dbg_level_1_mt6895_debug_gen(
		int level,
		struct conn_debug_info_mt6895 *pdbg_level_1_info);
void consys_print_bus_debug_dbg_level_2_mt6895_debug_gen(
		int level,
		struct conn_debug_info_mt6895 *pdbg_level_2_info);
void consys_print_bus_slpprot_debug_dbg_level_2_mt6895_debug_gen(
		int level,
		struct conn_debug_info_mt6895 *pdbg_level_2_info);
void consys_print_bus_slpprot_debug_dbg_level_0_mt6895_debug_gen(
		int level,
		struct conn_debug_info_mt6895 *pdbg_level_0_info);

/*************************************************************************************/
/* Base: SPM_REG_BASE (0x1C00_1000)                                                  */
/*************************************************************************************/
#define CONSYS_DBG_GEN_SPM_REQ_STA_1_OFFSET_ADDR                             0x83C
#define CONSYS_DBG_GEN_SPM_REQ_STA_2_OFFSET_ADDR                             0x840
#define CONSYS_DBG_GEN_CONN_PWR_CON_OFFSET_ADDR                              0xE04

/*************************************************************************************/
/* Base: CONSYS_DBG_GEN_SRCLKENRC_BASE_ADDR (0x1C00_D000)                            */
/*************************************************************************************/
#define CONSYS_DBG_GEN_SRCLKENRC_BASE_ADDR                                   0x1C00D000
#define CONSYS_DBG_GEN_FSM_STA_0_OFFSET_ADDR                                 0x100
#define CONSYS_DBG_GEN_CMD_STA_0_OFFSET_ADDR                                 0x104
#define CONSYS_DBG_GEN_CMD_STA_1_OFFSET_ADDR                                 0x108
#define CONSYS_DBG_GEN_SPI_STA_0_OFFSET_ADDR                                 0x10C
#define CONSYS_DBG_GEN_PI_PO_STA_0_OFFSET_ADDR                               0x110
#define CONSYS_DBG_GEN_M00_REQ_STA_0_OFFSET_ADDR                             0x114
#define CONSYS_DBG_GEN_M01_REQ_STA_0_OFFSET_ADDR                             0x118
#define CONSYS_DBG_GEN_M02_REQ_STA_0_OFFSET_ADDR                             0x11C
#define CONSYS_DBG_GEN_M03_REQ_STA_0_OFFSET_ADDR                             0x120
#define CONSYS_DBG_GEN_M04_REQ_STA_0_OFFSET_ADDR                             0x124
#define CONSYS_DBG_GEN_M05_REQ_STA_0_OFFSET_ADDR                             0x128
#define CONSYS_DBG_GEN_M06_REQ_STA_0_OFFSET_ADDR                             0x12C
#define CONSYS_DBG_GEN_M07_REQ_STA_0_OFFSET_ADDR                             0x130
#define CONSYS_DBG_GEN_M08_REQ_STA_0_OFFSET_ADDR                             0x134
#define CONSYS_DBG_GEN_M09_REQ_STA_0_OFFSET_ADDR                             0x138
#define CONSYS_DBG_GEN_M10_REQ_STA_0_OFFSET_ADDR                             0x13C
#define CONSYS_DBG_GEN_M11_REQ_STA_0_OFFSET_ADDR                             0x140
#define CONSYS_DBG_GEN_M12_REQ_STA_0_OFFSET_ADDR                             0x144
#define CONSYS_DBG_GEN_M13_REQ_STA_0_OFFSET_ADDR                             0x148
#define CONSYS_DBG_GEN_DEBUG_STA_0_OFFSET_ADDR                               0x14C
#define CONSYS_DBG_GEN_SPMI_P_STA_0_OFFSET_ADDR                              0x150
#define CONSYS_DBG_GEN_DEBUG_TRACE_0_LSB_OFFSET_ADDR                         0x700
#define CONSYS_DBG_GEN_DEBUG_TRACE_0_MSB_OFFSET_ADDR                         0x704
#define CONSYS_DBG_GEN_DEBUG_TRACE_1_LSB_OFFSET_ADDR                         0x708
#define CONSYS_DBG_GEN_DEBUG_TRACE_1_MSB_OFFSET_ADDR                         0x70C
#define CONSYS_DBG_GEN_DEBUG_TRACE_2_LSB_OFFSET_ADDR                         0x710
#define CONSYS_DBG_GEN_DEBUG_TRACE_2_MSB_OFFSET_ADDR                         0x714
#define CONSYS_DBG_GEN_DEBUG_TRACE_3_LSB_OFFSET_ADDR                         0x718
#define CONSYS_DBG_GEN_DEBUG_TRACE_3_MSB_OFFSET_ADDR                         0x71C
#define CONSYS_DBG_GEN_DEBUG_TRACE_4_LSB_OFFSET_ADDR                         0x720
#define CONSYS_DBG_GEN_DEBUG_TRACE_4_MSB_OFFSET_ADDR                         0x724
#define CONSYS_DBG_GEN_DEBUG_TRACE_5_LSB_OFFSET_ADDR                         0x728
#define CONSYS_DBG_GEN_DEBUG_TRACE_5_MSB_OFFSET_ADDR                         0x72C
#define CONSYS_DBG_GEN_DEBUG_TRACE_6_LSB_OFFSET_ADDR                         0x730
#define CONSYS_DBG_GEN_DEBUG_TRACE_6_MSB_OFFSET_ADDR                         0x734
#define CONSYS_DBG_GEN_DEBUG_TRACE_7_LSB_OFFSET_ADDR                         0x738
#define CONSYS_DBG_GEN_DEBUG_TRACE_7_MSB_OFFSET_ADDR                         0x73C
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_0_LSB_OFFSET_ADDR                     0x740
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_0_MSB_OFFSET_ADDR                     0x744
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_1_LSB_OFFSET_ADDR                     0x748
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_1_MSB_OFFSET_ADDR                     0x74C
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_2_LSB_OFFSET_ADDR                     0x750
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_2_MSB_OFFSET_ADDR                     0x754
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_3_LSB_OFFSET_ADDR                     0x758
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_3_MSB_OFFSET_ADDR                     0x75C
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_4_LSB_OFFSET_ADDR                     0x760
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_4_MSB_OFFSET_ADDR                     0x764
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_5_LSB_OFFSET_ADDR                     0x768
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_5_MSB_OFFSET_ADDR                     0x76C
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_6_LSB_OFFSET_ADDR                     0x770
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_6_MSB_OFFSET_ADDR                     0x774
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_7_LSB_OFFSET_ADDR                     0x778
#define CONSYS_DBG_GEN_SYS_TIMER_LATCH_7_MSB_OFFSET_ADDR                     0x77C

/*************************************************************************************/
/* Base: CONSYS_DBG_GEN_TOPCKGEN_BASE_ADDR (0x1000_0000)                             */
/*************************************************************************************/
#define CONSYS_DBG_GEN_TOPCKGEN_BASE_ADDR                                    0x10000000

/*************************************************************************************/
/* Base: CONN_HOST_CSR_TOP_BASE (0x1806_0000)                                        */
/*************************************************************************************/
#define CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR          0x15C
#define CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR                     0xa04
#define CONSYS_DBG_GEN_CONNSYS_PWR_STATES_OFFSET_ADDR                        0xA10

/*************************************************************************************/
/* Base: CONSYS_DBG_GEN_CONN_DBG_CTL_BASE_ADDR (0x1802_3000)                         */
/*************************************************************************************/
#define CONSYS_DBG_GEN_CONN_DBG_CTL_BASE_ADDR                                0x18023000
#define CONSYS_DBG_GEN_CONN_INFRA_MONFLAG_OUT_OFFSET_ADDR                    0x200
#define CONSYS_DBG_GEN_CONN_INFRA_BUS_TIMEOUT_IRQ_OFFSET_ADDR                0x400
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR                0x404
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR                0x408
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR              0x40c
#define CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_INFO_OFFSET_ADDR             0x410
#define CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_ADDR_OFFSET_ADDR             0x414
#define CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_WDATA_OFFSET_ADDR            0x418
#define CONSYS_DBG_GEN_CONN_INFRA_VON_BUS_DEBUG_INFO_OFFSET_ADDR             0x41c

/*************************************************************************************/
/* Base: CONN_CFG_BASE (0x1801_1000)                                                 */
/*************************************************************************************/
#define CONSYS_DBG_GEN_PLL_STATUS_OFFSET_ADDR                                0x30
#define CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR                                 0x100
#define CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR                               0x134

/*************************************************************************************/
/* Base: CONN_CLKGEN_TOP_BASE (0x1801_2000)                                          */
/*************************************************************************************/
#define CONSYS_DBG_GEN_CKGEN_BUS_OFFSET_ADDR                                 0x50

/*************************************************************************************/
/* Base: CONN_CFG_ON_BASE (0x1800_1000)                                              */
/*************************************************************************************/
#define CONSYS_DBG_GEN_CONN_INFRA_CFG_RC_STATUS_OFFSET_ADDR                  0x344
#define CONSYS_DBG_GEN_CONN_INFRA_CONN2AP_SLP_STATUS_OFFSET_ADDR             0x404
#define CONSYS_DBG_GEN_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_OFFSET_ADDR         0x408
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_SLP_STATUS_OFFSET_ADDR             0x434
#define CONSYS_DBG_GEN_CONN_INFRA_WF_SLP_STATUS_OFFSET_ADDR                  0x444
#define CONSYS_DBG_GEN_GALS_CONN2BT_SLP_STATUS_OFFSET_ADDR                   0x454
#define CONSYS_DBG_GEN_GALS_BT2CONN_SLP_STATUS_OFFSET_ADDR                   0x464
#define CONSYS_DBG_GEN_GALS_CONN2GPS_SLP_STATUS_OFFSET_ADDR                  0x474
#define CONSYS_DBG_GEN_GALS_GPS2CONN_SLP_STATUS_OFFSET_ADDR                  0x484

/*************************************************************************************/
/* Base: CONN_RGU_ON_BASE (0x1800_0000)                                              */
/*************************************************************************************/
#define CONSYS_DBG_GEN_WFSYS_ON_TOP_PWR_ST_OFFSET_ADDR                       0x400
#define CONSYS_DBG_GEN_BGFSYS_ON_TOP_PWR_ST_OFFSET_ADDR                      0x404

/*************************************************************************************/
/* Base: CONN_WT_SLP_CTL_REG_BASE (0x1800_3000)                                      */
/*************************************************************************************/
#define CONSYS_DBG_GEN_WB_CK_STA_OFFSET_ADDR                                 0xA8
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_0_OFFSET_ADDR                           0x120
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_1_OFFSET_ADDR                           0x124
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_2_OFFSET_ADDR                           0x128
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_3_OFFSET_ADDR                           0x12C
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_4_OFFSET_ADDR                           0x130
#define CONSYS_DBG_GEN_WB_SLP_TOP_CK_5_OFFSET_ADDR                           0x134

/*************************************************************************************/
/* Base: CONSYS_DBG_GEN_CONN_INFRA_SYSRAM_BASE_OFFSET_ADDR (0x1805_0000)             */
/*************************************************************************************/
#define CONSYS_DBG_GEN_CONN_INFRA_SYSRAM_BASE_OFFSET_ADDR                    0x18050000

/*************************************************************************************/
/* Base: CONN_RF_SPI_MST_REG_BASE (0x1804_2000)                                      */
/*************************************************************************************/
#define CONSYS_DBG_GEN_SPI_STA_OFFSET_ADDR                                   0x0
#define CONSYS_DBG_GEN_SPI_CRTL_OFFSET_ADDR                                  0x4
#define CONSYS_DBG_GEN_SPI_TOP_ADDR_OFFSET_ADDR                              0x50
#define CONSYS_DBG_GEN_SPI_TOP_WDAT_OFFSET_ADDR                              0x54
#define CONSYS_DBG_GEN_SPI_TOP_RDAT_OFFSET_ADDR                              0x58
#define CONSYS_DBG_GEN_SPI_HSCK_CTL_OFFSET_ADDR                              0x108

/*************************************************************************************/
/* Base: CONN_BUS_CR_ON_BASE (0x1800_e000)                                           */
/*************************************************************************************/
#define CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_0_OFFSET_ADDR      0x3c
#define CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_1_OFFSET_ADDR      0x40
#define CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_2_OFFSET_ADDR      0x44

/*************************************************************************************/
/* Base: CONN_OFF_DEBUG_CTRL_AO_BASE (0x1804_d000)                                   */
/*************************************************************************************/
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUG_CTRL_AO_1804d000_OFFSET_ADDR     0x0
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUG_CTRL_AO_1804d004_OFFSET_ADDR     0x4
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUG_CTRL_AO_1804d008_OFFSET_ADDR     0x8
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUG_CTRL_AO_1804d400_OFFSET_ADDR     0x400
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUG_CTRL_AO_1804d404_OFFSET_ADDR     0x404
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUG_CTRL_AO_1804d408_OFFSET_ADDR     0x408
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUG_CTRL_AO_1804d40c_OFFSET_ADDR     0x40c
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUG_CTRL_AO_1804d410_OFFSET_ADDR     0x410
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUG_CTRL_AO_1804d414_OFFSET_ADDR     0x414
#define CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUG_CTRL_AO_1804d418_OFFSET_ADDR     0x418

/*************************************************************************************/
/* Base: INFRACFG_AO_REG_BASE (0x1000_1000)                                          */
/*************************************************************************************/
#define CONSYS_DBG_GEN_INFRASYS_PROTECT_RDY_STA_1_OFFSET_ADDR                0xc5c
#define CONSYS_DBG_GEN_MCU_CONNSYS_PROTECT_RDY_STA_0_OFFSET_ADDR             0xc9c

#endif /* _PLATFORM_MT6895_DEBUG_GEN_H_ */
