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

#include <utf/include/mali_utf_suite.h>
#include <utf/include/mali_utf_mem.h>
#include <utf/include/mali_utf_resultset.h>
#include <utf/include/mali_utf_helpers.h>
#include <utf/include/mali_utf_main.h>
#include <base/mali_base.h>
#include <base/mali_base_submission_gpu.h>
#include <helpers/mali_base_helpers_csf.h>
#include <csf/helpers/mali_base_csf_scheduler_helpers.h>

#include "mali_kutf_csf_ipa_control_unit_test_extra.h"
#include "../mali_kutf_csf_ipa_control_unit_test.h"

struct csf_ipa_control_gpu_loop_job_data {
	base_context ctx;
	basep_test_single_cs_group group;
	basep_test_csf_job_resources job;
};

static basep_test_gpu_queue_group_params params = { .cs_min = 1,
						    .priority = BASE_QUEUE_GROUP_PRIORITY_MEDIUM,
						    .tiler_mask = UINT64_MAX,
						    .fragment_mask = UINT64_MAX,
						    .compute_mask = UINT64_MAX,
						    .tiler_max = 64,
						    .fragment_max = 64,
						    .compute_max = 64 };

static struct csf_ipa_control_gpu_loop_job_data gpu_loop_job_data;

static int csf_ipa_control_pretest(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	bool success;
	base_context *ctx = &gpu_loop_job_data.ctx;
	basep_test_single_cs_group *group = &gpu_loop_job_data.group;
	basep_test_csf_job_resources *job = &gpu_loop_job_data.job;

	success = base_context_init(ctx, BASE_CONTEXT_CSF_EVENT_THREAD);
	MALI_UTF_ASSERT_EX_M(success, "Failed to create context");

	if (!success)
		return -ENOMEM;

	fix->extra_funcs_data = (void *)&gpu_loop_job_data;

	if (basep_test_csf_jobs_and_groups_init(ctx, job, &params, group, 1, false)) {
		if (!basep_test_csf_enqueue_loop_jobs_noreqres_flush(job, group, 1,
								     INFINITE_LOOP_COUNT)) {
			mali_utf_test_fail("Failed to assemble group's job");
			return -ENOMEM;
		}
	} else {
		mali_utf_test_fail("Failed to initialize group");
		return -ENOMEM;
	}

	return 0;
}

static void csf_ipa_control_posttest(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	struct csf_ipa_control_gpu_loop_job_data *gpu_loop_job_data = fix->extra_funcs_data;
	base_context *ctx = &gpu_loop_job_data->ctx;
	basep_test_single_cs_group *group = &gpu_loop_job_data->group;
	basep_test_csf_job_resources *job = &gpu_loop_job_data->job;

	basep_test_csf_resource_terminate(ctx, job, group, 1);
	base_context_term(ctx);
}

struct kutf_extra_func_spec kutf_ipa_control_unit_test_extra_funcs[] = {
	{ CSF_IPA_CONTROL_APP_NAME,
	  CSF_IPA_CONTROL_QUERY_SUITE_NAME,
	  CSF_IPA_CONTROL_QUERY_UNIT_TEST_0,
	  { csf_ipa_control_pretest, NULL, csf_ipa_control_posttest } },
#if !CSTD_OS_ANDROID
	{ CSF_IPA_CONTROL_APP_NAME,
	  CSF_IPA_CONTROL_QUERY_SUITE_NAME,
	  CSF_IPA_CONTROL_QUERY_UNIT_TEST_1,
	  { csf_ipa_control_pretest, NULL, csf_ipa_control_posttest } },
#endif
	{ CSF_IPA_CONTROL_APP_NAME,
	  CSF_IPA_CONTROL_QUERY_SUITE_NAME,
	  CSF_IPA_CONTROL_QUERY_UNIT_TEST_2,
	  { csf_ipa_control_pretest, NULL, csf_ipa_control_posttest } },
#if !CSTD_OS_ANDROID
	{ CSF_IPA_CONTROL_APP_NAME,
	  CSF_IPA_CONTROL_QUERY_SUITE_NAME,
	  CSF_IPA_CONTROL_QUERY_UNIT_TEST_3,
	  { csf_ipa_control_pretest, NULL, csf_ipa_control_posttest } },
	{ CSF_IPA_CONTROL_APP_NAME,
	  CSF_IPA_CONTROL_RESET_SUITE_NAME,
	  CSF_IPA_CONTROL_RESET_UNIT_TEST_0,
	  { csf_ipa_control_pretest, NULL, csf_ipa_control_posttest } },
	{ CSF_IPA_CONTROL_APP_NAME,
	  CSF_IPA_CONTROL_RESET_SUITE_NAME,
	  CSF_IPA_CONTROL_RESET_UNIT_TEST_1,
	  { csf_ipa_control_pretest, NULL, csf_ipa_control_posttest } },
	{ CSF_IPA_CONTROL_APP_NAME,
	  CSF_IPA_CONTROL_RESET_SUITE_NAME,
	  CSF_IPA_CONTROL_RESET_UNIT_TEST_2,
	  { csf_ipa_control_pretest, NULL, csf_ipa_control_posttest } },
	{ CSF_IPA_CONTROL_APP_NAME,
	  CSF_IPA_CONTROL_RESET_SUITE_NAME,
	  CSF_IPA_CONTROL_RESET_UNIT_TEST_3,
	  { csf_ipa_control_pretest, NULL, csf_ipa_control_posttest } },
#endif
	{ CSF_IPA_CONTROL_APP_NAME,
	  CSF_IPA_CONTROL_RATE_CHANGE_SUITE_NAME,
	  CSF_IPA_CONTROL_RATE_CHANGE_UNIT_TEST_0,
	  { csf_ipa_control_pretest, NULL, csf_ipa_control_posttest } },
#if !CSTD_OS_ANDROID
	{ CSF_IPA_CONTROL_APP_NAME,
	  CSF_IPA_CONTROL_RATE_CHANGE_SUITE_NAME,
	  CSF_IPA_CONTROL_RATE_CHANGE_UNIT_TEST_1,
	  { csf_ipa_control_pretest, NULL, csf_ipa_control_posttest } },
#endif
	{ { 0 } } /* Marks the end of the list */
};
