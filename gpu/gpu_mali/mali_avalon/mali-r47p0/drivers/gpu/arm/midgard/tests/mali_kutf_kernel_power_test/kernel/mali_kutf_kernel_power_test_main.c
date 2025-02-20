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

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/compat.h>
#include <linux/version.h>
#include <kutf/kutf_suite.h>
#include <kutf/kutf_utils.h>
#include <kutf/kutf_helpers_user.h>
#include "../mali_kutf_kernel_power_test.h"
#include "mali_kutf_kernel_power_test_main.h"

static struct kutf_application *kutf_app;

#define MINOR_FOR_FIRST_KBASE_DEV (-1)

static void *mali_kutf_kernel_power_test_create_fixture(struct kutf_context *context)
{
	struct kutf_kernel_power_fixture_data *data = kutf_mempool_alloc(
		&context->fixture_pool, sizeof(struct kutf_kernel_power_fixture_data));
	if (!data) {
		kutf_test_fail(context, "Failed to allocate fixture data memory");
		return NULL;
	}
	*data = (struct kutf_kernel_power_fixture_data){ NULL, NULL };

	data->kbdev = kbase_find_device(MINOR_FOR_FIRST_KBASE_DEV);
	if (data->kbdev == NULL) {
		kutf_test_fail(context, "Failed to find kbase device");
		return NULL;
	}

	data->kctx = kbase_create_context(data->kbdev, in_compat_syscall(),
					  BASE_CONTEXT_SYSTEM_MONITOR_SUBMIT_DISABLED, 0, NULL);

	if (!data->kctx)
		kbase_release_device(data->kbdev);

	return data;
}

static void mali_kutf_kernel_power_test_remove_fixture(struct kutf_context *context)
{
	struct kutf_kernel_power_fixture_data *data = context->fixture;

	if (data->kctx)
		kbase_destroy_context(data->kctx);

	kbase_release_device(data->kbdev);
}

static int __init mali_kutf_kernel_power_test_main_init(void)
{
	struct kutf_suite *suite;
	unsigned int filters;
	union kutf_callback_data suite_data = { NULL };

	pr_debug("KUTF power test creation start\n");

	kutf_app = kutf_create_application(KERNEL_POWER_TEST_APP_NAME);

	if (kutf_app == NULL) {
		pr_warn("Creation of test app " KERNEL_POWER_TEST_APP_NAME " failed\n");
		return -ENOMEM;
	}

	/* Power Tests under a default poweroff policy specified from userspace */
	suite = kutf_create_suite_with_filters_and_data(kutf_app, KERNEL_POWER_TEST_DEFAULT_SUITE,
							1,
							mali_kutf_kernel_power_test_create_fixture,
							mali_kutf_kernel_power_test_remove_fixture,
							KUTF_F_TEST_GENERIC, suite_data);

	if (suite == NULL) {
		pr_warn("Creation of kutf suite for test app" KERNEL_POWER_TEST_DEFAULT_SUITE
			" failed\n");
		kutf_destroy_application(kutf_app);
		return -ENOMEM;
	}

	filters = suite->suite_default_flags;
	kutf_add_test_with_filters(suite, 0x0000, KERNEL_POWER_TEST_POWER_CYCLE,
				   mali_kutf_test_kernel_power_cycle, filters);
	kutf_add_test_with_filters(suite, 0x0001, KERNEL_POWER_TEST_POWER_CYCLE_ENQUEUED,
				   mali_kutf_test_kernel_power_cycle_whilst_wait_enqueued, filters);
	kutf_add_test_with_filters(suite, 0x0002, KERNEL_POWER_TEST_POWER_CORE_AVAILABILITY_CG0,
				   mali_kutf_test_kernel_power_core_availability_cg0, filters);
	kutf_add_test_with_filters(suite, 0x0003, KERNEL_POWER_TEST_POWER_POLICY_UNIT,
				   mali_kutf_test_kernel_power_policy, filters);

	/* Power Tests under an instant poweroff policy specified from userspace */
	suite = kutf_create_suite_with_filters_and_data(kutf_app,
							KERNEL_POWER_TEST_INSTANT_POWEROFF_SUITE, 1,
							mali_kutf_kernel_power_test_create_fixture,
							mali_kutf_kernel_power_test_remove_fixture,
							KUTF_F_TEST_GENERIC, suite_data);

	if (suite == NULL) {
		pr_warn("Creation of kutf suite for test app" KERNEL_POWER_TEST_INSTANT_POWEROFF_SUITE
			" failed\n");
		kutf_destroy_application(kutf_app);
		return -ENOMEM;
	}

	filters = suite->suite_default_flags;
	kutf_add_test_with_filters(suite, 0x0000, KERNEL_POWER_TEST_POWER_CYCLE,
				   mali_kutf_test_kernel_power_cycle, filters);
	kutf_add_test_with_filters(suite, 0x0001, KERNEL_POWER_TEST_POWER_CYCLE_ENQUEUED,
				   mali_kutf_test_kernel_power_cycle_whilst_wait_enqueued, filters);
	kutf_add_test_with_filters(suite, 0x0002, KERNEL_POWER_TEST_POWER_CORE_AVAILABILITY_CG0,
				   mali_kutf_test_kernel_power_core_availability_cg0, filters);
	kutf_add_test_with_filters(suite, 0x0003, KERNEL_POWER_TEST_POWER_POLICY_UNIT,
				   mali_kutf_test_kernel_power_policy, filters);

	/* Power tests involving the Power Core mask, with no userspace component */
	suite = kutf_create_suite_with_filters_and_data(kutf_app,
							KERNEL_POWER_TEST_POWER_CORE_MASK_SUITE, 1,
							mali_kutf_kernel_power_test_create_fixture,
							mali_kutf_kernel_power_test_remove_fixture,
							KUTF_F_TEST_GENERIC, suite_data);

#if MALI_USE_CSF && !IS_ENABLED(CONFIG_MALI_NO_MALI)
	if (suite == NULL) {
		pr_warn("Creation of kutf suite for test app" KERNEL_POWER_TEST_POWER_CORE_MASK_SUITE
			" failed\n");
		kutf_destroy_application(kutf_app);
		return -ENOMEM;
	}

	filters = suite->suite_default_flags;
	kutf_add_test_with_filters(suite, 0x0000, KERNEL_POWER_TEST_ACTIVE_GPU_POWER_CORE,
				   mali_kutf_test_kernel_power_core_mask_gpu_active, filters);
	kutf_add_test_with_filters(suite, 0x0001, KERNEL_POWER_TEST_IDLE_GPU_POWER_CORE,
				   mali_kutf_test_kernel_power_core_mask_gpu_idle, filters);
	kutf_add_test_with_filters(suite, 0x0002, KERNEL_POWER_TEST_GPU_RESET_POWER_CORE,
				   mali_kutf_test_kernel_power_core_mask_gpu_reset, filters);
	kutf_add_test_with_filters(suite, 0x0003, KERNEL_POWER_TEST_NO_INTERSECT_POWER_CORE,
				   mali_kutf_test_kernel_power_core_mask_no_intersect, filters);
#endif
	pr_debug("KUTF power test creation end\n");
	return 0;
}

static void __exit mali_kutf_kernel_power_test_main_exit(void)
{
	pr_debug("KUTF power test exit start\n");
	kutf_destroy_application(kutf_app);
	pr_debug("KUTF power test exit complete\n");
}

module_init(mali_kutf_kernel_power_test_main_init);
module_exit(mali_kutf_kernel_power_test_main_exit);

MODULE_LICENSE("GPL");
