// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 * Copyright:
 * ----------------------------------------------------------------------------
 * This confidential and proprietary software may be used only as authorized
 * by a licensing agreement from ARM Limited.
 *      (C) COPYRIGHT 2021 ARM Limited, ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorized copies and
 * copies may only be made to the extent permitted by a licensing agreement
 * from ARM Limited.
 * ----------------------------------------------------------------------------
 */

#include <tpi/mali_tpi.h>
#include <utf/include/mali_utf_main.h>
#include "mali_kutf_test_runner.h"
#include "mali_kutf_kernel_power_test_extra.h"
#include "../mali_kutf_kernel_power_test.h"

static int kernel_power_test_runner_run_func(mali_utf_test_specifier *test_spec)
{
	kutf_test_runner_filter_app_clear();

	kutf_test_runner_filter_app_add(KERNEL_POWER_TEST_APP_NAME);

	kutf_test_runner_test_extras_add(&power_test_default_power_cycle);
	kutf_test_runner_test_extras_add(&power_test_default_power_cycle_whilst_wait_enqueued);
	kutf_test_runner_test_extras_add(&power_test_default_power_core_availability_cg0);
	kutf_test_runner_test_extras_add(&power_test_default_power_policy_unit);
	kutf_test_runner_test_extras_add(&power_test_instant_poweroff_power_cycle);
	kutf_test_runner_test_extras_add(
		&power_test_instant_poweroff_power_cycle_whilst_wait_enqueued);
	kutf_test_runner_test_extras_add(&power_test_instant_poweroff_power_core_availability_cg0);
	kutf_test_runner_test_extras_add(&power_test_instant_poweroff_power_policy_unit);

	return kutf_test_runner_helper_run_func(test_spec);
}

static void kernel_power_test_runner_cli_info_func(void)
{
	mali_tpi_puts("    --kutf_base_dir=  Specify the base directory of kutf.\n");
}

int main(int argc, char **argv)
{
	int ret;

	if (!mali_tpi_init())
		return MALI_UTF_RESULT_FATAL;

	ret = kutf_test_runner_init();

	if (!ret) {
		ret = mali_utf_test_main(kernel_power_test_runner_run_func,
					 kutf_test_runner_helper_parse_func,
					 kernel_power_test_runner_cli_info_func, argc, argv);

		kutf_test_runner_term();
	}

	mali_tpi_shutdown();

	return ret;
}
