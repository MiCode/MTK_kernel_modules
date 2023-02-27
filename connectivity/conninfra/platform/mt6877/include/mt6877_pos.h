/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _PLATFORM_MT6877_POS_H_
#define _PLATFORM_MT6877_POS_H_

#include <linux/types.h>

unsigned int consys_emi_set_remapping_reg_mt6877(phys_addr_t, phys_addr_t, phys_addr_t);

int consys_conninfra_on_power_ctrl_mt6877(unsigned int enable);
int consys_conninfra_wakeup_mt6877(void);
int consys_conninfra_sleep_mt6877(void);
void consys_set_if_pinmux_mt6877(unsigned int enable);
int consys_polling_chipid_mt6877(void);

int connsys_d_die_cfg_mt6877(void);
int connsys_spi_master_cfg_mt6877(unsigned int);
int connsys_a_die_cfg_mt6877(void);
int connsys_afe_wbg_cal_mt6877(void);
int connsys_subsys_pll_initial_mt6877(void);
int connsys_low_power_setting_mt6877(unsigned int, unsigned int);

int consys_sema_acquire_timeout_mt6877(unsigned int index, unsigned int usec);
void consys_sema_release_mt6877(unsigned int index);

int consys_spi_read_mt6877(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data);
int consys_spi_write_mt6877(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data);
int consys_spi_update_bits_mt6877(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data, unsigned int mask);

int consys_spi_clock_switch_mt6877(enum connsys_spi_speed_type type);
int consys_subsys_status_update_mt6877(bool, int);
bool consys_is_rc_mode_enable_mt6877(void);
int connsys_adie_top_ck_en_ctl_mt6877(bool);

#endif /* _PLATFORM_MT6877_POS_H_ */

