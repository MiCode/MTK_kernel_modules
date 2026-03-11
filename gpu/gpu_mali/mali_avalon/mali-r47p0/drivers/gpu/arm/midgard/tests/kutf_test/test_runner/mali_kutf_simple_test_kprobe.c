/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2023 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */


#include <utf/include/mali_utf_suite.h>
#include <utf/include/mali_utf_mem.h>
#include <utf/include/mali_utf_resultset.h>
#include <utf/include/mali_utf_helpers.h>
#include <utf/include/mali_utf_main.h>
#include <base/tests/internal/api_tests/helpers/mali_base_helpers_kmsg.h>
#include "mali_kutf_test_helpers.h"
#include "mali_kutf_test_runner.h"
#include "mali_kutf_simple_test_kprobe.h"
#include <errno.h>

#define KUTF_PROBE_KERNEL_FUNCTION  "kutf_kp_sample_kernel_function"
#define KUTF_PROBE_HANDLER_FUNCTION "kutf_kp_sample_handler"
#define KUTF_PROBE_HANDLER_FUNCTION_ENTRY_ARG1 "entry1"
#define KUTF_PROBE_HANDLER_FUNCTION_ENTRY_ARG2 "entry2"
#define KUTF_PROBE_HANDLER_FUNCTION_EXIT_ARG1  "exit1"
#define KUTF_PROBE_HANDLER_FUNCTION_EXIT_ARG2  "exit2"

static FILE *kmsg_file;

static int kutf_test_pre_kprobe(struct mali_utf_suite *suite)
{
	int ret;

	if (!(kutf_test_runner_helper_is_kprobe_available())) {
		mali_utf_test_skip_msg("kprobe is not enabled in kernel");
		return 0;
	}

	/* register function at entry */
	ret = kutf_test_runner_helper_register_kprobe(KUTF_PROBE_KERNEL_FUNCTION,
						1, /*function entry*/
						KUTF_PROBE_HANDLER_FUNCTION,
						"%s %s",
					KUTF_PROBE_HANDLER_FUNCTION_ENTRY_ARG1,
					KUTF_PROBE_HANDLER_FUNCTION_ENTRY_ARG2);
	MALI_UTF_ASSERT_FAIL(ret == 0);

	/* register function at exit */
	ret = kutf_test_runner_helper_register_kprobe(KUTF_PROBE_KERNEL_FUNCTION,
						0, /*function exit*/
						KUTF_PROBE_HANDLER_FUNCTION,
						"%s %s",
					KUTF_PROBE_HANDLER_FUNCTION_EXIT_ARG1,
					KUTF_PROBE_HANDLER_FUNCTION_EXIT_ARG2);
	MALI_UTF_ASSERT_FAIL(ret == 0);
	/* try to register another kprobe at same function entry
	 * which is not supported currently
	 */
	ret = kutf_test_runner_helper_register_kprobe(KUTF_PROBE_KERNEL_FUNCTION,
						1, /*function entry*/
						KUTF_PROBE_HANDLER_FUNCTION,
						"%s %s",
						"arg1", "arg2");
	MALI_UTF_ASSERT_FAIL(ret == EEXIST);

	/* try to register to invalid kernel function.
	 */
	ret = kutf_test_runner_helper_register_kprobe("invalid_function",
						1, /*function entry*/
						KUTF_PROBE_HANDLER_FUNCTION,
						"%s %s",
						"arg1", "arg2");
	MALI_UTF_ASSERT_FAIL(ret == EINVAL);

	kmsg_file = base_apitest_kmsg_open(0, SEEK_END);
	MALI_UTF_ASSERT_FAIL(kmsg_file != NULL);

	return 0;
}

static void kutf_test_post_kprobe(struct mali_utf_suite *suite)
{
	int ret;

	ret = kutf_test_runner_helper_unregister_kprobe(KUTF_PROBE_KERNEL_FUNCTION,
							1 /*function entry*/
							);
	MALI_UTF_ASSERT_FAIL(ret == 0);

	ret = kutf_test_runner_helper_unregister_kprobe(KUTF_PROBE_KERNEL_FUNCTION,
							0 /*function exit*/
							);
	MALI_UTF_ASSERT_FAIL(ret == 0);

	/* try to register invalid handler for a valid kernel function
	 */
	ret = kutf_test_runner_helper_register_kprobe(KUTF_PROBE_KERNEL_FUNCTION,
						1, /*function entry*/
						"invalid_handler",
						"%s %s",
						"arg1", "arg2");
	MALI_UTF_ASSERT_FAIL(ret == EINVAL);

	/* try to unregister probe which is no longer registered
	 */
	ret = kutf_test_runner_helper_unregister_kprobe(KUTF_PROBE_KERNEL_FUNCTION,
							1 /*function entry*/
							);
	MALI_UTF_ASSERT_FAIL(ret == EINVAL);

	if (!(base_apitest_kmsg_find_msg(kmsg_file,
					KUTF_PROBE_HANDLER_FUNCTION " "\
				KUTF_PROBE_HANDLER_FUNCTION_ENTRY_ARG1)))
		mali_utf_test_fail("dmesg missing " \
				KUTF_PROBE_HANDLER_FUNCTION_ENTRY_ARG1);

	if (!(base_apitest_kmsg_find_msg(kmsg_file,
					KUTF_PROBE_HANDLER_FUNCTION " "\
				KUTF_PROBE_HANDLER_FUNCTION_EXIT_ARG1)))
		mali_utf_test_fail("dmesg missing " \
				KUTF_PROBE_HANDLER_FUNCTION_EXIT_ARG1);

	fclose(kmsg_file);
}

struct kutf_extra_func_spec kutf_simple_test_kprobe = {
	KPROBE_APP_NAME,   /* app_name */
	KPROBE_SUITE_NAME, /* suite_name */
	KPROBE_TEST_NAME,  /* test_name */
	{
		kutf_test_pre_kprobe, /* pretest */
		NULL,                 /* midtest */
		kutf_test_post_kprobe /* posttest */
	}
};
