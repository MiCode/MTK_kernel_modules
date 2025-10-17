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
#include "mali_kutf_kernel_power_helpers.h"
#include <device/mali_kbase_device.h>
#include <mali_kbase_hwaccess_pm.h>
#include <mali_kbase.h>

bool mali_kutf_test_set_power_policy_by_name(struct kbase_device *kbdev, char const *policy)
{
	struct kbase_pm_policy const *desired_policy = NULL;
	struct kbase_pm_policy const *const *policy_list;
	size_t policy_count;
	size_t i;

	policy_count = kbase_pm_list_policies(kbdev, &policy_list);
	for (i = 0; i != policy_count; ++i) {
		if (!strcmp(policy_list[i]->name, policy))
			desired_policy = policy_list[i];
	}
	if (!desired_policy) {
		pr_err("Required power policy %s is unavailable", policy);
		return false;
	}

	kbase_pm_set_policy(kbdev, desired_policy);

	return true;
}
