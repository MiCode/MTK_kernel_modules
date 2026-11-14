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

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <mali_kbase.h>
#include <device/mali_kbase_device.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <kutf/kutf_helpers_user.h>

#include "mali_kutf_kernel_power_test_main.h"
#include "mali_kutf_kernel_power_helpers.h"

#ifndef CONFIG_ANDROID
static uint64_t read_gpu_control_reg(struct kbase_context *kctx, uint16_t offset)
{
	return kbase_reg_read64(kctx->kbdev, offset);
}
#endif

void mali_kutf_test_kernel_power_cycle(struct kutf_context *context)
{
#ifdef CONFIG_ANDROID
	kutf_test_skip_msg(context, "kernel_power_cycle is not supported on Android");
#else
	struct kutf_kernel_power_fixture_data *data = context->fixture;
	struct kbase_context *kctx = data->kctx;
	struct kbase_pm_policy const *current_policy;
	uint64_t shader_present_bitmap;
	uint64_t shader_ready;
	uint64_t stack_present_bitmap = 0;
	uint64_t stack_ready = 0;
	bool correct_power_state;

	if (!kctx) {
		kutf_test_fail(context, "Unexpected NULL kctx in kernel_power_cycle test");
		return;
	}

	current_policy = kbase_pm_get_policy(kctx->kbdev);

	if (!mali_kutf_test_set_power_policy_by_name(kctx->kbdev, "always_on_demand")) {
		kutf_test_fail(context, "Could not set power policy in kernel_power_cycle test");
		return;
	}

	shader_present_bitmap = kctx->kbdev->gpu_props.shader_present;
	if (corestack_driver_control)
		stack_present_bitmap = kctx->kbdev->gpu_props.stack_present;

	/* Core group 0: All cores must now be in powered state */
	shader_ready = read_gpu_control_reg(kctx, GPU_CONTROL_ENUM(SHADER_READY));
	correct_power_state = (shader_ready & shader_present_bitmap) != 0;
	if (corestack_driver_control) {
		/* All core stacks must also have been powered on */
		stack_ready = read_gpu_control_reg(kctx, GPU_CONTROL_ENUM(STACK_READY));
		correct_power_state = correct_power_state &&
				      ((stack_ready & stack_present_bitmap) != 0);
	}
	if (!correct_power_state) {
		kutf_test_fail(context,
			       "Cores/core stacks mis-match between present and ready mask.");
		return;
	}

	/* Core group 0: Power off all cores test */
	/* The basic assumption for the CSF case is that there is a
     * queue running with a long lasting loop job. This is arranged
     * with the test application on the user-side. The corresponding
     * action for triggering a power down case on shader cores thus
     * needs a scheduler pm suspend.
     */
	kbase_csf_scheduler_pm_suspend(kctx->kbdev);
	kbase_pm_wait_for_desired_state(kctx->kbdev);

	/* Core group 0: All cores must now be in power off state */
	shader_ready = read_gpu_control_reg(kctx, GPU_CONTROL_ENUM(SHADER_READY));
	correct_power_state = !(shader_ready & shader_present_bitmap);
	if (corestack_driver_control) {
		/* All core stacks must also have been powered off */
		stack_ready = read_gpu_control_reg(kctx, GPU_CONTROL_ENUM(STACK_READY));
		correct_power_state &= !(stack_ready & stack_present_bitmap);
	}
	if (!correct_power_state) {
		kutf_test_fail(context,
			       "Cores/core stacks still in powered state during power off test.");
	}

	kbase_pm_set_policy(kctx->kbdev, current_policy);
	/* CSF scheduler pm resumes to restore the CSG running state */
	kbase_csf_scheduler_pm_resume(kctx->kbdev);
#endif /* CONFIG_ANDROID */
}

/**
 * mali_kutf_test_kernel_power_cycle_whilst_wait_enqueued - cycles power on and
 * off whilst a wait is enqueued to ensure that the GPU doesn't get left
 * active when it shouldn't.
 * @context:    kutf context within which to perform the test
 *
 * Expected result: test runs to completion
 */
void mali_kutf_test_kernel_power_cycle_whilst_wait_enqueued(struct kutf_context *context)
{
#ifdef CONFIG_ANDROID
	kutf_test_skip_msg(context, "kernel_power_cycle is not supported on Android");
#else
	struct kutf_kernel_power_fixture_data *data = context->fixture;
	struct kbase_context *kctx = data->kctx;
	uint64_t shader_present_bitmap;
	uint64_t shader_ready;
	uint64_t stack_present_bitmap = 0;
	uint64_t stack_ready = 0;
	int max_attempts = 5;
	bool correct_power_state;
	struct kbase_pm_policy const *current_policy;
	uint64_t l2_ready = 0x1, l2_trans = 0x1;
	int i;

	if (!kctx) {
		kutf_test_fail(context, "Unexpected NULL kctx in kernel_power_cycle test");
		return;
	}

	/* We need a policy which won't keep the cores on unconditionally */
	current_policy = kbase_pm_get_policy(kctx->kbdev);
	if (!mali_kutf_test_set_power_policy_by_name(kctx->kbdev, "coarse_demand"))
		kutf_test_fail(context, "Could not set coarse_demand policy in kernel_power_cycle");

	shader_present_bitmap = kctx->kbdev->gpu_props.shader_present;
	if (corestack_driver_control)
		stack_present_bitmap = kctx->kbdev->gpu_props.stack_present;

	for (i = 0; i < max_attempts; i++) {
		unsigned long flags;

		/* First ensure the GPU is active, skipping with CSF case
	     * as the powering up is assumed from its user-side test
	     * application via a long lasting running loop-job.
	     */
		/* Core group 0: All cores must now be in powered state */
		shader_ready = read_gpu_control_reg(kctx, GPU_CONTROL_ENUM(SHADER_READY));
		correct_power_state = (shader_ready & shader_present_bitmap) != 0;
		if (corestack_driver_control) {
			/* All core stacks must also have been powered on */
			stack_ready = read_gpu_control_reg(kctx, GPU_CONTROL_ENUM(STACK_READY));
			correct_power_state = correct_power_state &&
					      ((stack_ready & stack_present_bitmap) != 0);
		}
		if (!correct_power_state) {
			kutf_test_fail(
				context,
				"Cores/core stacks mis-match between present and ready mask");
		}

		/* Now idle the GPU */
		/* The basic assumption for the CSF case is that there is a
	     * queue running with a long lasting loop-job. This is arranged
	     * with the test application on the user-side. The
	     * corresponding action for triggering a power down case on
	     * shader cores thus needs a scheduler pm suspend.
	     */
		kbase_csf_scheduler_pm_suspend(kctx->kbdev);

		/* Wait for the L2 state machine to turn off L2, as after that only the
		 * gpu_poweroff_wait_work is enqueued inside gpu_poweroff_wait_wq.
		 * This could help provoke the problem more reliably.
		 */
		spin_lock_irqsave(&kctx->kbdev->hwaccess_lock, flags);
		while (kctx->kbdev->pm.backend.l2_state != KBASE_L2_OFF) {
			spin_unlock_irqrestore(&kctx->kbdev->hwaccess_lock, flags);
			udelay(100);
			spin_lock_irqsave(&kctx->kbdev->hwaccess_lock, flags);
		}

		spin_unlock_irqrestore(&kctx->kbdev->hwaccess_lock, flags);

		/* back-to-back active and idle */
		kbase_csf_scheduler_pm_resume(kctx->kbdev);
		kbase_csf_scheduler_pm_suspend(kctx->kbdev);

		/**
	     * This sleep seems to make the test more likely to fail, if the driver
	     * bug is present
	     */
		msleep(200);

		mutex_lock(&kctx->kbdev->pm.lock);
		spin_lock_irqsave(&kctx->kbdev->hwaccess_lock, flags);
		if (kctx->kbdev->pm.backend.gpu_powered) {
			/* Don't forget, only access whilst they're powered */
			l2_ready = read_gpu_control_reg(kctx, GPU_CONTROL_ENUM(L2_READY));
			l2_trans = read_gpu_control_reg(kctx, GPU_CONTROL_ENUM(L2_PWRTRANS));
			pr_warn("l2_ready = 0x%llx, trans=0x%llx\n", (unsigned long long)l2_ready,
				(unsigned long long)l2_trans);
		}
		spin_unlock_irqrestore(&kctx->kbdev->hwaccess_lock, flags);
		mutex_unlock(&kctx->kbdev->pm.lock);

		/* Counter-step to the earlier scheduler pm suspend, this
	     * resumes the user flushed queue-group into running with
	     * the test arrangement. A wait is required for the cores
	     * to settle down as its managed by the MCU.
	     */
		kbase_csf_scheduler_pm_resume(kctx->kbdev);
		msleep(3000);
		if (l2_ready && !l2_trans) {
			/* Fail the test */
			kutf_test_fail(context, "L2 is ready and not transitioning");
		}
	}

	kbase_pm_set_policy(kctx->kbdev, current_policy);
#endif /* CONFIG_ANDROID */
}
