// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2023 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#include <kutf/kutf_helpers_user.h>
#include "mali_kutf_kernel_defect_test_main.h"
#include <mali_kbase.h>
#include <kutf/kutf_utils.h>
#include "backend/gpu/mali_kbase_pm_internal.h"

#if MALI_USE_CSF && !IS_ENABLED(CONFIG_MALI_NO_MALI)

#define MAX_HYSTERESIS ((uint32_t)0xFFFFFFFF)

/* Tick processing timeout in ms */
#define TICK_PROCESSING_TIMEOUT_MS 1000

/* Used to restore gpu idle hysteresis after the test */
static u32 gpu_idle_hysteresis_ns;

/* Disables doorbell mirror interrupts so the scheduler won't be awaken by them. */
static int disable_db_mirror_interrupt(struct kbase_device *const kbdev)
{
	unsigned long flags;
	const unsigned int fw_timeout_ms = kbase_get_timeout_ms(kbdev, CSF_FIRMWARE_TIMEOUT);

	/* Wait until doorbell mirror interrupts are disabled. This happens during scheduler wake up
	 * procedure.
	 */
	if (!wait_event_timeout(kbdev->csf.event_wait,
				!kbdev->pm.backend.db_mirror_interrupt_enabled,
				msecs_to_jiffies(kbase_get_timeout_ms(kbdev, fw_timeout_ms)))) {
		return -1;
	}

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	/* This will prevent from enabling doorbell mirror interrupts in the next scheduler sleep
	 * procedure.
	 */
	kbdev->pm.backend.db_mirror_interrupt_enabled = true;
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	return 0;
}

static bool is_tick_processed(unsigned long prev_last_schedule,
			      struct kbase_csf_scheduler *scheduler)
{
	bool ret;

	mutex_lock(&scheduler->lock);
	ret = !(scheduler->last_schedule == prev_last_schedule);
	mutex_unlock(&scheduler->lock);

	return ret;
}

/* Disables gpu idle event by setting its hysteresis time to maximum value.
 * This prevents any unwanted gpu idle events from occurring.
 */
static void disable_gpu_idle_event(struct kbase_device *const kbdev)
{
	unsigned long flags;

	kbase_csf_scheduler_spin_lock(kbdev, &flags);
	gpu_idle_hysteresis_ns = kbdev->csf.gpu_idle_hysteresis_ns;
	kbase_csf_scheduler_spin_unlock(kbdev, flags);

	kbase_csf_firmware_set_gpu_idle_hysteresis_time(kbdev, MAX_HYSTERESIS);
}

/* Restores the previous value of gpu idle hysteresis. */
static void enable_gpu_idle_event(struct kbase_device *const kbdev)
{
	kbase_csf_firmware_set_gpu_idle_hysteresis_time(kbdev, gpu_idle_hysteresis_ns);
}

/**
 * mali_kutf_GPUCORE39614_test_function - Test function that is executed in
 *                                        a kthread
 *
 * @context: pointer to the @p struct kutf_context object
 *
 * This is the kernel counterpart for the associated userspace test. Refer to
 * GPUCORE39614_test_func in ../test_runner/GPUCORE-39614.c for a more precise overview
 * of the race condition.
 *
 * Kernel flow:
 * 1. Disable GPU idle event to explicitly control them from kernel. Any unwanted GPU idle event
 *		breaks the race simulation.
 * 2. Put scheduler to sleep by triggering GPU idle worker.
 * 3. Once userspace signals that the scheduler is asleep, the processing of tick scheduling is
 *		disabled.
 * 4. Wait until userspace signals CQS_WAIT.
 * 5. Disable doorbell mirror interrupts while scheduler is being waken up by
 *		check_group_sync_update_worker. These interrupts are not part of the tested race
 *		condition. They can wake up the scheduler, thus the need of disabling them.
 * 6. Trigger gpu_idle_worker without modifying scheduler->gpu_no_longer_idle (this flag is modified
 *		by check_group_sync_update_worker). This simulates a situation where gpu_idle_worker
 *		waits to acquire the scheduler lock after check_group_sync_update_worker.
 * 7. Once gpu_idle_worker ends, enable tick processing.
 * 8. Check if scheduling was performed (success) or skipped (fail).
 *
 * The synchronization between userspace and kernelspace is done via the KUTF data mechanism.
 */
static void mali_kutf_GPUCORE39614_test_function(struct kutf_context *context)
{
	struct kutf_kernel_defect_fixture_data *data = context->fixture;
	struct kbase_device *const kbdev = data->kbdev;
	struct kbase_csf_scheduler *scheduler;
	unsigned long prev_last_schedule;
	unsigned long flags;

	int err = 0;
	struct kutf_helper_named_val val;

	if (!kbdev) {
		kutf_test_fail(context, "Unexpected NULL kbdev");
		notify_user_val(context, "GPUCORE39614_KERNEL_FAIL", 0);
		return;
	}

	if (!kbase_hw_has_feature(kbdev, BASE_HW_FEATURE_GPU_SLEEP)) {
		kutf_test_skip_msg(
			context,
			"GPUCORE-39614 defect test only available for CSF GPUs with sleep feature");
		return;
	}

	scheduler = &kbdev->csf.scheduler;

	err = kutf_helper_receive_check_val(&val, context,
					    "GPUCORE39614_USERSPACE_DISABLE_GPU_IDLE_EVENT",
					    KUTF_HELPER_VALTYPE_U64);
	if (err || val.type == KUTF_HELPER_VALTYPE_INVALID) {
		kutf_test_fail(context,
			       "Userspace failed to send 'disable gpu idle event' request");
		if (err)
			notify_user_val(context, "GPUCORE39614_KERNEL_FAIL", 0);

		return;
	}

	/* GPU idle worker will only be triggered explicitly in the test. */
	disable_gpu_idle_event(kbdev);

	kutf_helper_send_named_u64(context, "GPUCORE39614_KERNEL_GPU_IDLE_EVENT_OFF", 1);

	err = kutf_helper_receive_check_val(&val, context, "GPUCORE39614_USERSPACE_SCHEDULER_SLEEP",
					    KUTF_HELPER_VALTYPE_U64);
	if (err || val.type == KUTF_HELPER_VALTYPE_INVALID) {
		kutf_test_fail(context, "Userspace failed to send 'scheduler sleep' request");
		if (err)
			notify_user_val(context, "GPUCORE39614_KERNEL_FAIL", 0);

		goto cleanup;
	}

	/* Trigger GPU idle worker to put scheduler to sleep. */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	spin_lock(&scheduler->interrupt_lock);

	scheduler->fast_gpu_idle_handling = true;
	atomic_set(&scheduler->gpu_no_longer_idle, false);
	queue_work(scheduler->idle_wq, &scheduler->gpu_idle_work);

	spin_unlock(&scheduler->interrupt_lock);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	/* Wait for GPU idle worker to complete. */
	flush_workqueue(scheduler->idle_wq);

	if (scheduler->state != SCHED_SLEEPING) {
		kutf_test_fail(context, "Scheduler is not sleeping");
		notify_user_val(context, "GPUCORE39614_KERNEL_FAIL", 0);
		goto cleanup;
	}

	if (kbase_pm_wait_for_desired_state(kbdev)) {
		kutf_test_fail(context, "Timed out waiting for PM desired state");
		notify_user_val(context, "GPUCORE39614_KERNEL_FAIL", 0);
		goto cleanup;
	}
	/* Pre-conditions of the race are achieved at this point. The race can start. */

	/* Disable tick processing by setting pending_tick_work to invalid value. */
	atomic_set(&scheduler->pending_tick_work, -1);

	/* Userspace should now trigger check_group_sync_update_worker. */
	kutf_helper_send_named_u64(context, "GPUCORE39614_USERSPACE_TICK_PROCESSING_DISABLED", 1);

	err = kutf_helper_receive_check_val(&val, context,
					    "GPUCORE39614_USERSPACE_TRIGGER_GPU_IDLE_WORKER",
					    KUTF_HELPER_VALTYPE_U64);
	if (err || val.type == KUTF_HELPER_VALTYPE_INVALID) {
		kutf_test_fail(context, "Userspace failed to send 'GPU idle worker' request");
		atomic_set(&scheduler->pending_tick_work, false);
		if (err)
			notify_user_val(context, "GPUCORE39614_KERNEL_FAIL", 0);

		goto cleanup;
	}

	/* Doorbell mirror interrupts are not part of the tested race condition. */
	if (disable_db_mirror_interrupt(kbdev)) {
		kutf_test_fail(context, "Failed to disable doorbell mirror interrupt");
		atomic_set(&scheduler->pending_tick_work, false);
		notify_user_val(context, "GPUCORE39614_KERNEL_FAIL", 0);
		goto cleanup;
	}

	/* Simulate a GPU idle worker that acquires the scheduler lock after
	 * check_group_sync_update_worker.
	 */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	spin_lock(&scheduler->interrupt_lock);

	scheduler->fast_gpu_idle_handling = true;
	queue_work(scheduler->idle_wq, &scheduler->gpu_idle_work);

	spin_unlock(&scheduler->interrupt_lock);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	/* Wait for GPU idle worker to complete. */
	flush_workqueue(scheduler->idle_wq);

	/* Get last_schedule before enabling the tick processing. */
	prev_last_schedule = scheduler->last_schedule;

	/* Enable tick processing */
	atomic_set(&scheduler->pending_tick_work, true);
	/* Signal the scheduler thread to process the pending tick. */
	complete(&scheduler->kthread_signal);

	/* Wait for scheduler to process the tick. */
	if (!wait_event_timeout(kbdev->csf.event_wait,
				(atomic_read(&scheduler->pending_tick_work) == false) &&
					is_tick_processed(prev_last_schedule, scheduler),
				msecs_to_jiffies(TICK_PROCESSING_TIMEOUT_MS))) {
		kutf_test_fail(context, "Timed out waiting for tick processing");
	}

	kutf_helper_send_named_u64(context, "GPUCORE39614_KERNEL_FINISH", 1);

cleanup:
	if (scheduler->state == SCHED_SLEEPING)
		kbase_csf_scheduler_force_wakeup(kbdev);

	enable_gpu_idle_event(kbdev);
}
#endif /* MALI_USE_CSF && !IS_ENABLED(CONFIG_MALI_NO_MALI) */

/**
 * mali_kutf_kernel_defect_GPUCORE_39614 - Entry point for the test
 * @context: KUTF context
 *
 */
void mali_kutf_kernel_defect_GPUCORE_39614(struct kutf_context *context)
{
#if MALI_USE_CSF && !IS_ENABLED(CONFIG_MALI_NO_MALI)
	mali_kutf_GPUCORE39614_test_function(context);
#else /* MALI_USE_CSF && !IS_ENABLED(CONFIG_MALI_NO_MALI) */
	kutf_test_skip_msg(context, "The GPUCORE_39614 test is only applicable to CSF GPUs");
#endif /* MALI_USE_CSF && !IS_ENABLED(CONFIG_MALI_NO_MALI) */
}
