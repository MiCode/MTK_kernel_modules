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
#include <linux/types.h>
#include "conninfra.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#define CONN_SEMA_GET_SUCCESS	0
#define CONN_SEMA_GET_FAIL	1

#define CONN_SEMA_TIMEOUT	(1*1000) /* 1ms */

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
typedef int(*CONSYS_PLT_CLK_DETACH) (void);
typedef int(*CONSYS_PLT_READ_REG_FROM_DTS) (struct platform_device *pdev);

typedef int(*CONSYS_PLT_CLOCK_BUFFER_CTRL) (unsigned int enable);
typedef int(*CONSYS_PLT_CONNINFRA_ON_POWER_CTRL) (unsigned int enable);
typedef void(*CONSYS_PLT_SET_IF_PINMUX) (unsigned int enable);

typedef int(*CONSYS_PLT_POLLING_CONSYS_CHIPID) (void);
typedef int(*CONSYS_PLT_D_DIE_CFG) (void);
typedef int(*CONSYS_PLT_SPI_MASTER_CFG) (unsigned int next_status);
typedef int(*CONSYS_PLT_A_DIE_CFG) (void);
typedef void(*CONSYS_PLT_AFE_SW_PATCH) (void);
typedef int(*CONSYS_PLT_AFE_WBG_CAL) (void);
typedef int(*CONSYS_PLT_SUBSYS_PLL_INITIAL) (void);
typedef int(*CONSYS_PLT_LOW_POWER_SETTING) (unsigned int curr_status, unsigned int next_status);
typedef int(*CONSYS_PLT_CONNINFRA_WAKEUP) (void);
typedef int(*CONSYS_PLT_CONNINFRA_SLEEP) (void);

typedef void(*CONSYS_PLT_AFE_REG_SETTING) (void);
typedef unsigned int(*CONSYS_PLT_SOC_CHIPID_GET) (void);

typedef void(*CONSYS_PLT_FORCE_TRIGGER_ASSERT_DEBUG_PIN) (void);
typedef int(*CONSYS_PLT_CO_CLOCK_TYPE) (void);

typedef int(*CONSYS_PLT_CHECK_REG_READABLE) (void);
typedef void(*CONSYS_PLT_CLOCK_FAIL_DUMP) (void);
typedef int(*CONSYS_PLT_IS_CONNSYS_REG) (unsigned int addr);


typedef int(*CONSYS_PLT_SPI_READ)(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data);
typedef int(*CONSYS_PLT_SPI_WRITE)(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data);
typedef int(*CONSYS_PLT_SPI_UPDATE_BITS)(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data, unsigned int mask);

typedef int(*CONSYS_PLT_ADIE_TOP_CK_EN_ON)(enum consys_adie_ctl_type type);
typedef int(*CONSYS_PLT_ADIE_TOP_CK_EN_OFF)(enum consys_adie_ctl_type type);

typedef int(*CONSYS_PLT_SPI_CLOCK_SWITCH)(enum connsys_spi_speed_type type);

typedef bool(*CONSYS_PLT_IS_RC_MODE_ENABLE)(void);
typedef int(*CONSYS_PLT_SUBSYS_STATUS_UPDATE)(bool on, int radio);

typedef unsigned int(*CONSYS_PLT_GET_HW_VER)(void);
typedef int(*CONSYS_PLT_THERMAL_QUERY)(void);

typedef int(*CONSYS_PLT_ENABLE_POWER_DUMP)(void);
typedef int(*CONSYS_PLT_RESET_POWER_STATE)(void);
typedef int(*CONSYS_PLT_POWER_STATE)(char *buf, unsigned int size);

typedef void(*CONSYS_PLT_CONFIG_SETUP)(void);

typedef int(*CONSYS_PLT_BUS_CLOCK_CTRL)(enum consys_drv_type drv_type, unsigned int, int);
typedef u64(*CONSYS_PLT_SOC_TIMESTAMP_GET)(void);

typedef unsigned int (*CONSYS_PLT_ADIE_DETECTION)(void);

typedef void (*CONSYS_PLT_SET_MCU_CONTROL)(int type, bool onoff);

typedef int (*CONSYS_PLT_PRE_CAL_BACKUP)(unsigned int offset, unsigned int size);
typedef int (*CONSYS_PLT_PRE_CAL_CLEAN_DATA)(void);

struct consys_hw_ops_struct {
	/* load from dts */
	CONSYS_PLT_CLK_GET_FROM_DTS consys_plt_clk_get_from_dts;
	CONSYS_PLT_CLK_DETACH consys_plt_clk_detach;

	/* clock */
	CONSYS_PLT_CLOCK_BUFFER_CTRL consys_plt_clock_buffer_ctrl;
	CONSYS_PLT_CO_CLOCK_TYPE consys_plt_co_clock_type;

	/* POS */
	CONSYS_PLT_CONNINFRA_ON_POWER_CTRL consys_plt_conninfra_on_power_ctrl;
	CONSYS_PLT_SET_IF_PINMUX consys_plt_set_if_pinmux;

	CONSYS_PLT_POLLING_CONSYS_CHIPID consys_plt_polling_consys_chipid;
	CONSYS_PLT_D_DIE_CFG consys_plt_d_die_cfg;
	CONSYS_PLT_SPI_MASTER_CFG consys_plt_spi_master_cfg;
	CONSYS_PLT_A_DIE_CFG consys_plt_a_die_cfg;
	CONSYS_PLT_AFE_SW_PATCH consys_plt_afe_sw_patch;
	CONSYS_PLT_AFE_WBG_CAL consys_plt_afe_wbg_cal;
	CONSYS_PLT_SUBSYS_PLL_INITIAL consys_plt_subsys_pll_initial;
	CONSYS_PLT_LOW_POWER_SETTING consys_plt_low_power_setting;

	CONSYS_PLT_AFE_REG_SETTING consys_plt_afe_reg_setting;

	CONSYS_PLT_SOC_CHIPID_GET consys_plt_soc_chipid_get;
	CONSYS_PLT_CONNINFRA_WAKEUP consys_plt_conninfra_wakeup;
	CONSYS_PLT_CONNINFRA_SLEEP consys_plt_conninfra_sleep;
	CONSYS_PLT_IS_RC_MODE_ENABLE consys_plt_is_rc_mode_enable;

	CONSYS_PLT_CHECK_REG_READABLE consys_plt_check_reg_readable;
	/* debug */
	CONSYS_PLT_CLOCK_FAIL_DUMP consys_plt_clock_fail_dump;
	CONSYS_PLT_GET_HW_VER consys_plt_get_hw_ver;

	/* For SPI operation */
	CONSYS_PLT_SPI_READ consys_plt_spi_read;
	CONSYS_PLT_SPI_WRITE consys_plt_spi_write;
	CONSYS_PLT_SPI_UPDATE_BITS consys_plt_spi_update_bits;

	/* For a-die top_ck_en control */
	CONSYS_PLT_ADIE_TOP_CK_EN_ON consys_plt_adie_top_ck_en_on;
	CONSYS_PLT_ADIE_TOP_CK_EN_OFF consys_plt_adie_top_ck_en_off;

	/* For SPI clock switch */
	CONSYS_PLT_SPI_CLOCK_SWITCH consys_plt_spi_clock_switch;
	CONSYS_PLT_SUBSYS_STATUS_UPDATE consys_plt_subsys_status_update;

	/* thermal */
	CONSYS_PLT_THERMAL_QUERY consys_plt_thermal_query;

	/* power state */
	CONSYS_PLT_ENABLE_POWER_DUMP consys_plt_enable_power_dump;
	CONSYS_PLT_RESET_POWER_STATE consys_plt_reset_power_state;
	CONSYS_PLT_POWER_STATE consys_plt_power_state;

	CONSYS_PLT_CONFIG_SETUP consys_plt_config_setup;

	CONSYS_PLT_BUS_CLOCK_CTRL consys_plt_bus_clock_ctrl;

	CONSYS_PLT_SOC_TIMESTAMP_GET consys_plt_soc_timestamp_get;

	CONSYS_PLT_ADIE_DETECTION consys_plt_adie_detection;

	CONSYS_PLT_SET_MCU_CONTROL consys_plt_set_mcu_control;

	CONSYS_PLT_PRE_CAL_BACKUP consys_plt_pre_cal_backup;
	CONSYS_PLT_PRE_CAL_CLEAN_DATA consys_plt_pre_cal_clean_data;
};

struct conninfra_dev_cb {
	int (*conninfra_suspend_cb) (void);
	int (*conninfra_resume_cb) (void);
	int (*conninfra_pmic_event_notifier) (unsigned int, unsigned int);
};

struct consys_hw_env {
	unsigned int adie_hw_version;
	int is_rc_mode;
	bool tcxo_support;
};

struct conninfra_plat_data {
	const unsigned int chip_id;
	const unsigned int consys_hw_version;
	const void* hw_ops;
	const void* reg_ops;
	const void* platform_emi_ops;
	const void* platform_pmic_ops;
	const void* platform_coredump_ops;
	const void* connsyslog_config;
};

extern struct consys_hw_env conn_hw_env;
extern struct consys_base_addr conn_reg;
extern struct pinctrl *g_conninfra_pinctrl_ptr;
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
int consys_hw_init(struct conninfra_dev_cb *dev_cb);
int consys_hw_deinit(void);

int consys_hw_pwr_off(unsigned int curr_status, unsigned int off_radio);
int consys_hw_pwr_on(unsigned int curr_status, unsigned int on_radio);

int consys_hw_wifi_power_ctl(unsigned int enable);
int consys_hw_bt_power_ctl(unsigned int enable);
int consys_hw_gps_power_ctl(unsigned int enable);
int consys_hw_fm_power_ctl(unsigned int enable);
int consys_hw_pmic_event_cb(unsigned int id, unsigned int event);

unsigned int consys_hw_chipid_get(void);

int consys_hw_get_clock_schematic(void);
unsigned int consys_hw_get_hw_ver(void);

/*
 * return
 * 1 : can read
 * 0 : can't read
 * -1: not consys register
 */
int consys_hw_reg_readable(void);
int consys_hw_reg_readable_for_coredump(void);
int consys_hw_is_connsys_reg(phys_addr_t addr);
/*
 * 0 means NO hang
 * > 0 means hang!!
 */
int consys_hw_is_bus_hang(void);
int consys_hw_dump_bus_status(void);

int consys_hw_spi_read(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data);
int consys_hw_spi_write(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data);
int consys_hw_spi_update_bits(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data, unsigned int mask);

int consys_hw_adie_top_ck_en_on(enum consys_adie_ctl_type type);
int consys_hw_adie_top_ck_en_off(enum consys_adie_ctl_type type);

/* NOTE: debug only*/
int consys_hw_force_conninfra_wakeup(void);
int consys_hw_force_conninfra_sleep(void);

int consys_hw_spi_clock_switch(enum connsys_spi_speed_type type);

struct platform_device *get_consys_device(void);
struct consys_base_addr *get_conn_reg_base_addr(void);

int consys_hw_therm_query(int *temp_ptr);
void consys_hw_clock_fail_dump(void);

/* Low debug */
int consys_hw_enable_power_dump(void);
int consys_hw_reset_power_state(void);
int consys_hw_dump_power_state(char *buf, unsigned int size);


void consys_hw_config_setup(void);
int consys_hw_bus_clock_ctrl(enum consys_drv_type drv_type, unsigned int bus_clock, int status);
/* raise: raise voltage or not
 * onoff: raise voltage because of power on/off or not
 * 	true: yes, raise voltage because power on/off
 * 	false: no, raise voltage by scenario
 */
int consys_hw_raise_voltage(enum consys_drv_type drv_type, bool raise, bool onoff);
/* Get soc timestamp (non-sleep timer)
 * unit: ms
 */
u64 consys_hw_soc_timestamp_get(void);

// Auto a-die detection
unsigned int consys_hw_detect_adie_chipid(void);
unsigned int consys_hw_get_ic_info(enum connsys_ic_info_type type);

int consys_hw_set_platform_config(int value);
int consys_hw_get_platform_config(void);

void consys_hw_set_mcu_control(int type, bool onoff);

/* Pre-cal */
int consys_hw_pre_cal_backup(unsigned int offset, unsigned int size);
int consys_hw_pre_cal_clean_data(void);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif				/* _PLATFORM_CONSYS_HW_H_ */
