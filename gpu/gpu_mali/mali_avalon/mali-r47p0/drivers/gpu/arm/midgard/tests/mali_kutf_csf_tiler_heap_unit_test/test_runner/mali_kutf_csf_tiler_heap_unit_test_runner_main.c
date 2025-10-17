/*
 * Copyright:
 * ----------------------------------------------------------------------------
 * This confidential and proprietary software may be used only as authorized
 * by a licensing agreement from ARM Limited.
 *      (C) COPYRIGHT 2019-2023 ARM Limited, ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorized copies and
 * copies may only be made to the extent permitted by a licensing agreement
 * from ARM Limited.
 * ----------------------------------------------------------------------------
 */

#include <tpi/mali_tpi.h>
#include <utf/include/mali_utf_main.h>
#include "mali_kutf_test_runner.h"
#include "mali_kutf_csf_tiler_heap_unit_test_extra.h"

#include "../mali_kutf_csf_tiler_heap_unit_test.h"

static int csf_tiler_heap_unit_test_runner_run_func(struct mali_utf_test_specifier *test_spec)
{
	int i = 0;

	kutf_test_runner_filter_app_clear();
	kutf_test_runner_filter_app_add(CSF_TILER_HEAP_APP_NAME);

	/* Add extra testing functions to the test */
	while (kutf_tiler_heap_unit_test_extra_funcs[i].app_name[0] != 0) {
		int err =
			kutf_test_runner_test_extras_add(&kutf_tiler_heap_unit_test_extra_funcs[i]);
		if (err)
			return err;
		i++;
	}

	return kutf_test_runner_helper_run_func(test_spec);
}

static void csf_tiler_heap_unit_test_runner_cli_info_func(void)
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
		ret = mali_utf_test_main(csf_tiler_heap_unit_test_runner_run_func,
					 kutf_test_runner_helper_parse_func,
					 csf_tiler_heap_unit_test_runner_cli_info_func, argc, argv);

		kutf_test_runner_term();
	}

	mali_tpi_shutdown();

	return ret;
}
