// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2021-2023 ARM Limited. All rights reserved.
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

#include <linux/delay.h>
#include <device/mali_kbase_device.h>
#include <mali_kbase_reset_gpu.h>
#include <kutf/kutf_helpers_user.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include "mali_kutf_kernel_power_test_main.h"

#if MALI_USE_CSF && !IS_ENABLED(CONFIG_MALI_NO_MALI)
/**
 * mali_kutf_test_kernel_power_core_mask_gpu_active - checks to see if the core
 * mask gets updated through the devfreq function when the GPU is active
 * @context:    kutf context within which to perform the test
 *
 * Expected result: runs to completion
 */
void mali_kutf_test_kernel_power_core_mask_gpu_active(struct kutf_context *context)
{
#ifndef CONFIG_MALI_DEVFREQ
	kutf_test_skip_msg(context, "Test is not supported as devfreq is not enabled");
#else
	struct kutf_kernel_power_fixture_data *data = context->fixture;
	struct kbase_device *kbdev;
	struct kbase_pm_backend_data *backend;
	uint64_t new_shaders_avail;
	int max_numbered_core;
	uint64_t target_mask;
	unsigned long flags;

	if (!data->kctx) {
		kutf_test_fail(context,
			       "Unexpected NULL kctx in kernel_power_core_mask_gpu_active test");
	}

	kbdev = data->kbdev;
	backend = &kbdev->pm.backend;

	if (is_power_of_2(kbdev->gpu_props.shader_present)) {
		kutf_test_skip_msg(context,
				   "Test is not supported as there is only a single core present");
		return;
	}

	kbase_csf_scheduler_pm_active(kbdev);
	kbase_csf_scheduler_wait_mcu_active(kbdev);

	max_numbered_core = fls64(kbdev->gpu_props.shader_present) - 1;
	target_mask = 1ULL << max_numbered_core;

	/* It is assumed here that whilst this test is running, there will be
	 * no core mask change request from the actual DVFS side and there
	 * will be no other GPU activity happening.
	 */
	kbase_devfreq_set_core_mask(kbdev, target_mask);
	/* Wait enough for the state machine to handle the core mask change */
	msleep(100);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	new_shaders_avail = backend->shaders_avail;
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	kbase_csf_scheduler_pm_idle(kbdev);

	if (new_shaders_avail != target_mask) {
		kutf_test_fail(context, "Core mask is not the same as target mask");
	}
#endif /* CONFIG_MALI_DEVFREQ */
}

/**
 * mali_kutf_test_kernel_power_core_mask_gpu_idle - checks to see if the core
 * mask gets updated through the devfreq function when the GPU is idle
 * @context:    kutf context within which to perform the test
 *
 * Expected result: runs to completion
 *
 */
void mali_kutf_test_kernel_power_core_mask_gpu_idle(struct kutf_context *context)
{
#ifndef CONFIG_MALI_DEVFREQ
	kutf_test_skip_msg(context, "Test is not supported as devfreq is not enabled");
#else
	struct kutf_kernel_power_fixture_data *data = context->fixture;
	struct kbase_context *kctx = data->kctx;
	struct kbase_device *kbdev;
	struct kbase_pm_backend_data *backend;
	uint64_t orig_shaders_avail, new_shaders_avail;
	int max_numbered_core;
	uint64_t target_mask;
	unsigned long flags;

	if (!kctx) {
		kutf_test_fail(context,
			       "Unexpected NULL kctx in kernel_power_core_mask_gpu_active test");
	}
	kbdev = kctx->kbdev;
	backend = &kbdev->pm.backend;

	if (is_power_of_2(kbdev->gpu_props.shader_present)) {
		kutf_test_skip_msg(context,
				   "Test is not supported as there is only a single core present");
		return;
	}

	if (backend->pm_current_policy == &kbase_pm_always_on_policy_ops) {
		kutf_test_skip_msg(context,
				   "Test is not supported as always on power policy is in use");
		return;
	}

	max_numbered_core = fls64(kbdev->gpu_props.shader_present) - 1;
	target_mask = 1ULL << max_numbered_core;

	orig_shaders_avail = backend->shaders_avail;

	/* It is assumed here that whilst this test is running, there will be
	 * no core mask change request from the actual DVFS side and there
	 * will be no other GPU activity happening.
	 */
	kbase_devfreq_set_core_mask(kbdev, target_mask);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	new_shaders_avail = backend->shaders_avail;
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	if (new_shaders_avail != orig_shaders_avail) {
		kutf_test_fail(context,
			       "Core mask different from original mask even when GPU is idle");
	}

	kbase_csf_scheduler_pm_active(kbdev);
	kbase_csf_scheduler_wait_mcu_active(kbdev);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	new_shaders_avail = backend->shaders_avail;
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	kbase_csf_scheduler_pm_idle(kbdev);

	if (new_shaders_avail != target_mask) {
		kutf_test_fail(context,
			       "core mask not same as target mask after activating the GPU");
	}
#endif /* CONFIG_MALI_DEVFREQ */
}

/**
 * mali_kutf_test_kernel_power_core_mask_gpu_reset - checks that the core mask
 * gets updated through devfreq after triggering a gpu reset
 * @context:    kutf context within which to perform the test
 *
 * expected result: test runs to completion
 *
 */
void mali_kutf_test_kernel_power_core_mask_gpu_reset(struct kutf_context *context)
{
#ifndef CONFIG_MALI_DEVFREQ
	kutf_test_skip_msg(context, "Test is not supported as devfreq is not enabled");
#else
	struct kutf_kernel_power_fixture_data *data = context->fixture;
	struct kbase_context *kctx = data->kctx;
	struct kbase_device *kbdev;
	struct kbase_pm_backend_data *backend;
	uint64_t new_shaders_avail;
	int max_numbered_core;
	uint64_t target_mask;
	unsigned long flags;

	if (!kctx) {
		kutf_test_fail(context, "Unexpected NULL kctx in power_core_mask_gpu_reset test");
	}

	kbdev = data->kbdev;
	backend = &kbdev->pm.backend;

	if (is_power_of_2(kbdev->gpu_props.shader_present)) {
		kutf_test_skip_msg(context,
				   "Test is not supported as there is only a single core present");
		return;
	}

	max_numbered_core = fls64(kbdev->gpu_props.shader_present) - 1;
	target_mask = 1ULL << max_numbered_core;

	if (kbase_prepare_to_reset_gpu(kctx->kbdev, RESET_FLAGS_NONE)) {
		kbase_reset_gpu(kbdev);

		/* It is assumed here that whilst this test is running, there
		 * will be no core mask change request from the actual DVFS
		 * side and there will be no other GPU activity happening.
		 */
		kbase_devfreq_set_core_mask(kbdev, target_mask);

		kbase_reset_gpu_wait(kbdev);
	} else {
		kutf_test_fail(context, "GPU reset already in progress unexpectedly");
	}

	kbase_csf_scheduler_pm_active(kbdev);
	kbase_csf_scheduler_wait_mcu_active(kbdev);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	new_shaders_avail = backend->shaders_avail;
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	kbase_csf_scheduler_pm_idle(kbdev);

	if (new_shaders_avail != target_mask) {
		kutf_test_fail(context, "core mask not same as target mask");
	}
#endif /* CONFIG_MALI_DEVFREQ */
}

/**
 * mali_kutf_test_kernel_power_core_mask_no_intersect - ensures that the core
 * mask value set by the devfreq function doesn't intersect with the debug core
 * mask value
 * @context:    kutf context within which to perform the test
 *
 * expected result: test runs to completion
 *
 */
void mali_kutf_test_kernel_power_core_mask_no_intersect(struct kutf_context *context)
{
#ifndef CONFIG_MALI_DEVFREQ
	kutf_test_skip_msg(context, "Test is not supported as devfreq is not enabled");
#else
	struct kutf_kernel_power_fixture_data *data = context->fixture;
	struct kbase_device *kbdev;
	struct kbase_pm_backend_data *backend;
	unsigned long min_numbered_core;
	int max_numbered_core;
	uint64_t target_mask;
	unsigned long flags;

	if (!data->kctx) {
		kutf_test_fail(context,
			       "Unexpected NULL kctx in kernel_power_core_mask_gpu_active test");
		return;
	}
	kbdev = data->kbdev;
	backend = &kbdev->pm.backend;

	if (is_power_of_2(kbdev->gpu_props.shader_present)) {
		kutf_test_skip_msg(context,
				   "Test is not supported as there is only a single core present");
		return;
	}

	max_numbered_core = fls64(kbdev->gpu_props.shader_present) - 1;
	target_mask = 1ULL << max_numbered_core;

	mutex_lock(&kbdev->pm.lock);
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_pm_set_debug_core_mask(kbdev, target_mask);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	mutex_unlock(&kbdev->pm.lock);

	min_numbered_core = __ffs64(kbdev->gpu_props.shader_present);
	target_mask = 1ULL << min_numbered_core;

	/* It is assumed here that whilst this test is running, there will be
	 * no core mask change request from the actual DVFS side and there
	 * will be no other GPU activity happening.
	 */
	kbase_devfreq_set_core_mask(kbdev, target_mask);

	if (backend->ca_cores_enabled == target_mask) {
		kutf_test_fail(
			context,
			"core mask set by devfreq was successful even though it did not interect with debug mask");
	}
#endif /* CONFIG_MALI_DEVFREQ */
}
#endif /* MALI_USE_CSF && !IS_ENABLED(CONFIG_MALI_NO_MALI) */
