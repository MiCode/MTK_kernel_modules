/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _PLATFORM_MT6989_POS_H_
#define _PLATFORM_MT6989_POS_H_

#include <linux/types.h>

unsigned int consys_emi_set_remapping_reg_mt6989(phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr, phys_addr_t gps_emi_base_addr);
unsigned int consys_emi_set_remapping_reg_mt6989_atf(phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr, phys_addr_t gps_emi_base_addr);

bool consys_is_rc_mode_enable_mt6989(void);
int consys_conninfra_on_power_ctrl_mt6989(unsigned int enable);
int consys_polling_chipid_mt6989(void);
int connsys_d_die_cfg_mt6989(void);
int connsys_spi_master_cfg_mt6989(unsigned int curr_status, unsigned int next_status);
int consys_conninfra_wakeup_mt6989(void);
int consys_conninfra_sleep_mt6989(void);
void connsys_afe_sw_patch_mt6989(void);
int connsys_subsys_pll_initial_mt6989(void);
int connsys_low_power_setting_mt6989(unsigned int curr_status, unsigned int next_status);
int consys_subsys_status_update_mt6989(bool on, int radio);

int consys_spi_read_mt6989(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data);
int consys_spi_write_mt6989(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data);
int consys_spi_update_bits_mt6989(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data, unsigned int mask);


#endif /* _PLATFORM_MT6989_POS_H_ */
