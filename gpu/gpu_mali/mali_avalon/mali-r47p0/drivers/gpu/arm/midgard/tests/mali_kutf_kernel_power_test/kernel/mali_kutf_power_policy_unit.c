// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2021-2022 ARM Limited. All rights reserved.
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

#include <linux/errno.h>
#include <linux/printk.h>
#include <mali_kbase.h>
#include <kutf/kutf_helpers_user.h>
#include <backend/gpu/mali_kbase_pm_defs.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include "mali_kutf_kernel_power_test_main.h"

#ifndef CONFIG_ANDROID

/*
 * Set of PM policy states
 */
#define KBASE_PM_POLICY_STATE_IDLE (0x01)
#define KBASE_PM_POLICY_STATE_ACTIVE (0x01 << 1)

#endif /* CONFIG_ANDROID */

typedef uint8_t kbase_pm_policy_state;

/**
 * struct kbase_policy_info - Set of data specifying expected behavior of each policy
 * @id: An ID of the policy.
 * @active_state_mask: Bit mask of policy states when GPU is expected to be powered.
 * @shaders_needed_mask: Bit mask of policy states when shaders_needed is expected to be true.
 */
struct kbase_policy_info {
	enum kbase_pm_policy_id id;
	uint32_t active_state_mask;
	uint32_t shaders_needed_mask;
};

#ifndef CONFIG_ANDROID
/* An array of set of descriptors for each power policy */
static const struct kbase_policy_info kbase_policy_data_set[] = {
	{ .id = KBASE_PM_POLICY_ID_ALWAYS_ON,
	  .active_state_mask = KBASE_PM_POLICY_STATE_IDLE | KBASE_PM_POLICY_STATE_ACTIVE,
	  .shaders_needed_mask = KBASE_PM_POLICY_STATE_IDLE | KBASE_PM_POLICY_STATE_ACTIVE },

	{ .id = KBASE_PM_POLICY_ID_COARSE_DEMAND,
	  .active_state_mask = KBASE_PM_POLICY_STATE_ACTIVE,
	  .shaders_needed_mask = KBASE_PM_POLICY_STATE_ACTIVE },

#if !MALI_CUSTOMER_RELEASE
	{ .id = KBASE_PM_POLICY_ID_ALWAYS_ON_DEMAND,
	  .active_state_mask = KBASE_PM_POLICY_STATE_IDLE | KBASE_PM_POLICY_STATE_ACTIVE,
	  .shaders_needed_mask = KBASE_PM_POLICY_STATE_ACTIVE },
#endif
};

/**
 * mali_kernel_power_policy_get_policy_info - Get the information of
 *                                            expected behavior for given policy
 *
 * @policy_id: Policy id to get the information for
 *
 * Return: Pointer to the information on success, NULL otherwise.
 */
static const struct kbase_policy_info *
mali_kernel_power_policy_get_policy_info(enum kbase_pm_policy_id policy_id)
{
	switch (policy_id) {
	case KBASE_PM_POLICY_ID_ALWAYS_ON:
		return &kbase_policy_data_set[0];

	case KBASE_PM_POLICY_ID_COARSE_DEMAND:
		return &kbase_policy_data_set[1];

#if !MALI_CUSTOMER_RELEASE
	case KBASE_PM_POLICY_ID_ALWAYS_ON_DEMAND:
		return &kbase_policy_data_set[2];
#endif
	default:
		pr_warn("WARN: Unidentified policy id: %d ", policy_id);
		return NULL;
	}
}

/**
 * mali_kernel_power_policy_verify_core - Verify that get_core_mask and
 * get_core_active functions for given power policy provide results as expected
 *
 * @kbdev:        Kbase device pointer
 * @pm_policy:    PM policy
 * @policy_state: Expected policy state
 *
 * Return: Zero on success, Linux error code on failure
 */
static int mali_kernel_power_policy_verify_core(struct kbase_device *kbdev,
						const struct kbase_pm_policy *pm_policy,
						kbase_pm_policy_state policy_state)
{
	bool current_gpu_state, expected_gpu_state;
	bool current_cores_state, expected_cores_state;

	/* Get the policy info */
	const struct kbase_policy_info *const policy_info =
		mali_kernel_power_policy_get_policy_info(pm_policy->id);
	if (!policy_info) {
		pr_err("Failed to find policy %s\n", pm_policy->name);
		return -EINVAL;
	}

	if (policy_info->id != pm_policy->id) {
		pr_err("Found policy doesn't match %i vs. %i\n", policy_info->id, pm_policy->id);
		return -EINVAL;
	}

	current_gpu_state = pm_policy->get_core_active(kbdev);
	expected_gpu_state = !!(policy_info->active_state_mask & policy_state);
	if (current_gpu_state != expected_gpu_state) {
		pr_err("%s: GPU state (%u) doesn't match what is expected (%u)\n", pm_policy->name,
		       current_gpu_state, expected_gpu_state);
		return -EINVAL;
	}

	current_cores_state = pm_policy->shaders_needed(kbdev);
	expected_cores_state = !!(policy_info->shaders_needed_mask & policy_state);
	if (current_cores_state != expected_cores_state) {
		pr_err("%s: Cores state (%u) doesn't match what is expected (%u)\n",
		       pm_policy->name, current_cores_state, expected_cores_state);
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_ANDROID */

/**
 * mali_kutf_test_kernel_power_policy() - Tests power policies and shaders_needed
 * and get_core_active functions
 * @context:    kutf context within which to perform the test
 *
 * Select each power policy in turn, move through states of idle, active,
 * cores requested, cores released, and idle. Test that shaders_needed and
 * get_core_active return expected results for each policy.
 *
 * Expected result: runs to completion
 *
 */
void mali_kutf_test_kernel_power_policy(struct kutf_context *context)
{
#ifdef CONFIG_ANDROID
	kutf_test_skip_msg(context, "kernel_power_cycle is not applicable to Android");
#else
	struct kutf_kernel_power_fixture_data *data = context->fixture;
	struct kbase_context *kctx = data->kctx;
	const struct kbase_pm_policy *current_policy;
	const struct kbase_pm_policy *const *policy_list;
	int policy_count;
	int n;

	if (!kctx) {
		kutf_test_fail(context, "Unexpected NULL kctx in kernel_power_cycle test");
		return;
	}

	/* Get current policy */
	current_policy = kbase_pm_get_policy(kctx->kbdev);
	policy_count = kbase_pm_list_policies(kctx->kbdev, &policy_list);

	/* Go through all supported power policies*/
	for (n = 0; n < policy_count; ++n) {
		const struct kbase_pm_policy *pm_policy = policy_list[n];

		/* Set the policy */
		kbase_pm_set_policy(kctx->kbdev, pm_policy);

		if (kbase_pm_get_policy(kctx->kbdev)->id != pm_policy->id) {
			kutf_test_fail(
				context,
				"Could not set the power policy in test mali_kernel_power_power_policy_unit");

		} else {
			int status = 0;

			/* Go through the different states*/
			status = mali_kernel_power_policy_verify_core(kctx->kbdev, pm_policy,
								      KBASE_PM_POLICY_STATE_IDLE);

			if (status == 0) {
				kbase_csf_scheduler_pm_active(kctx->kbdev);
				status = mali_kernel_power_policy_verify_core(
					kctx->kbdev, pm_policy, KBASE_PM_POLICY_STATE_ACTIVE);

				kbase_csf_scheduler_pm_idle(kctx->kbdev);
				if (status == 0)
					status = mali_kernel_power_policy_verify_core(
						kctx->kbdev, pm_policy, KBASE_PM_POLICY_STATE_IDLE);
			}

			if (status != 0)
				kutf_test_fail(context, "Could not set the power policy to idle");
		}
	}

	/* Restore the original power and core availability policies */
	kbase_pm_set_policy(kctx->kbdev, current_policy);
#endif /* CONFIG_ANDROID */
}
