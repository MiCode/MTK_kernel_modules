// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2022-2023 ARM Limited. All rights reserved.
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
#include <mali_kbase.h>
#include <kutf/kutf_utils.h>
#include <kutf/kutf_helpers_user.h>
#include "mali_kutf_kernel_defect_test_main.h"

#if !MALI_USE_CSF
/**
 * mali_kutf_GPUCORE37201_test_function - Test function that is executed in
 *                                        a kthread
 *
 * @context: pointer to the @p struct kutf_context object
 *
 * This is the kernel counterpart for the associated userspace test.
 * Refer to GPUCORE37201_test_func in ../test_runner/GPUCORE-37201.c
 * for a more precise overview of the control flow.
 * After retrieving that user-space is ready for analyzing kmsg,
 * this side of the test creates a new kbase context witch will fail
 * at kbase_region_tracker_init.
 * After that user-space side is responsible to check if warning is detected.
 * Both synchronization (at each step) and data exchange are done via the KUTF
 * data mechanism.
 */

static void mali_kutf_GPUCORE37201_test_function(struct kutf_context *context)
{
	struct kutf_kernel_defect_fixture_data *data = context->fixture;
	int err = 0;
	u8 va_bits = 0;
	struct kbase_context *kctx;
	struct kutf_helper_named_val val;

	if (data->kbdev == NULL) {
		pr_warn("%s: Unexpected NULL kbdev\n", __func__);
		kutf_test_fail(context, "Unexpected kbdev kctx in test GPUCORE37201");
		return;
	}
	/* prepare kernel space */
	va_bits = data->kbdev->gpu_props.mmu.va_bits;

	err = kutf_helper_receive_check_val(&val, context, "GPUCORE37201_USERSPACE_READY",
					    KUTF_HELPER_VALTYPE_U64);
	if (err != 0) {
		kutf_test_fail(context, "Failed to communicate with userspace");
		return;
	}
	/* Set va_bits witch will cause a fail on context_init sequence */
	data->kbdev->gpu_props.mmu.va_bits = 22;

	kctx = kbase_create_context(data->kbdev, true, BASE_CONTEXT_SYSTEM_MONITOR_SUBMIT_DISABLED,
				    0, NULL);

	/* Restore va_bits */
	data->kbdev->gpu_props.mmu.va_bits = va_bits;

	/* Signal userspace that we can analyze the dmesg */
	kutf_helper_send_named_u64(context, "GPUCORE37201_KERNEL_READY", 0);
	if (kctx)
		kbase_destroy_context(kctx);
}
#endif

/**
 * mali_kutf_kernel_defect_GPUCORE_37201 - Entry point for the test
 * @context: KUTF context
 *
 */
void mali_kutf_kernel_defect_GPUCORE_37201(struct kutf_context *context)
{
#if !MALI_USE_CSF
	mali_kutf_GPUCORE37201_test_function(context);
#else /* MALI_USE_CSF */
	kutf_test_skip_msg(context,
			   "The kernel_defect_GPUCORE_37201 test is only applicable to JM GPUs");
#endif /* MALI_USE_CSF */
}
