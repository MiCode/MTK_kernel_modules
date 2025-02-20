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
#include <linux/workqueue.h>
#include <device/mali_kbase_device.h>
#include <mali_kbase_mem.h>
#include <kutf/kutf_helpers_user.h>
#include "mali_kutf_kernel_defect_test_main.h"

/**
 * mali_kutf_GPUCORE35490_test_function - Main test function (kernel-side)
 *
 * @context: pointer to the @p struct kutf_context object
 *
 * This is the kernel counterpart for the associated userspace test.
 * Refer to GPUCORE35490_test_func in ../test_runner/GPUCORE-35490.c
 * for a more precise overview of the control flow.
 *
 * After retriveing the user-allocated regions, this side of the test
 * does a few things on behalf of userspace:
 *   - manipulate and monitor/dump the no_user_free_count of the first two
 *     regions
 *   - respectively set ACTIVE_JIT_ALLOC/DONT_NEED on the last two regions.
 *     This isn't too great but avoids a lot of hassle for the user-side
 * Both synchronization (at each step) and data exchange are done via the KUTF
 * data mechanism.
 */
static void mali_kutf_GPUCORE35490_test_function(struct kutf_context *context)
{
	struct kutf_kernel_defect_fixture_data *data = context->fixture;
	int err = 0;
	u64 gpu_va[4] = { 0 };
	u32 ctx_id = 0;
	struct kbase_va_region *regs[4] = { NULL };
	struct kbase_context *kctx;
	int i;
	struct kutf_helper_named_val val;
	int refcnt, refcnt_2;

	if (data->kbdev == NULL) {
		pr_warn("%s: Unexpected NULL kbdev\n", __func__);
		kutf_test_fail(context, "Unexpected kbdev kctx in test GPUCORE35490");
		return;
	}

	err = kutf_helper_receive_check_val(&val, context, "GPUCORE35490_USERSPACE_READY",
					    KUTF_HELPER_VALTYPE_U64);
	if (err != 0) {
		kutf_test_fail(context, "Failed to communicate with userspace");
		return;
	} else if (val.u.val_u64 == 0) {
		/* Userspace failed to allocate mem & will signal the test failure */
		return;
	}

	/* Get the kctx it for the context userspace created */
	err = kutf_helper_receive_check_val(&val, context, "GPUCORE35490_CTX_ID",
					    KUTF_HELPER_VALTYPE_U64);
	if (err != 0) {
		kutf_test_fail(context, "Failed to communicate with userspace");
		return;
	}
	ctx_id = (u32)val.u.val_u64;

	/* Get the regions gpu_va */
	for (i = 0; i < 4; i++) {
		static const char *const names[] = { "GPUCORE35490_BUF_VA", "GPUCORE35490_BUF_VA_2",
						     "GPUCORE35490_BUF_VA_3",
						     "GPUCORE35490_BUF_VA_4" };
		err = kutf_helper_receive_check_val(&val, context, names[i],
						    KUTF_HELPER_VALTYPE_U64);
		if (err != 0) {
			kutf_test_fail(context, "Failed to communicate with userspace");
			return;
		}
		gpu_va[i] = val.u.val_u64;
	}

	/* Past this point we assume kutf kernel<>user data exchange never fails */

	kctx = get_kbase_ctx_ptr_from_id(data->kbdev, ctx_id);
	if (kctx == NULL) {
		notify_user_val(context, "GPUCORE35490_KERNEL_READY", 0);
		kutf_test_fail(context, "Userspace sent invalid ctx id");
		return;
	}

	kbase_gpu_vm_lock(kctx);
	for (i = 0; i < 4; i++) {
		regs[i] = kbase_region_tracker_find_region_enclosing_address(kctx, gpu_va[i]);
		if (kbase_is_region_invalid_or_free(regs[i])) {
			kbase_gpu_vm_unlock(kctx);
			notify_user_val(context, "GPUCORE35490_KERNEL_READY", 0);
			kutf_test_fail(context, "Userspace sent invalid memory region");
			return;
		}
	}

	/* Increment no_user_free_count so we can check if tiler heap init and CSF queue init
	 * still accept those regions.
	 */
	kbase_va_region_no_user_free_inc(regs[0]);
	kbase_va_region_no_user_free_inc(regs[1]);
	kbase_gpu_vm_unlock(kctx);

	notify_user_val(context, "GPUCORE35490_KERNEL_READY", 1);

	/* Userspace has performed tiler heap init and CSF queue init (whether it failed or not).*/
	wait_user_val(context, "GPUCORE35490_INIT_DONE");

	/* Dump current refcounts */
	kbase_gpu_vm_lock(kctx);
	refcnt = atomic64_read(&regs[0]->no_user_free_count);
	refcnt_2 = atomic64_read(&regs[1]->no_user_free_count);
	kbase_gpu_vm_unlock(kctx);
	notify_user_val(context, "GPUCORE35490_REFCNT_1", (u64)refcnt);
	notify_user_val(context, "GPUCORE35490_REFCNT_2", (u64)refcnt_2);

	/* Userspace has had an opportunity to perform tiler heap init and CSF queue termination */
	wait_user_val(context, "GPUCORE35490_TERM_DONE");

	/* Dump current refcounts (again), and alter the remaining regions' flags.
	 * Not the best way to do it, but we just want to check whether these flags
	 * are rejected early in tiler heap init/CSF queue init.
	 */
	kbase_gpu_vm_lock(kctx);
	refcnt = atomic64_read(&regs[0]->no_user_free_count);
	refcnt_2 = atomic64_read(&regs[1]->no_user_free_count);
	regs[2]->flags |= KBASE_REG_ACTIVE_JIT_ALLOC;
	regs[3]->flags |= KBASE_REG_DONT_NEED;
	kbase_gpu_vm_unlock(kctx);
	notify_user_val(context, "GPUCORE35490_REFCNT_1_2", (u64)refcnt);
	notify_user_val(context, "GPUCORE35490_REFCNT_2_2", (u64)refcnt_2);

	/* Sync on exit */
	wait_user_val(context, "GPUCORE35490_DO_CLEANUP");

	kbase_gpu_vm_lock(kctx);
	kbase_va_region_no_user_free_dec(regs[0]);
	kbase_va_region_no_user_free_dec(regs[1]);
	regs[2]->flags &= ~KBASE_REG_ACTIVE_JIT_ALLOC;
	regs[3]->flags &= ~KBASE_REG_DONT_NEED;
	kbase_gpu_vm_unlock(kctx);
	notify_user_val(context, "GPUCORE35490_CLEANUP_DONE", 1);
}


/**
 * mali_kutf_kernel_defect_GPUCORE_35490 - Entry point for the test
 * @context: KUTF context
 *
 */
void mali_kutf_kernel_defect_GPUCORE_35490(struct kutf_context *context)
{
	mali_kutf_GPUCORE35490_test_function(context);
}
