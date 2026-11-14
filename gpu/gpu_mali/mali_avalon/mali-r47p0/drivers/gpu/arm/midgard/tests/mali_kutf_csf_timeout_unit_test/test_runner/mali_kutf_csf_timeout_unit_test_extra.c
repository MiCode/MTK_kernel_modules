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
#include "mali_kutf_csf_timeout_unit_test_extra.h"
#include "mali_kutf_test_helpers.h"
#include "../mali_kutf_csf_timeout_unit_test.h"
#include <fcntl.h>
#include <unistd.h>
#include <helpers/mali_base_helpers.h>
#include <helpers/mali_base_helpers_csf.h>
#include <csf/helpers/mali_base_csf_scheduler_helpers.h>
#include <base/tests/common/mali_base_user_common.h>
#include "uapi/gpu/arm/midgard/csf/mali_kbase_csf_errors_dumpfault.h"

#define WAIT_PROGRESS_TIMEOUT_NS ((uint64_t)500 * 1E6)
#define FAULT_WAIT_TIME_NS ((uint64_t)1E9) /* 1 sec */

static mali_error send_values(struct mali_utf_suite *const suite, unsigned int const ctx_id,
			      mali_addr64 const gpu_queue_va)
{
	int err;

	mali_utf_logdbg("Sending values to kernel space\n");

	/* Send Global ID of the Base context */
	mali_utf_loginf(BASE_CTX_ID "=%d\n", ctx_id);
	err = kutf_test_helpers_userdata_send_named_u64(suite, BASE_CTX_ID, ctx_id);

	/* Send GPU VA of the GPU queue */
	if (!err) {
		mali_utf_loginf(GPU_QUEUE_VA "=%" PRIx64 "\n", gpu_queue_va);
		err = kutf_test_helpers_userdata_send_named_u64(suite, GPU_QUEUE_VA, gpu_queue_va);
	}

	return err ? MALI_ERROR_FUNCTION_FAILED : MALI_ERROR_NONE;
}

static void wait_for_kernel(struct mali_utf_suite *suite)
{
	struct kutf_test_helpers_named_val dummy;

	mali_utf_logdbg("Waiting for kernel space\n");

	(void)kutf_test_helpers_userdata_receive_check_val(&dummy, suite, TIMER_EVENT_SYNC,
							   KUTF_TEST_HELPERS_VALTYPE_U64);

	mali_utf_logdbg("Received sync from kernel space\n");
}

static void progress_timeout_error_cb(void *user_data,
				      struct base_gpu_queue_group_error *error_data)
{
	*(struct base_gpu_queue_group_error *)user_data = *error_data;
}

/* Check callback data to judge whether fault is notified as expected */
static void check_result(struct base_gpu_queue_group_error const *cb_data,
			 const unsigned int fixture_index)
{
	for (unsigned int gr = 0; gr < CSF_TIMEOUT_SUITE_FIXTURES; gr++) {
		if (gr == fixture_index) {
			MALI_UTF_ASSERT_UINT_EQ_M(cb_data[gr].error_type,
						  BASE_GPU_QUEUE_GROUP_ERROR_TIMEOUT,
						  "Group[%d] isn't notified of timeout fault[%d]",
						  gr, cb_data[gr].error_type);
			for (unsigned int i = 0; i < NELEMS(cb_data[gr].padding); i++)
				MALI_UTF_ASSERT_UINT_EQ(cb_data[gr].padding[i], 0);
			char const *payloadp = (char const *)&cb_data[gr].payload;
			for (char const *p = payloadp; p < payloadp + sizeof(cb_data[0].payload);
			     p++)
				MALI_UTF_ASSERT_UINT_EQ_M(*p, 0, "Non zero payload");

		} else {
			MALI_UTF_ASSERT_UINT_EQ_M(cb_data[gr].error_type,
						  BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
						  "Group[%d] is unexpectedly notified of fault[%d]",
						  gr, cb_data[gr].error_type);
		}
	}
}

/**
 * check_dump_on_fault() - Check if the expected context ID and PROGRESS_TIMER_TIMEOUT
 *                    error code is returned by kbase via the "csf_fault" debugfs file.
 *
 * @suite:  Pointer to the test suite.
 * @cb_data: Pointer to the structure containing unrecoverable fault error information.
 * @dof_fd: File descriptor of the "csf_fault" debugfs file.
 * @ctx_id: ID of the context corresponding to the GPU queue for which the CS fault
 *          event was generated.
 */
static void check_dump_on_fault(struct mali_utf_suite *suite,
				struct base_gpu_queue_group_error const *cb_data, int dof_fd,
				unsigned int ctx_id)
{
	struct kutf_test_helpers_named_val dummy;
	int error = kutf_test_helpers_userdata_receive_check_val(&dummy, suite, DOF_CHECK_DONE,
								 KUTF_TEST_HELPERS_VALTYPE_U64);

	if (!error) {
		uint32_t tgid = 0, df_ctx_id = 0;
		uint32_t _err = 0;
		char buf[256];
		int ret;
		const ssize_t num_bytes = read(dof_fd, buf, sizeof(buf));

		if (num_bytes < 0) {
			mali_utf_test_fail("Failed to read fault info from csf_fault debugfs file");
			return;
		}

		ret = sscanf(buf, "%u_%u_%u", &tgid, &df_ctx_id, &_err);
		MALI_UTF_ASSERT_INT_EQ_M(ret, 3, "Unexpected data read from csf_fault file");
		MALI_UTF_ASSERT_UINT_EQ_M(ctx_id, df_ctx_id, "Unexpected ctx id");
		MALI_UTF_ASSERT_UINT_EQ_M(_err, DF_PROGRESS_TIMER_TIMEOUT, "Unexpected error code");

		mali_tpi_sleep_ns(FAULT_WAIT_TIME_NS, false);

		MALI_UTF_ASSERT_UINT_EQ_M(cb_data[suite->fixture_index].error_type,
					  BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
					  "Group[%d] unexpectedly notified of the timeout fault",
					  suite->fixture_index);

		write(dof_fd, "done", 4);
	}
}

static void csf_timeout_event_test_common(struct mali_utf_suite *suite, bool test_dof)
{
	if (!base_is_debugfs_supported()) {
		mali_utf_test_skip_msg("Skip test as DEBUGFS is not enabled.");
		return;
	}

	base_context ctx;
	int dof_fd = -1;

	MALI_UTF_ASSERT_EX(base_context_init(&ctx, BASE_CONTEXT_CSF_EVENT_THREAD));

	basep_test_single_cs_group groups[CSF_TIMEOUT_SUITE_FIXTURES];
	basep_test_csf_job_resources job_res = { { { 0 } } };
	static const struct base_gpu_queue_group_error data = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = { 1, 1, 1, 1, 1, 1, 1 },
	};
	struct base_gpu_queue_group_error cb_data[CSF_TIMEOUT_SUITE_FIXTURES];
	basep_test_gpu_queue_group_params params[CSF_TIMEOUT_SUITE_FIXTURES];

	static const basep_test_gpu_queue_group_params param = {
		.cs_min = CSF_NR_INTERFACES,
		.priority = BASE_QUEUE_GROUP_PRIORITY_MEDIUM,
		.tiler_mask = CSF_TILER_MASK,
		.fragment_mask = CSF_FRAGMENT_MASK,
		.compute_mask = CSF_COMPUTE_MASK,
		.tiler_max = CSF_TILER_MAX,
		.fragment_max = CSF_FRAGMENT_MAX,
		.compute_max = CSF_COMPUTE_MAX,
		.error_callback = progress_timeout_error_cb,
	};

	for (unsigned int gr = 0; gr < CSF_TIMEOUT_SUITE_FIXTURES; gr++) {
		cb_data[gr] = data;
		params[gr] = param;
		params[gr].error_data = &cb_data[gr];
		/* Set priority of one the group to be higher than other groups
		 * to avoid any priority update requests for the groups.
		 */
		if (gr == suite->fixture_index) {
			params[gr].priority = BASE_QUEUE_GROUP_PRIORITY_HIGH;
		}
	}

	if (test_dof) {
		char fname[256];

		snprintf(fname, sizeof(fname), KBASE_DEBUGFS_DIR_PREFIX "%u/csf_fault", 0);
		dof_fd = open(fname, O_RDWR);
		if (dof_fd < 0)
			mali_utf_test_fail("Failed to open %s", fname);
	}

	/* Create GPU command queue groups and a queue per group in ctx */
	if (basep_test_csf_jobs_and_groups_init_multi_group_cfgs(
		    &ctx, &job_res, params, groups, CSF_TIMEOUT_SUITE_FIXTURES, false)) {
		/* This would make the kernel driver schedule the queue group,
		 * otherwise the fake interrupt generated by the test module
		 * would not be handled by the kernel driver as it won't expect
		 * an interrupt from firmware for a queue group which hasn't
		 * been started/scheduled.
		 */
		for (unsigned int gr = 0; gr < CSF_TIMEOUT_SUITE_FIXTURES; gr++) {
			basep_gpu_queue_kick(groups[gr].queue, false);
		}

		/* Wait for kernel to trigger fake progress timeout fault. */
		MALI_UTF_ASSERT_UINT_EQ_M(mali_tpi_sleep_ns(WAIT_PROGRESS_TIMEOUT_NS, false),
					  MALI_TPI_SLEEP_STATUS_OK, "Failed to wait sched");

		unsigned int ctx_id = base_get_context_id(&ctx);

		/* Send timeout event parameters to kernel space */
		mali_addr64 const gpu_queue_va =
			base_mem_gpu_address(groups[suite->fixture_index].queue->basep.buffer_h, 0);

		if (mali_error_no_error(send_values(suite, ctx_id, gpu_queue_va))) {
			if (dof_fd >= 0)
				check_dump_on_fault(suite, cb_data, dof_fd, ctx_id);

			/* Wait for progress timeout fault notified */
			wait_for_kernel(suite);
			MALI_UTF_ASSERT_UINT_EQ_M(mali_tpi_sleep_ns(FAULT_WAIT_TIME_NS, false),
						  MALI_TPI_SLEEP_STATUS_OK,
						  "Failed to wait for callback");

			check_result(cb_data, suite->fixture_index);
		}

		basep_test_csf_resource_terminate(&ctx, &job_res, groups,
						  CSF_TIMEOUT_SUITE_FIXTURES);
	}

	if (dof_fd >= 0)
		close(dof_fd);
	base_context_term(&ctx);
}

/**
 * This test tries to verify the handling of progress timer timeout event
 * on Kbase side.
 *
 * @suite:   Test suite
 */
static void csf_timeout_event_test(struct mali_utf_suite *suite)
{
	csf_timeout_event_test_common(suite, false);
}

/**
 * This test tries to verify the dump on fault functionality in Kbase when a
 * progress timer timeout occurs.
 * It opens the "csf_fault" debugfs file before injecting the fault and
 * wait for a signal from test module that a timeout error has been generated
 * and then reads the debugfs file to check for the error code and context ID
 * reported and also confirms that the error callback has not been invoked.
 *
 * @suite:   Test suite
 */
static void csf_timeout_event_test_with_dof(struct mali_utf_suite *suite)
{
#if CSTD_OS_ANDROID
	CSTD_UNUSED(suite);
	mali_utf_test_skip_msg("Test skipped on Android platform");
#else
	csf_timeout_event_test_common(suite, true);
#endif
}

struct kutf_extra_func_spec kutf_timeout_unit_test_extra_funcs[] = {
	{ CSF_TIMEOUT_APP_NAME,
	  CSF_TIMEOUT_SUITE_NAME,
	  UNIT_TEST_0,
	  { NULL, csf_timeout_event_test, NULL } },
	{ CSF_TIMEOUT_APP_NAME,
	  CSF_TIMEOUT_SUITE_NAME,
	  UNIT_TEST_1,
	  { NULL, csf_timeout_event_test_with_dof, NULL } },
	{ { 0 } } /* Marks the end of the list */
};
