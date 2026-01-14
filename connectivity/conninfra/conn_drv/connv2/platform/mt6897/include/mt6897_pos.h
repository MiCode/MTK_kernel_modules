/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef MT6897_POS_H
#define MT6897_POS_H

#include "../../include/plat_library.h"

unsigned int consys_emi_set_remapping_reg_mt6897(phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr, phys_addr_t gps_emi_base_addr);
unsigned int consys_emi_set_remapping_reg_mt6897_atf(phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr, phys_addr_t gps_emi_base_addr);

int consys_conninfra_wakeup_mt6897(void);
int consys_conninfra_sleep_mt6897(void);
int consys_polling_chipid_mt6897(void);

int connsys_d_die_cfg_mt6897(void);
int connsys_spi_master_cfg_mt6897(unsigned int curr_status, unsigned int next_status);
int connsys_a_die_cfg_mt6897(unsigned int curr_status, unsigned int next_status);
void connsys_afe_sw_patch_mt6897(void);
int connsys_afe_wbg_cal_mt6897(void);
int connsys_subsys_pll_initial_mt6897(void);
int connsys_low_power_setting_mt6897(unsigned int curr_status, unsigned int next_status);

int consys_sema_acquire_timeout_mt6897(unsigned int index, unsigned int usec);
void consys_sema_release_mt6897(unsigned int index);

int consys_spi_read_mt6897(enum sys_spi_subsystem subsystem, unsigned int addr,
				   unsigned int *data);
int consys_spi_write_mt6897(enum sys_spi_subsystem subsystem, unsigned int addr,
			    unsigned int data);
int consys_spi_update_bits_mt6897(enum sys_spi_subsystem subsystem, unsigned int addr,
				  unsigned int data, unsigned int mask);

int consys_spi_clock_switch_mt6897(enum connsys_spi_speed_type type);
int consys_subsys_status_update_mt6897(bool on, int radio);
bool consys_is_rc_mode_enable_mt6897(void);
int connsys_adie_top_ck_en_ctl_mt6897(bool onoff);

int consys_get_sleep_mode_mt6897(void);

int consys_spi_read_nolock_mt6897(enum sys_spi_subsystem subsystem, unsigned int addr,
				  unsigned int *data);
int consys_spi_write_nolock_mt6897(enum sys_spi_subsystem subsystem, unsigned int addr,
				   unsigned int data);
#ifndef CONFIG_FPGA_EARLY_PORTING
void consys_spi_write_offset_range_nolock_mt6897(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int value,
	unsigned int reg_offset, unsigned int value_offset, unsigned int size);
#endif

#endif /* MT6897_POS_H */

