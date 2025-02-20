/*
 * Copyright:
 * ----------------------------------------------------------------------------
 * This confidential and proprietary software may be used only as authorized
 * by a licensing agreement from ARM Limited.
 *      (C) COPYRIGHT 2019-2023 ARM Limited, ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorized copies and
 * copies may only be made to the extent permitted by a licensing agreement
 * from ARM Limited.
 * ----------------------------------------------------------------------------
 */

#include <utf/include/mali_utf_suite.h>
#include <utf/include/mali_utf_mem.h>
#include <utf/include/mali_utf_resultset.h>
#include <utf/include/mali_utf_helpers.h>
#include <utf/include/mali_utf_main.h>
#include <base/mali_base.h>
#include <base/mali_base_submission_gpu.h>
#include <base/mali_base_tiler_heap.h>
#include <base/tests/internal/api_tests/helpers/mali_base_helpers_power.h>
#include <base/tests/common/mali_base_user_common.h>
#include "uapi/gpu/arm/midgard/csf/mali_kbase_csf_errors_dumpfault.h"

#include "mali_kutf_csf_tiler_heap_unit_test_extra.h"
#include "mali_kutf_test_helpers.h"

#include "../mali_kutf_csf_tiler_heap_unit_test.h"

#include <fcntl.h>
#include <unistd.h>

#define BASE_QUEUE_PRIORITY_MEDIUM 8

#define CSF_TILER_MASK ((uint64_t)0)
#define CSF_FRAGMENT_MASK ((uint64_t)0)
#define CSF_COMPUTE_MASK (UINT64_MAX)
#define CSF_TILER_MAX ((uint8_t)0)
#define CSF_FRAGMENT_MAX ((uint8_t)0)
#define CSF_COMPUTE_MAX ((uint8_t)64)
#define CSF_QUEUE_SIZE ((uint32_t)4096)
#define CSF_INTERFACE_INDEX ((uint8_t)0)
#define CSF_NR_INTERFACES ((uint8_t)1)

#define CHUNK_SIZE (test_fix->chunk_size_in_pages * OSU_CONFIG_CPU_PAGE_SIZE)
#define INVALID_GPU_HEAP_VA ((uint64_t)0xFFFFFFFF)

#define JIT_REGION_VA_PAGES ((uint64_t)65536)
#define JIT_MAX_ALLOCATIONS ((uint8_t)255)
#define JIT_TRIM_LEVEL ((uint8_t)0)

typedef struct test_fixture {
	bool use_invalid_gpu_heap_va;
	uint32_t vt_start;
	uint32_t vt_end;
	uint32_t frag_end;
	uint16_t target_in_flight;
	uint32_t max_chunks;
	uint32_t initial_chunks;
	uint32_t chunk_size_in_pages;
	uint8_t csi_handler_flags;
	enum oom_event_action expected_action;
	struct base_gpu_queue_group_error expected_error;
} test_fixture;

/* List of test fixtures.
 * Attempt to provide a range of userspace settings for the Tiler Heap and statistics of the Heap.
 */
static const test_fixture heap_settings[CSF_TILER_HEAP_SUITE_FIXTURES] = {
	/* chunk_count < max_chunks and nr_render_passes_in_flight < target_in_flight */
	{ .vt_start = 9,
	  .vt_end = 7,
	  .frag_end = 6,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_NEW_CHUNK_ALLOCATED,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },
	},

	/* chunk_count == max_chunks and nr_render_passes_in_flight < target_in_flight and pending_frag_count == 0 */
	{
	 .vt_start = 6,
	 .vt_end = 4,
	 .frag_end = 4,
	 .target_in_flight = 5,
	 .max_chunks = 10,
	 .initial_chunks = 10,
	 .chunk_size_in_pages = 1,
	 .expected_action = OOM_EVENT_CSG_TERMINATED,
	 .expected_error = { .error_type = BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM },
	},

	/* chunk_count + 1 == max_chunks and nr_render_passes_in_flight < target_in_flight */
	{ .vt_start = 9,
	  .vt_end = 7,
	  .frag_end = 6,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 9,
	  .chunk_size_in_pages = 2,
	  .expected_action = OOM_EVENT_NEW_CHUNK_ALLOCATED,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },
	},

	/* chunk_count == max_chunks and nr_render_passes_in_flight < target_in_flight */
	{ .vt_start = 9,
	  .vt_end = 7,
	  .frag_end = 6,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 10,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_NULL_CHUNK,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },
	},

	/* chunk_count < max_chunks and nr_render_passes_in_flight == target_in_flight */
	{ .vt_start = 9,
	  .vt_end = 5,
	  .frag_end = 4,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 3,
	  .expected_action = OOM_EVENT_NEW_CHUNK_ALLOCATED,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },
	},

	/* chunk_count == max_chunks and nr_render_passes_in_flight == 1 */
	{ .vt_start = 9,
	  .vt_end = 8,
	  .frag_end = 8,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 10,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_CSG_TERMINATED,
	  .expected_error = { .error_type = BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM },
	},

	/* chunk_count == max_chunks and nr_render_passes_in_flight == 1 and frag_end == 0 */
	{ .vt_start = 1,
	  .vt_end = 0,
	  .frag_end = 0,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 10,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_CSG_TERMINATED,
	  .expected_error = { .error_type = BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM },
	},


	/* chunk_count < max_chunks and nr_render_passes_in_flight > target_in_flight */
	{ .vt_start = 11,
	  .vt_end = 7,
	  .frag_end = 4,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_NULL_CHUNK,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },	},

	/* chunk_count == max_chunks and nr_render_passes_in_flight > target_in_flight */
	{ .vt_start = 11,
	  .vt_end = 7,
	  .frag_end = 4,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 10,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_NULL_CHUNK,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },	},

	/* chunk_count < max_chunks and nr_render_passes_in_flight > target_in_flight and frag_end == 0 */
	{ .vt_start = 20,
	  .vt_end = 0,
	  .frag_end = 0,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_NULL_CHUNK,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },	},

	/* Invalid value for nr_render_passes_in_flight (== 0) */
	{ .vt_start = 9,
	  .vt_end = 9,
	  .frag_end = 9,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_CSG_TERMINATED,
	  .expected_error = { .error_type = BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM },
	},

	/* Invalid value for nr_render_passes_in_flight (== 0) and vt_start = 0 */
	{ .vt_start = 0,
	  .vt_end = 0,
	  .frag_end = 0,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_CSG_TERMINATED,
	  .expected_error = { .error_type = BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM },
	},

	/* Invalid value for gpu_heap_va */
	{ .use_invalid_gpu_heap_va = true,
	  .vt_start = 9,
	  .vt_end = 7,
	  .frag_end = 6,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_CSG_TERMINATED,
	  .expected_error = { .error_type = BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM },
	},

	/* Invalid value for counters comprising Heap statistics, vt_start < vt_end < frag_end */
	{ .vt_start = 1,
	  .vt_end = 2,
	  .frag_end = 4,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_CSG_TERMINATED,
	  .expected_error = { .error_type = BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM },
	},

	/* Invalid value for counters comprising Heap statistics, vt_end < frag_end */
	{ .vt_start = 8,
	  .vt_end = 5,
	  .frag_end = 6,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_CSG_TERMINATED,
	  .expected_error = { .error_type = BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM },
	},

	/* Invalid value for counters comprising Heap statistics, vt_start < vt_end */
	{ .vt_start = 1,
	  .vt_end = 5,
	  .frag_end = 4,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_CSG_TERMINATED,
	  .expected_error = { .error_type = BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM },
	},

	/* Invalid value for counters comprising Heap statistics, vt_start == vt_end */
	{ .vt_start = 5,
	  .vt_end = 5,
	  .frag_end = 4,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .expected_action = OOM_EVENT_CSG_TERMINATED,
	  .expected_error = { .error_type = BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM },
	},

	/* Additional fixtures with csi_handler_flags set to BASE_CSF_TILER_OOM_EXCEPTION_FLAG,
	 * which changes some expected actions (compared to without the flag) to incremental
	 * rendering, i.e
	 *  OOM_EVENT_CSG_TERMINATED / OOM_EVENT_NULL_CHUNK => OOM_EVENT_INCREMENTAL_RENDER
	 * Note: some cases remain unchanged as incremental rendering has other conditions.
	 */

	/* chunk_count < max_chunks and nr_render_passes_in_flight < target_in_flight */
	{ .vt_start = 9,
	  .vt_end = 7,
	  .frag_end = 6,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .csi_handler_flags = BASE_CSF_TILER_OOM_EXCEPTION_FLAG,
	  .expected_action = OOM_EVENT_NEW_CHUNK_ALLOCATED,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },
	},

	/* chunk_count == max_chunks and
	 * nr_render_passes_in_flight < target_in_flight and pending_frag_count == 0
	 */
	{
	 .vt_start = 6,
	 .vt_end = 4,
	 .frag_end = 4,
	 .target_in_flight = 5,
	 .max_chunks = 10,
	 .initial_chunks = 10,
	 .chunk_size_in_pages = 1,
	 .csi_handler_flags = BASE_CSF_TILER_OOM_EXCEPTION_FLAG,
	 .expected_action = OOM_EVENT_INCREMENTAL_RENDER,
	 .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },
	},

	/* chunk_count == max_chunks and nr_render_passes_in_flight < target_in_flight */
	{ .vt_start = 9,
	  .vt_end = 7,
	  .frag_end = 6,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 10,
	  .chunk_size_in_pages = 1,
	  .csi_handler_flags = BASE_CSF_TILER_OOM_EXCEPTION_FLAG,
	  .expected_action = OOM_EVENT_NULL_CHUNK,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },
	},

	/* chunk_count < max_chunks and nr_render_passes_in_flight == target_in_flight */
	{ .vt_start = 9,
	  .vt_end = 5,
	  .frag_end = 4,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 3,
	  .csi_handler_flags = BASE_CSF_TILER_OOM_EXCEPTION_FLAG,
	  .expected_action = OOM_EVENT_NEW_CHUNK_ALLOCATED,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },
	},

	/* chunk_count == max_chunks and nr_render_passes_in_flight == 1 */
	{ .vt_start = 9,
	  .vt_end = 8,
	  .frag_end = 8,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 10,
	  .chunk_size_in_pages = 1,
	  .csi_handler_flags = BASE_CSF_TILER_OOM_EXCEPTION_FLAG,
	  .expected_action = OOM_EVENT_INCREMENTAL_RENDER,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },
	},

	/* chunk_count == max_chunks and nr_render_passes_in_flight == 1 and frag_end == 0 */
	{ .vt_start = 1,
	  .vt_end = 0,
	  .frag_end = 0,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 10,
	  .chunk_size_in_pages = 1,
	  .csi_handler_flags = BASE_CSF_TILER_OOM_EXCEPTION_FLAG,
	  .expected_action = OOM_EVENT_INCREMENTAL_RENDER,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },
	},

	/* chunk_count < max_chunks and nr_render_passes_in_flight > target_in_flight */
	{ .vt_start = 11,
	  .vt_end = 7,
	  .frag_end = 4,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .csi_handler_flags = BASE_CSF_TILER_OOM_EXCEPTION_FLAG,
	  .expected_action = OOM_EVENT_NULL_CHUNK,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },	},

	/* chunk_count < max_chunks and
	 * nr_render_passes_in_flight > target_in_flight and frag_end == 0
	 */
	{ .vt_start = 20,
	  .vt_end = 0,
	  .frag_end = 0,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .csi_handler_flags = BASE_CSF_TILER_OOM_EXCEPTION_FLAG,
	  .expected_action = OOM_EVENT_INCREMENTAL_RENDER,
	  .expected_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = {1, 1, 1, 1, 1, 1, 1},
	  },	},

	/* Invalid value for vt_start == vt_end */
	{ .vt_start = 9,
	  .vt_end = 9,
	  .frag_end = 9,
	  .target_in_flight = 5,
	  .max_chunks = 10,
	  .initial_chunks = 6,
	  .chunk_size_in_pages = 1,
	  .csi_handler_flags = BASE_CSF_TILER_OOM_EXCEPTION_FLAG,
	  .expected_action = OOM_EVENT_CSG_TERMINATED,
	  .expected_error = { .error_type = BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM },
	},
};

/* All of these tests share the same userdata fixture */
static int csf_tiler_heap_oom_event_pretest(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;

	fix->extra_funcs_data =
		(void *)&heap_settings[suite->fixture_index % NELEMS(heap_settings)];
	return 0; /* success */
}

static int send_values(struct mali_utf_suite *const suite, unsigned int ctx_id,
		       uint64_t gpu_heap_va, uint64_t gpu_queue_va)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	const test_fixture *const test_fix = fix->extra_funcs_data;
	int err;

	mali_utf_logdbg("Sending values to kernel space\n");

	/* Send Global ID of the Base context */
	mali_utf_loginf(BASE_CTX_ID "=%d\n", ctx_id);
	err = kutf_test_helpers_userdata_send_named_u64(suite, BASE_CTX_ID, ctx_id);

	/* Send GPU VA of the Heap context */
	if (!err) {
		mali_utf_loginf(GPU_HEAP_VA "=%" PRIx64 "\n", gpu_heap_va);
		err = kutf_test_helpers_userdata_send_named_u64(suite, GPU_HEAP_VA, gpu_heap_va);
	}

	/* Send size of the Heap chunk */
	if (!err) {
		mali_utf_loginf(HEAP_CHUNK_SIZE "=%x\n", CHUNK_SIZE);
		err = kutf_test_helpers_userdata_send_named_u64(suite, HEAP_CHUNK_SIZE, CHUNK_SIZE);
	}

	/* Send GPU VA of the GPU queue */
	if (!err) {
		mali_utf_loginf(GPU_QUEUE_VA "=%" PRIx64 "\n", gpu_queue_va);
		err = kutf_test_helpers_userdata_send_named_u64(suite, GPU_QUEUE_VA, gpu_queue_va);
	}

	/* Send number of vertex/tiling operations which have started on the Heap */
	if (!err) {
		mali_utf_loginf(HEAP_VT_START "=%u\n", test_fix->vt_start);
		err = kutf_test_helpers_userdata_send_named_u64(suite, HEAP_VT_START,
								test_fix->vt_start);
	}

	/* Send number of vertex/tiling operations which have completed on the Heap */
	if (!err) {
		mali_utf_loginf(HEAP_VT_END "=%u\n", test_fix->vt_end);
		err = kutf_test_helpers_userdata_send_named_u64(suite, HEAP_VT_END,
								test_fix->vt_end);
	}

	/* Send number of fragment operations which have completed on the Heap */
	if (!err) {
		mali_utf_loginf(HEAP_FRAG_END "=%u\n", test_fix->frag_end);
		err = kutf_test_helpers_userdata_send_named_u64(suite, HEAP_FRAG_END,
								test_fix->frag_end);
	}

	return err;
}

static void csf_tiler_heap_oom_event_test_error_cb(void *user_data,
						   struct base_gpu_queue_group_error *error_data)
{
	struct base_gpu_queue_group_error *const reported_error = user_data;
	*reported_error = *error_data;
}

static void check_dump_on_fault(struct mali_utf_suite *suite,
				struct base_gpu_queue_group_error *reported_error, int dof_fd,
				unsigned int ctx_id)
{
	uint32_t tgid = 0;
	uint32_t df_ctx_id = 0;
	uint32_t _err = 0;
	char buf[256];
	ssize_t num_bytes;
	int ret;
	const struct kutf_fixture_data *fix = suite->fixture;
	const struct test_fixture *test_fix = fix->extra_funcs_data;

	/* Only invalid OoM events would generate a fault */
	if (test_fix->expected_action != OOM_EVENT_CSG_TERMINATED)
		return;

	num_bytes = read(dof_fd, buf, sizeof(buf));
	if (num_bytes < 0) {
		mali_utf_test_fail("Failed to read fault info from csf_fault debugfs file");
		return;
	}

	ret = sscanf(buf, "%u_%u_%u", &tgid, &df_ctx_id, &_err);
	MALI_UTF_ASSERT_INT_EQ_M(ret, 3, "Unexpected data read from csf_fault file");
	MALI_UTF_ASSERT_UINT_EQ_M(ctx_id, df_ctx_id, "Unexpected ctx id");
	MALI_UTF_ASSERT_UINT_EQ_M(_err, DF_TILER_OOM, "Unexpected error code");

	/* Error callback shouldn't get called until Kbase is notified that dumping
	 * is complete
	 */
	MALI_UTF_ASSERT_UINT_EQ_M(reported_error->error_type,
				  BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
				  "Unexpectedly notified of the OOM fault");

	write(dof_fd, "done", 4);

	/* Wait for the error callback to get invoked */
	mali_tpi_sleep_ns(2E9, true);
}

static void csf_tiler_heap_oom_event_test_common(struct mali_utf_suite *suite, bool test_dof)
{
	base_context ctx;
	base_gpu_command_queue_group *group = NULL;
	base_gpu_command_queue *queue = NULL;
	base_tiler_heap heap;
	base_mem_handle mem_hdl = BASE_MEM_INVALID_HANDLE;
	gpu_buffer *ptr_buf_descr = NULL;
	/* The default value matches the expected value when no error was reported */
	struct base_gpu_queue_group_error reported_error = {
		.error_type = BASE_GPU_QUEUE_GROUP_ERROR_FATAL_COUNT,
		.padding = { 1, 1, 1, 1, 1, 1, 1 },
	};

	struct kutf_fixture_data *const fix = suite->fixture;
	const test_fixture *test_fix = fix->extra_funcs_data;
	int dof_fd = -1;

	bool success = base_context_init(&ctx, BASE_CONTEXT_CSF_EVENT_THREAD);
	MALI_UTF_ASSERT_EX_M(success, "Failed to create context");

	/* Compatibility check for incremental rendering, skip it if feature not supported */
	if ((test_fix->csi_handler_flags & BASE_CSF_TILER_OOM_EXCEPTION_FLAG) &&
	    !base_context_supports_incremental_render(&ctx)) {
		mali_utf_test_skip_msg("Incremantal rendering not supported\n");
		base_context_term(&ctx);
		return;
	}

	if (test_dof) {
		char fname[256];

		snprintf(fname, sizeof(fname), KBASE_DEBUGFS_DIR_PREFIX "%u/csf_fault", 0);
		dof_fd = open(fname, O_RDWR);
		if (dof_fd < 0)
			mali_utf_test_fail("Failed to open %s", fname);
	}

	char saved_power_policy[256] = { '\0' };

	mali_error err = base_test_get_power_policy(saved_power_policy, NELEMS(saved_power_policy));

	if (mali_error_is_error(err)) {
		mali_utf_test_fail("Could not get existing power policy");
		saved_power_policy[0] = '\0';
	} else {
		err = base_test_set_power_policy("always_on");
		if (mali_error_is_error(err)) {
			mali_utf_test_fail("Could not set power policy 'always_on'");
			saved_power_policy[0] = '\0';
		}
	}

	if (mali_error_no_error(err)) {
		err = base_mem_jit_init(&ctx, JIT_REGION_VA_PAGES, JIT_MAX_ALLOCATIONS,
					JIT_TRIM_LEVEL, BASE_MEM_GROUP_DEFAULT,
					JIT_REGION_VA_PAGES);

		MALI_UTF_ASSERT_M(mali_error_no_error(err),
				  "Failed to initialize the custom VA zone");
	}

	/* Tiler heap reclaim changes brought in some strict check on buffer descriptor address. So
	 * prepare the buffer-descriptor address ready here for use
	 */
	if (mali_error_no_error(err)) {
		unsigned int flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
				     BASE_MEM_PROT_GPU_WR | BASE_MEM_PROT_GPU_RD | BASE_MEM_SAME_VA;
		mem_hdl = base_mem_alloc(&ctx, 1, 1, 0, flags);
		if (base_mem_handle_is_invalid(mem_hdl)) {
			err = MALI_ERROR_OUT_OF_MEMORY;
		} else {
			ptr_buf_descr = base_mem_cpu_address(mem_hdl, 0);
			memset(ptr_buf_descr, 0, OSU_CONFIG_CPU_PAGE_SIZE);
		}

		MALI_UTF_ASSERT_M(mali_error_no_error(err),
				  "Failed to allocate beffer decriptor page");
	}

	if (mali_error_no_error(err)) {
		base_gpu_command_queue_group_cfg cfg = {
			.cs_min = CSF_NR_INTERFACES,
			.priority = BASE_QUEUE_GROUP_PRIORITY_MEDIUM,
			.tiler_mask = CSF_TILER_MASK,
			.fragment_mask = CSF_FRAGMENT_MASK,
			.compute_mask = CSF_COMPUTE_MASK,
			.tiler_max = CSF_TILER_MAX,
			.fragment_max = CSF_FRAGMENT_MAX,
			.compute_max = CSF_COMPUTE_MAX,
			.csi_handler_flags = test_fix->csi_handler_flags,
			.error_callback = &csf_tiler_heap_oom_event_test_error_cb,
			.user_data = &reported_error,
			.scratch_pages = 0
		};
		group = base_gpu_queue_group_new(&ctx, &cfg);

		if (!group) {
			mali_utf_test_fail("Failed to create gpu queue group");
			err = MALI_ERROR_FUNCTION_FAILED;
		}
	}

	if (mali_error_no_error(err)) {
		queue = base_gpu_queue_new(&ctx, CSF_QUEUE_SIZE, BASE_QUEUE_PRIORITY_MEDIUM,
					   BASE_GPU_QUEUE_FLAG_NONE);

		if (!queue) {
			mali_utf_test_fail("Failed to create gpu queue");
			err = MALI_ERROR_FUNCTION_FAILED;
		}
	}

	if (mali_error_no_error(err)) {
		err = base_gpu_queue_group_bind(group, queue, CSF_INTERFACE_INDEX);
		MALI_UTF_ASSERT_M(mali_error_no_error(err), "Failed to bind queue to group");
	}

	if (mali_error_no_error(err)) {
		/* This would make the kernel driver schedule the queue group,
		 * otherwise the fake interrupt generated by the test module
		 * would not be handled by the kernel driver as it won't expect
		 * an interrupt from firmware for a queue group which hasn't
		 * been started/scheduled.
		 */
		basep_gpu_queue_kick(queue, false);
		mali_tpi_sleep_ns(500 * 1E6, false);

		err = base_tiler_heap_init(&heap, &ctx, test_fix->target_in_flight,
					   test_fix->max_chunks, test_fix->initial_chunks,
					   CHUNK_SIZE, BASE_MEM_GROUP_DEFAULT, ptr_buf_descr);
		MALI_UTF_ASSERT_M(mali_error_no_error(err), "Failed to create the tiler heap");
	}

	if (mali_error_no_error(err)) {
		uint64_t gpu_heap_va = base_tiler_heap_get_address(&heap);
		uint64_t gpu_queue_va = base_mem_gpu_address(queue->basep.buffer_h, 0);
		unsigned int ctx_id = base_get_context_id(&ctx);

		if (test_fix->use_invalid_gpu_heap_va) {
			gpu_heap_va = INVALID_GPU_HEAP_VA;
		}

		/* Send OoM event parameters to kernel space */
		int ret = send_values(suite, ctx_id, gpu_heap_va, gpu_queue_va);
		if (!ret) {
			struct kutf_test_helpers_named_val action;

			if (dof_fd >= 0)
				check_dump_on_fault(suite, &reported_error, dof_fd, ctx_id);

			/* Receive OoM event action */
			ret = kutf_test_helpers_userdata_receive_check_val(
				&action, suite, OOM_EVENT_ACTION, KUTF_TEST_HELPERS_VALTYPE_U64);
			if (!ret) {
				MALI_UTF_ASSERT_UINT_EQ(action.u.val_u64,
							test_fix->expected_action);
				MALI_UTF_ASSERT_UINT_EQ(reported_error.error_type,
							test_fix->expected_error.error_type);
				for (size_t i = 0; i != NELEMS(reported_error.padding); ++i) {
					MALI_UTF_ASSERT_UINT_EQ(
						reported_error.padding[i],
						test_fix->expected_error.padding[i]);
				}
			}
		}

		base_tiler_heap_term(&heap);
	}

	base_mem_free(&ctx, mem_hdl, 1);

	base_gpu_queue_delete(queue);

	base_gpu_queue_group_delete(group);

	if (saved_power_policy[0] != '\0') {
		err = base_test_set_power_policy(saved_power_policy);
		MALI_UTF_ASSERT_M(mali_error_no_error(err), "Failed to restore power_policy '%s'",
				  saved_power_policy);
	}

	if (dof_fd >= 0)
		close(dof_fd);
	base_context_term(&ctx);
}

/**
 * This test tries to verify the handling of Tiler OoM events in Kbase, for
 * both valid and invalid ones.
 *
 * @suite:   Test suite
 */
static void csf_tiler_heap_oom_event_test(struct mali_utf_suite *suite)
{
	csf_tiler_heap_oom_event_test_common(suite, false);
}

/**
 * This test tries to verify the dump on fault functionality in Kbase when an
 * invalid Tiler OoM event occurs.
 * It reuses all the fixtures and code of "oom_event" test.
 * It opens the "csf_fault" debugfs file before triggering the OoM events and
 * waits for some time for kernel to handle the OoM event and if the event is
 * supposed to be an invalid one, it reads the debugfs file to check for the
 * error code and context ID reported.
 *
 * @suite:   Test suite
 */
static void csf_tiler_heap_oom_event_test_with_dof(struct mali_utf_suite *suite)
{
	csf_tiler_heap_oom_event_test_common(suite, true);
}

struct kutf_extra_func_spec kutf_tiler_heap_unit_test_extra_funcs[] = {
	{ CSF_TILER_HEAP_APP_NAME,
	  CSF_TILER_HEAP_SUITE_NAME,
	  UNIT_TEST_0,
	  { csf_tiler_heap_oom_event_pretest, csf_tiler_heap_oom_event_test, NULL } },
	{ CSF_TILER_HEAP_APP_NAME,
	  CSF_TILER_HEAP_SUITE_NAME,
	  UNIT_TEST_1,
	  { csf_tiler_heap_oom_event_pretest, csf_tiler_heap_oom_event_test_with_dof, NULL } },
	{ { 0 } } /* Marks the end of the list */
};
