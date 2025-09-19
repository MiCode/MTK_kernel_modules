/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2022-2023 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file GPUCORE-35490.c
 *
 * Defect Test for GPUCORE-35490
 */

#include <base/mali_base.h>
#include <base/mali_base_kernel.h>
#include <base/mali_base_context.h>
#include <base/mali_base_ioctl.h>
#include <base/src/mali_base_kbase.h>
#include <base/tests/common/mali_base_user_common.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utf/mali_utf.h>
#include "mali_kutf_test_helpers.h"
#include "mali_kutf_kernel_defect_test_helpers.h"


#include <base/mali_base_tiler_heap.h>
#include <base/mali_base_submission_gpu.h>

/** Number of pages per allocated region. */
#define ALLOC_SIZE_PAGES 1u

/** Number of allocated regions in this test. */
#define NUM_ALLOC_REGIONS 4u

/** The size of the memory region for just-in-time allocations,
 * in pages. Must be greater then zero.
 */
#define JIT_REGION_VA_PAGES ((uint64_t)65536)

/** Maximum number of concurrent just-in-time allocations per base context. */
#define JIT_MAX_ALLOCATIONS ((uint8_t)255)

/** Level of trimming of the memory region for just-in-time allocations
 * to perform on free (0-100%).
 */
#define JIT_TRIM_LEVEL ((uint8_t)0)

#define TARGET_RENDER_PASSES ((uint32_t)5)
#define INITIAL_NUM_CHUNKS ((uint32_t)5)
#define MAX_NUM_CHUNKS ((uint32_t)200)
#define CHUNK_SIZE ((uint32_t)OSU_CONFIG_CPU_PAGE_SIZE)

#define CSF_QUEUE_SIZE ((uint32_t)4096)

/* Wrapper for CSF queue register, calling the IOCTL directly
 * so as to avoid dealing with extension fields
 */
static mali_error register_queue_wrapper(base_context *ctx, void *addr)
{
	/* Assume SAME_VA */
	struct kbase_ioctl_cs_queue_register reg = { .buffer_gpu_addr = (uintptr_t)addr,
						     .buffer_size = CSF_QUEUE_SIZE,
						     .priority = BASE_QUEUE_GROUP_PRIORITY_MEDIUM };

	return basep_ioctl_cs_queue_register(&ctx->uk_ctx, &reg);
}

/* Wrapper for CSF queue deregister, calling the IOCTL directly */
static mali_error deregister_queue_wrapper(base_context *ctx, void *addr)
{
	/* Assume SAME_VA */
	struct kbase_ioctl_cs_queue_terminate term = { .buffer_gpu_addr = (uintptr_t)addr };

	return basep_ioctl_cs_queue_terminate(&ctx->uk_ctx, &term);
}

static void test_flag_rejected(mali_utf_suite *suite, base_context *ctx, void *addr,
			       const char *flag_name)
{
	base_tiler_heap heap;
	mali_error err;

	err = base_tiler_heap_init(&heap, ctx, TARGET_RENDER_PASSES, MAX_NUM_CHUNKS,
				   INITIAL_NUM_CHUNKS, CHUNK_SIZE, BASE_MEM_GROUP_DEFAULT,
				   (gpu_buffer *)addr);

	if (mali_error_no_error(err)) {
		base_tiler_heap_term(&heap);
		mali_utf_test_fail(
			"base_tiler_heap_init did not fail despite region having flag %s",
			flag_name);
		return; /* region might be left in an improper state */
	}

	err = register_queue_wrapper(ctx, addr);
	if (mali_error_no_error(err)) {
		mali_utf_test_fail(
			"basep_gpu_queue_register did not fail despite region having flag %s",
			flag_name);
	}
}

/**
 * Main test function for the defect test associated with GPUCORE-35490.
 *
 * With the assistance of a KUTF kernel function, this test does two main things:
 *   - monitor the no_user_free_count management done in tiler heap init and
 *     CSF queue init and ensure it is done properly, and that regions with
 *     no_user_free_count > 0 are not rejected
 *   - ensure those functions reject regions that have the ACTIVE_JIT_ALLOC
 *     or DONT_NEED flag set
 *
 * Code flow is more or less as follows:
 *   - allocate memory (1 page for each region), with SAME_VA option
 *   - send the VAs (plus the ctx ID) to the kernel so that it can retrieve
 *     the corresponding regions (and then preemptively increment no_user_free_count)
 *   - create a tiler heap and a CSF queue with the first two regions
 *   - check the values of their no_user_free_count
 *   - destroy both the tiler heap and CSF queue
 *   - check those values again
 *   - check that tiler heap init and CSF queue init both fail on the last two regions
 *     (as they have flags that are supposed to be rejected)
 *   - clean up
 *
 * Both synchronization (at each step) and data exchange are done via the KUTF
 * data mechanism.
 */
static void GPUCORE35490_test_func(mali_utf_suite *suite)
{
	base_context ctx;
	mali_error err, err_2;
	base_tiler_heap heap;
	struct basep_test_buf_descr tst_buf_descr[NUM_ALLOC_REGIONS];
	struct kutf_test_helpers_named_val named_val, named_val_2;
	uint32_t ctx_id;

	MALI_UTF_ASSERT_EX_M(base_context_init(&ctx, BASE_CONTEXT_CSF_EVENT_THREAD),
			     "Failed to create a base context");

	err = base_mem_jit_init(&ctx, JIT_REGION_VA_PAGES, JIT_MAX_ALLOCATIONS, JIT_TRIM_LEVEL,
				BASE_MEM_GROUP_DEFAULT, JIT_REGION_VA_PAGES);
	if (mali_error_is_error(err)) {
		MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
					       suite, "GPUCORE35490_USERSPACE_READY", 0),
				       0);
		mali_utf_test_fail("base_mem_jit_init failed");
		base_context_term(&ctx);
		return;
	}

	/* Initialize the tst_buf_descr object, failure leads to exit */
	if (!buf_descr_array_init(&ctx, NELEMS(tst_buf_descr), tst_buf_descr)) {
		MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
					       suite, "GPUCORE35490_USERSPACE_READY", 0),
				       0);
		mali_utf_test_fail("failed to allocate buffer descriptor");
		base_context_term(&ctx);
		return;
	}

	MALI_UTF_ASSERT_INT_EQ(
		kutf_test_helpers_userdata_send_named_u64(suite, "GPUCORE35490_USERSPACE_READY", 1),
		0);

	ctx_id = base_get_context_id(&ctx);
	MALI_UTF_ASSERT_INT_EQ(
		kutf_test_helpers_userdata_send_named_u64(suite, "GPUCORE35490_CTX_ID", ctx_id), 0);

	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, "GPUCORE35490_BUF_VA",
				       (uintptr_t)tst_buf_descr[0].buf_descr_cpu_va),
			       0);
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, "GPUCORE35490_BUF_VA_2",
				       (uintptr_t)tst_buf_descr[1].buf_descr_cpu_va),
			       0);
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, "GPUCORE35490_BUF_VA_3",
				       (uintptr_t)tst_buf_descr[2].buf_descr_cpu_va),
			       0);
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, "GPUCORE35490_BUF_VA_4",
				       (uintptr_t)tst_buf_descr[3].buf_descr_cpu_va),
			       0);

	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val, suite, "GPUCORE35490_KERNEL_READY",
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);

	if (named_val.u.val_u64 != 1) {
		/* Kernel has failed to parse the va and has signaled the failure */
		goto cleanup;
	}

	err = base_tiler_heap_init(&heap, &ctx, TARGET_RENDER_PASSES, MAX_NUM_CHUNKS,
				   INITIAL_NUM_CHUNKS, CHUNK_SIZE, BASE_MEM_GROUP_DEFAULT,
				   (gpu_buffer *)tst_buf_descr[0].buf_descr_cpu_va);

	if (mali_error_is_error(err)) {
		mali_utf_test_fail("tiler_heap_init unexpectedly failed");
	}

	err_2 = register_queue_wrapper(&ctx, tst_buf_descr[1].buf_descr_cpu_va);
	if (mali_error_is_error(err_2)) {
		mali_utf_test_fail("basep_gpu_queue_register unexpectedly failed");
	}

	MALI_UTF_ASSERT_INT_EQ(
		kutf_test_helpers_userdata_send_named_u64(suite, "GPUCORE35490_INIT_DONE", 1), 0);

	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val, suite, "GPUCORE35490_REFCNT_1",
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val_2, suite, "GPUCORE35490_REFCNT_2",
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);

	if (mali_error_no_error(err)) {
		if (named_val.u.val_u64 != 2) {
			mali_utf_test_fail(
				"Wrong refcount while tiler heap alive, expected 2 got %d",
				(int)named_val.u.val_u64);
		}
		base_tiler_heap_term(&heap);
	}
	if (mali_error_no_error(err_2)) {
		if (named_val_2.u.val_u64 != 2) {
			mali_utf_test_fail(
				"Wrong refcount while CSF queue alive, expected 2 got %d",
				(int)named_val_2.u.val_u64);
		}
		deregister_queue_wrapper(&ctx, tst_buf_descr[1].buf_descr_cpu_va);
	}

	MALI_UTF_ASSERT_INT_EQ(
		kutf_test_helpers_userdata_send_named_u64(suite, "GPUCORE35490_TERM_DONE", 1), 0);
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val, suite, "GPUCORE35490_REFCNT_1_2",
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val_2, suite, "GPUCORE35490_REFCNT_2_2",
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);

	if (mali_error_no_error(err) && named_val.u.val_u64 != 1) {
		mali_utf_test_fail("Wrong refcount after tiler heap termination, expected 1 got %d",
				   (int)named_val.u.val_u64);
	}
	if (mali_error_no_error(err_2) && named_val_2.u.val_u64 != 1) {
		mali_utf_test_fail("Wrong refcount after CSF queue termination, expected 1 got %d",
				   (int)named_val_2.u.val_u64);
	}

	test_flag_rejected(suite, &ctx, tst_buf_descr[2].buf_descr_cpu_va,
			   "KBASE_REG_ACTIVE_JIT_ALLOC");
	test_flag_rejected(suite, &ctx, tst_buf_descr[3].buf_descr_cpu_va, "KBASE_REG_DONT_NEED");

	MALI_UTF_ASSERT_INT_EQ(
		kutf_test_helpers_userdata_send_named_u64(suite, "GPUCORE35490_DO_CLEANUP", 1), 0);
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val, suite, "GPUCORE35490_CLEANUP_DONE",
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);

cleanup:
	buf_descr_array_term(&ctx, NELEMS(tst_buf_descr), tst_buf_descr);
	base_context_term(&ctx);
}

void GPUCORE35490(mali_utf_suite *suite)
{
	GPUCORE35490_test_func(suite);
}
