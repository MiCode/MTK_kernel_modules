/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


/* AUTO-GENERATED FILE.  DO NOT MODIFY.
 *
 * This file, mt6985_pos_gen.h was automatically generated
 * by the tool from the POS data DE provided.
 * It should not be modified by hand.
 *
 * Reference POS file,
 * - Dxxxc_power_on_sequence_20220227.xlsx
 * - Dxxxc_conn_infra_sub_task_220222.xlsx
 * - conn_infra_cmdbt_instr_autogen_20220216.txt
 */


#ifndef _PLATFORM_MT6985_POS_GEN_H_
#define _PLATFORM_MT6985_POS_GEN_H_

void consys_set_gpio_tcxo_mode_mt6985_gen(unsigned int tcxo_mode, unsigned int enable);
int consys_conninfra_on_power_ctrl_mt6985_gen(unsigned int enable);
int consys_polling_chipid_mt6985_gen(unsigned int *pconsys_ver_id);
unsigned int consys_emi_set_remapping_reg_mt6985_gen(
		phys_addr_t con_emi_base_addr,
		phys_addr_t md_shared_emi_base_addr,
		phys_addr_t gps_emi_base_addr,
		unsigned int emi_base_addr_offset);
void consys_init_conninfra_sysram_mt6985_gen(void);
void connsys_get_d_die_efuse_mt6985_gen(unsigned int *p_d_die_efuse);
int connsys_d_die_cfg_mt6985_gen(void);
void connsys_wt_slp_top_ctrl_adie6686_mt6985_gen(void);
int connsys_afe_wbg_cal_mt6985_gen(
		unsigned int spi_semaphore_index,
		unsigned int spi_semaphore_timeout_usec);
int connsys_subsys_pll_initial_xtal_26000k_mt6985_gen(void);
int connsys_low_power_setting_mt6985_gen(void);
int consys_conninfra_wakeup_mt6985_gen(void);
int consys_conninfra_sleep_mt6985_gen(void);

/***********************************************************************/
/* Base: CONSYS_GEN_GPIO_BASE_ADDR (0x1000_5000)                       */
/***********************************************************************/
#define CONSYS_GEN_GPIO_BASE_ADDR                              0x10005000
#define CONSYS_GEN_GPIO_MODE19_OFFSET_ADDR                     0x430

/***********************************************************************/
/* Base: SPM_BASE_ADDR (0x1C00_1000)                                   */
/***********************************************************************/
#define CONSYS_GEN_POWERON_CONFIG_EN_OFFSET_ADDR               0x0
#define CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR                    0xE04

/***********************************************************************/
/* Base: CONSYS_GEN_IFRBUS_AO_REG_BASE_ADDR (0x1002_C000)              */
/***********************************************************************/
#define CONSYS_GEN_IFRBUS_AO_REG_BASE_ADDR                     0x1002C000
#define CONSYS_GEN_SLEEP_PROT_EN_W1S_0_OFFSET_ADDR             0x4
#define CONSYS_GEN_SLEEP_PROT_EN_W1C_0_OFFSET_ADDR             0x8
#define CONSYS_GEN_SLEEP_PROT_RDY_0_OFFSET_ADDR                0xC
#define CONSYS_GEN_SLEEP_PROT_EN_W1S_14_OFFSET_ADDR            0x1C4
#define CONSYS_GEN_SLEEP_PROT_EN_W1C_14_OFFSET_ADDR            0x1C8
#define CONSYS_GEN_SLEEP_PROT_RDY_14_OFFSET_ADDR               0x1CC

/***********************************************************************/
/* Base: CONN_CFG_BASE (0x1801_1000)                                   */
/***********************************************************************/
#define CONSYS_GEN_IP_VERSION_OFFSET_ADDR                      0x0
#define CONSYS_GEN_EFUSE_OFFSET_ADDR                           0x20
#define CONSYS_GEN_CMDBT_FETCH_START_ADDR0_OFFSET_ADDR         0x50
#define CONSYS_GEN_EMI_CTL_0_OFFSET_ADDR                       0x100

/***********************************************************************/
/* Base: CONSYS_GEN_CONN_HW_VER (0x0205_0300)                          */
/***********************************************************************/
#define CONSYS_GEN_CONN_HW_VER                                 0x2050300

/***********************************************************************/
/* Base: CONN_HOST_CSR_TOP_BASE (0x1806_0000)                          */
/***********************************************************************/
#define CONSYS_GEN_CONN_INFRA_WAKEPU_TOP_OFFSET_ADDR           0x1a0
#define CONSYS_GEN_CONN_HOST_CSR_TOP_18060354_OFFSET           0x354
#define CONSYS_GEN_CONN_HOST_CSR_TOP_1806035C_OFFSET           0x35C
#define CONSYS_GEN_CONN_HOST_CSR_TOP_18060360_OFFSET           0x360
#define CONSYS_GEN_HOST_CONN_INFRA_SLP_CNT_CTL_OFFSET_ADDR     0x380

/***********************************************************************/
/* Base: CONN_BUS_CR_BASE (0x1804_B000)                                */
/***********************************************************************/
#define CONSYS_GEN_INFRA_BUS_1804B024_OFFSET                   0x24
#define CONSYS_GEN_INFRA_BUS_1804B070_OFFSET                   0x70
#define CONSYS_GEN_INFRA_BUS_1804B074_OFFSET                   0x74
#define CONSYS_GEN_INFRA_BUS_1804B364_OFFSET                   0x364
#define CONSYS_GEN_INFRA_BUS_1804B368_OFFSET                   0x368
#define CONSYS_GEN_INFRA_BUS_1804B36C_OFFSET                   0x36C

/***********************************************************************/
/* Base: CONSYS_GEN_CONN_INFRA_SYSRAM_OFFSET_ADDR (0x1805_0000)        */
/***********************************************************************/
#define CONSYS_GEN_CONN_INFRA_SYSRAM_OFFSET_ADDR               0x18050000

/***********************************************************************/
/* Base: CONN_RGU_ON_BASE (0x1800_0000)                                */
/***********************************************************************/
#define CONSYS_GEN_BGFYS_ON_TOP_PWR_CTL_OFFSET_ADDR            0x20
#define CONSYS_GEN_SYSRAM_HWCTL_PDN_OFFSET_ADDR                0x50
#define CONSYS_GEN_SYSRAM_HWCTL_SLP_OFFSET_ADDR                0x54
#define CONSYS_GEN_MAWD_MEM_HWCTL_PDN_OFFSET_ADDR              0xb0
#define CONSYS_GEN_MAWD_MEM_HWCTL_SLP_OFFSET_ADDR              0xb4

/***********************************************************************/
/* Base: CONN_WT_SLP_CTL_REG_BASE (0x1800_3000)                        */
/***********************************************************************/
#define CONSYS_GEN_WB_TOP_CK_ADDR_OFFSET_ADDR                  0x84
#define CONSYS_GEN_WB_GPS_CK_ADDR_OFFSET_ADDR                  0x88
#define CONSYS_GEN_WB_GPS_RFBUF_ADR_OFFSET_ADDR                0x94
#define CONSYS_GEN_WB_GPS_L5_EN_ADDR_OFFSET_ADDR               0x98

/***********************************************************************/
/* Base: CONN_AFE_CTL_BASE (0x1804_1000)                               */
/***********************************************************************/
#define CONSYS_GEN_RG_DIG_EN_02_OFFSET_ADDR                    0x4
#define CONSYS_GEN_RG_DIG_TOP_01_OFFSET_ADDR                   0xC
#define CONSYS_GEN_RG_PLL_STB_TIME_OFFSET_ADDR                 0xF4

/***********************************************************************/
/* Base: CONN_CFG_ON_BASE (0x1800_1000)                                */
/***********************************************************************/
#define CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL0_OFFSET_ADDR         0x200
#define CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL1_OFFSET_ADDR         0x210
#define CONSYS_GEN_OSC_CTL_0_OFFSET_ADDR                       0x300
#define CONSYS_GEN_OSC_CTL_1_OFFSET_ADDR                       0x304
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_OFFSET_ADDR         0x340
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_OFFSET_ADDR         0x348
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_GPS_OFFSET_ADDR     0x350
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_GPS_OFFSET_ADDR     0x354
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_TOP_OFFSET_ADDR     0x380
#define CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_TOP_OFFSET_ADDR     0x384

/***********************************************************************/
/* Base: CONN_BUS_CR_ON_BASE (0x1800_E000)                             */
/***********************************************************************/
#define CONSYS_GEN_INFRA_BUS_ON_1800E024_OFFSET                0x24
#define CONSYS_GEN_INFRA_BUS_ON_1800E038_OFFSET                0x38
#define CONSYS_GEN_CONN_VON_BUS_DCM_CTL_1_OFFSET_ADDR          0x104
#define CONSYS_GEN_CONN_OFF_BUS_DCM_CTL_1_OFFSET_ADDR          0x110

/***********************************************************************/
/* Base: CONN_OFF_DEBUG_CTRL_AO_BASE (0x1804_D000)                     */
/***********************************************************************/
#define CONSYS_GEN_INFRA_OFF_DCTRL_AO_1804D000_OFFSET          0x0

/***********************************************************************/
/* Base: CONN_CLKGEN_TOP_BASE (0x1801_2000)                            */
/***********************************************************************/
#define CONSYS_GEN_CKGEN_BUS_BPLL_DIV_1_OFFSET_ADDR            0x0
#define CONSYS_GEN_CKGEN_BUS_BPLL_DIV_2_OFFSET_ADDR            0x4
#define CONSYS_GEN_CLKGEN_RFSPI_CK_CTRL_OFFSET_ADDR            0x38
#define CONSYS_GEN_CKGEN_BUS_OFFSET_ADDR                       0x50

#endif /* _PLATFORM_MT6985_POS_GEN_H_ */
