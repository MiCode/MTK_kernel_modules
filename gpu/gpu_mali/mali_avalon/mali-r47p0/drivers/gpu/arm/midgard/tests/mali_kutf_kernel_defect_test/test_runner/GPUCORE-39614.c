/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2023 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file GPUCORE-39614.c
 *
 * Defect Test for GPUCORE-39614
 */
#include "mali_kutf_test_helpers.h"
#include <base/tests/common/mali_base_user_common.h>
#include <base/tests/internal/api_tests/helpers/mali_base_helpers.h>
#include <mali_base_hwconfig.h>

#if MALI_USE_CSF && !MALI_NO_MALI && BASE_HW_FEATURE_GPU_SLEEP
#include <base/tests/internal/api_tests/csf/helpers/mali_base_csf_scheduler_helpers.h>

#define AUTOSUSPEND_DELAY_MS 5000
#define EMPTY_QUEUE_TIMEOUT_NS ((uint64_t)2 * NS_PER_SEC)
#define JOB_COMPLETION_WAIT_NS ((uint64_t)1 * NS_PER_SEC)

#define CLEANUP_ID_ON_FAIL(cond, cleanup_id, ...)                                                \
	do {                                                                                     \
		if (!(cond)) {                                                                   \
			mali_utf_test_fail(__VA_ARGS__);                                         \
			MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(        \
						       suite, "GPUCORE39614_USERSPACE_FAIL", 0), \
					       0);                                               \
			goto cleanup_id;                                                         \
		}                                                                                \
	} while (0)

#define CLEANUP_ON_FAIL(cond, ...) CLEANUP_ID_ON_FAIL(cond, cleanup, __VA_ARGS__)

/* GPU_IDLE_STREAM - Command stream used to trigger GPU idle worker
 * WAIT_STREAM - Command stream used to trigger check_group_sync_update_worker
 */
enum { GPU_IDLE_STREAM = 0, WAIT_STREAM, STREAM_CNT };

enum { CQS_ARR_SET_IDX = 0, CQS_ARR_WAIT_IDX, CQS_ARR_CNT };

static bool basep_test_wait_for_empty_queue(base_gpu_command_queue *const queue,
					    unsigned int max_wait_ns)
{
	const uint64_t start_time = mali_tpi_get_time_ns();

	while (1) {
		if (basep_queue_get_complete_offset(&queue->basep.command_queue) ==
		    basep_queue_get_write_offset(&queue->basep.command_queue)) {
			break;
		}
		basep_queue_get_space(&queue->basep.command_queue, true);

		const uint64_t elapsed = mali_tpi_get_time_ns() - start_time;

		if (elapsed > max_wait_ns)
			return false;
	}

	return true;
}

/**
 * Main test function for the defect test associated with GPUCORE-39614.
 *
 * The test simulates a race between check_group_sync_update_worker and gpu_idle_worker, causing a
 * possible skip of scheduling. It represents a corner case where a scheduler wakeup is triggered
 * due to resolved CQS WAIT, where at the same time a GPU idle worker is trying to put scheduler
 * back to sleep. As a result, scheduling of the work submitted to GPU is delayed.
 *
 * The real case scenario is as follows:
 * 0. Scheduler is sleeping with a GPU queue blocked on CQS_WAIT.
 * 1. Worker check_group_sync_update_worker starts and holds scheduler->lock.
 * 2. Worker gpu_idle_worker is invoked and waits for scheduler->lock.
 * 3. check_group_sync_update_worker calling check_sync_update_in_sleep_mode calling
 *		check_sync_update_for_on_slot_group, and SYNC_WAIT for one of command stream was
 *		resolved.
 * 4. sync_update is true, calling scheduler_wakeup, sets scheduler->state = SCHED_INACTIVE and
 *		invokes the scheduler tick.
 * 5. Worker check_group_sync_update_worker ends.
 * 6. Worker gpu_idle_worker gets the scheduler->lock and starts.
 * 7. Calling scheduler_sleep_on_idle, sets kbdev->pm.backend.exit_gpu_sleep_mode as false and
 *		scheduler->state = SCHED_SLEEPING.
 * 8. Schedule_on_tick starts, and skips due to can_skip_scheduling return true since
 *		scheduler->state == SCHED_SLEEPING and
 *		kbdev->pm.backend.exit_gpu_sleep_mode == false.
 *
 * The test simulates this race condition, with the help of its kernel counterpart, as follows:
 * 1. Scheduler is put to sleep with a WAIT_STREAM blocked on CQS_WAIT
 * 2. Kernel temporarily disables the tick processing.
 * 3. CQS_WAIT is signalized so the check_group_sync_update_worker can do its routine, then it
 *		wakes up the scheduler.
 * 4. Kernel directly triggers the gpu_idle_worker. Important: a standard GPU idle event
 *		processing is skipped to simulate a state where gpu_idle_worker is already waiting
 *		for scheduler lock while check_group_sync_update_worker is working.
 * 5. Kernel enables the tick processing.
 * 6. It is checked whether the scheduling was performed (success) or skipped (fail).
 *
 * The synchronization between userspace and kernelspace is done via the KUTF data mechanism.
 */
static void GPUCORE39614_test_func(mali_utf_suite *suite)
{
	base_context ctx;
	struct kutf_test_helpers_named_val named_val;

	MALI_UTF_ASSERT_FAIL_EX_M(base_context_init(&ctx, BASE_CONTEXT_CSF_EVENT_THREAD),
				  "Failed to create a base context");

	int as_delay_restore = 0;

	/* Prevent uncontrolled autosuspend */
	CLEANUP_ID_ON_FAIL(base_test_set_autosuspend_delay_ms(AUTOSUSPEND_DELAY_MS,
							      &as_delay_restore),
			   cleanup_ctx, "Userspace failed to modify autosuspend delay");

	basep_test_csf_job_resources job_res;

	CLEANUP_ID_ON_FAIL(basep_test_csf_job_resources_init(&ctx, &job_res, STREAM_CNT, false,
							     false),
			   cleanup_autosuspend, "Userspace failed to init job resources");

	const basep_test_gpu_queue_group_params group_cfgs = {
		.cs_min = 1,
		.priority = BASE_QUEUE_GROUP_PRIORITY_HIGH,
		.tiler_mask = UINT64_MAX,
		.fragment_mask = UINT64_MAX,
		.compute_mask = UINT64_MAX,
		.tiler_max = 64,
		.fragment_max = 64,
		.compute_max = 64
	};
	basep_test_single_cs_group groups[STREAM_CNT];

	STDLIB_MEMSET(groups, 0, sizeof(groups));
	CLEANUP_ID_ON_FAIL(basep_test_single_cs_group_init_with_param(&groups[GPU_IDLE_STREAM],
								      &ctx, &group_cfgs),
			   cleanup_job, "Userspace failed to init cs groups");
	CLEANUP_ID_ON_FAIL(basep_test_single_cs_group_init_with_param(&groups[WAIT_STREAM], &ctx,
								      &group_cfgs),
			   cleanup_gpu_idle_stream, "Userspace failed to init cs groups");

	CLEANUP_ID_ON_FAIL(basep_test_csf_enqueue_loop_jobs_flush(
				   &job_res, &groups[GPU_IDLE_STREAM], 1, INFINITE_LOOP_COUNT),
			   cleanup_cs_groups,
			   "Userspace failed to enqueue loop jobss on GPU_IDLE_STREAM");

	base_cqs_object cqs_array[CQS_ARR_CNT] = { { { 0 } } };

	CLEANUP_ID_ON_FAIL(basep_test_init_cqs_array(cqs_array, NELEMS(cqs_array), &ctx),
			   cleanup_cs_groups, "Userspace failed to init CQS array");

	base_cqs_object *const p_cqs_set[] = { &cqs_array[CQS_ARR_SET_IDX] };
	uint32_t sb_mask = 0;
	uint8_t sb_entry = 0;
	base_cqs_notification_scope sync_scope = BASE_CQS_NOTIFICATION_SCOPE_NONE;

	mali_error err = base_gpu_queue_enqueue_cqs_set(groups[WAIT_STREAM].queue, p_cqs_set,
							&sb_mask, &sb_entry, &sync_scope,
							NELEMS(p_cqs_set));
	MALI_UTF_ASSERT_FAIL_M(mali_error_no_error(err), "Failed to enqueue CQS set");

	/* CQS Wait will be signaled to trigger check_group_sync_update_worker */
	base_cqs_object *const p_cqs_wait[] = { &cqs_array[CQS_ARR_WAIT_IDX] };

	err = base_gpu_queue_enqueue_cqs_wait(groups[WAIT_STREAM].queue, p_cqs_wait, NULL,
					      NELEMS(p_cqs_wait));
	MALI_UTF_ASSERT_FAIL_M(mali_error_no_error(err), "Failed to enqueue CQS wait");

	/* WAIT_STREAM doesn't contain any loop jobs because it needs to go idle once the CQS WAIT
	 * is resolved.
	 */
	base_gpu_queue_flush(groups[WAIT_STREAM].queue);

	CLEANUP_ON_FAIL(basep_test_wait_cqs_signaled(&cqs_array[CQS_ARR_SET_IDX],
						     JOB_COMPLETION_WAIT_NS),
			"CQS SET was not signalized in time");

	/* Kernel should now disable GPU idle events */
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, "GPUCORE39614_USERSPACE_DISABLE_GPU_IDLE_EVENT", 1),
			       0);

	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val, suite, "GPUCORE39614_KERNEL_GPU_IDLE_EVENT_OFF",
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);
	CLEANUP_ON_FAIL(named_val.u.val_u64 == KUTF_TEST_HELPERS_VALTYPE_U64,
			"Kernel failed to turn off GPU idle events");

	/* Quit job in GPU_IDLE_STREAM so the scheduler can go to sleep */
	basep_test_csf_job_data_quit_now(&job_res.jobs[GPU_IDLE_STREAM]);
	CLEANUP_ON_FAIL(basep_test_wait_for_empty_queue(groups[GPU_IDLE_STREAM].queue,
							EMPTY_QUEUE_TIMEOUT_NS),
			"Wait for GPU_IDLE_STREAM queue to get empty failed");

	/* Kernel needs to put scheduler to sleep to prepare race condition */
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, "GPUCORE39614_USERSPACE_SCHEDULER_SLEEP", 1),
			       0);

	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val, suite,
				       "GPUCORE39614_USERSPACE_TICK_PROCESSING_DISABLED",
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);
	CLEANUP_ON_FAIL(named_val.u.val_u64 == KUTF_TEST_HELPERS_VALTYPE_U64,
			"Failed to wait for kernel to disable tick processing");

	/* Trigger the check_group_sync_update_worker */
	base_cqs_signal(&cqs_array[CQS_ARR_WAIT_IDX], false);
	CLEANUP_ON_FAIL(basep_test_wait_cqs_signaled(&cqs_array[CQS_ARR_WAIT_IDX],
						     JOB_COMPLETION_WAIT_NS),
			"CQS WAIT was not signalized in time");

	/* Kernel should trigger gpu_idle_worker */
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, "GPUCORE39614_USERSPACE_TRIGGER_GPU_IDLE_WORKER", 1),
			       0);

	/* The rest of the test is done in kernel part */
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val, suite, "GPUCORE39614_KERNEL_FINISH",
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);
	CLEANUP_ON_FAIL(named_val.u.val_u64 == KUTF_TEST_HELPERS_VALTYPE_U64,
			"Failed to wait for kernel to finish the test");

cleanup:
	basep_test_term_cqs_array(cqs_array, NELEMS(cqs_array));
cleanup_cs_groups:
	basep_test_single_cs_group_term(&groups[WAIT_STREAM]);
cleanup_gpu_idle_stream:
	basep_test_single_cs_group_term(&groups[GPU_IDLE_STREAM]);
cleanup_job:
	basep_test_csf_job_resources_term(&ctx, &job_res);
cleanup_autosuspend:
	base_test_set_autosuspend_delay_ms(as_delay_restore, NULL);
cleanup_ctx:
	base_context_term(&ctx);
}

#endif /* MALI_USE_CSF && !MALI_NO_MALI && BASE_HW_FEATURE_GPU_SLEEP */

void GPUCORE39614(mali_utf_suite *suite)
{
#if MALI_USE_CSF && !MALI_NO_MALI && BASE_HW_FEATURE_GPU_SLEEP
	GPUCORE39614_test_func(suite);
#else
	CSTD_UNUSED(suite);
	mali_utf_test_skip_msg("%s defect test only available for CSF GPUs with sleep feature",
			       __func__);
#endif /* MALI_USE_CSF && !MALI_NO_MALI && BASE_HW_FEATURE_GPU_SLEEP */
}
