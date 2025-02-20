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
#include <mali_kbase.h>
#include <kutf/kutf_utils.h>
#include <kutf/kutf_helpers_user.h>
#include "mali_kutf_kernel_defect_test_main.h"

#define NR_PAGES 64U

/**
 * mali_kutf_GPUCORE27226_test_function - Test function that is executed in
 *                                        a kthread
 *
 * @context: pointer to the @p struct kutf_context object
 *
 * This test checks if kbase_mem_pool_free_pages_locked
 * causes the memory pool to exceed the maximum pool size
 *
 * The test allocates a region of memory then sets the
 * maximum pool size to half that of the allocated region.
 * The test calls kbase_mem_pool_free_pages_locked for
 * the size of the allocated region.
 *
 * The test fails if the number of current pages in the pool
 * exceeds the maximum pool size.
 */
static void mali_kutf_GPUCORE27226_test_function(struct kutf_context *context)
{
	struct kbase_va_region *region;
	struct kbase_mem_pool *pool;
	struct tagged_addr *tp;
	u64 gpu_va;
	size_t cur_size, max_size;
	struct kutf_kernel_defect_fixture_data *data = context->fixture;
	struct kbase_context *kctx = data->kctx;

	/* Calls to this function are inherently asynchronous, with respect to
	 * MMU operations.
	 */
	const enum kbase_caller_mmu_sync_info mmu_sync_info = CALLER_MMU_ASYNC;

	u64 flags = BASE_MEM_PROT_CPU_WR | BASE_MEM_PROT_GPU_RD | BASE_MEM_SAME_VA |
		    BASE_MEM_GROW_ON_GPF;

	if (!kctx) {
		pr_warn("%s: Unexpected NULL kctx\n", __func__);
		kutf_test_fail(context, "Unexpected NULL kctx in test GPUCORE27226");
		return;
	}

#if IS_ENABLED(CONFIG_MALI_MTK_MEMORY_DEBUG)
	region =
		kbase_mem_alloc(kctx, NR_PAGES, NR_PAGES, NR_PAGES, &flags, &gpu_va, mmu_sync_info, KBASE_MEM_UNKNOWN);
#else
	region =
		kbase_mem_alloc(kctx, NR_PAGES, NR_PAGES, NR_PAGES, &flags, &gpu_va, mmu_sync_info);
#endif /* CONFIG_MALI_MTK_MEMORY_DEBUG */

	if (!region) {
		pr_warn("unable to allocate region");
		kutf_test_fail(context, "Failed to allocate region");
	} else {
		tp = region->gpu_alloc->pages;
		pool = &kctx->mem_pools.small[0];

		kbase_mem_pool_set_max_size(pool, NR_PAGES / 2);

		spin_lock(&kctx->mem_partials_lock);
		kbase_mem_pool_lock(pool);
		kbase_free_phy_pages_helper_locked(region->gpu_alloc, pool, tp, NR_PAGES);
		kbase_mem_pool_unlock(pool);
		spin_unlock(&kctx->mem_partials_lock);

		kbase_mem_free(kctx, gpu_va);

		cur_size = kbase_mem_pool_size(pool);
		max_size = kbase_mem_pool_max_size(pool);

		if (cur_size > max_size) {
			const char *message = kutf_dsprintf(
				&context->fixture_pool,
				"pool_size exceeded pool_max_size %zu > %zu", cur_size, max_size);
			kutf_test_fail(context, message);
		}
	}
}

/**
 * mali_kutf_kernel_defect_GPUCORE_27226 - Entry point for the test
 * @context: KUTF context
 *
 */
void mali_kutf_kernel_defect_GPUCORE_27226(struct kutf_context *context)
{
	/* Run the test function in a kthread */
	mali_kutf_GPUCORE27226_test_function(context);
}
