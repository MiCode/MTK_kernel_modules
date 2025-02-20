/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2017-2023 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include <tpi/mali_tpi.h>
#include <utf/include/mali_utf_main.h>
#include <utf/include/mali_utf_helpers.h>
#include "mali_kutf_test_runner.h"

/* Main entry point. */
int main(int argc, char **argv)
{
	int ret = 0;

	if (!mali_tpi_init())
		return MALI_UTF_RESULT_FATAL;

	mali_tpi_puts(MALI_UTF_SEP_THICK);
	mali_tpi_puts("  kutf test runner\n");
	mali_tpi_puts(MALI_UTF_SEP_THICK);

	ret = kutf_test_runner_init();
	if (ret)
		goto out_init_fail;

	/* The default test runner just uses the helper functions */
	ret = mali_utf_test_main(kutf_test_runner_helper_run_func,
				 kutf_test_runner_helper_parse_func,
				 kutf_test_runner_helper_cli_info_func, argc, argv);

	kutf_test_runner_term();

out_init_fail:
	mali_tpi_shutdown();

	return ret;
}
