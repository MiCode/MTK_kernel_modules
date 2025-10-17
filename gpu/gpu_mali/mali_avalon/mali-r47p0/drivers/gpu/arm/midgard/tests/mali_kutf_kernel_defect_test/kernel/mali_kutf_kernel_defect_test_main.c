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
#include <linux/compat.h>
#include <kutf/kutf_suite.h>
#include <kutf/kutf_utils.h>
#include <kutf/kutf_helpers_user.h>
#include "../mali_kutf_kernel_defect_test.h"
#include "mali_kutf_kernel_defect_test_main.h"

#define MINOR_FOR_FIRST_KBASE_DEV (-1)


void notify_user_val(struct kutf_context *context, const char *name, u64 val)
{
	/* Discard any failure */
	(void)kutf_helper_send_named_u64(context, name, val);
}

void wait_user_val(struct kutf_context *context, const char *name)
{
	/* Discard any failure */
	struct kutf_helper_named_val dummy;

	(void)kutf_helper_receive_check_val(&dummy, context, name, KUTF_HELPER_VALTYPE_U64);
}

struct kbase_context *get_kbase_ctx_ptr_from_id(struct kbase_device *kbdev, u32 id)
{
	struct kbase_context *target_kctx = NULL;
	struct kbase_context *kctx;

	mutex_lock(&kbdev->kctx_list_lock);
	list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
		if (kctx->id == id) {
			target_kctx = kctx;
			break;
		}
	}
	mutex_unlock(&kbdev->kctx_list_lock);

	return target_kctx;
}

/* KUTF test application pointer for this test */
static struct kutf_application *kutf_app;

static void *mali_kutf_kutf_kernel_defect_test_create_fixture(struct kutf_context *context)
{
	struct kutf_kernel_defect_fixture_data *data;

	data = kutf_mempool_alloc(&context->fixture_pool, sizeof(*data));
	if (!data) {
		kutf_test_fail(context, "Failed to allocate fixture data memory");
		return NULL;
	}
	*data = (const struct kutf_kernel_defect_fixture_data){ NULL, NULL };

	/* Acquire the kbase device */
	data->kbdev = kbase_find_device(MINOR_FOR_FIRST_KBASE_DEV);
	if (data->kbdev == NULL) {
		kutf_test_fail(context, "Failed to find kbase device");
		return NULL;
	}

	data->kctx = kbase_create_context(data->kbdev, in_compat_syscall(),
					  BASE_CONTEXT_SYSTEM_MONITOR_SUBMIT_DISABLED, 0, NULL);
	if (!data->kctx) {
		kbase_release_device(data->kbdev);
		return NULL;
	}

	return data;
}

static void mali_kutf_kutf_kernel_defect_test_remove_fixture(struct kutf_context *context)
{
	struct kutf_kernel_defect_fixture_data *data = context->fixture;

	/* If the kctx is not destroyed in the test run, destroy it here */
	if (data->kctx)
		kbase_destroy_context(data->kctx);

	kbase_release_device(data->kbdev);
}

/**
 * mali_kutf_kernel_defect_test_main_init() - Module entry point.
 *
 * Return: 0 on success.
 */
static int __init mali_kutf_kernel_defect_test_main_init(void)
{
	struct kutf_suite *suite;
	unsigned int filters;
	union kutf_callback_data suite_data = { NULL };

	kutf_app = kutf_create_application(KERNEL_DEFECT_TEST_APP_NAME);

	if (kutf_app == NULL) {
		pr_warn("Creation of test app " KERNEL_DEFECT_TEST_APP_NAME " failed\n");
		return -ENOMEM;
	}

	suite = kutf_create_suite_with_filters_and_data(
		kutf_app, KERNEL_DEFECT_TEST_SUITE_NAME, 1,
		mali_kutf_kutf_kernel_defect_test_create_fixture,
		mali_kutf_kutf_kernel_defect_test_remove_fixture, KUTF_F_TEST_GENERIC, suite_data);

	if (suite == NULL) {
		pr_warn("Creation of kutf suite for test app " KERNEL_DEFECT_TEST_SUITE_NAME
			" failed\n");
		kutf_destroy_application(kutf_app);
		return -ENOMEM;
	}

	filters = suite->suite_default_flags;
	kutf_add_test_with_filters(suite, 0x0124, KERNEL_DEFECT_GPUCORE_27226,
				   mali_kutf_kernel_defect_GPUCORE_27226, filters);
	kutf_add_test_with_filters(suite, 0x0125, KERNEL_DEFECT_GPUCORE_35490,
				   mali_kutf_kernel_defect_GPUCORE_35490, filters);
	kutf_add_test_with_filters(suite, 0x0126, KERNEL_DEFECT_GPUCORE_37201,
				   mali_kutf_kernel_defect_GPUCORE_37201, filters);
	kutf_add_test_with_filters(suite, 0x0127, KERNEL_DEFECT_GPUCORE_37465,
				   mali_kutf_kernel_defect_GPUCORE_37465, filters);
	kutf_add_test_with_filters(suite, 0x0128, KERNEL_DEFECT_GPUCORE_39614,
				   mali_kutf_kernel_defect_GPUCORE_39614, filters);
	return 0;
}

/**
 *mali_kutf_kernel_defect_test_main_exit() - Module exit point.
 */
static void __exit mali_kutf_kernel_defect_test_main_exit(void)
{
	pr_debug("Exit start\n");
	kutf_destroy_application(kutf_app);
	pr_debug("Exit complete\n");
}

module_init(mali_kutf_kernel_defect_test_main_init);
module_exit(mali_kutf_kernel_defect_test_main_exit);

MODULE_LICENSE("GPL");
