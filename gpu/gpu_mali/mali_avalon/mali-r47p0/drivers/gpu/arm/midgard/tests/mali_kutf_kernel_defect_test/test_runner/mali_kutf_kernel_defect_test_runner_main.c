/*
 * Copyright:
 * ----------------------------------------------------------------------------
 * This confidential and proprietary software may be used only as authorized
 * by a licensing agreement from ARM Limited.
 *      (C) COPYRIGHT 2021-2023 ARM Limited, ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorized copies and
 * copies may only be made to the extent permitted by a licensing agreement
 * from ARM Limited.
 * ----------------------------------------------------------------------------
 */

#include <tpi/mali_tpi.h>
#include <utf/include/mali_utf_main.h>
#include "mali_kutf_test_runner.h"

#include "mali_kutf_kernel_defect_test_extra.h"
#include "../mali_kutf_kernel_defect_test.h"

static int kernel_defect_test_runner_run_func(mali_utf_test_specifier *test_spec)
{
	kutf_test_runner_filter_app_clear();
	kutf_test_runner_filter_app_add(KERNEL_DEFECT_TEST_APP_NAME);
	kutf_test_runner_test_extras_add(&defect_test_GPUCORE27226);
	kutf_test_runner_test_extras_add(&defect_test_GPUCORE35490);
	kutf_test_runner_test_extras_add(&defect_test_GPUCORE37201);
	kutf_test_runner_test_extras_add(&defect_test_GPUCORE37465);
	kutf_test_runner_test_extras_add(&defect_test_GPUCORE39614);

	return kutf_test_runner_helper_run_func(test_spec);
}

static void misc_test_runner_cli_info_func(void)
{
	/* --app is a hidden option that is accepted but ignored */
	mali_tpi_puts("    --kutf_base_dir=  Specify the base directory of kutf.\n");
}

/* Main entry point. */
int main(int argc, char **argv)
{
	int ret;

	if (!mali_tpi_init())
		return MALI_UTF_RESULT_FATAL;

	ret = kutf_test_runner_init();

	if (!ret) {
		/* The runner and cli_info functions are overridden.
		 * The parse function is the stock helper for the moment,
		 * and will silently accept (but ignore) the --app
		 * argument. In future we could improve that to ensure we
		 * only accept the arguments supported by the core of the
		 * test runner
		 */
		ret = mali_utf_test_main(kernel_defect_test_runner_run_func,
					 kutf_test_runner_helper_parse_func,
					 misc_test_runner_cli_info_func, argc, argv);

		kutf_test_runner_term();
	}

	mali_tpi_shutdown();

	return ret;
}
