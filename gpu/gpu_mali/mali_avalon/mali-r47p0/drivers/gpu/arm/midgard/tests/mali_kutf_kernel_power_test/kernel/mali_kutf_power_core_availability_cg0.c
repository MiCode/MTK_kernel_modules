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

#include <device/mali_kbase_device.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include "mali_kutf_kernel_power_helpers.h"
#include "mali_kutf_kernel_power_test_main.h"

/**
 * mali_kutf_test_kernel_power_core_availability_cg0 - requests and releases all
 * cores and test core powered status for core group 0.
 * @context:    kutf context within which to perform the test
 *
 * Expected result: runs to completion
 *
 */
void mali_kutf_test_kernel_power_core_availability_cg0(struct kutf_context *context)
{
#ifdef CONFIG_ANDROID
	kutf_test_skip_msg(context, "power_core_availability_cg0 is not supported on Android");
#else
	struct kutf_kernel_power_fixture_data *data = context->fixture;
	struct kbase_context *kctx;
	uint64_t cg0_mask, ready_mask;
	struct kbase_pm_policy const *current_policy;

	if (!data->kctx) {
		kutf_test_fail(context, "Unexpected NULL kctx in kernel_power_cycle test");
		return;
	}

	kctx = data->kctx;

	/* We need a policy which won't keep the cores on unconditionally */
	current_policy = kbase_pm_get_policy(kctx->kbdev);
	if (!mali_kutf_test_set_power_policy_by_name(kctx->kbdev, "always_on_demand")) {
		kutf_test_fail(context, "Could not set power policy");
		return;
	}

	cg0_mask = kctx->kbdev->gpu_props.coherency_info.group.core_mask;
	kbase_pm_wait_for_desired_state(kctx->kbdev);

	ready_mask = kbase_reg_read64(kctx->kbdev, GPU_CONTROL_ENUM(SHADER_READY));

	/* Core group 0 must now be powered */
	if (!(ready_mask & cg0_mask))
		kutf_test_fail(context, "Core group 0 is not powered during power on test.");

	kbase_pm_set_policy(kctx->kbdev, current_policy);
#endif /* CONFIG_ANDROID */
}
