/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _PLATFORM_MT6893_POS_H_
#define _PLATFORM_MT6893_POS_H_


unsigned int consys_emi_set_remapping_reg_mt6893(phys_addr_t, phys_addr_t, phys_addr_t);

int consys_conninfra_on_power_ctrl_mt6893(unsigned int enable);
int consys_conninfra_wakeup_mt6893(void);
int consys_conninfra_sleep_mt6893(void);
void consys_set_if_pinmux_mt6893(unsigned int enable);
int consys_polling_chipid_mt6893(void);

int connsys_d_die_cfg_mt6893(void);
int connsys_spi_master_cfg_mt6893(unsigned int);
int connsys_a_die_cfg_mt6893(void);
int connsys_afe_wbg_cal_mt6893(void);
int connsys_subsys_pll_initial_mt6893(void);
int connsys_low_power_setting_mt6893(unsigned int, unsigned int);

int consys_sema_acquire_timeout_mt6893(unsigned int index, unsigned int usec);
void consys_sema_release_mt6893(unsigned int index);

int consys_spi_read_mt6893(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data);
int consys_spi_write_mt6893(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data);
int consys_spi_write_offset_range_mt6893(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int value,
	unsigned int reg_offset, unsigned int value_offset, unsigned int size);
int consys_spi_update_bits_mt6893(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data, unsigned int mask);

int consys_adie_top_ck_en_on_mt6893(enum consys_adie_ctl_type type);
int consys_adie_top_ck_en_off_mt6893(enum consys_adie_ctl_type type);

int consys_spi_clock_switch_mt6893(enum connsys_spi_speed_type type);
int consys_subsys_status_update_mt6893(bool, int);
bool consys_is_rc_mode_enable_mt6893(void);

void consys_config_setup_mt6893(void);

#endif				/* _PLATFORM_MT6893_POS_H_ */
