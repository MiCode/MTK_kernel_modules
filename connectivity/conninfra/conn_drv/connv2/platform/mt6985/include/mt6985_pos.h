/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _PLATFORM_MT6985_POS_H_
#define _PLATFORM_MT6985_POS_H_

#include <linux/types.h>

unsigned int consys_emi_set_remapping_reg_mt6985(phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr, phys_addr_t gps_emi_base_addr);
unsigned int consys_emi_set_remapping_reg_mt6985_atf(phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr, phys_addr_t gps_emi_base_addr);

bool consys_is_rc_mode_enable_mt6985(void);
int consys_conninfra_on_power_ctrl_mt6985(unsigned int enable);
int consys_polling_chipid_mt6985(void);
int connsys_d_die_cfg_mt6985(void);
int connsys_spi_master_cfg_mt6985(unsigned int curr_status, unsigned int next_status);
int consys_conninfra_wakeup_mt6985(void);
int consys_conninfra_sleep_mt6985(void);
int connsys_subsys_pll_initial_mt6985(void);
int connsys_low_power_setting_mt6985(unsigned int, unsigned int);
int consys_subsys_status_update_mt6985(bool on, int radio);

int consys_spi_read_mt6985(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data);
int consys_spi_write_mt6985(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data);
int consys_spi_update_bits_mt6985(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data, unsigned int mask);


#endif /* _PLATFORM_MT6985_POS_H_ */
