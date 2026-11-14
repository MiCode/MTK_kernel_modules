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
#include "mali_kutf_test_runner.h"
#include "mali_kutf_simple_test_userdata.h"
#include "mali_kutf_simple_test_kprobe.h"

static int kutf_test_simple_test_runner_run_func(mali_utf_test_specifier *test_spec)
{
	int err;

	/* Limit testing to just the known simple apps */
	kutf_test_runner_filter_app_clear();
	err = kutf_test_runner_filter_app_add("test_app");
	if (err)
		return err;
	err = kutf_test_runner_filter_app_add("test_app2");
	if (err)
		return err;

	err = kutf_test_runner_filter_app_add(USERDATA_APP_NAME);
	if (err)
		return err;
	err = kutf_test_runner_filter_app_add(KPROBE_APP_NAME);
	if (err)
		return err;

	/* Add extra testing functions to some of the tests */
	err = kutf_test_runner_test_extras_add(&kutf_simple_test_add_two_values);
	if (err)
		return err;
	err = kutf_test_runner_test_extras_add(&kutf_simple_test_increasing_str_sizes);
	if (err)
		return err;
	err = kutf_test_runner_test_extras_add(&kutf_simple_test_random_str_sizes);
	if (err)
		return err;
	err = kutf_test_runner_test_extras_add(&kutf_simple_test_kprobe);
	if (err)
		return err;
	return kutf_test_runner_helper_run_func(test_spec);
}

static void kutf_test_simple_test_runner_cli_info_func(void)
{
	/* --app is a hidden option that is accepted but ignored */
	mali_tpi_puts("    --kutf_base_dir=  Specify the base directory of kutf.\n");
}

/* Main entry point. */
int main(int argc, char **argv)
{
	int ret = 0;

	if (!mali_tpi_init())
		return MALI_UTF_RESULT_FATAL;

	ret = kutf_test_runner_init();
	if (ret)
		goto out_init_fail;

	/* The runner and cli_info functions are overridden
	 * The parse function is the stock helper for the moment, and will silently accept (but ignore) the --app
	 * argument. In future we could improve that to ensure we only accept the arguments supported by the core of the
	 * test runner
	 */
	ret = mali_utf_test_main(kutf_test_simple_test_runner_run_func,
				 kutf_test_runner_helper_parse_func,
				 kutf_test_simple_test_runner_cli_info_func, argc, argv);

	kutf_test_runner_term();

out_init_fail:
	mali_tpi_shutdown();

	return ret;
}
