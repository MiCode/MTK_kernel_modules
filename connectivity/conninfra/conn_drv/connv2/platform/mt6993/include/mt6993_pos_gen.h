/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */


/* AUTO-GENERATED FILE.  DO NOT MODIFY.
 *
 * This file, mt6993_pos_gen.h was automatically generated
 * by the tool from the POS data DE provided.
 * It should not be modified by hand.
 *
 * Reference POS file,
 * - Jxxxr_power_on_sequence_20241113_001.xlsx
 * - Jxxxr_conn_infra_sub_task_initial.xlsx
 * - conn_infra_cmdbt_instr_autogen_20241111.txt
 */


#ifndef MT6993_POS_GEN_H
#define MT6993_POS_GEN_H

int consys_conninfra_on_power_ctrl_mt6993_gen(
		unsigned int pbus_mode,
		unsigned int trapping,
		unsigned int enable);
int consys_polling_chipid_mt6993_gen(unsigned int *pconsys_ver_id);
unsigned int consys_emi_set_remapping_reg_mt6993_gen(
		phys_addr_t con_emi_base_addr,
		phys_addr_t md_shared_emi_base_addr,
		phys_addr_t gps_emi_base_addr,
		unsigned int emi_base_addr_offset);
void consys_init_conninfra_sysram_mt6993_gen(void);
void connsys_get_d_die_efuse_mt6993_gen(unsigned int *p_d_die_efuse);
int connsys_d_die_cfg_mt6993_gen(void);
void connsys_wt_slp_top_ctrl_adie6686_mt6993_gen(void);
int connsys_subsys_pll_initial_xtal_26000k_mt6993_gen(void);
int connsys_low_power_setting_mt6993_gen(void);
int consys_conninfra_wakeup_mt6993_gen(void);

/****************************************************************************************************/
/* Base: CONSYS_GEN_CKSYS_TSE_BASE_ADDR (0x1C21_0000)                                               */
/****************************************************************************************************/
#define CONSYS_GEN_CKSYS_TSE_BASE_ADDR                                                      0x1C210000
#define CONSYS_GEN_CLK_CFG_6_SET_OFFSET_ADDR                                                0x74
#define CONSYS_GEN_CLK_CFG_6_CLR_OFFSET_ADDR                                                0x78

/****************************************************************************************************/
/* Base: CONN_CFG_BASE (0x4001_1000)                                                                */
/****************************************************************************************************/
#define CONSYS_GEN_IP_VERSION_OFFSET_ADDR                                                   0x0
#define CONSYS_GEN_EFUSE_OFFSET_ADDR                                                        0x20
#define CONSYS_GEN_CMDBT_FETCH_START_ADDR0_OFFSET_ADDR                                      0x50
#define CONSYS_GEN_EMI_CTL_0_OFFSET_ADDR                                                    0x100
#define CONSYS_GEN_CONN_INFRA_CFG_EMI_CTL_0_OFFSET_ADDR                                     0x100
#define CONSYS_GEN_CONN_INFRA_CFG_EMI_CTL_WLA_OFFSET_ADDR                                   0x420
#define CONSYS_GEN_CONN_INFRA_CFG_CONN_INFRA_CFG_EMI_CTL_WLA_OFFSET_ADDR                    0x420
#define CONSYS_GEN_CONN_INFRA_CFG_EMI_CTL_WLA_REQ2ACK_TIMEOUT_OFFSET_ADDR                   0x424
#define CONSYS_GEN_CONN_INFRA_CFG_EMI_CTL_WLA_URG2ACK_TIMEOUT_OFFSET_ADDR                   0x428
#define CONSYS_GEN_CONN_INFRA_CFG_EMI_CTL_WLA_EN2RDY_TIMEOUT_OFFSET_ADDR                    0x42C
#define CONSYS_GEN_CONN_INFRA_CFG_EMI_CTL_HRT_TIMEOUT_OFFSET_ADDR                           0x430

/****************************************************************************************************/
/* Base: CONSYS_GEN_CONN_HW_VER (0x0205_0800)                                                       */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_HW_VER                                                              0x2050800

/****************************************************************************************************/
/* Base: CONN_BUS_CR_BASE (0x4004_B000)                                                             */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_OFF_BUS_TIMEOUT_CTRL_OFFSET_ADDR                              0x24
#define CONSYS_GEN_CONN_INFRA_BUS_CR_CONN_INFRA_OFF_BUS_TIMEOUT_CTRL_OFFSET_ADDR            0x24
#define CONSYS_GEN_CONN_INFRA_CONN2AP_EMI_PATH_ADDR_START_OFFSET_ADDR                       0x70
#define CONSYS_GEN_CONN_INFRA_CONN2AP_EMI_PATH_ADDR_END_OFFSET_ADDR                         0x74
#define CONSYS_GEN_CONN_INFRA_EMI_WLA_DEBUG_CTRL_ADDR                                       0x148
#define CONSYS_GEN_CONN2AP_REMAP_WF_PERI_BASE_ADDR_OFFSET_ADDR                              0x364
#define CONSYS_GEN_CONN2AP_REMAP_BT_PERI_BASE_ADDR_OFFSET_ADDR                              0x368
#define CONSYS_GEN_CONN2AP_REMAP_GPS_PERI_BASE_ADDR_OFFSET_ADDR                             0x36C
#define CONSYS_GEN_SCPSYS_SRAM_BASE_ADDR_OFFSET_ADDR                                        0x370
#define CONSYS_GEN_LIGHT_SECURITY_CTRL_OFFSET_ADDR                                          0x374
#define CONSYS_GEN_M3_LIGHT_SECURITY_START_ADDR_2_OFFSET_ADDR                               0x3D8
#define CONSYS_GEN_M3_LIGHT_SECURITY_END_ADDR_2_OFFSET_ADDR                                 0x3DC

/****************************************************************************************************/
/* Base: CONSYS_GEN_CONN_INFRA_SYSRAM_OFFSET_ADDR (0x4005_0000)                                     */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_SYSRAM_OFFSET_ADDR                                            0x40050000

/****************************************************************************************************/
/* Base: CONN_RGU_ON_BASE (0x4000_0000)                                                             */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_OFF_TOP_PWR_CTL_OFFSET_ADDR                                   0x0
#define CONSYS_GEN_BGFYS_ON_TOP_PWR_CTL_OFFSET_ADDR                                         0x20
#define CONSYS_GEN_SYSRAM_HWCTL_PDN_OFFSET_ADDR                                             0x50
#define CONSYS_GEN_SYSRAM_HWCTL_SLP_OFFSET_ADDR                                             0x54
#define CONSYS_GEN_MAWD_MEM_HWCTL_PDN_OFFSET_ADDR                                           0xb0
#define CONSYS_GEN_MAWD_MEM_HWCTL_SLP_OFFSET_ADDR                                           0xb4

/****************************************************************************************************/
/* Base: CONN_WT_SLP_CTL_REG_BASE (0x4000_3000)                                                     */
/****************************************************************************************************/
#define CONSYS_GEN_WB_TOP_CK_ADDR_OFFSET_ADDR                                               0x84
#define CONSYS_GEN_WB_GPS_CK_ADDR_OFFSET_ADDR                                               0x88
#define CONSYS_GEN_WB_GPS_RFBUF_ADR_OFFSET_ADDR                                             0x94
#define CONSYS_GEN_WB_GPS_L5_EN_ADDR_OFFSET_ADDR                                            0x98

/****************************************************************************************************/
/* Base: CONN_AFE_CTL_BASE (0x4004_1000)                                                            */
/****************************************************************************************************/
#define CONSYS_GEN_RG_DIG_EN_02_OFFSET_ADDR                                                 0x4
#define CONSYS_GEN_RG_DIG_TOP_01_OFFSET_ADDR                                                0xC
#define CONSYS_GEN_RG_PLL_STB_TIME_OFFSET_ADDR                                              0xF4
#define CONSYS_GEN_RG_WBG_AFE_01_ADDR                                                       0x10

/****************************************************************************************************/
/* Base: CONN_CFG_ON_BASE (0x4000_1000)                                                             */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL0_OFFSET_ADDR                                      0x200
#define CONSYS_GEN_CONN_INFRA_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_OFFSET_ADDR                    0x200
#define CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL1_OFFSET_ADDR                                      0x210
#define CONSYS_GEN_OSC_CTL_0_OFFSET_ADDR                                                    0x300
#define CONSYS_GEN_CONN_INFRA_CFG_ON_OSC_CTL_0_OFFSET_ADDR                                  0x300
#define CONSYS_GEN_OSC_CTL_1_OFFSET_ADDR                                                    0x304
#define CONSYS_GEN_CONN_INFRA_CFG_ON_OSC_CTL_1_OFFSET_ADDR                                  0x304
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_OFFSET_ADDR                                      0x340
#define CONSYS_GEN_CONN_INFRA_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OFFSET_ADDR                    0x340
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_OFFSET_ADDR                                      0x348
#define CONSYS_GEN_CONN_INFRA_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_OFFSET_ADDR                    0x348
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_GPS_OFFSET_ADDR                                  0x350
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_GPS_OFFSET_ADDR                                  0x354
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_TOP_OFFSET_ADDR                                  0x380
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_TOP_OFFSET_ADDR                                  0x384

/****************************************************************************************************/
/* Base: CONN_BUS_CR_ON_BASE (0x4000_E000)                                                          */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_VON_BUS_TIMEOUT_CTRL_OFFSET_ADDR                              0x24
#define CONSYS_GEN_CONN_INFRA_BUS_CR_ON_CONN_INFRA_VON_BUS_TIMEOUT_CTRL_OFFSET_ADDR         0x24
#define CONSYS_GEN_CONN_INFRA_ON_BUS_TIMEOUT_CTRL_OFFSET_ADDR                               0x38
#define CONSYS_GEN_CONN_INFRA_BUS_CR_ON_CONN_INFRA_ON_BUS_TIMEOUT_CTRL_OFFSET_ADDR          0x38
#define CONSYS_GEN_CONN_VON_BUS_DCM_CTL_1_OFFSET_ADDR                                       0x104
#define CONSYS_GEN_CONN_INFRA_BUS_CR_ON_CONN_VON_BUS_DCM_CTL_1_OFFSET_ADDR                  0x104
#define CONSYS_GEN_CONN_OFF_BUS_DCM_CTL_1_OFFSET_ADDR                                       0x110
#define CONSYS_GEN_CONN_INFRA_BUS_CR_ON_CONN_OFF_BUS_DCM_CTL_1_OFFSET_ADDR                  0x110
#define CONSYS_GEN_CONN_INFRA_CKSYS_CK_CTRL_OFFSET_ADDR                                     0x120

/****************************************************************************************************/
/* Base: CONN_OFF_DEBUG_CTRL_AO_BASE (0x4004_D000)                                                  */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_OFFSET_ADDR     0x0
#define CONSYS_GEN_INFRA_OFF_DCTRL_AO_1804D000_OFFSET                                       0x0

/****************************************************************************************************/
/* Base: CONN_CLKGEN_TOP_BASE (0x4001_2000)                                                         */
/****************************************************************************************************/
#define CONSYS_GEN_CKGEN_BUS_BPLL_DIV_1_OFFSET_ADDR                                         0x0
#define CONSYS_GEN_CKGEN_BUS_BPLL_DIV_2_OFFSET_ADDR                                         0x4
#define CONSYS_GEN_CLKGEN_RFSPI_CK_CTRL_OFFSET_ADDR                                         0x38
#define CONSYS_GEN_CONN_INFRA_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_OFFSET_ADDR                   0x38
#define CONSYS_GEN_CKGEN_BUS_OFFSET_ADDR                                                    0x50

/****************************************************************************************************/
/* Base: CONN_HOST_CSR_TOP_BASE (0x4006_0000)                                                       */
/****************************************************************************************************/
#define CONSYS_GEN_CONN_INFRA_EMI_THROTTLE_CFG_OFFSET_ADDR                                  0x128
#define CONSYS_GEN_CONN_HOST_CSR_TOP_CONN_INFRA_EMI_THROTTLE_CFG_OFFSET_ADDR                0x128
#define CONSYS_GEN_CONN_INFRA_WAKEPU_TOP_OFFSET_ADDR                                        0x1a0
#define CONSYS_GEN_HOST_CONN_INFRA_SLP_CNT_CTL_OFFSET_ADDR                                  0x380
#define CONSYS_GEN_CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_OFFSET_ADDR                0x380

/***********************************************************************/
/* Base: CONSYS_GEN_AFE_EFUSE_BASE_ADDR (0x1016_7000)                  */
/***********************************************************************/
#define CONSYS_GEN_AFE_EFUSE_BASE_ADDR                                                0x10167000
#define CONSYS_GEN_AFE_EFUSE_OFFSET_ADDR                                              0x60

#endif /* MT6993_POS_GEN_H */
