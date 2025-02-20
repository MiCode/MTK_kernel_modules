/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2023 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file GPUCORE-37465.c
 *
 * Defect Test for GPUCORE-37465
 */

#include <base/mali_base.h>
#include <base/mali_base_kernel.h>
#include <base/mali_base_context.h>
#include <base/mali_base_ioctl.h>
#include <base/src/mali_base_kbase.h>
#include <base/tests/common/mali_base_user_common.h>
#include <base/tests/internal/api_tests/helpers/mali_base_helpers_power.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utf/mali_utf.h>
#include "mali_kutf_test_helpers.h"
#include "mali_kutf_kernel_defect_test_helpers.h"
#include "mali_kutf_kernel_defect_test_helpers.h"
#include "../mali_kutf_kernel_defect_test_GPUCORE_37465.h"


#include <base/mali_base_tiler_heap.h>
#include <base/mali_base_submission_gpu.h>

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

/* Len of array used to store current power policy*/
#define POWER_POLICY_ARRAY_LEN 256

#define BASE_QUEUE_PRIORITY_MEDIUM 8

#define TARGET_RENDER_PASSES ((uint32_t)5)
#define INITIAL_NUM_CHUNKS ((uint32_t)5)
#define MAX_NUM_CHUNKS ((uint32_t)200)
#define CHUNK_SIZE ((uint32_t)OSU_CONFIG_CPU_PAGE_SIZE)

#define CSF_TILER_MASK ((uint64_t)0)
#define CSF_FRAGMENT_MASK ((uint64_t)0)
#define CSF_COMPUTE_MASK (UINT64_MAX)
#define CSF_TILER_MAX ((uint8_t)0)
#define CSF_FRAGMENT_MAX ((uint8_t)0)
#define CSF_COMPUTE_MAX ((uint8_t)64)
#define CSF_QUEUE_SIZE ((uint32_t)4096)
#define CSF_INTERFACE_INDEX ((uint8_t)0)
#define CSF_NR_INTERFACES ((uint8_t)1)

#define CSF_QUEUE_SIZE ((uint32_t)4096)

#define NS_PER_MSEC ((uint64_t)1E6)

/* CSF CSI EXCEPTION_HANDLER_FLAGS */
#define BASE_CSF_TILER_OOM_EXCEPTION_FLAG (1u << 0)
#define BASE_CSF_EXCEPTION_HANDLER_FLAGS_MASK (BASE_CSF_TILER_OOM_EXCEPTION_FLAG)

/* custom function to free queue memory */
static void custom_basep_gpu_queue_free_memory(cutils_refcount *const rc)
{
	CDBG_ASSERT_POINTER(rc);

	base_gpu_command_queue *queue = CONTAINER_OF(rc, base_gpu_command_queue, basep.refcount);
	uint64_t nr_pages = BYTES_TO_PAGES(basep_queue_get_size(&queue->basep.command_queue));

	basep_context_internal_alloc_histogram_sub(queue->basep.ctx,
						   nr_pages << OSU_CONFIG_CPU_PAGE_SIZE_LOG2);
	base_mem_free(queue->basep.ctx, queue->basep.buffer_h, nr_pages);
	osu_sync_object_term(&queue->basep.event_sync);

	stdlib_free(queue);
}

/**
 * Main test function for the defect test associated with GPUCORE-37465.
 *
 * With the assistance of a KUTF kernel function, this test does two main things:
 *   - artificially set driver internal state to simulate ongoing GPU reset
 *   - generate artificial Tiler Out Of Memory IRQ to generate Tiler Out Of Memory event
 *
 * Code flow is more or less as follows:
 *   - store current power policy
 *   - set power policy to always on
 *   - initialize
 *      * mem_jit
 *      * queue group
 *      * queue
 *   - kick-up queue to allow accepting artificial irq
 *   - initialize Tiler Heap
 *   - send various Tiler Out Of Memory IRQ parameters to kernel part of test
 *   - notify kernel part of test that every think is ready
 *   - wait for kernel part of test to finish
 *   - clean up
 *
 * Both synchronization (at each step) and data exchange are done via the KUTF
 * data mechanism.
 */
static void GPUCORE37465_test_func(mali_utf_suite *suite)
{
	base_context ctx;
	mali_error err;
	base_gpu_command_queue_group *group = NULL;
	base_gpu_command_queue *queue = NULL;
	base_tiler_heap heap;
	struct basep_test_buf_descr tst_buf_descr[NUM_ALLOC_REGIONS];
	struct kutf_test_helpers_named_val named_val;
	uint32_t ctx_id;

	MALI_UTF_ASSERT_EX_M(base_context_init(&ctx, BASE_CONTEXT_CSF_EVENT_THREAD),
			     "Failed to create a base context");

	char saved_power_policy[POWER_POLICY_ARRAY_LEN] = { '\0' };

	err = base_test_get_power_policy(saved_power_policy, POWER_POLICY_ARRAY_LEN);
	if (mali_error_is_error(err)) {
		MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
					       suite, "GPUCORE37465_USERSPACE_READY",
					       GPUCORE37465_NOK),
				       0);
		mali_utf_test_fail("base_test_get_power_policy failed");
		goto base_context_cleanup;
	}

	err = base_test_set_power_policy("always_on");

	if (mali_error_is_error(err)) {
		MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
					       suite, "GPUCORE37465_USERSPACE_READY",
					       GPUCORE37465_NOK),
				       0);
		mali_utf_test_fail("base_test_set_power_policy failed");
		goto base_context_cleanup;
	}
	err = base_mem_jit_init(&ctx, JIT_REGION_VA_PAGES, JIT_MAX_ALLOCATIONS, JIT_TRIM_LEVEL,
				BASE_MEM_GROUP_DEFAULT, JIT_REGION_VA_PAGES);
	if (mali_error_is_error(err)) {
		MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
					       suite, "GPUCORE37465_USERSPACE_READY",
					       GPUCORE37465_NOK),
				       0);
		mali_utf_test_fail("base_mem_jit_init failed");
		goto base_context_cleanup;
	}

	/* Initialize the tst_buf_descr object, failure leads to exit */
	if (!buf_descr_array_init(&ctx, NELEMS(tst_buf_descr), tst_buf_descr)) {
		MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
					       suite, GPUCORE37465_USERSPACE_READY, 0),
				       0);
		mali_utf_test_fail("failed to allocate buffer descriptor");
		goto base_context_cleanup;
	}

	base_gpu_command_queue_group_cfg cfg = { .cs_min = CSF_NR_INTERFACES,
						 .priority = BASE_QUEUE_GROUP_PRIORITY_MEDIUM,
						 .tiler_mask = CSF_TILER_MASK,
						 .fragment_mask = CSF_FRAGMENT_MASK,
						 .compute_mask = CSF_COMPUTE_MASK,
						 .tiler_max = CSF_TILER_MAX,
						 .fragment_max = CSF_FRAGMENT_MAX,
						 .compute_max = CSF_COMPUTE_MAX,
						 .csi_handler_flags =
							 BASE_CSF_TILER_OOM_EXCEPTION_FLAG,
						 .error_callback = NULL,
						 .user_data = NULL,
						 .scratch_pages = 0 };

	group = base_gpu_queue_group_new(&ctx, &cfg);

	if (!group) {
		MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
					       suite, GPUCORE37465_USERSPACE_READY,
					       GPUCORE37465_NOK),
				       0);
		mali_utf_test_fail("Failed to create gpu queue group");
		goto buf_descr_array_cleanup;
	}

	queue = base_gpu_queue_new(&ctx, CSF_QUEUE_SIZE, BASE_QUEUE_PRIORITY_MEDIUM,
				   BASE_GPU_QUEUE_FLAG_NONE);

	if (!queue) {
		MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
					       suite, GPUCORE37465_USERSPACE_READY,
					       GPUCORE37465_NOK),
				       0);
		mali_utf_test_fail("Failed to create gpu queue");
		goto base_gpu_queue_group_cleanup;
	}

	err = base_gpu_queue_group_bind(group, queue, CSF_INTERFACE_INDEX);
	if (mali_error_is_error(err)) {
		MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
					       suite, GPUCORE37465_USERSPACE_READY,
					       GPUCORE37465_NOK),
				       0);
		mali_utf_test_fail("failed to bind queue to group");
		goto base_gpu_queue_cleanup;
	}

	/* This would make the kernel driver schedule the queue group,
	 * otherwise the fake interrupt generated by the test module
	 * would not be handled by the kernel driver as it won't expect
	 * an interrupt from firmware for a queue group which hasn't
	 * been started/scheduled.
	 */
	basep_gpu_queue_kick(queue, false);
	mali_tpi_sleep_ns(500 * NS_PER_MSEC, false);

	err = base_tiler_heap_init(&heap, &ctx, TARGET_RENDER_PASSES, MAX_NUM_CHUNKS,
				   INITIAL_NUM_CHUNKS, CHUNK_SIZE, BASE_MEM_GROUP_DEFAULT,
				   (gpu_buffer *)tst_buf_descr[0].buf_descr_cpu_va);

	if (mali_error_is_error(err)) {
		mali_utf_test_fail("tiler_heap_init unexpectedly failed");
		MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
					       suite, GPUCORE37465_USERSPACE_READY,
					       GPUCORE37465_NOK),
				       0);
		goto base_gpu_queue_cleanup;
	}

	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, GPUCORE37465_USERSPACE_READY, GPUCORE37465_OK),
			       0);

	/* Send GPU VA of the Heap context */
	uint64_t gpu_heap_va = base_tiler_heap_get_address(&heap);

	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, GPUCORE37465_USERSPACE_GPU_HEAP_VA, gpu_heap_va),
			       0);

	/* Send size of the Heap chunk */
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, GPUCORE37465_USERSPACE_HEAP_CHUNK_SIZE, CHUNK_SIZE),
			       0);

	/* Send GPU VA of the GPU queue */
	uint64_t gpu_queue_va = base_mem_gpu_address(queue->basep.buffer_h, 0);

	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, GPUCORE37465_USERSPACE_GPU_QUEUE_VA, gpu_queue_va),
			       0);

	/* Send number of vertex/tiling operations which have started on the Heap */
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, GPUCORE37465_USERSPACE_HEAP_VT_START, 9),
			       0);

	/* Send number of vertex/tiling operations which have completed on the Heap */
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, GPUCORE37465_USERSPACE_HEAP_VT_END, 7),
			       0);

	/* Send number of fragment operations which have completed on the Heap */
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, GPUCORE37465_USERSPACE_HEAP_FRAG_END, 6),
			       0);

	/* Send CTX ID */
	ctx_id = base_get_context_id(&ctx);
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, GPUCORE37465_USERSPACE_CTX_ID, ctx_id),
			       0);

	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val, suite, GPUCORE37465_KERNEL_READY,
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);

	if (named_val.u.val_u64 != GPUCORE37465_OK) {
		/* Kernel has signaled the failure */
		mali_utf_test_fail("kernel not ready");
		goto base_gpu_queue_cleanup;
	}

	/*  wait for kernel to execute test*/
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val, suite, GPUCORE37465_KERNEL_TEST_DONE,
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);

	/* Cleanup */
	base_tiler_heap_term(&heap);

	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, GPUCORE37465_USERSPACE_TERM_DONE, GPUCORE37465_OK),
			       0);

	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_send_named_u64(
				       suite, GPUCORE37465_USERSPACE_DO_CLEANUP, GPUCORE37465_OK),
			       0);
	MALI_UTF_ASSERT_INT_EQ(kutf_test_helpers_userdata_receive_check_val(
				       &named_val, suite, GPUCORE37465_KERNEL_CLEANUP_DONE,
				       KUTF_TEST_HELPERS_VALTYPE_U64),
			       0);

	/* Use custom function to free queue to avoid queue clean up in kernel
	 * and to force context termination routine to clean up queue and check reference count
	 */
	queue->basep.refcount.cutilsp_refcount.delete_callback = custom_basep_gpu_queue_free_memory;

base_gpu_queue_cleanup:
	base_gpu_queue_delete(queue);

base_gpu_queue_group_cleanup:
	base_gpu_queue_group_delete(group);

buf_descr_array_cleanup:
	buf_descr_array_term(&ctx, NELEMS(tst_buf_descr), tst_buf_descr);

base_context_cleanup:
	base_context_term(&ctx);

	/* Restore  power policy */
	if (saved_power_policy[0] != '\0') {
		err = base_test_set_power_policy(saved_power_policy);
		MALI_UTF_ASSERT_M(mali_error_no_error(err), "Failed to restore power_policy '%s'",
				  saved_power_policy);
	}
}

void GPUCORE37465(mali_utf_suite *suite)
{
	GPUCORE37465_test_func(suite);
}
