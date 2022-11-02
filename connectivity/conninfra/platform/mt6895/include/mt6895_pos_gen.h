/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


/* AUTO-GENERATED FILE.  DO NOT MODIFY.
 *
 * This file, mt6895_pos_gen.h was automatically generated
 * by the tool from the POS data DE provided.
 * It should not be modified by hand.
 *
 * Reference POS file,
 * - Pxxxxn_power_on_sequence_20211124.xlsx
 * - Pxxxxn_conn_infra_sub_task_211117.xlsx
 * - conn_infra_cmdbt_instr_autogen_20211025.txt
 */


#ifndef _PLATFORM_MT6895_POS_GEN_H_
#define _PLATFORM_MT6895_POS_GEN_H_

void consys_set_if_pinmux_mt6895_gen(unsigned int enable);
void consys_set_gpio_tcxo_mode_mt6895_gen(unsigned int tcxo_mode, unsigned int enable);
int consys_conninfra_on_power_ctrl_mt6895_gen(unsigned int enable);
void consys_update_ap2conn_hclk_mt6895_gen(void);
int consys_polling_chipid_mt6895_gen(unsigned int *pconsys_ver_id);
unsigned int consys_emi_set_remapping_reg_mt6895_gen(
		phys_addr_t con_emi_base_addr,
		phys_addr_t md_shared_emi_base_addr,
		phys_addr_t gps_emi_base_addr,
		unsigned int emi_base_addr_offset);
void consys_init_conninfra_sysram_mt6895_gen(void);
void connsys_get_d_die_efuse_mt6895_gen(unsigned int *p_d_die_efuse);
int connsys_d_die_cfg_mt6895_gen(void);
void connsys_wt_slp_top_ctrl_adie6637_mt6895_gen(void);
int connsys_a_die_switch_to_gpio_mode_mt6895_gen(void);
int connsys_adie_top_ck_en_ctl_mt6895_gen(unsigned int enable);
int connsys_a_die_cfg_adie6637_deassert_adie_reset_mt6895_gen(void);
int connsys_a_die_cfg_adie6637_read_adie_id_mt6895_gen(
		unsigned int *padie_id,
		unsigned int *phw_ver_id);
int connsys_a_die_cfg_adie6637_PART1_mt6895_gen(void);
int connsys_a_die_efuse_read_adie6637_check_efuse_valid_mt6895_gen(bool *pefuse_valid);
void connsys_a_die_efuse_read_adie6637_get_efuse_count_mt6895_gen(unsigned int *pefuse_count);
int connsys_a_die_efuse_read_adie6637_get_efuse0_info_mt6895_gen(
		bool efuse_valid,
		unsigned int *pefuse0);
int connsys_a_die_efuse_read_adie6637_get_efuse1_info_mt6895_gen(
		bool efuse_valid,
		unsigned int *pefuse1);
int connsys_a_die_efuse_read_adie6637_get_efuse2_info_mt6895_gen(
		bool efuse_valid,
		unsigned int *pefuse2);
int connsys_a_die_efuse_read_adie6637_get_efuse3_info_mt6895_gen(
		bool efuse_valid,
		unsigned int *pefuse3);
int connsys_a_die_thermal_cal_adie6637_conf_mt6895_gen(
		bool efuse_valid,
		unsigned int *pefuse_list,
		unsigned int efuse_size,
		int *pslop_molecule,
		int *pthermal_b,
		int *poffset);
int connsys_a_die_cfg_adie6637_PART2_mt6895_gen(unsigned int hw_ver_id);
int connsys_a_die_cfg_deassert_adie_reset_mt6895_gen(void);
int connsys_a_die_cfg_read_adie_id_mt6895_gen(unsigned int *padie_id, unsigned int *phw_ver_id);
int connsys_a_die_efuse_read_get_efuse_info_mt6895_gen(
		void __iomem **psysram_efuse_list,
		int *pslop_molecule,
		int *pthermal_b,
		int *poffset);
int connsys_a_die_cfg_PART2_mt6895_gen(unsigned int hw_ver_id);
int connsys_a_die_switch_to_conn_mode_mt6895_gen(void);
void connsys_wt_slp_top_power_saving_ctrl_adie6637_sleep_mode_1_mt6895_gen(void);
void connsys_wt_slp_top_power_saving_ctrl_adie6637_sleep_mode_2_mt6895_gen(void);
void connsys_wt_slp_top_power_saving_ctrl_adie6637_sleep_mode_3_mt6895_gen(void);
void connsys_wt_slp_top_power_saving_ctrl_adie6637_mt6895_gen(
		unsigned int hw_version,
		unsigned int sleep_mode);
void connsys_afe_sw_patch_mt6895_gen(void);
int connsys_afe_wbg_cal_mt6895_gen(
		unsigned int spi_semaphore_index,
		unsigned int spi_semaphore_timeout_usec);
int connsys_subsys_pll_initial_xtal_26000k_mt6895_gen(void);
int connsys_low_power_setting_mt6895_gen(void);
int consys_conninfra_wakeup_mt6895_gen(void);
int connsys_adie_clock_buffer_setting_mt6895_gen(
		unsigned int curr_status,
		unsigned int next_status,
		unsigned int hw_version,
		unsigned int spi_semaphore_index,
		unsigned int spi_semaphore_timeout_usec);
int consys_conninfra_sleep_mt6895_gen(void);

/****************************************************************************************************/
/* Base: GPIO_REG_BASE (0x1000_5000)                                                                */
/****************************************************************************************************/
#define CONSYS_GEN_GPIO_DIR5_OFFSET_ADDR                                                    0x50
#define CONSYS_GEN_GPIO_DOUT5_OFFSET_ADDR                                                   0x150
#define CONSYS_GEN_GPIO_MODE13_OFFSET_ADDR                                                  0x3D0
#define CONSYS_GEN_GPIO_MODE20_OFFSET_ADDR                                                  0x440
#define CONSYS_GEN_GPIO_MODE21_OFFSET_ADDR                                                  0x450

/****************************************************************************************************/
/* Base: IOCFG_RT_REG_BASE (0x11C3_0000)                                                            */
/****************************************************************************************************/
#define CONSYS_GEN_DRV_CFG2_OFFSET_ADDR                                                     0x20
#define CONSYS_GEN_IOCFG_RT_PD_CFG0_OFFSET_ADDR                                             0x80
#define CONSYS_GEN_IOCFG_RT_PU_CFG0_OFFSET_ADDR                                             0xA0

/****************************************************************************************************/
/* Base: IOCFG_RTT_REG_BASE (0x11EA_0000)                                                           */
/****************************************************************************************************/
#define CONSYS_GEN_DRV_CFG0_OFFSET_ADDR                                                     0x0
#define CONSYS_GEN_PD_CFG0_OFFSET_ADDR                                                      0x40
#define CONSYS_GEN_PU_CFG0_OFFSET_ADDR                                                      0x50

/****************************************************************************************************/
/* Base: CONSYS_GEN_IOCFG_TR_BASE_ADDR (0x11EB_0000)                                                */
/****************************************************************************************************/
#define CONSYS_GEN_IOCFG_TR_BASE_ADDR                                                       0x11EB0000
#define CONSYS_GEN_IOCFG_TR_DRV_CFG0_OFFSET_ADDR                                            0x0
#define CONSYS_GEN_DRV_CFG1_OFFSET_ADDR                                                     0x10

/****************************************************************************************************/
/* Base: SPM_REG_BASE (0x1C00_1000)                                                                 */
/****************************************************************************************************/
#define CONSYS_GEN_POWERON_CONFIG_EN_OFFSET_ADDR                                            0x0
#define CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR                                                 0xE04

/****************************************************************************************************/
/* Base: INFRACFG_AO_REG_BASE (0x1000_1000)                                                         */
/****************************************************************************************************/
#define CONSYS_GEN_INFRASYS_PROTECT_EN_STA_0_OFFSET_ADDR                                    0xC40
#define CONSYS_GEN_INFRASYS_PROTECT_RDY_STA_0_OFFSET_ADDR                                   0xC4C
#define CONSYS_GEN_INFRASYS_PROTECT_EN_STA_1_OFFSET_ADDR                                    0xC50
#define CONSYS_GEN_INFRASYS_PROTECT_RDY_STA_1_OFFSET_ADDR                                   0xC5C
#define CONSYS_GEN_MCU_CONNSYS_PROTECT_EN_STA_0_OFFSET_ADDR                                 0xC90
#define CONSYS_GEN_MCU_CONNSYS_PROTECT_RDY_STA_0_OFFSET_ADDR                                0xC9C

/****************************************************************************************************/
/* Base: CONSYS_GEN_TOPCKGEN_BASE_ADDR (0x1000_0000)                                                */
/****************************************************************************************************/
#define CONSYS_GEN_TOPCKGEN_BASE_ADDR                                                       0x10000000
#define CONSYS_GEN_CLK_CFG_UPDATE2_OFFSET_ADDR                                              0xc
#define CONSYS_GEN_CLK_CFG_20_SET_OFFSET_ADDR                                               0x154
#define CONSYS_GEN_CLK_CFG_20_CLR_OFFSET_ADDR                                               0x158

/****************************************************************************************************/
/* Base: CONN_CFG_BASE (0x1801_1000)                                                                */
/****************************************************************************************************/
#define CONSYS_GEN_IP_VERSION_OFFSET_ADDR                                                   0x0
#define CONSYS_GEN_EFUSE_OFFSET_ADDR                                                        0x20
#define CONSYS_GEN_CMDBT_FETCH_START_ADDR0_OFFSET_ADDR                                      0x50
#define CONSYS_GEN_EMI_CTL_0_OFFSET_ADDR                                                    0x100

/****************************************************************************************************/
/* Base: CONSYS_GEN_CONN_HW_VER (0x0205_0100)                                                       */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_HW_VER                                                              0x2050100

/****************************************************************************************************/
/* Base: CONN_BUS_CR_BASE (0x1804_B000)                                                             */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_OFF_BUS_TIMEOUT_CTRL_OFFSET_ADDR                              0x24
#define CONSYS_GEN_CONN_INFRA_CONN2AP_EMI_PATH_ADDR_START_OFFSET_ADDR                       0x70
#define CONSYS_GEN_CONN_INFRA_CONN2AP_EMI_PATH_ADDR_END_OFFSET_ADDR                         0x74
#define CONSYS_GEN_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_OFFSET_ADDR                              0x354
#define CONSYS_GEN_CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR_OFFSET_ADDR                         0x35C
#define CONSYS_GEN_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_OFFSET_ADDR                              0x360
#define CONSYS_GEN_CONN2AP_REMAP_WF_PERI_BASE_ADDR_OFFSET_ADDR                              0x364
#define CONSYS_GEN_CONN2AP_REMAP_BT_PERI_BASE_ADDR_OFFSET_ADDR                              0x368
#define CONSYS_GEN_CONN2AP_REMAP_GPS_PERI_BASE_ADDR_OFFSET_ADDR                             0x36C
#define CONSYS_GEN_SCPSYS_SRAM_BASE_ADDR_OFFSET_ADDR                                        0x370
#define CONSYS_GEN_LIGHT_SECURITY_CTRL_OFFSET_ADDR                                          0x374

/****************************************************************************************************/
/* Base: CONSYS_GEN_CONN_INFRA_SYSRAM_BASE_OFFSET_ADDR (0x1805_0000)                                */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_SYSRAM_BASE_OFFSET_ADDR                                       0x18050000

/****************************************************************************************************/
/* Base: CONN_RGU_ON_BASE (0x1800_0000)                                                             */
/****************************************************************************************************/
#define CONSYS_GEN_WFSYS_ON_TOP_PWR_CTL_OFFSET_ADDR                                         0x10
#define CONSYS_GEN_BGFYS_ON_TOP_PWR_CTL_OFFSET_ADDR                                         0x20
#define CONSYS_GEN_SYSRAM_HWCTL_PDN_OFFSET_ADDR                                             0x50
#define CONSYS_GEN_SYSRAM_HWCTL_SLP_OFFSET_ADDR                                             0x54
#define CONSYS_GEN_CO_EXT_MEM_HWCTL_PDN_OFFSET_ADDR                                         0x70
#define CONSYS_GEN_CO_EXT_MEM_HWCTL_SLP_OFFSET_ADDR                                         0x74

/****************************************************************************************************/
/* Base: CONN_WT_SLP_CTL_REG_BASE (0x1800_3000)                                                     */
/****************************************************************************************************/
#define CONSYS_GEN_WB_SLP_CTL_OFFSET_ADDR                                                   0x4
#define CONSYS_GEN_WB_BG_ADDR1_OFFSET_ADDR                                                  0x10
#define CONSYS_GEN_WB_BG_ADDR2_OFFSET_ADDR                                                  0x14
#define CONSYS_GEN_WB_BG_ADDR3_OFFSET_ADDR                                                  0x18
#define CONSYS_GEN_WB_BG_ADDR4_OFFSET_ADDR                                                  0x1C
#define CONSYS_GEN_WB_BG_ADDR5_OFFSET_ADDR                                                  0x20
#define CONSYS_GEN_WB_BG_ADDR6_OFFSET_ADDR                                                  0x24
#define CONSYS_GEN_WB_BG_ON1_OFFSET_ADDR                                                    0x30
#define CONSYS_GEN_WB_BG_ON2_OFFSET_ADDR                                                    0x34
#define CONSYS_GEN_WB_BG_ON3_OFFSET_ADDR                                                    0x38
#define CONSYS_GEN_WB_BG_ON4_OFFSET_ADDR                                                    0x3C
#define CONSYS_GEN_WB_BG_ON5_OFFSET_ADDR                                                    0x40
#define CONSYS_GEN_WB_BG_ON6_OFFSET_ADDR                                                    0x44
#define CONSYS_GEN_WB_BG_OFF1_OFFSET_ADDR                                                   0x50
#define CONSYS_GEN_WB_BG_OFF2_OFFSET_ADDR                                                   0x54
#define CONSYS_GEN_WB_BG_OFF3_OFFSET_ADDR                                                   0x58
#define CONSYS_GEN_WB_BG_OFF4_OFFSET_ADDR                                                   0x5C
#define CONSYS_GEN_WB_BG_OFF5_OFFSET_ADDR                                                   0x60
#define CONSYS_GEN_WB_BG_OFF6_OFFSET_ADDR                                                   0x64
#define CONSYS_GEN_WB_WF_CK_ADDR_OFFSET_ADDR                                                0x70
#define CONSYS_GEN_WB_WF_WAKE_ADDR_OFFSET_ADDR                                              0x74
#define CONSYS_GEN_WB_WF_ZPS_ADDR_OFFSET_ADDR                                               0x78
#define CONSYS_GEN_WB_BT_CK_ADDR_OFFSET_ADDR                                                0x7C
#define CONSYS_GEN_WB_BT_WAKE_ADDR_OFFSET_ADDR                                              0x80
#define CONSYS_GEN_WB_TOP_CK_ADDR_OFFSET_ADDR                                               0x84
#define CONSYS_GEN_WB_GPS_CK_ADDR_OFFSET_ADDR                                               0x88
#define CONSYS_GEN_WB_WF_B0_CMD_ADDR_OFFSET_ADDR                                            0x8c
#define CONSYS_GEN_WB_WF_B1_CMD_ADDR_OFFSET_ADDR                                            0x90
#define CONSYS_GEN_WB_GPS_RFBUF_ADR_OFFSET_ADDR                                             0x94
#define CONSYS_GEN_WB_GPS_L5_EN_ADDR_OFFSET_ADDR                                            0x98
#define CONSYS_GEN_WB_SLP_TOP_CK_0_OFFSET_ADDR                                              0x120

/****************************************************************************************************/
/* Base: CONN_CFG_ON_BASE (0x1800_1000)                                                             */
/****************************************************************************************************/
#define CONSYS_GEN_ADIE_CTL_OFFSET_ADDR                                                     0x10
#define CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL0_OFFSET_ADDR                                      0x200
#define CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL1_OFFSET_ADDR                                      0x210
#define CONSYS_GEN_OSC_CTL_0_OFFSET_ADDR                                                    0x300
#define CONSYS_GEN_OSC_CTL_1_OFFSET_ADDR                                                    0x304
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_OFFSET_ADDR                                      0x340
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_OFFSET_ADDR                                      0x348
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_GPS_OFFSET_ADDR                                  0x350
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_GPS_OFFSET_ADDR                                  0x354
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_BT_OFFSET_ADDR                                   0x360
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_BT_OFFSET_ADDR                                   0x364
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_WF_OFFSET_ADDR                                   0x370
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_WF_OFFSET_ADDR                                   0x374
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_TOP_OFFSET_ADDR                                  0x380
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_TOP_OFFSET_ADDR                                  0x384

/****************************************************************************************************/
/* Base: SYS_SPI_TOP (MT6637)                                                                       */
/****************************************************************************************************/
#define CONSYS_GEN_ADIE6637_ATOP_RG_TOP_PULL                                                0xC
#define CONSYS_GEN_ADIE6637_ATOP_CHIP_ID                                                    0x2C
#define CONSYS_GEN_ADIE6637_ATOP_RG_TOP_THADC_BG                                            0x34
#define CONSYS_GEN_ADIE6637_ATOP_RG_TOP_THADC                                               0x38
#define CONSYS_GEN_ADIE6637_ATOP_WRI_CTR2                                                   0x64
#define CONSYS_GEN_ADIE6637_ATOP_RG_ENCAL_WFBT_IF_SW_01                                     0x70
#define CONSYS_GEN_ADIE6637_ATOP_SMCTK11                                                    0xBC
#define CONSYS_GEN_ADIE6637_ATOP_EFUSE_CTRL                                                 0x108
#define CONSYS_GEN_ADIE6637_ATOP_EFUSE_RDATA0                                               0x130
#define CONSYS_GEN_ADIE6637_ATOP_EFUSE_RDATA1                                               0x134
#define CONSYS_GEN_ADIE6637_ATOP_EFUSE_RDATA2                                               0x138
#define CONSYS_GEN_ADIE6637_ATOP_EFUSE_RDATA3                                               0x13c
#define CONSYS_GEN_ADIE6637_ATOP_RG_FMCTL4                                                  0x160
#define CONSYS_GEN_ADIE6637_ATOP_RG_BT0_BG_TIME                                             0x304
#define CONSYS_GEN_ADIE6637_ATOP_RG_BT0_ISO_TIME                                            0x308
#define CONSYS_GEN_ADIE6637_ATOP_RG_WF0_BG                                                  0x344
#define CONSYS_GEN_ADIE6637_ATOP_RG_WF1_RFLDO_TIME                                          0x358
#define CONSYS_GEN_ADIE6637_ATOP_RG_WF1_BG                                                  0x374
#define CONSYS_GEN_ADIE6637_ATOP_RG_WF2_POS_01                                              0x380
#define CONSYS_GEN_ADIE6637_ATOP_RG_WF2_TIME_COUNT_01                                       0x384
#define CONSYS_GEN_ADIE6637_ATOP_RG_WF2_POS_02                                              0x390
#define CONSYS_GEN_ADIE6637_ATOP_RG_WF2_TIME_COUNT_02                                       0x394
#define CONSYS_GEN_ADIE6637_ATOP_RG_BT0_XOBUF_TIME                                          0x430
#define CONSYS_GEN_ADIE6637_ATOP_RG_BT0_CKEN_TIME                                           0x438
#define CONSYS_GEN_ADIE6637_ATOP_RG_WF0_TOP_01                                              0x754
#define CONSYS_GEN_ADIE6637_ATOP_RG_WF1_TOP_01                                              0x758
#define CONSYS_GEN_ADIE6637_ATOP_RG_TOP_XO_02                                               0xB04
#define CONSYS_GEN_ADIE6637_ATOP_RG_TOP_XO_03                                               0xB08
#define CONSYS_GEN_ADIE6637_ATOP_RG_TOP_XO_07                                               0xB18
#define CONSYS_GEN_ADIE6637_ATOP_RG_WF1_RFDIG_OFF_TIME                                      0xDF8
#define CONSYS_GEN_ADIE6637_ATOP_RG_WAKEUPSLEEP_PON_OFF                                     0xFFC

/****************************************************************************************************/
/* Base: CONN_RF_SPI_MST_REG_BASE (0x1804_2000)                                                     */
/****************************************************************************************************/
#define CONSYS_GEN_FM_CTRL_OFFSET_ADDR                                                      0xC

/****************************************************************************************************/
/* Base: CONN_THERM_CTL_BASE (0x1804_0000)                                                          */
/****************************************************************************************************/
#define CONSYS_GEN_THERM_AADDR_OFFSET_ADDR                                                  0x18

/****************************************************************************************************/
/* Base: CONN_AFE_CTL_BASE (0x1804_1000)                                                            */
/****************************************************************************************************/
#define CONSYS_GEN_RG_DIG_EN_01_OFFSET_ADDR                                                 0x0
#define CONSYS_GEN_RG_DIG_EN_02_OFFSET_ADDR                                                 0x4
#define CONSYS_GEN_RG_DIG_EN_03_OFFSET_ADDR                                                 0x8
#define CONSYS_GEN_RG_DIG_TOP_01_OFFSET_ADDR                                                0xC
#define CONSYS_GEN_RG_WBG_PLL_01_OFFSET_ADDR                                                0x24
#define CONSYS_GEN_RG_WBG_PLL_02_OFFSET_ADDR                                                0x28
#define CONSYS_GEN_RG_WBG_PLL_03_OFFSET_ADDR                                                0x2C
#define CONSYS_GEN_RG_WBG_PLL_04_OFFSET_ADDR                                                0x30
#define CONSYS_GEN_RG_WBG_PLL_05_OFFSET_ADDR                                                0x34
#define CONSYS_GEN_RG_WBG_GL1_01_OFFSET_ADDR                                                0x40
#define CONSYS_GEN_RG_WBG_GL1_02_OFFSET_ADDR                                                0x44
#define CONSYS_GEN_RG_WBG_BT0_TX_03_OFFSET_ADDR                                             0x58
#define CONSYS_GEN_RG_WBG_WF0_TX_03_OFFSET_ADDR                                             0x78
#define CONSYS_GEN_RG_WBG_WF1_TX_03_OFFSET_ADDR                                             0x94
#define CONSYS_GEN_RG_PLL_STB_TIME_OFFSET_ADDR                                              0xF4
#define CONSYS_GEN_RG_WBG_GL5_01_OFFSET_ADDR                                                0x100

/****************************************************************************************************/
/* Base: SYS_SPI_TOP                                                                                */
/****************************************************************************************************/
#define CONSYS_GEN_ATOP_RG_ENCAL_WBTAC_IF_SW                                                0x70
#define CONSYS_GEN_ATOP_RG_TOP_XO_2                                                         0xb04
#define CONSYS_GEN_ATOP_RG_TOP_XO_3                                                         0xb08
#define CONSYS_GEN_ATOP_RG_TOP_XO_7                                                         0xb18

/****************************************************************************************************/
/* Base: CONN_BUS_CR_ON_BASE (0x1800_E000)                                                          */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_VON_BUS_TIMEOUT_CTRL_OFFSET_ADDR                              0x24
#define CONSYS_GEN_CONN_INFRA_ON_BUS_TIMEOUT_CTRL_OFFSET_ADDR                               0x38
#define CONSYS_GEN_CONN_VON_BUS_DCM_CTL_1_OFFSET_ADDR                                       0x104
#define CONSYS_GEN_CONN_OFF_BUS_DCM_CTL_1_OFFSET_ADDR                                       0x110

/****************************************************************************************************/
/* Base: CONN_OFF_DEBUG_CTRL_AO_BASE (0x1804_D000)                                                  */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_OFFSET_ADDR     0x0

/****************************************************************************************************/
/* Base: CONN_CLKGEN_TOP_BASE (0x1801_2000)                                                         */
/****************************************************************************************************/
#define CONSYS_GEN_CKGEN_BUS_BPLL_DIV_1_OFFSET_ADDR                                         0x0
#define CONSYS_GEN_CKGEN_BUS_BPLL_DIV_2_OFFSET_ADDR                                         0x4
#define CONSYS_GEN_CKGEN_BUS_WPLL_DIV_1_OFFSET_ADDR                                         0x8
#define CONSYS_GEN_CKGEN_BUS_WPLL_DIV_2_OFFSET_ADDR                                         0xC
#define CONSYS_GEN_CLKGEN_RFSPI_CK_CTRL_OFFSET_ADDR                                         0x38
#define CONSYS_GEN_CKGEN_BUS_OFFSET_ADDR                                                    0x50
#define CONSYS_GEN_CKGEN_COEX_0_OFFSET_ADDR                                                 0x60
#define CONSYS_GEN_CKGEN_COEX_1_OFFSET_ADDR                                                 0x70

/****************************************************************************************************/
/* Base: CONN_HOST_CSR_TOP_BASE (0x1806_0000)                                                       */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_WAKEPU_TOP_OFFSET_ADDR                                        0x1a0
#define CONSYS_GEN_HOST_CONN_INFRA_SLP_CNT_CTL_OFFSET_ADDR                                  0x380

#endif /* _PLATFORM_MT6895_POS_GEN_H_ */
