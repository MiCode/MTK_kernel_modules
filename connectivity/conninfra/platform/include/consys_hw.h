/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

#ifndef _PLATFORM_CONSYS_HW_H_
#define _PLATFORM_CONSYS_HW_H_

#include <linux/platform_device.h>

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
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

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

typedef int(*CONSYS_PLT_CLK_GET_FROM_DTS) (struct platform_device *pdev);
typedef int(*CONSYS_PLT_READ_REG_FROM_DTS) (struct platform_device *pdev);

typedef int(*CONSYS_PLT_CLOCK_BUFFER_CTRL) (unsigned int enable);
typedef int(*CONSYS_PLT_CONNINFRA_ON_POWER_CTRL) (unsigned int enable);
typedef void(*CONSYS_PLT_SET_IF_PINMUX) (unsigned int enable);

typedef int(*CONSYS_PLT_POLLING_CONSYS_CHIPID) (void);
typedef int(*CONSYS_PLT_D_DIE_CFG) (void);
typedef int(*CONSYS_PLT_SPI_MASTER_CFG) (void);
typedef int(*CONSYS_PLT_A_DIE_CFG) (void);
typedef int(*CONSYS_PLT_AFE_WBG_CAL) (void);
typedef int(*CONSYS_PLT_LOW_POWER_SETTING) (void);

typedef void(*CONSYS_PLT_AFE_REG_SETTING) (void);
typedef unsigned int(*CONSYS_PLT_SOC_CHIPID_GET) (void);

typedef void(*CONSYS_PLT_FORCE_TRIGGER_ASSERT_DEBUG_PIN) (void);
typedef int(*CONSYS_PLT_CO_CLOCK_TYPE) (void);

typedef int(*CONSYS_PLT_CHECK_REG_READABLE) (void);
typedef void(*CONSYS_PLT_CLOCK_FAIL_DUMP) (void);
typedef unsigned int(*CONSYS_PLT_READ_CPUPCR) (void);
typedef int(*CONSYS_PLT_IS_CONNSYS_REG) (unsigned int addr);

typedef void(*CONSYS_PLT_RESUME_DUMP_INFO) (void);
typedef void(*CONSYS_PLT_SET_PDMA_AXI_RREADY_FORCE_HIGH) (unsigned int enable);

struct consys_hw_ops_struct {
	/* load from dts */
	CONSYS_PLT_CLK_GET_FROM_DTS consys_plt_clk_get_from_dts;
	/*CONSYS_IC_PMIC_GET_FROM_DTS consys_ic_pmic_get_from_dts;*/
	//CONSYS_PLT_READ_REG_FROM_DTS consys_plt_read_reg_from_dts;
	/* irq, do we need? */
	/*CONSYS_IC_READ_IRQ_INFO_FROM_DTS consys_ic_read_irq_info_from_dts;*/

	/* clock */
	CONSYS_PLT_CLOCK_BUFFER_CTRL consys_plt_clock_buffer_ctrl;
	CONSYS_PLT_CO_CLOCK_TYPE consys_plt_co_clock_type;

	/* POS */
	/*CONSYS_IC_HW_SPM_CLK_GATING_ENABLE consys_ic_hw_spm_clk_gating_enable;*/
	CONSYS_PLT_CONNINFRA_ON_POWER_CTRL consys_plt_conninfra_on_power_ctrl;
	CONSYS_PLT_SET_IF_PINMUX consys_plt_set_if_pinmux;

	/*CONSYS_IC_AHB_CLOCK_CTRL consys_ic_ahb_clock_ctrl;*/
	CONSYS_PLT_POLLING_CONSYS_CHIPID consys_plt_polling_consys_chipid;
	CONSYS_PLT_D_DIE_CFG consys_plt_d_die_cfg;
	CONSYS_PLT_SPI_MASTER_CFG consys_plt_spi_master_cfg;
	CONSYS_PLT_A_DIE_CFG consys_plt_a_die_cfg;
	CONSYS_PLT_AFE_WBG_CAL consys_plt_afe_wbg_cal;
	CONSYS_PLT_LOW_POWER_SETTING consys_plt_low_power_setting;

	/*UPDATE_CONSYS_ROM_DESEL_VALUE update_consys_rom_desel_value;*/
	/*CONSYS_HANG_DEBUG consys_hang_debug;*/
	/*CONSYS_IC_ARC_REG_SETTING consys_ic_acr_reg_setting;*/
	CONSYS_PLT_AFE_REG_SETTING consys_plt_afe_reg_setting;

	CONSYS_PLT_SOC_CHIPID_GET consys_plt_soc_chipid_get;

	/*IC_BT_WIFI_SHARE_V33_SPIN_LOCK_INIT ic_bt_wifi_share_v33_spin_lock_init;*/
	/*CONSYS_PLT_FORCE_TRIGGER_ASSERT_DEBUG_PIN consys_plt_force_trigger_assert_debug_pin;*/

	/*CONSYS_IC_SET_DL_ROM_PATCH_FLAG consys_ic_set_dl_rom_patch_flag;*/
	/*CONSYS_IC_DEDICATED_LOG_PATH_INIT consys_ic_dedicated_log_path_init;*/
	/*CONSYS_IC_DEDICATED_LOG_PATH_DEINIT consys_ic_dedicated_log_path_deinit;*/

	/* debug */
	CONSYS_PLT_CHECK_REG_READABLE consys_plt_check_reg_readable;
	CONSYS_PLT_CLOCK_FAIL_DUMP consys_plt_clock_fail_dump;
	CONSYS_PLT_READ_CPUPCR consys_plt_cread_cpupcr;

	/* debug, used by STEP */
	CONSYS_PLT_IS_CONNSYS_REG consys_plt_is_connsys_reg;

	CONSYS_PLT_RESUME_DUMP_INFO consys_plt_resume_dump_info;

	/* for reset */
	CONSYS_PLT_SET_PDMA_AXI_RREADY_FORCE_HIGH consys_plt_set_pdma_axi_rready_force_high;
};


extern struct consys_base_addr conn_reg;
/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
int consys_hw_init(void);
int consys_hw_deinit(void);

int consys_hw_pwr_off(void);
int consys_hw_pwr_on(void);

int consys_hw_wifi_power_ctl(unsigned int enable);
int consys_hw_bt_power_ctl(unsigned int enable);
int consys_hw_gps_power_ctl(unsigned int enable);
int consys_hw_fm_power_ctl(unsigned int enable);

/*******************************************************************************
* tempoary for STEP
********************************************************************************
*/
/*
 * return
 * 1 : can read
 * 0 : can't read
 * -1: not consys register
 */
int consys_hw_reg_readable(void);
int consys_hw_is_connsys_reg(phys_addr_t addr);


struct platform_device *get_consys_device(void);
struct consys_base_addr *get_conn_reg_base_addr(void);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif				/* _PLATFORM_CONSYS_HW_H_ */
