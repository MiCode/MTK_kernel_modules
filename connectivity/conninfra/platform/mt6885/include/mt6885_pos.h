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

#ifndef _PLATFORM_MT6885_POS_H_
#define _PLATFORM_MT6885_POS_H_


unsigned int consys_emi_set_remapping_reg_mt6885(phys_addr_t, phys_addr_t, phys_addr_t);

int consys_conninfra_on_power_ctrl_mt6885(unsigned int enable);
int consys_conninfra_wakeup_mt6885(void);
int consys_conninfra_sleep_mt6885(void);
void consys_set_if_pinmux_mt6885(unsigned int enable);
int consys_polling_chipid_mt6885(void);

int connsys_d_die_cfg_mt6885(void);
int connsys_spi_master_cfg_mt6885(unsigned int);
int connsys_a_die_cfg_mt6885(void);
int connsys_afe_wbg_cal_mt6885(void);
int connsys_subsys_pll_initial_mt6885(void);
int connsys_low_power_setting_mt6885(unsigned int, unsigned int);

int consys_sema_acquire_timeout_mt6885(unsigned int index, unsigned int usec);
void consys_sema_release_mt6885(unsigned int index);

int consys_spi_read_mt6885(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data);
int consys_spi_write_mt6885(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data);
int consys_spi_write_offset_range_mt6885(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int value,
	unsigned int reg_offset, unsigned int value_offset, unsigned int size);
int consys_spi_update_bits_mt6885(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data, unsigned int mask);

int consys_adie_top_ck_en_on_mt6885(enum consys_adie_ctl_type type);
int consys_adie_top_ck_en_off_mt6885(enum consys_adie_ctl_type type);

int consys_spi_clock_switch_mt6885(enum connsys_spi_speed_type type);
int consys_subsys_status_update_mt6885(bool, int);
bool consys_is_rc_mode_enable_mt6885(void);

void consys_config_setup_mt6885(void);

#endif				/* _PLATFORM_MT6885_POS_H_ */
