/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


/* AUTO-GENERATED FILE.  DO NOT MODIFY.
 *
 * This file, mt6781_pos_gen.h was automatically generated
 * by the tool from the POS data DE provided.
 * It should not be modified by hand.
 */


#ifndef _PLATFORM_MT6781_POS_GEN_H_
#define _PLATFORM_MT6781_POS_GEN_H_

void consys_set_if_pinmux_mt6781_gen(int enable);
void consys_hw_reset_bit_set_mt6781_gen(int enable);
void consys_hw_spm_clk_gating_enable_mt6781_gen(void);
int consys_hw_power_ctrl_mt6781_gen(int enable);
int polling_consys_chipid_mt6781_gen(unsigned int *pconsys_ver_id, unsigned int *pconsys_fw_ver);
void consys_set_access_emi_hw_mode_mt6781_gen(void);
void consys_afe_reg_setting_mt6781_gen(void);
void consys_emi_entry_address_mt6781_gen(void);
void consys_set_xo_osc_ctrl_mt6781_gen(void);
void consys_identify_adie_mt6781_gen(void);
void consys_wifi_ctrl_setting_mt6781_gen(void);
void consys_bus_timeout_config_mt6781_gen(void);
void consys_set_mcu_mem_pdn_delay_mt6781_gen(void);
int consys_polling_goto_idle_mt6781_gen(unsigned int *pconsys_ver_id);
void consys_wifi_ctrl_switch_conn_mode_mt6781_gen(void);

/*************************************************************************/
/* Base: CONSYS_GEN_GPIO_BASE_ADDR (0x1000_5000)                         */
/*************************************************************************/
#define CONSYS_GEN_GPIO_BASE_ADDR                                0x10005000
#define CONSYS_GEN_GPIO_DIR6_OFFSET_ADDR                         0x60
#define CONSYS_GEN_GPIO_DOUT6_OFFSET_ADDR                        0x160
#define CONSYS_GEN_GPIO_MODE9_OFFSET_ADDR                        0x390
#define CONSYS_GEN_GPIO_MODE23_OFFSET_ADDR                       0x470
#define CONSYS_GEN_GPIO_MODE24_OFFSET_ADDR                       0x480

/*************************************************************************/
/* Base: CONSYS_GEN_IOCFG_LT_BASE_ADDR (0x1000_2000)                     */
/*************************************************************************/
#define CONSYS_GEN_IOCFG_LT_BASE_ADDR                            0x10002000

/*************************************************************************/
/* Base: conn_reg.ap_rgu_base (0x1000_7000)                              */
/*************************************************************************/
#define CONSYS_GEN_WDT_SWSYSRST_OFFSET_ADDR                      0x18

/*************************************************************************/
/* Base: conn_reg.spm_base (0x1000_6000)                                 */
/*************************************************************************/
#define CONSYS_GEN_POWERON_CONFIG_EN_OFFSET_ADDR                 0x0
#define CONSYS_GEN_PWR_STATUS_OFFSET_ADDR                        0x16C
#define CONSYS_GEN_PWR_STATUS_2ND_OFFSET_ADDR                    0x170
#define CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR                      0x304

/*************************************************************************/
/* Base: conn_reg.topckgen_base (0x1000_1000)                            */
/*************************************************************************/
#define CONSYS_GEN_INFRA_TOPAXI_PROTECTEN_STA1_OFFSET_ADDR       0x228
#define CONSYS_GEN_INFRA_TOPAXI_PROTECTEN_STA1_1_OFFSET_ADDR     0x258
#define CONSYS_GEN_INFRA_TOPAXI_PROTECTEN_SET_OFFSET_ADDR        0x2A0
#define CONSYS_GEN_INFRA_TOPAXI_PROTECTEN_CLR_OFFSET_ADDR        0x2A4
#define CONSYS_GEN_INFRA_TOPAXI_PROTECTEN_1_SET_OFFSET_ADDR      0x2A8
#define CONSYS_GEN_INFRA_TOPAXI_PROTECTEN_1_CLR_OFFSET_ADDR      0x2AC

/*************************************************************************/
/* Base: conn_reg.mcu_top_misc_off_base (0x180B_1000)                    */
/*************************************************************************/
#define CONSYS_GEN_CONN_CONNSYS_VERSION_OFFSET_ADDR              0x10
#define CONSYS_GEN_CONN_CONNSYS_CONFIG_OFFSET_ADDR               0x1C

/*************************************************************************/
/* Base: CONN_HW_VER (0x100B_0000)                                       */
/*************************************************************************/
#define CONN_HW_VER                                              0x100B0000

/*************************************************************************/
/* Base: conn_reg.mcu_base (0x1800_2000)                                 */
/*************************************************************************/
#define CONSYS_GEN_HW_VER_OFFSET_ADDR                            0x0
#define CONSYS_GEN_FW_VER_OFFSET_ADDR                            0x4
#define CONSYS_GEN_BUSHANGCR_OFFSET_ADDR                         0x440
#define CONSYS_GEN_BT_EMI_ENTRY_ADDR_OFFSET_ADDR                 0x504
#define CONSYS_GEN_WF_EMI_ENTRY_ADDR_OFFSET_ADDR                 0x508
#define CONSYS_GEN_COM_REG0_OFFSET_ADDR                          0x600

/*************************************************************************/
/* Base: conn_reg.mcu_top_misc_on_base (0x180C_1000)                     */
/*************************************************************************/
#define CONSYS_GEN_CONN_ON_RSV_OFFSET_ADDR                       0x130
#define CONSYS_GEN_CONN_ON_MCU_EMI_SLPPROT_EN_OFFSET_ADDR        0x168
#define CONSYS_GEN_CONN_ON_OSC_CTL_OFFSET_ADDR                   0x200
#define CONSYS_GEN_CONN_ON_OSC_CTL_2_OFFSET_ADDR                 0x204
#define CONSYS_GEN_CONN_ON_RC_OSC_CTL_0_OFFSET_ADDR              0x240
#define CONSYS_GEN_CONN_ON_RC_OSC_CTL_GPS_0_OFFSET_ADDR          0x244
#define CONSYS_GEN_CONN_ON_RC_OSC_CTL_GPS_1_OFFSET_ADDR          0x248
#define CONSYS_GEN_CONN_ON_RC_OSC_CTL_BT_0_OFFSET_ADDR           0x24c
#define CONSYS_GEN_CONN_ON_RC_OSC_CTL_BT_1_OFFSET_ADDR           0x250
#define CONSYS_GEN_CONN_ON_RC_OSC_CTL_WF_0_OFFSET_ADDR           0x254
#define CONSYS_GEN_CONN_ON_RC_OSC_CTL_WF_1_OFFSET_ADDR           0x258
#define CONSYS_GEN_CONN_ON_RC_OSC_CTL_TOP_0_OFFSET_ADDR          0x25c
#define CONSYS_GEN_CONN_ON_RC_OSC_CTL_TOP_1_OFFSET_ADDR          0x260

/*************************************************************************/
/* Base: CONSYS_GEN_CONN_AFE_CTL_BASE_ADDR (0x180B_3000)                 */
/*************************************************************************/
#define CONSYS_GEN_CONN_AFE_CTL_BASE_ADDR                        0x180B3000
#define CONSYS_GEN_RG_WBG_PLL_04_OFFSET_ADDR                     0x30
#define CONSYS_GEN_RG_WBG_GL5_02_OFFSET_ADDR                     0x104

/*************************************************************************/
/* Base: CONSYS_GEN_CONN_RGU_ON_BASE_ADDR (0x180C_0000)                  */
/*************************************************************************/
#define CONSYS_GEN_CONN_RGU_ON_BASE_ADDR                         0x180C0000
#define CONSYS_GEN_MCUSYS_DLY_CHAIN_CTL_OFFSET_ADDR              0x284

#endif /* _PLATFORM_MT6781_POS_GEN_H_ */
