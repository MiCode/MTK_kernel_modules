// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "../include/consys_hw.h"
#include "../include/consys_reg_mng.h"

#include "include/mt6985.h"
#include "include/mt6985_atf.h"
#include "include/mt6985_pos.h"
#include "include/mt6985_consys_reg.h"
#include "include/mt6985_consys_reg_offset.h"
#include "include/mt6985_connsyslog.h"
#include "include/mt6985_soc.h"

struct consys_hw_ops_struct g_consys_hw_ops_mt6985 = {
	.consys_plt_init_atf_data = consys_init_atf_data_mt6985_atf,
	.consys_plt_clk_get_from_dts = consys_clk_get_from_dts_mt6985,
	.consys_plt_clock_buffer_ctrl = consys_clock_buffer_ctrl_mt6985,
	.consys_plt_co_clock_type = consys_co_clock_type_mt6985,
	/* POS */
	.consys_plt_conninfra_on_power_ctrl = consys_conninfra_on_power_ctrl_mt6985,
	.consys_plt_set_if_pinmux = consys_set_if_pinmux_mt6985,
	.consys_plt_polling_consys_chipid = consys_polling_chipid_mt6985,
	.consys_plt_d_die_cfg = connsys_d_die_cfg_mt6985,
	.consys_plt_spi_master_cfg = connsys_spi_master_cfg_mt6985,
	.consys_plt_subsys_pll_initial = connsys_subsys_pll_initial_mt6985,
	.consys_plt_low_power_setting = connsys_low_power_setting_mt6985,
	.consys_plt_soc_chipid_get = consys_soc_chipid_get_mt6985,
	.consys_plt_conninfra_wakeup = consys_conninfra_wakeup_mt6985,
	.consys_plt_conninfra_sleep = consys_conninfra_sleep_mt6985,
	.consys_plt_is_rc_mode_enable = consys_is_rc_mode_enable_mt6985,

	.consys_plt_clock_fail_dump = NULL, // consys_clock_fail_dump_mt6985,
	.consys_plt_get_hw_ver = consys_get_hw_ver_mt6985,
	.consys_plt_spi_read = consys_spi_read_mt6985,
	.consys_plt_spi_write = consys_spi_write_mt6985,
	.consys_plt_spi_update_bits = consys_spi_update_bits_mt6985,
	.consys_plt_spi_clock_switch = NULL, // consys_spi_clock_switch_mt6985,
	.consys_plt_subsys_status_update = consys_subsys_status_update_mt6985,

	.consys_plt_enable_power_dump = consys_enable_power_dump_mt6985,
	.consys_plt_reset_power_state = consys_reset_power_state_mt6985,
	.consys_plt_power_state = consys_power_state_dump_mt6985,
	.consys_plt_soc_timestamp_get = NULL, // consys_soc_timestamp_get_mt6985,
	.consys_plt_adie_detection = consys_get_adie_chipid_mt6985,
	.consys_plt_set_mcu_control = NULL, //consys_set_mcu_control_mt6985,
};

extern struct consys_hw_ops_struct g_consys_hw_ops_mt6985;
extern struct consys_platform_emi_ops g_consys_platform_emi_ops_mt6985;
extern struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6985;
extern struct consys_platform_coredump_ops g_consys_platform_coredump_ops_mt6985;

struct consys_reg_mng_ops g_dev_consys_reg_ops_mt6985 = {
	.consys_reg_mng_init = consys_reg_init_mt6985,
	.consys_reg_mng_deinit = consys_reg_deinit_mt6985,
	.consys_reg_mng_check_reable = consys_check_reg_readable_mt6985,
	.consys_reg_mng_check_reable_for_coredump = consys_check_reg_readable_for_coredump_mt6985,
	.consys_reg_mng_is_bus_hang = consys_is_bus_hang_mt6985,
	.consys_reg_mng_is_consys_reg = consys_is_consys_reg_mt6985,
	.consys_reg_mng_debug_init = consys_debug_init_mt6985,
	.consys_reg_mng_debug_deinit = consys_debug_deinit_mt6985,
};

const struct conninfra_plat_data mt6985_plat_data = {
	.chip_id = PLATFORM_SOC_CHIP_MT6985,
	.consys_hw_version = CONN_HW_VER_MT6985,
	.hw_ops = &g_consys_hw_ops_mt6985,
	.reg_ops = &g_dev_consys_reg_ops_mt6985,
	.platform_emi_ops = &g_consys_platform_emi_ops_mt6985,
	.platform_pmic_ops = &g_consys_platform_pmic_ops_mt6985,
	.platform_coredump_ops = &g_consys_platform_coredump_ops_mt6985,
	.connsyslog_config = &g_connsyslog_config_mt6985,
};


/* ATF data
 */
extern struct consys_platform_coredump_ops g_consys_platform_coredump_ops_mt6985_atf;

struct consys_hw_ops_struct g_consys_hw_ops_mt6985_atf = {
	.consys_plt_init_atf_data = consys_init_atf_data_mt6985_atf,
	.consys_plt_clk_get_from_dts = consys_clk_get_from_dts_mt6985,
	.consys_plt_clock_buffer_ctrl = consys_clock_buffer_ctrl_mt6985,
	.consys_plt_co_clock_type = consys_co_clock_type_mt6985,
	/* POS */
	.consys_plt_conninfra_on_power_ctrl = consys_conninfra_on_power_ctrl_mt6985,
	.consys_plt_set_if_pinmux = consys_set_if_pinmux_mt6985,
	.consys_plt_polling_consys_chipid = consys_polling_chipid_mt6985_atf,
	.consys_plt_d_die_cfg = connsys_d_die_cfg_mt6985_atf,
	.consys_plt_spi_master_cfg = connsys_spi_master_cfg_mt6985_atf,
	.consys_plt_subsys_pll_initial = connsys_subsys_pll_initial_mt6985_atf,
	.consys_plt_low_power_setting = connsys_low_power_setting_mt6985_atf,
	.consys_plt_soc_chipid_get = consys_soc_chipid_get_mt6985,
	.consys_plt_conninfra_wakeup = consys_conninfra_wakeup_mt6985_atf,
	.consys_plt_conninfra_sleep = consys_conninfra_sleep_mt6985_atf,
	.consys_plt_is_rc_mode_enable = consys_is_rc_mode_enable_mt6985,

	.consys_plt_clock_fail_dump = NULL,
	.consys_plt_get_hw_ver = consys_get_hw_ver_mt6985,
	.consys_plt_spi_read = consys_spi_read_mt6985_atf,
	.consys_plt_spi_write = consys_spi_write_mt6985_atf,
	.consys_plt_spi_update_bits = consys_spi_update_bits_mt6985_atf,
	.consys_plt_spi_clock_switch = NULL,
	.consys_plt_subsys_status_update = consys_subsys_status_update_mt6985_atf,

	.consys_plt_enable_power_dump = consys_enable_power_dump_mt6985,
	.consys_plt_reset_power_state = consys_reset_power_state_mt6985_atf,
	.consys_plt_power_state = consys_power_state_dump_mt6985_atf,
	.consys_plt_soc_timestamp_get = NULL,
	.consys_plt_adie_detection = consys_get_adie_chipid_mt6985,
	.consys_plt_set_mcu_control = consys_set_mcu_control_mt6985_atf,
};

struct consys_reg_mng_ops g_dev_consys_reg_ops_mt6985_atf = {
	.consys_reg_mng_init = consys_reg_init_mt6985,
	.consys_reg_mng_deinit = consys_reg_deinit_mt6985,
	.consys_reg_mng_debug_init = consys_debug_init_mt6985_atf,
	.consys_reg_mng_debug_deinit = consys_debug_deinit_mt6985_atf,
	.consys_reg_mng_is_consys_reg = consys_is_consys_reg_mt6985,
	.consys_reg_mng_check_readable_conninfra_on_status
		= consys_check_ap2conn_infra_on_mt6985,
	.consys_reg_mng_check_readable_conninfra_off_status
		= consys_check_conninfra_off_domain_status_mt6985_atf,
	.consys_reg_mng_check_readable_conninfra_log = consys_print_debug_mt6985_atf,
};

const struct conninfra_plat_data mt6985_plat_data_atf = {
	.chip_id = PLATFORM_SOC_CHIP_MT6985,
	.consys_hw_version = CONN_HW_VER_MT6985,
	.hw_ops = &g_consys_hw_ops_mt6985_atf,
	.reg_ops = &g_dev_consys_reg_ops_mt6985_atf,
	.platform_emi_ops = &g_consys_platform_emi_ops_mt6985,
	.platform_pmic_ops = &g_consys_platform_pmic_ops_mt6985,
	.platform_coredump_ops = &g_consys_platform_coredump_ops_mt6985_atf,
	.connsyslog_config = &g_connsyslog_config_mt6985,
};
