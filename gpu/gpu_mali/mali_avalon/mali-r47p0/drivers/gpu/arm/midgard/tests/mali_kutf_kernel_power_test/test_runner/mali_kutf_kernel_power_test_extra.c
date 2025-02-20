// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
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

#include <utf/mali_utf.h>
#include <base/mali_base.h>
#include <kutf/kutf_resultset.h>
#include <base/tests/common/mali_base_user_common.h>
#include "mali_kutf_kernel_power_test_extra.h"
#include <base/tests/internal/api_tests/helpers/mali_base_helpers.h>

#include <base/tests/internal/api_tests/helpers/mali_base_helpers_csf.h>
#include <base/tests/internal/api_tests/csf/helpers/mali_base_csf_scheduler_helpers.h>

#define NS_PER_SEC ((uint64_t)1E9)

static char *default_poweroff_settings = "400000 2 2";
static char *instant_poweroff_settings = "400000 0 0";

/*
 * mali_kutf_kernel_power_test_csf_setup - creates an infinitely looping
 * active job claiming the shader cores, required by some of the poweroff
 * tests. Returns upon setup completion, at which point the kernel-side
 * of the test is run.
 *
 * Return: Zero on success, non-zero on failure
 */
static int mali_kutf_kernel_power_test_csf_setup(mali_utf_suite *test_suite)
{
	struct kutf_userspace_power_fixture_data *fix = test_suite->fixture;
	base_context *ctx = &fix->ctx;
	basep_test_csf_job_resources *jobs_res = &fix->jobs_res;
	bool success = basep_test_csf_job_resources_init(ctx, jobs_res, 1, false, false);

	if (success) {
		const basep_test_gpu_queue_group_params csg_cfg = {
			.cs_min = 1,
			.priority = BASE_QUEUE_GROUP_PRIORITY_HIGH,
			.tiler_mask = UINT64_MAX,
			.fragment_mask = UINT64_MAX,
			.compute_mask = UINT64_MAX,
			.tiler_max = 64,
			.fragment_max = 64,
			.compute_max = 64,
		};

		if (basep_test_single_cs_group_init_with_param(&fix->group, ctx, &csg_cfg) &&
		    basep_test_csf_enqueue_loop_jobs_flush(jobs_res, &fix->group, 1,
							   INFINITE_LOOP_COUNT)) {
			/* wait until the MCU is settled with the core requests */
			mali_tpi_sleep_ns(2 * NS_PER_SEC, true);

			return 0;
		}
		mali_utf_logerr("Failed to assemble the gpu-queue looping job");
	}
	mali_utf_logerr("Could not initialize csf resources");
	return 1;
}

/*
 * mali_kutf_kernel_power_test_poweroff_setup - sets a new poweroff policy
 * before starting the tests.
 *
 * Return: Zero on success, non-zero on failure
 */
static int mali_kutf_kernel_power_test_poweroff_setup(mali_utf_suite *test_suite,
						      char const *poweroff)
{
	struct kutf_userspace_power_fixture_data *fix = test_suite->fixture;

	if (mali_error_is_error(base_test_get_pm_poweroff(fix->old_pm_poweroff,
							  sizeof(fix->old_pm_poweroff)))) {
		mali_utf_logerr("could not get pm_poweroff\n");
		return 1;
	}

	if (mali_error_is_error(base_test_set_pm_poweroff(poweroff))) {
		mali_utf_logerr("could not set pm_poweroff\n");
		return 1;
	}

	return 0;
}

/* mali_kutf_kernel_power_test_pretest - sets up the appropriate power policy for the test
 * and initializes the CSF looping job if the tests are compiled with CSF support.
 *
 * @test_suite: Mali UTF Test Suite
 * @poweroff:   Poweroff policy string
 *
 * Return: Zero on success, non-zero on failure
 */
static int mali_kutf_kernel_power_test_pretest(mali_utf_suite *test_suite, char const *poweroff)
{
	mali_utf_mempool *pool = &test_suite->fixture_pool;
	struct kutf_userspace_power_fixture_data *fix =
		mali_utf_mempool_alloc(pool, sizeof(struct kutf_userspace_power_fixture_data));
	if (fix == NULL)
		return 1;

	test_suite->fixture = (void *)fix;

	fix->group = (basep_test_single_cs_group){ 0 };
	base_context *ctx = &fix->ctx;

	if (!base_context_init(ctx, BASE_CONTEXT_CSF_EVENT_THREAD)) {
		mali_utf_logerr("could not initialize base_context\n");
		return 1;
	}

	if (mali_kutf_kernel_power_test_csf_setup(test_suite))
		return 1;
	return mali_kutf_kernel_power_test_poweroff_setup(test_suite, poweroff);
}

/* mali_kutf_kernel_power_test_default_pretest - wrapper to setup the pretest
 * for tests using the default power policy
 *
 * Return: Zero on success, non-zero on failure
 */
int mali_kutf_kernel_power_test_default_pretest(mali_utf_suite *test_suite)
{
	return mali_kutf_kernel_power_test_pretest(test_suite, default_poweroff_settings);
}

/* mali_kutf_kernel_power_test_instant_poweroff_pretest - wrapper to setup the pretest
 * for instant poweroff policy tests.
 *
 * Return: Zero on success, non-zero on failure
 */
int mali_kutf_kernel_power_test_instant_poweroff_pretest(mali_utf_suite *test_suite)
{
	return mali_kutf_kernel_power_test_pretest(test_suite, instant_poweroff_settings);
}

/* mali_kutf_kernel_power_test_default_poweroff_no_csf_pretest - sets up instant
 * poweroff without setting up CSF. Some of the kernel-only test cases only require
 * a default power policy and do not require userspace CSF interaction.
 *
 * Return: Zero on success, non-zero on failure
 */
int mali_kutf_kernel_power_test_default_poweroff_no_csf_pretest(mali_utf_suite *test_suite)
{
	mali_utf_mempool *pool = &test_suite->fixture_pool;
	struct kutf_userspace_power_fixture_data *fix =
		mali_utf_mempool_alloc(pool, sizeof(struct kutf_userspace_power_fixture_data));
	if (fix == NULL)
		return 1;

	test_suite->fixture = (void *)fix;

	return mali_kutf_kernel_power_test_poweroff_setup(test_suite, default_poweroff_settings);
}

/* mali_kutf_kernel_power_test_instant_poweroff_no_csf_pretest - sets up instant
 * poweroff without setting up CSF. Some of the kernel-only test cases only require
 * a default power policy and do not require userspace CSF interaction.
 *
 * Return: Zero on success, non-zero on failure
 */
int mali_kutf_kernel_power_test_instant_poweroff_no_csf_pretest(mali_utf_suite *test_suite)
{
	mali_utf_mempool *pool = &test_suite->fixture_pool;
	struct kutf_userspace_power_fixture_data *fix =
		mali_utf_mempool_alloc(pool, sizeof(struct kutf_userspace_power_fixture_data));
	if (fix == NULL)
		return 1;

	test_suite->fixture = (void *)fix;

	return mali_kutf_kernel_power_test_poweroff_setup(test_suite, instant_poweroff_settings);
}

/* mali_kutf_kernel_power_test_teardown - removes the looping CSF job
 * and restores the previously held poweroff policy.
 */
void mali_kutf_kernel_power_test_teardown(mali_utf_suite *test_suite)
{
	struct kutf_userspace_power_fixture_data *fix = test_suite->fixture;

	base_test_set_pm_poweroff(fix->old_pm_poweroff);
	basep_test_csf_job_resources *jobs_res = &fix->jobs_res;

	basep_test_csf_job_data_quit_now(jobs_res->jobs);
	basep_test_single_cs_group_term(&fix->group);
	basep_test_csf_job_resources_term(&fix->ctx, jobs_res);
	base_context_term(&fix->ctx);
}

/* mali_kutf_kernel_power_test_no_csf_teardown - restores the previous
 * power policy.
 */
void mali_kutf_kernel_power_test_no_csf_teardown(mali_utf_suite *test_suite)
{
	struct kutf_userspace_power_fixture_data *fix = test_suite->fixture;

	base_test_set_pm_poweroff(fix->old_pm_poweroff);
}

struct kutf_extra_func_spec power_test_default_power_cycle = {
	KERNEL_POWER_TEST_APP_NAME,
	KERNEL_POWER_TEST_DEFAULT_SUITE,
	KERNEL_POWER_TEST_POWER_CYCLE,
	{ mali_kutf_kernel_power_test_default_pretest, NULL, mali_kutf_kernel_power_test_teardown }
};

struct kutf_extra_func_spec power_test_default_power_cycle_whilst_wait_enqueued = {

	KERNEL_POWER_TEST_APP_NAME,
	KERNEL_POWER_TEST_DEFAULT_SUITE,
	KERNEL_POWER_TEST_POWER_CYCLE_ENQUEUED,
	{ mali_kutf_kernel_power_test_default_pretest, NULL, mali_kutf_kernel_power_test_teardown }
};

struct kutf_extra_func_spec power_test_default_power_core_availability_cg0 = {
	KERNEL_POWER_TEST_APP_NAME,
	KERNEL_POWER_TEST_DEFAULT_SUITE,
	KERNEL_POWER_TEST_POWER_CORE_AVAILABILITY_CG0,
	{ mali_kutf_kernel_power_test_default_pretest, NULL, mali_kutf_kernel_power_test_teardown }
};

struct kutf_extra_func_spec power_test_default_power_policy_unit = {
	KERNEL_POWER_TEST_APP_NAME,
	KERNEL_POWER_TEST_DEFAULT_SUITE,
	KERNEL_POWER_TEST_POWER_POLICY_UNIT,
	{ mali_kutf_kernel_power_test_default_poweroff_no_csf_pretest, NULL,
	  mali_kutf_kernel_power_test_no_csf_teardown }
};

struct kutf_extra_func_spec power_test_instant_poweroff_power_cycle = {
	KERNEL_POWER_TEST_APP_NAME,
	KERNEL_POWER_TEST_INSTANT_POWEROFF_SUITE,
	KERNEL_POWER_TEST_POWER_CYCLE,
	{ mali_kutf_kernel_power_test_instant_poweroff_pretest, NULL,
	  mali_kutf_kernel_power_test_teardown }
};

struct kutf_extra_func_spec power_test_instant_poweroff_power_cycle_whilst_wait_enqueued = {
	KERNEL_POWER_TEST_APP_NAME,
	KERNEL_POWER_TEST_INSTANT_POWEROFF_SUITE,
	KERNEL_POWER_TEST_POWER_CYCLE_ENQUEUED,
	{ mali_kutf_kernel_power_test_instant_poweroff_pretest, NULL,
	  mali_kutf_kernel_power_test_teardown }
};

struct kutf_extra_func_spec power_test_instant_poweroff_power_core_availability_cg0 = {
	KERNEL_POWER_TEST_APP_NAME,
	KERNEL_POWER_TEST_INSTANT_POWEROFF_SUITE,
	KERNEL_POWER_TEST_POWER_CORE_AVAILABILITY_CG0,
	{ mali_kutf_kernel_power_test_instant_poweroff_pretest, NULL,
	  mali_kutf_kernel_power_test_teardown }
};

struct kutf_extra_func_spec power_test_instant_poweroff_power_policy_unit = {
	KERNEL_POWER_TEST_APP_NAME,
	KERNEL_POWER_TEST_INSTANT_POWEROFF_SUITE,
	KERNEL_POWER_TEST_POWER_POLICY_UNIT,
	{ mali_kutf_kernel_power_test_default_poweroff_no_csf_pretest, NULL,
	  mali_kutf_kernel_power_test_no_csf_teardown }
};
