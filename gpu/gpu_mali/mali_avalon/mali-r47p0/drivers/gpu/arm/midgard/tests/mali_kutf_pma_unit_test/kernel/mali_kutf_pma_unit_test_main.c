// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2019-2023 ARM Limited. All rights reserved.
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

#include <linux/module.h>
#include <linux/of_platform.h>

#include "mali_kbase.h"
#include <linux/protected_memory_allocator.h>

#include <kutf/kutf_suite.h>

/* Name of the KUTF application for the suite. */
#define PMA_APP_NAME "pma_unit"

/* Number of fixtures to create for each test. */
#define PMA_SUITE_FIXTURES 1

/* Number of allocations to create for each test. */
#define NUM_ALLOCS 100

/* Prime number, used to visit allocations once in a different order. */
#define PRIME_NUM 7

/* Maximum allocation size to test, as a base-2 logarithm. */
#define MAX_SIZE_LOG2 (__builtin_ffs(SZ_2M / PAGE_SIZE) - 1)

/* KUTF test application pointer for this test. */
static struct kutf_application *pma_app;

/**
 * mali_kutf_pma_unique_physical_address - Test physical addresses
 *
 * @context: KUTF context
 *
 * Verify that the physical addresses returned for each allocation made
 * by the protected memory allocator are unique.
 */
static void mali_kutf_pma_unique_physical_address(struct kutf_context *context)
{
	struct protected_memory_allocator_device *pma_dev = context->fixture;
	struct protected_memory_allocation *allocs[NUM_ALLOCS];
	phys_addr_t phys_addrs[NUM_ALLOCS];
	int alloc_idx, i, j;

	for (alloc_idx = 0; alloc_idx < NUM_ALLOCS; alloc_idx++) {
		allocs[alloc_idx] = pma_dev->ops.pma_alloc_page(pma_dev, 0);
		if (!allocs[alloc_idx])
			break;
		phys_addrs[alloc_idx] = pma_dev->ops.pma_get_phys_addr(pma_dev, allocs[alloc_idx]);
	}

	if (alloc_idx < NUM_ALLOCS) {
		kutf_test_fail(context, "Failed to allocate protected memory");
		goto alloc_cleanup;
	}

	for (i = 0; i < NUM_ALLOCS - 1; i++) {
		for (j = i + 1; j < NUM_ALLOCS; j++) {
			if (phys_addrs[i] == phys_addrs[j]) {
				kutf_test_fail(context, "Found identical physical addresses");
			}
		}
	}

alloc_cleanup:
	while (alloc_idx > 0)
		pma_dev->ops.pma_free_page(pma_dev, allocs[--alloc_idx]);
}

/**
 * mali_kutf_pma_free_any_order - Test memory release
 *
 * @context: KUTF context
 *
 * Verify that protected memory allocations can be freed in any order
 * and independently of one another.
 */
static void mali_kutf_pma_free_any_order(struct kutf_context *context)
{
	struct protected_memory_allocator_device *pma_dev = context->fixture;
	struct protected_memory_allocation *allocs[NUM_ALLOCS];
	int alloc_idx, i, cnt;

	for (alloc_idx = 0; alloc_idx < NUM_ALLOCS; alloc_idx++) {
		allocs[alloc_idx] = pma_dev->ops.pma_alloc_page(pma_dev, 0);
		if (!allocs[alloc_idx])
			break;
	}

	/* Cleanup in case of failure shall still be attempted, as we do
	 * for other unit tests in the same suite.
	 */
	if (alloc_idx < NUM_ALLOCS) {
		kutf_test_fail(context, "Failed to allocate protected memory");
		while (alloc_idx > 0)
			pma_dev->ops.pma_free_page(pma_dev, allocs[--alloc_idx]);
		return;
	}

	/* Increase the index by a prime number in order to walk through
	 * all the allocations exactly once.
	 */
	for (cnt = 0, i = PRIME_NUM; cnt < NUM_ALLOCS; cnt++) {
		pma_dev->ops.pma_free_page(pma_dev, allocs[i]);
		i = (i + PRIME_NUM) % NUM_ALLOCS;
	}
}

/**
 * mali_kutf_pma_alloc_free_any_size - Test memory allocation size
 *
 * @context: KUTF context
 *
 * Verify that protected memory allocations of any size can be created
 * and freed in the range from order 0 (1 page, i.e. PAGE_SIZE) to
 * - order 9 (when PAGE_SIZE is 4KB, 2^9 * 4KB = 2 MB)
 * - order 7 (when PAGE_SIZE is 16KB, 2^7 * 16KB = 2 MB)
 * - order 5 (when PAGE_SIZE is 64KB, 2^5 * 64KB = 2 MB)
 */
static void mali_kutf_pma_alloc_free_any_size(struct kutf_context *context)
{
	struct protected_memory_allocator_device *pma_dev = context->fixture;
	struct protected_memory_allocation *allocs[MAX_SIZE_LOG2 + 1];
	unsigned int i;

	for (i = 0; i <= MAX_SIZE_LOG2; i++) {
		allocs[i] = pma_dev->ops.pma_alloc_page(pma_dev, i);
		if (!allocs[i])
			break;
	}

	if (i <= MAX_SIZE_LOG2)
		kutf_test_fail(context, "Failed to allocate protected memory");

	while (i > 0)
		pma_dev->ops.pma_free_page(pma_dev, allocs[--i]);
}

#define NUM_PAGES ((u32)10)
#define NUM_WORK_ITEMS ((u32)10)
#define NUM_ITERATIONS ((u32)50)

struct pma_alloc_free_work {
	struct work_struct alloc_free_work;
	struct kutf_context *context;
};

static void pma_alloc_free_worker(struct work_struct *work)
{
	struct pma_alloc_free_work *pma_work =
		container_of(work, struct pma_alloc_free_work, alloc_free_work);
	struct kutf_context *context = pma_work->context;
	struct protected_memory_allocator_device *pma_dev = context->fixture;
	struct protected_memory_allocation *pma[NUM_PAGES];
	u32 i;

	for (i = 0; i < NUM_PAGES; i++) {
		pma[i] = pma_dev->ops.pma_alloc_page(pma_dev, 0);
		if (!pma[i]) {
			kutf_test_fail(context, "Failed to allocate protected memory");
			break;
		}
	}

	while (i-- > 0)
		pma_dev->ops.pma_free_page(pma_dev, pma[i]);
}

/**
 * mali_kutf_pma_alloc_free_multi_thread - Exercise muli threaded allocation
 *                                         and freeing of protected memory
 *                                         pages.
 * @context: KUTF context
 *
 * This test exercises the concurrent allocation and freeing of protected
 * memory pages from multiple kernel threads. If no lock is used inside the
 * protected memory allocator module (to serialize the allocation and freeing
 * of pages from protected memory region) then this test is able to trigger
 * the warnings from allocator module.
 */
static void mali_kutf_pma_alloc_free_multi_thread(struct kutf_context *context)
{
	struct pma_alloc_free_work pma_work[NUM_WORK_ITEMS];
	u32 i, j;

	for (i = 0; i < NUM_WORK_ITEMS; i++) {
		pma_work[i].context = context;

		INIT_WORK_ONSTACK(&pma_work[i].alloc_free_work, pma_alloc_free_worker);
	}

	for (i = 0; i < NUM_ITERATIONS; i++) {
		for (j = 0; j < NUM_WORK_ITEMS; j++) {
			flush_work(&pma_work[j].alloc_free_work);
			queue_work(system_wq, &pma_work[j].alloc_free_work);
		}
	}

	for (i = 0; i < NUM_WORK_ITEMS; i++)
		flush_work(&pma_work[i].alloc_free_work);
}

static void *mali_kutf_pma_unit_create_fixture(struct kutf_context *context)
{
	struct device_node *pma_node;
	struct kbase_device *kbdev;
	struct platform_device *pdev;
	struct protected_memory_allocator_device *pma_dev;

	/* Acquire the kbase device */
	pr_debug("Finding device\n");
	kbdev = kbase_find_device(-1);
	if (!kbdev) {
		kutf_test_fail(context, "Failed to find kbase device");
		return NULL;
	}

	pma_node = of_parse_phandle(kbdev->dev->of_node, "protected-memory-allocator", 0);
	if (!pma_node) {
		kutf_test_fail(context, "Failed to find protected memory allocator");
		return NULL;
	}

	pdev = of_find_device_by_node(pma_node);

	if (!pdev) {
		kutf_test_fail(context, "Protected memory allocator device was not found");
		of_node_put(pma_node);
		return NULL;
	}

	pma_dev = platform_get_drvdata(pdev);
	if (!pma_dev) {
		kutf_test_fail(context, "Protected memory allocator driver is not ready");
		of_node_put(pma_node);
		return NULL;
	}

	if (!try_module_get(pma_dev->owner)) {
		kutf_test_fail(context, "Failed to get protected memory allocator module");
		of_node_put(pma_node);
		return NULL;
	}

	of_node_put(pma_node);
	pr_debug("Created fixture\n");
	return pma_dev;
}

static void mali_kutf_pma_unit_remove_fixture(struct kutf_context *context)
{
	struct protected_memory_allocator_device *pma_dev = context->fixture;

	if (pma_dev)
		module_put(pma_dev->owner);

	pr_debug("Destroyed fixture\n");
}

static int __init mali_kutf_pma_unit_test_main_init(void)
{
	struct kutf_suite *suite;
	unsigned int filters;

	pr_debug("Creating app\n");
	pma_app = kutf_create_application(PMA_APP_NAME);

	pr_debug("Create suite\n");
	suite = kutf_create_suite_with_filters(pma_app, "pma_unit_test", PMA_SUITE_FIXTURES,
					       mali_kutf_pma_unit_create_fixture,
					       mali_kutf_pma_unit_remove_fixture,
					       KUTF_F_TEST_GENERIC);

	filters = suite->suite_default_flags;
	kutf_add_test_with_filters(suite, 0x0, "unique_physical_address",
				   mali_kutf_pma_unique_physical_address, filters);
	kutf_add_test_with_filters(suite, 0x1, "free_any_order", mali_kutf_pma_free_any_order,
				   filters);
	kutf_add_test_with_filters(suite, 0x2, "alloc_free_any_size",
				   mali_kutf_pma_alloc_free_any_size, filters);
	kutf_add_test_with_filters(suite, 0x3, "alloc_free_multi_threaded",
				   mali_kutf_pma_alloc_free_multi_thread, filters);

	pr_debug("Init complete\n");
	return 0;
}

static void __exit mali_kutf_pma_unit_test_main_exit(void)
{
	pr_debug("Exit start\n");
	kutf_destroy_application(pma_app);
	pr_debug("Exit complete\n");
}

module_init(mali_kutf_pma_unit_test_main_init);
module_exit(mali_kutf_pma_unit_test_main_exit);

MODULE_LICENSE("GPL");
