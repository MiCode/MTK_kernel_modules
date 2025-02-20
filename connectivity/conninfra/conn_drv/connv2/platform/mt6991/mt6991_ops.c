// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "../include/consys_hw.h"
#include "../include/consys_reg_mng.h"

#include "include/mt6991.h"
#include "include/mt6991_pos.h"
#include "include/mt6991_consys_reg.h"
#include "include/mt6991_consys_reg_offset.h"
#include "include/mt6991_connsyslog.h"
#include "include/mt6991_soc.h"

struct consys_hw_ops_struct g_consys_hw_ops_mt6991 = {
	.consys_plt_clk_get_from_dts = consys_clk_get_from_dts_mt6991,
	.consys_plt_clock_buffer_ctrl = consys_clock_buffer_ctrl_mt6991,
	.consys_plt_co_clock_type = consys_co_clock_type_mt6991,
	/* POS */
	.consys_plt_conninfra_on_power_ctrl = consys_conninfra_on_power_ctrl_mt6991,
	.consys_plt_polling_consys_chipid = consys_polling_chipid_mt6991,
	.consys_plt_d_die_cfg = connsys_d_die_cfg_mt6991,
	.consys_plt_spi_master_cfg = connsys_spi_master_cfg_mt6991,
	.consys_plt_afe_sw_patch = connsys_afe_sw_patch_mt6991_atf,
	.consys_plt_subsys_pll_initial = connsys_subsys_pll_initial_mt6991,
	.consys_plt_low_power_setting = connsys_low_power_setting_mt6991,
	.consys_plt_soc_chipid_get = consys_soc_chipid_get_mt6991,
	.consys_plt_conninfra_wakeup = consys_conninfra_wakeup_mt6991,
	.consys_plt_conninfra_sleep = consys_conninfra_sleep_mt6991,
	.consys_plt_is_rc_mode_enable = consys_is_rc_mode_enable_mt6991,

	.consys_plt_get_hw_ver = consys_get_hw_ver_mt6991,
	.consys_plt_spi_read = consys_spi_read_mt6991,
	.consys_plt_spi_write = consys_spi_write_mt6991,
	.consys_plt_spi_update_bits = consys_spi_update_bits_mt6991,
	.consys_plt_subsys_status_update = consys_subsys_status_update_mt6991,

	.consys_plt_enable_power_dump = consys_enable_power_dump_mt6991,
	.consys_plt_reset_power_state = consys_reset_power_state_mt6991,
	.consys_plt_power_state = consys_power_state_dump_mt6991,
	.consys_plt_adie_detection = consys_get_adie_chipid_mt6991,
	.consys_plt_register_irq = consys_register_irq_mt6991,
	.consys_plt_unregister_irq = consys_unregister_irq_mt6991,
	.consys_plt_set_mcu_control = consys_set_mcu_control_mt6991,
};

struct consys_reg_mng_ops g_dev_consys_reg_ops_mt6991 = {
	.consys_reg_mng_init = consys_reg_init_mt6991,
	.consys_reg_mng_deinit = consys_reg_deinit_mt6991,
	.consys_reg_mng_check_reable = consys_check_reg_readable_mt6991,
	.consys_reg_mng_check_reable_for_coredump = consys_check_reg_readable_for_coredump_mt6991,
	.consys_reg_mng_is_bus_hang = consys_is_bus_hang_mt6991,
	.consys_reg_mng_is_consys_reg = consys_is_consys_reg_mt6991,
	.consys_reg_mng_debug_init = consys_debug_init_mt6991,
	.consys_reg_mng_debug_deinit = consys_debug_deinit_mt6991,
};

extern struct consys_hw_ops_struct g_consys_hw_ops_mt6991;
extern struct consys_reg_mng_ops g_dev_consys_reg_ops_mt6991;
extern struct consys_platform_emi_ops g_consys_platform_emi_ops_mt6991;
extern struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6991;
extern struct consys_platform_coredump_ops g_consys_platform_coredump_ops_mt6991;

const struct conninfra_plat_data mt6991_plat_data = {
	.chip_id = PLATFORM_SOC_CHIP_MT6991,
	.consys_hw_version = CONN_HW_VER_MT6991,
	.hw_ops = &g_consys_hw_ops_mt6991,
	.reg_ops = &g_dev_consys_reg_ops_mt6991,
	.platform_emi_ops = &g_consys_platform_emi_ops_mt6991,
	.platform_pmic_ops = &g_consys_platform_pmic_ops_mt6991,
	.platform_coredump_ops = &g_consys_platform_coredump_ops_mt6991,
	.connsyslog_config = &g_connsyslog_config_mt6991,
};
