// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "../include/consys_hw.h"
#include "../include/consys_reg_mng.h"
#include "include/mt6878.h"
#include "include/mt6878_atf.h"
#include "include/mt6878_connsyslog.h"
#include "include/mt6878_consys_reg.h"
#include "include/mt6878_consys_reg_offset.h"
#include "include/mt6878_pos.h"
#include "include/mt6878_soc.h"
#include "include/mt6878_pmic.h"

extern struct consys_platform_emi_ops g_consys_platform_emi_ops_mt6878;
extern struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6878_6637;
extern struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6878_6631;
extern struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6878_6631_6686;
extern struct consys_platform_coredump_ops g_consys_platform_coredump_ops_mt6878;



struct consys_hw_ops_struct g_consys_hw_ops_mt6878_6637 = {
	/* load from dts */
	/* TODO: mtcmos should move to a independent module */
	.consys_plt_clk_get_from_dts = consys_clk_get_from_dts_mt6878,

	/* clock */
	.consys_plt_co_clock_type = consys_get_co_clock_type_mt6878,
	.consys_plt_clock_buffer_ctrl = consys_clock_buffer_ctrl_mt6878,

	/* POS */
	.consys_plt_conninfra_on_power_ctrl = consys_conninfra_on_power_ctrl_mt6878,
	.consys_plt_set_if_pinmux = consys_set_if_pinmux_mt6878,

	.consys_plt_polling_consys_chipid = consys_polling_chipid_mt6878,
	.consys_plt_d_die_cfg = connsys_d_die_cfg_mt6878,
	.consys_plt_spi_master_cfg = connsys_spi_master_cfg_mt6878,
	.consys_plt_a_die_cfg = connsys_a_die_cfg_mt6878,
	.consys_plt_afe_wbg_cal = connsys_afe_wbg_cal_mt6878,
	.consys_plt_afe_sw_patch = connsys_afe_sw_patch_mt6878,
	.consys_plt_subsys_pll_initial = connsys_subsys_pll_initial_mt6878,
	.consys_plt_low_power_setting = connsys_low_power_setting_mt6878,
	.consys_plt_soc_chipid_get = consys_soc_chipid_get_mt6878,
	.consys_plt_conninfra_wakeup = consys_conninfra_wakeup_mt6878,
	.consys_plt_conninfra_sleep = consys_conninfra_sleep_mt6878,
	.consys_plt_is_rc_mode_enable = consys_is_rc_mode_enable_mt6878,

	/* debug */
	.consys_plt_get_hw_ver = consys_get_hw_ver_mt6878,

	.consys_plt_spi_read = consys_spi_read_mt6878,
	.consys_plt_spi_write = consys_spi_write_mt6878,
	.consys_plt_spi_update_bits = consys_spi_update_bits_mt6878,
	.consys_plt_spi_clock_switch = consys_spi_clock_switch_mt6878,
	.consys_plt_subsys_status_update = consys_subsys_status_update_mt6878,

	.consys_plt_thermal_query = consys_thermal_query_adie6637_mt6878,
	.consys_plt_enable_power_dump = consys_enable_power_dump_mt6878,
	.consys_plt_reset_power_state = consys_reset_power_state_mt6878,
	.consys_plt_power_state = consys_power_state_dump_mt6878,
	.consys_plt_soc_timestamp_get = consys_soc_timestamp_get_mt6878,
	.consys_plt_adie_detection = consys_adie_detection_adie6637_mt6878,
	.consys_plt_set_mcu_control = consys_set_mcu_control_mt6878,

	.consys_plt_pre_cal_backup = consys_pre_cal_backup_mt6878,
	.consys_plt_pre_cal_clean_data = consys_pre_cal_clean_data_mt6878,
};

struct consys_hw_ops_struct g_consys_hw_ops_mt6878_6631 = {
	/* load from dts */
	/* TODO: mtcmos should move to a independent module */
	.consys_plt_clk_get_from_dts = consys_clk_get_from_dts_mt6878,
	.consys_plt_clock_buffer_ctrl = consys_clock_buffer_ctrl_mt6878,

	/* clock */
	.consys_plt_co_clock_type = consys_get_co_clock_type_mt6878,

	/* POS */
	.consys_plt_conninfra_on_power_ctrl = consys_conninfra_on_power_ctrl_mt6878,
	.consys_plt_set_if_pinmux = consys_set_if_pinmux_mt6878,

	.consys_plt_polling_consys_chipid = consys_polling_chipid_mt6878,
	.consys_plt_d_die_cfg = connsys_d_die_cfg_mt6878,
	.consys_plt_spi_master_cfg = connsys_spi_master_cfg_mt6878,
	.consys_plt_a_die_cfg = connsys_a_die_cfg_mt6878,
	.consys_plt_afe_wbg_cal = connsys_afe_wbg_cal_mt6878,
	.consys_plt_afe_sw_patch = connsys_afe_sw_patch_mt6878,
	.consys_plt_subsys_pll_initial = connsys_subsys_pll_initial_mt6878,
	.consys_plt_low_power_setting = connsys_low_power_setting_mt6878,
	.consys_plt_soc_chipid_get = consys_soc_chipid_get_mt6878,
	.consys_plt_conninfra_wakeup = consys_conninfra_wakeup_mt6878,
	.consys_plt_conninfra_sleep = consys_conninfra_sleep_mt6878,
	.consys_plt_is_rc_mode_enable = consys_is_rc_mode_enable_mt6878,

	/* debug */
	.consys_plt_get_hw_ver = consys_get_hw_ver_mt6878,

	.consys_plt_spi_read = consys_spi_read_mt6878,
	.consys_plt_spi_write = consys_spi_write_mt6878,
	.consys_plt_spi_update_bits = consys_spi_update_bits_mt6878,
	.consys_plt_spi_clock_switch = consys_spi_clock_switch_mt6878,
	.consys_plt_subsys_status_update = consys_subsys_status_update_mt6878,

	.consys_plt_thermal_query = consys_thermal_query_adie6631_mt6878,
	.consys_plt_enable_power_dump = consys_enable_power_dump_mt6878,
	.consys_plt_reset_power_state = consys_reset_power_state_mt6878,
	.consys_plt_power_state = consys_power_state_dump_mt6878,
	.consys_plt_soc_timestamp_get = consys_soc_timestamp_get_mt6878,
	.consys_plt_adie_detection = consys_adie_detection_adie6631_mt6878,
	.consys_plt_set_mcu_control = consys_set_mcu_control_mt6878,
};

struct consys_hw_ops_struct g_consys_hw_ops_mt6878_6631_6686 = {
	/* load from dts */
	/* TODO: mtcmos should move to a independent module */
	.consys_plt_clk_get_from_dts = consys_clk_get_from_dts_mt6878_6686,

	/* clock */
	.consys_plt_co_clock_type = consys_get_co_clock_type_mt6878,
	.consys_plt_clock_buffer_ctrl = consys_clock_buffer_ctrl_mt6878,

	/* POS */
	.consys_plt_conninfra_on_power_ctrl = consys_conninfra_on_power_ctrl_mt6878,
	.consys_plt_set_if_pinmux = consys_set_if_pinmux_mt6878_6686,

	.consys_plt_polling_consys_chipid = consys_polling_chipid_mt6878,
	.consys_plt_d_die_cfg = connsys_d_die_cfg_mt6878,
	.consys_plt_spi_master_cfg = connsys_spi_master_cfg_mt6878,
	.consys_plt_a_die_cfg = connsys_a_die_cfg_mt6878,
	.consys_plt_afe_wbg_cal = connsys_afe_wbg_cal_mt6878,
	.consys_plt_afe_sw_patch = connsys_afe_sw_patch_mt6878,
	.consys_plt_subsys_pll_initial = connsys_subsys_pll_initial_mt6878,
	.consys_plt_low_power_setting = connsys_low_power_setting_mt6878,
	.consys_plt_soc_chipid_get = consys_soc_chipid_get_mt6878,
	.consys_plt_conninfra_wakeup = consys_conninfra_wakeup_mt6878,
	.consys_plt_conninfra_sleep = consys_conninfra_sleep_mt6878,
	.consys_plt_is_rc_mode_enable = consys_is_rc_mode_enable_mt6878,

	/* debug */
	.consys_plt_get_hw_ver = consys_get_hw_ver_mt6878,

	.consys_plt_spi_read = consys_spi_read_mt6878,
	.consys_plt_spi_write = consys_spi_write_mt6878,
	.consys_plt_spi_update_bits = consys_spi_update_bits_mt6878,

	.consys_plt_spi_1_read = consys_spi_1_read_mt6878,
	.consys_plt_spi_1_write = consys_spi_1_write_mt6878,
	.consys_plt_spi_1_update_bits = consys_spi_1_update_bits_mt6878,

	.consys_plt_spi_clock_switch = consys_spi_clock_switch_mt6878,
	.consys_plt_subsys_status_update = consys_subsys_status_update_mt6878,

	.consys_plt_thermal_query = consys_thermal_query_adie6631_mt6878,
	.consys_plt_enable_power_dump = consys_enable_power_dump_mt6878,
	.consys_plt_reset_power_state = consys_reset_power_state_mt6878,
	.consys_plt_power_state = consys_power_state_dump_mt6878,
	.consys_plt_soc_timestamp_get = consys_soc_timestamp_get_mt6878,
	.consys_plt_adie_detection = consys_adie_detection_adie6686_mt6878,
	.consys_plt_set_mcu_control = consys_set_mcu_control_mt6878,
};

struct consys_reg_mng_ops g_dev_consys_reg_ops_mt6878 = {
	.consys_reg_mng_init = consys_reg_init_mt6878,
	.consys_reg_mng_deinit = consys_reg_deinit_mt6878,
	.consys_reg_mng_debug_init = consys_debug_init_mt6878,
	.consys_reg_mng_debug_deinit = consys_debug_deinit_mt6878,

	.consys_reg_mng_is_consys_reg = consys_is_consys_reg_mt6878,

	.consys_reg_mng_check_readable_conninfra_on_status
		= consys_check_conninfra_on_domain_status_mt6878,
	.consys_reg_mng_check_readable_conninfra_bus_clock_status
		= consys_check_conninfra_bus_clock_status_mt6878,
	.consys_reg_mng_check_readable_conninfra_off_status
		= consys_check_conninfra_off_domain_status_mt6878,
	.consys_reg_mng_check_readable_conninfra_irq
		= consys_check_conninfra_irq_status_mt6878,
	.consys_reg_mng_check_readable_conninfra_log = consys_print_debug_mt6878,
	.consys_reg_mng_check_readable_conninfra_pmic_log = consys_pmic_debug_log_mt6878,
};

const struct conninfra_plat_data mt6878_6637_plat_data = {
	.chip_id = PLATFORM_SOC_CHIP,
	.consys_hw_version = CONN_HW_VER,
	.hw_ops = &g_consys_hw_ops_mt6878_6637,
	.reg_ops = &g_dev_consys_reg_ops_mt6878,
	.platform_emi_ops = &g_consys_platform_emi_ops_mt6878,
	.platform_pmic_ops = &g_consys_platform_pmic_ops_mt6878_6637,
	.platform_coredump_ops = &g_consys_platform_coredump_ops_mt6878,
	.connsyslog_config = &g_connsyslog_config,
};

const struct conninfra_plat_data mt6878_6631_plat_data = {
	.chip_id = PLATFORM_SOC_CHIP,
	.consys_hw_version = CONN_HW_VER,
	.hw_ops = &g_consys_hw_ops_mt6878_6631,
	.reg_ops = &g_dev_consys_reg_ops_mt6878,
	.platform_emi_ops = &g_consys_platform_emi_ops_mt6878,
	.platform_pmic_ops = &g_consys_platform_pmic_ops_mt6878_6631,
	.platform_coredump_ops = &g_consys_platform_coredump_ops_mt6878,
	.connsyslog_config = &g_connsyslog_config,
};

const struct conninfra_plat_data mt6878_6631_6686_plat_data = {
	.chip_id = PLATFORM_SOC_CHIP,
	.consys_hw_version = CONN_HW_VER,
	.hw_ops = &g_consys_hw_ops_mt6878_6631_6686,
	.reg_ops = &g_dev_consys_reg_ops_mt6878,
	.platform_emi_ops = &g_consys_platform_emi_ops_mt6878,
	.platform_pmic_ops = &g_consys_platform_pmic_ops_mt6878_6631_6686,
	.platform_coredump_ops = &g_consys_platform_coredump_ops_mt6878,
	.connsyslog_config = &g_connsyslog_config,
};
