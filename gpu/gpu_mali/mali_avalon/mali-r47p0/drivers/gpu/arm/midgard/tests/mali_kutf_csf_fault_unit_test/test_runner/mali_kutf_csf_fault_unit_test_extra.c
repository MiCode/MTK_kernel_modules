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

#include <tpi/mali_tpi.h>
#include <utf/include/mali_utf_suite.h>
#include <utf/include/mali_utf_mem.h>
#include <utf/include/mali_utf_resultset.h>
#include <utf/include/mali_utf_helpers.h>
#include <utf/include/mali_utf_main.h>
#include <base/mali_base.h>
#include <base/mali_base_submission_gpu.h>
#include <base/mali_base_global_interface.h>
#include "csf/mali_kbase_csf_registers.h"
#include <hw_access/mali_kbase_hw_access_regmap.h>
#include <hw_access/mali_kbase_hw_access_regmap_legacy.h>

#include "mali_kutf_csf_fault_unit_test_extra.h"
#include "mali_kutf_test_helpers.h"

#include "../mali_kutf_csf_fault_unit_test.h"

#include <fcntl.h>
#include <unistd.h>
#include <helpers/mali_base_helpers_kmsg.h>
#include <helpers/mali_base_helpers_csf.h>
#include <helpers/mali_base_helpers.h>
#include <csf/helpers/mali_base_csf_scheduler_helpers.h>
#include <base/tests/internal/api_tests/helpers/mali_base_helpers_file.h>
#include <base/tests/common/mali_base_user_common.h>
#include "uapi/gpu/arm/midgard/csf/mali_kbase_csf_errors_dumpfault.h"

#define CSF_INTERFACE_INDEX ((uint8_t)0)

struct fault_test_fixture {
	uint8_t fault_exception_type;
	uint32_t fault_exception_data;
	uint64_t fault_info_exception_data;

	uint32_t fault_trace_id0;
	uint32_t fault_trace_id1;
	uint32_t fault_trace_task;

	enum fault_event_action expected_action;
};

/* List of test fixtures.
 * Attempt to provide a range of fault types and fault data
 */
static const struct fault_test_fixture cs_fault_settings[CS_FAULT_SUITE_FIXTURES] = {
	/* Undefined exception types */
	{
		.fault_exception_type = 0x02,
		.fault_exception_data = 0x855551,
		.fault_info_exception_data = 0x800AABBCCDDEEFF1,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},

	{
		.fault_exception_type = 0xFF,
		.fault_exception_data = 0x866661,
		.fault_info_exception_data = 0x111,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},

	/* Defined fault exception types */
	{
		.fault_exception_type =
#if (GPU_ARCH_VERSION_MAJOR == 12 && GPU_ARCH_VERSION_REVISION >= 1)
			CS_FAULT_EXCEPTION_TYPE_KABOOM,
#else
			/* Use undefined exception type for earlier architectures */
		0x03,
#endif
		.fault_exception_data = 0x877771,
		.fault_info_exception_data = 0x8FFEEDDCCBBAA991,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},
	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_CS_RESOURCE_TERMINATED,
		.fault_exception_data = 0x888881,
		.fault_info_exception_data = 0x8112233445566771,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},

	{
		.fault_exception_type =
#if (GPU_ARCH_VERSION_MAJOR == 12 && GPU_ARCH_VERSION_REVISION >= 1)
			CS_FAULT_EXCEPTION_TYPE_CS_BUS_FAULT,
#else
			/* Use undefined exception type for earlier architectures */
		0x04,
#endif
		.fault_exception_data = 0x899991,
		.fault_info_exception_data = 0x8EEDDCCBBAA99881,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},

	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_CS_INHERIT_FAULT,
		.fault_exception_data = 0x899991,
		.fault_info_exception_data = 0x800AABBCCDDEEFF1,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},

	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_INSTR_INVALID_PC,
		.fault_exception_data = 0x8EEEE1,
		.fault_info_exception_data = 0x8112233445566771,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},

	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_INSTR_INVALID_ENC,
		.fault_exception_data = 0x8FFFF1,
		.fault_info_exception_data = 0x800AABBCCDDEEFF1,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},

	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_INSTR_BARRIER_FAULT,
		.fault_exception_data = 0x800111,
		.fault_info_exception_data = 0x8112233445566771,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},


	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_DATA_INVALID_FAULT,
		.fault_exception_data = 0x800221,
		.fault_info_exception_data = 0x111,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},

	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_TILE_RANGE_FAULT,
		.fault_exception_data = 0x800331,
		.fault_info_exception_data = 0x8062F71,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},

	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_ADDR_RANGE_FAULT,
		.fault_exception_data = 0x800441,
		.fault_info_exception_data = 0x800AABBCCDDEEFF1,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},

	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_IMPRECISE_FAULT,
		.fault_exception_data = 0x800551,
		.fault_info_exception_data = 0x8112233445566771,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},

	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_RESOURCE_EVICTION_TIMEOUT,
		.fault_exception_data = 0x811111,
		.fault_info_exception_data = 0x800AABBCCDDEEFF1,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	},
};

static const struct fault_test_fixture trace_info_fault_settings[TRACE_INFO_FAULT_SUITE_FIXTURES] = {
	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_IMPRECISE_FAULT,
		.fault_exception_data = 0x800551,
		.fault_info_exception_data = 0x8112233445566771,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
		.fault_trace_id0 = 0xb0a49d3e,
		.fault_trace_id1 = 0xdebc3f5a,
		.fault_trace_task = 0xd3ac00b,
	},

	{
		.fault_exception_type = CS_FAULT_EXCEPTION_TYPE_RESOURCE_EVICTION_TIMEOUT,
		.fault_exception_data = 0x811111,
		.fault_info_exception_data = 0x800AABBCCDDEEFF1,
		.expected_action = FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
		.fault_trace_id0 = 0xface80b0,
		.fault_trace_id1 = 0xdebfa2c8,
		.fault_trace_task = 0x410d30b,
	},
};

/* All of these tests share the same userdata fixture */
static int cs_fault_event_pretest(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;

	fix->extra_funcs_data =
		(void *)&cs_fault_settings[suite->fixture_index % NELEMS(cs_fault_settings)];
	return 0; /* success */
}

static int trace_info_fault_event_pretest(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;

	fix->extra_funcs_data =
		(void *)&trace_info_fault_settings[suite->fixture_index %
						   NELEMS(trace_info_fault_settings)];
	return 0; /* success */
}

/* Macro definitions to inject GPU faults */
#define FAULT_AS 1
#define VALID 1
#define INVALID 0

#define VALID_GPU_FAULT_DATA                                                                   \
	((uint32_t)((GPU_FAULTSTATUS_ACCESS_TYPE_WRITE << GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT) | \
		    (VALID << GPU_FAULTSTATUS_ADDRESS_VALID_SHIFT) |                           \
		    (VALID << GPU_FAULTSTATUS_JASID_VALID_SHIFT) |                             \
		    (FAULT_AS << GPU_FAULTSTATUS_JASID_SHIFT)))

#define INVALID_GPU_FAULT_DATA                                                                 \
	((uint32_t)((GPU_FAULTSTATUS_ACCESS_TYPE_WRITE << GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT) | \
		    (VALID << GPU_FAULTSTATUS_ADDRESS_VALID_SHIFT) |                           \
		    (INVALID << GPU_FAULTSTATUS_JASID_VALID_SHIFT) |                           \
		    (FAULT_AS << GPU_FAULTSTATUS_JASID_SHIFT)))

#define AS_VALID_FAULT_ADDRESS ((uint64_t)0x0123456789ABCDEF)
#define AS_INVALID_FAULT_ADDRESS ((uint64_t)0xFEDCBA9876543210)

static const struct fault_test_fixture gpu_fault_settings[GPU_FAULT_SUITE_FIXTURES] = {
	/* Fixtures with valid address space ID */
	{
		.fault_exception_type = GPU_FAULTSTATUS_EXCEPTION_TYPE_GPU_BUS_FAULT,
		.fault_exception_data = VALID_GPU_FAULT_DATA >> GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT,
		.fault_info_exception_data = AS_VALID_FAULT_ADDRESS,
	},
	{
		.fault_exception_type = GPU_FAULTSTATUS_EXCEPTION_TYPE_GPU_SHAREABILITY_FAULT,
		.fault_exception_data = VALID_GPU_FAULT_DATA >> GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT,
		.fault_info_exception_data = AS_VALID_FAULT_ADDRESS,
	},
	{
		.fault_exception_type = GPU_FAULTSTATUS_EXCEPTION_TYPE_SYSTEM_SHAREABILITY_FAULT,
		.fault_exception_data = VALID_GPU_FAULT_DATA >> GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT,
		.fault_info_exception_data = AS_VALID_FAULT_ADDRESS,
	},
	{
		.fault_exception_type = GPU_FAULTSTATUS_EXCEPTION_TYPE_GPU_CACHEABILITY_FAULT,
		.fault_exception_data = VALID_GPU_FAULT_DATA >> GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT,
		.fault_info_exception_data = AS_VALID_FAULT_ADDRESS,
	},
	/* Fixtures with invalid address space ID */
	{
		.fault_exception_type = GPU_FAULTSTATUS_EXCEPTION_TYPE_GPU_BUS_FAULT,
		.fault_exception_data = INVALID_GPU_FAULT_DATA >> GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT,
		.fault_info_exception_data = AS_INVALID_FAULT_ADDRESS,
	},
	{
		.fault_exception_type = GPU_FAULTSTATUS_EXCEPTION_TYPE_GPU_SHAREABILITY_FAULT,
		.fault_exception_data = INVALID_GPU_FAULT_DATA >> GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT,
		.fault_info_exception_data = AS_INVALID_FAULT_ADDRESS,
	},
	{
		.fault_exception_type = GPU_FAULTSTATUS_EXCEPTION_TYPE_SYSTEM_SHAREABILITY_FAULT,
		.fault_exception_data = INVALID_GPU_FAULT_DATA >> GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT,
		.fault_info_exception_data = AS_INVALID_FAULT_ADDRESS,
	},
	{
		.fault_exception_type = GPU_FAULTSTATUS_EXCEPTION_TYPE_GPU_CACHEABILITY_FAULT,
		.fault_exception_data = INVALID_GPU_FAULT_DATA >> GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT,
		.fault_info_exception_data = AS_INVALID_FAULT_ADDRESS,
	},
};

static int gpu_fault_pretest(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;

	fix->extra_funcs_data =
		(void *)&gpu_fault_settings[suite->fixture_index % NELEMS(gpu_fault_settings)];
	return 0;
}

/* List of fixtures for firmware CS fatal error. */
static const struct fault_test_fixture fw_cs_fault_settings[FW_CS_FAULT_SUITE_FIXTURES] = {
	{
		.fault_exception_type = CS_FATAL_EXCEPTION_TYPE_FIRMWARE_INTERNAL_ERROR,
		.fault_exception_data = 0x888881,
		.fault_info_exception_data = 0x8112233445566771,
		.expected_action = FAULT_EVENT_CSG_TERMINATED,
	},
};

/**
 * fw_cs_fault_event_pretest() - Set fixture and pause dmesg parser for firmware CS fatal error.
 *
 * @suite:   Test suite
 */
static int fw_cs_fault_event_pretest(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;

	fix->extra_funcs_data =
		(void *)&fw_cs_fault_settings[suite->fixture_index % NELEMS(fw_cs_fault_settings)];

	(void)!system("echo \"Ignore FIRMWARE_INTERNAL_ERROR\" > /dev/kmsg");
	return 0;
}

/**
 * fw_cs_fault_event_posttest() - Restore dmesg parser after firmware CS fatal error.
 *
 * @suite:   Test suite
 */
static void fw_cs_fault_event_posttest(struct mali_utf_suite *suite)
{
	(void)!system("echo \"Finished FIRMWARE_INTERNAL_ERROR\" > /dev/kmsg");
}

/* List of fixtures for firmware CS fatal cs unrecoverable error. */
static const struct fault_test_fixture
	fw_cs_fault_cs_unrecoverable_settings[FW_CS_FAULT_CS_UNRECOVERABLE_SUITE_FIXTURES] = {
		{
			.fault_exception_type = CS_FATAL_EXCEPTION_TYPE_CS_UNRECOVERABLE,
			.fault_exception_data = 0x888441,
			.fault_info_exception_data = 0x8112233445566888,
			.expected_action = FAULT_EVENT_CSG_TERMINATED,
		},
	};

/**
 * fw_cs_fault_cs_unrecoverable_event_pretest() - Set fixture and pause dmesg parser for CS fatal
 *                                                cs unrecoverable error.
 *
 * @suite:   Test suite
 */
static int fw_cs_fault_cs_unrecoverable_event_pretest(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;

	fix->extra_funcs_data = (void *)&fw_cs_fault_cs_unrecoverable_settings
		[suite->fixture_index % NELEMS(fw_cs_fault_cs_unrecoverable_settings)];

	system("echo \"Ignore CS_UNRECOVERABLE\" > /dev/kmsg");
	return 0;
}

/**
 * fw_cs_fault_cs_unrecoverable_event_posttest() - Restore dmesg parser after firmware CS fatal
 *                                                 cs unrecoverable error.
 *
 * @suite:   Test suite
 */
static void fw_cs_fault_cs_unrecoverable_event_posttest(struct mali_utf_suite *suite)
{
	system("echo \"Finished CS_UNRECOVERABLE\" > /dev/kmsg");
}

static int send_fixture_values(struct mali_utf_suite *const suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	const struct fault_test_fixture *const fix_data = fix->extra_funcs_data;
	int err;

	mali_utf_logdbg("Sending fault values to kernel space\n");

	mali_utf_loginf(FAULT_EXCEPTION_TYPE "=%d\n", fix_data->fault_exception_type);
	err = kutf_test_helpers_userdata_send_named_u64(suite, FAULT_EXCEPTION_TYPE,
							fix_data->fault_exception_type);

	if (!err) {
		mali_utf_loginf(FAULT_EXCEPTION_DATA "=%" PRIu32 "\n",
				fix_data->fault_exception_data);
		err = kutf_test_helpers_userdata_send_named_u64(suite, FAULT_EXCEPTION_DATA,
								fix_data->fault_exception_data);
	}

	if (!err) {
		mali_utf_loginf(FAULT_INFO_EXCEPTION_DATA "=%" PRIx64 "\n",
				fix_data->fault_info_exception_data);
		err = kutf_test_helpers_userdata_send_named_u64(
			suite, FAULT_INFO_EXCEPTION_DATA, fix_data->fault_info_exception_data);
	}

	return err;
}

static int send_trace_info_fixture_values(struct mali_utf_suite *const suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	const struct fault_test_fixture *const fix_data = fix->extra_funcs_data;
	int err;

	mali_utf_logdbg("Sending trace info fault values to kernel space\n");

	mali_utf_loginf(FAULT_TRACE_ID0 "=%x\n", fix_data->fault_trace_id0);
	err = kutf_test_helpers_userdata_send_named_u64(suite, FAULT_TRACE_ID0,
							fix_data->fault_trace_id0);

	if (!err) {
		mali_utf_loginf(FAULT_TRACE_ID1 "=%x\n", fix_data->fault_trace_id1);
		err = kutf_test_helpers_userdata_send_named_u64(suite, FAULT_TRACE_ID1,
								fix_data->fault_trace_id1);
	}

	if (!err) {
		mali_utf_loginf(FAULT_TRACE_TASK "=%x\n", fix_data->fault_trace_task);
		err = kutf_test_helpers_userdata_send_named_u64(suite, FAULT_TRACE_TASK,
								fix_data->fault_trace_task);
	}

	return err;
}

static int send_values(struct mali_utf_suite *const suite, unsigned int ctx_id,
		       uint64_t gpu_queue_va, bool include_trace_info)
{
	int err;

	mali_utf_logdbg("Sending values to kernel space\n");

	/* Send Global ID of the Base context */
	mali_utf_loginf(BASE_CTX_ID "=%d\n", ctx_id);
	err = kutf_test_helpers_userdata_send_named_u64(suite, BASE_CTX_ID, ctx_id);

	if (!err) {
		mali_utf_loginf(GPU_QUEUE_VA "=%" PRIx64 "\n", gpu_queue_va);
		err = kutf_test_helpers_userdata_send_named_u64(suite, GPU_QUEUE_VA, gpu_queue_va);
	}

	if (!err)
		err = send_fixture_values(suite);

	if (include_trace_info && !err)
		err = send_trace_info_fixture_values(suite);

	return err;
}

/**
 * verify_dmesg_output() - Verify that an expected fault was reported in
 *                         diagnostic message output from the kernel.
 * @f:    Address of the stream to read.
 * @fix:  Address of the test fixture describing the expected fault.
 * @verify_trace_info: Indicates whether trace info should be verified
 *
 * f must have been returned by a previous successful call to
 * base_apitest_kmsg_open().
 */
static void verify_dmesg_output(FILE *const f, const struct fault_test_fixture *fix,
				bool verify_trace_info)
{
	char *msg = NULL;
	size_t msg_length = 0;
	const char *const fault_reported = "CSI:";

	/* Skip any preceding output */
	bool line_found = false;

	while (getline(&msg, &msg_length, f) != -1) {
		if (strstr(msg, fault_reported) != NULL) {
			line_found = true;
			break;
		}
	}

	if (line_found) {
		char buf[32];

		snprintf(buf, sizeof(buf), "0x%x", fix->fault_exception_type);
		MALI_UTF_ASSERT_M(strstr(msg, buf) != NULL,
				  "Failed to find CS_FAULT.EXCEPTION_TYPE");

		snprintf(buf, sizeof(buf), "0x%x", fix->fault_exception_data);
		MALI_UTF_ASSERT_M(strstr(msg, buf) != NULL,
				  "Failed to find CS_FAULT.EXCEPTION_DATA");

		snprintf(buf, sizeof(buf), "0x%" PRIx64, fix->fault_info_exception_data);
		MALI_UTF_ASSERT_M(strstr(msg, buf) != NULL,
				  "Failed to find CS_FAULT_INFO.EXCEPTION_DATA");

		if (verify_trace_info) {
			snprintf(buf, sizeof(buf), "0x%x", fix->fault_trace_id0);
			MALI_UTF_ASSERT_M(strstr(msg, buf) != NULL,
					  "Failed to find CS_FAULT_TRACE_ID0.EXCEPTION_TRACE_ID0");

			snprintf(buf, sizeof(buf), "0x%x", fix->fault_trace_id1);
			MALI_UTF_ASSERT_M(strstr(msg, buf) != NULL,
					  "Failed to find CS_FAULT_TRACE_ID1.EXCEPTION_TRACE_ID1");

			snprintf(buf, sizeof(buf), "0x%x", fix->fault_trace_task);
			MALI_UTF_ASSERT_M(
				strstr(msg, buf) != NULL,
				"Failed to find CS_FAULT_TRACE_TASK.EXCEPTION_TRACE_TASK");
		}
	} else {
		mali_utf_test_fail("Failed to find expected dmesg output");
	}

	free(msg);
}

/**
 * check_dump_on_fault() - Check if the expected context ID and error code was returned
 *                         by kbase for the CS fault event via the "csf_fault" debugfs file.
 *
 * @suite:  Pointer to the test suite.
 * @dof_fd: File descriptor of the "csf_fault" debugfs file.
 * @ctx_id: ID of the context corresponding to the GPU queue for which the CS fault
 *          event was generated.
 */
static void check_dump_on_fault(struct mali_utf_suite *suite, int dof_fd, unsigned int ctx_id)
{
	struct kutf_test_helpers_named_val dummy;
	struct kutf_fixture_data *const fix = suite->fixture;
	const struct fault_test_fixture *const fix_data = fix->extra_funcs_data;

	if ((fix_data->fault_exception_type == CS_FAULT_EXCEPTION_TYPE_CS_RESOURCE_TERMINATED) ||
	    (fix_data->fault_exception_type == CS_FAULT_EXCEPTION_TYPE_CS_INHERIT_FAULT))
		return;

	int error = kutf_test_helpers_userdata_receive_check_val(&dummy, suite, DOF_CHECK_DONE,
								 KUTF_TEST_HELPERS_VALTYPE_U64);
	if (!error) {
		uint32_t tgid = 0, df_ctx_id = 0;
		uint32_t _err = 0;
		char buf[256];
		int ret;
		const ssize_t num_bytes = read(dof_fd, buf, sizeof(buf));

		if (num_bytes < 0) {
			mali_utf_test_fail("Failed to read fault info from csf_fault debugfs file");
			return;
		}

		ret = sscanf(buf, "%u_%u_%u", &tgid, &df_ctx_id, &_err);
		MALI_UTF_ASSERT_INT_EQ_M(ret, 3, "Unexpected data read from csf_fault file");
		MALI_UTF_ASSERT_UINT_EQ_M(ctx_id, df_ctx_id, "Unexpected ctx id");
		MALI_UTF_ASSERT_UINT_EQ_M(_err, DF_CS_FAULT, "Unexpected error code");

		write(dof_fd, "done", 4);
	}
}

static void fault_event_test_common(struct mali_utf_suite *suite, enum fault_test_mode test_mode,
				    bool include_trace_info)
{
	base_context ctx;
	base_gpu_command_queue_group *group = NULL;
	base_gpu_command_queue *queue = NULL;
	FILE *f = 0;
	int dof_fd = -1;

	struct kutf_fixture_data *const fix = suite->fixture;
	const struct fault_test_fixture *fix_data = fix->extra_funcs_data;

	bool success = base_context_init(&ctx, BASE_CONTEXT_CSF_EVENT_THREAD);
	MALI_UTF_ASSERT_EX_M(success, "Failed to create context");

	mali_error err = MALI_ERROR_NONE;

	base_gpu_command_queue_group_cfg cfg = { .cs_min = CSF_NR_INTERFACES,
						 .priority = BASE_QUEUE_GROUP_PRIORITY_MEDIUM,
						 .tiler_mask = CSF_TILER_MASK,
						 .fragment_mask = CSF_FRAGMENT_MASK,
						 .compute_mask = CSF_COMPUTE_MASK,
						 .tiler_max = CSF_TILER_MAX,
						 .fragment_max = CSF_FRAGMENT_MAX,
						 .compute_max = CSF_COMPUTE_MAX,
						 .csi_handler_flags =
							 BASE_CSF_EXCEPTION_HANDLER_FLAG_NONE,
						 .error_callback = NULL,
						 .user_data = NULL,
						 .scratch_pages = 0 };
	group = base_gpu_queue_group_new(&ctx, &cfg);

	if (!group) {
		mali_utf_test_fail("Failed to create gpu queue group");
		err = MALI_ERROR_FUNCTION_FAILED;
	} else {
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

		f = base_apitest_kmsg_open(0, SEEK_END);
		if (!f)
			err = MALI_ERROR_FUNCTION_FAILED;
	}

	if (mali_error_no_error(err)) {
		if (test_mode == FAULT_TEST_DOF) {
			char fname[256];

			snprintf(fname, sizeof(fname), KBASE_DEBUGFS_DIR_PREFIX "%u/csf_fault", 0);
			dof_fd = open(fname, O_RDWR);
			if (dof_fd < 0) {
				mali_utf_test_fail("Failed to open %s", fname);
				err = MALI_ERROR_FUNCTION_FAILED;
			}
		}
	}

	if (mali_error_no_error(err)) {
		uint64_t gpu_queue_va = base_mem_gpu_address(queue->basep.buffer_h, 0);
		unsigned int ctx_id = base_get_context_id(&ctx);

		/* Send fault event parameters to kernel space
		 */
		int error = send_values(suite, ctx_id, gpu_queue_va, include_trace_info);

		if (!error) {
			struct kutf_test_helpers_named_val action;

			if (dof_fd >= 0)
				check_dump_on_fault(suite, dof_fd, ctx_id);

			/* Receive fault event action */
			error = kutf_test_helpers_userdata_receive_check_val(
				&action, suite, FAULT_EVENT_ACTION, KUTF_TEST_HELPERS_VALTYPE_U64);
			if (!error) {
				/* When group suspend is also involved the
				 * comparison cannot be done as there is no
				 * certainity, the group could go off slot
				 * before the interrupt for fault event is
				 * manually generated.
				 */
				if (test_mode != FAULT_TEST_SUSPEND) {
					MALI_UTF_ASSERT_UINT_EQ(action.u.val_u64,
								fix_data->expected_action);

					verify_dmesg_output(f, fix_data, include_trace_info);
				} else if (base_apitest_kmsg_find_msg(f, "Call trace:"))
					mali_utf_test_fail("Kernel warning message generated");
			}
		}

		fclose(f);
		if (dof_fd >= 0)
			close(dof_fd);
	}

	base_gpu_queue_delete(queue);
	base_gpu_queue_group_delete(group);
	base_context_term(&ctx);
}

static void gpu_bus_fault_error_cb(void *user_data, struct base_gpu_queue_group_error *error_data)
{
	struct base_gpu_queue_group_error_fatal_payload *cb_data = user_data;

	if (error_data->error_type != BASE_GPU_QUEUE_GROUP_ERROR_FATAL)
		mali_utf_test_warn("Unexpected error_type:%u is ignored", error_data->error_type);
	else
		*cb_data = error_data->payload.fatal_group;
}

static bool check_result(struct base_gpu_queue_group_error_fatal_payload *cb_data,
			 struct base_gpu_queue_group_error_fatal_payload *result)
{
	if (cb_data->sideband == result->sideband && cb_data->status == result->status &&
	    cb_data->padding == result->padding)
		return true;

	return false;
}

#define FAULT_WAIT_TIME_NS ((uint64_t)1E9) /* 1 sec */
#define SCHED_WAIT_TIME_NS ((uint64_t)500 * 1E6) /* 0.5 sec */
#define PAYLOAD_SIZE sizeof(struct base_gpu_queue_group_error_fatal_payload)
#define NR_GRPS 7 /* One less than the max number of CSG slots */

/**
 * gpu_fault_event_test_internal() - Test function for GPU fault error handling
 *
 * It creates GPU command queue groups per context and injects an internal
 * GPU fault to exercise fault handling in kernel space.
 *
 * It's expected that all groups of all active contexts are terminated
 * and notified of the fault if the address space ID is invalid; otherwise,
 * it is expected that only groups using the faulting address space are
 * terminated and notified.
 *
 * @suite:       Test suite
 * @ctx1:        Base Context 1
 * @ctx2:        Base Context 2
 */

static void gpu_fault_event_test_internal(struct mali_utf_suite *const suite, base_context *ctx1,
					  base_context *ctx2)
{
	static const struct base_gpu_queue_group_error_fatal_payload cb_data = {
		.sideband = UINT64_MAX,
		.status = UINT32_MAX,
		.padding = UINT32_MAX,
	};

	static const basep_test_gpu_queue_group_params param = {
		.cs_min = CSF_NR_INTERFACES,
		.priority = BASE_QUEUE_GROUP_PRIORITY_MEDIUM,
		.tiler_mask = CSF_TILER_MASK,
		.fragment_mask = CSF_FRAGMENT_MASK,
		.compute_mask = CSF_COMPUTE_MASK,
		.tiler_max = CSF_TILER_MAX,
		.fragment_max = CSF_FRAGMENT_MAX,
		.compute_max = CSF_COMPUTE_MAX,
		.error_callback = gpu_bus_fault_error_cb,
	};

	struct base_gpu_queue_group_error_fatal_payload cb_data1[NR_GRPS];
	basep_test_gpu_queue_group_params params1[NR_GRPS];

	for (int gr = 0; gr < NR_GRPS; gr++) {
		cb_data1[gr] = cb_data;
		params1[gr] = param;
		params1[gr].error_data = &cb_data1[gr];
	}

	basep_test_single_cs_group groups1[NR_GRPS];
	basep_test_csf_job_resources job_res1 = { { { 0 } } };

	/* Create NR_GRPS groups in ctx1 */
	if (basep_test_csf_jobs_and_groups_init_multi_group_cfgs(ctx1, &job_res1, params1, groups1,
								 NR_GRPS, false)) {
		struct base_gpu_queue_group_error_fatal_payload cb_data2[NR_GRPS];
		basep_test_gpu_queue_group_params params2[NR_GRPS];
		basep_test_csf_job_resources job_res2 = { { { 0 } } };

		for (int gr = 0; gr < NR_GRPS; gr++) {
			cb_data2[gr] = cb_data;
			params2[gr] = param;
			params2[gr].error_data = &cb_data2[gr];
		}

		for (int gr = 0; gr < NR_GRPS; gr++) {
			job_res1.jobs[gr].loop_cnt_val = 0;
			if (basep_test_assemble_gpu_busy_loop_jobs_no_req_resource(
				    &job_res1.jobs[gr]))
				basep_test_enqueue_job_helper(groups1[gr].queue,
							      &job_res1.jobs[gr]);
		}

		basep_test_single_cs_group groups2[NR_GRPS];

		/* Create single cs group2 in ctx2 */
		if (basep_test_csf_jobs_and_groups_init_multi_group_cfgs(ctx2, &job_res2, params2,
									 groups2, NR_GRPS, false)) {
			struct kutf_fixture_data *const fix = suite->fixture;
			const struct fault_test_fixture *fix_data = fix->extra_funcs_data;

			/* Setup expected results */
			struct base_gpu_queue_group_error_fatal_payload
				expected_result1 =
					{
						.sideband =
							fix_data->fault_info_exception_data,
						.status =
							fix_data->fault_exception_type |
							fix_data->fault_exception_data
								<< GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT,
					},
				expected_result2;

			const bool jasid_valid = expected_result1.status &
						 GPU_FAULTSTATUS_JASID_VALID_MASK;

			if (jasid_valid)
				expected_result2 = cb_data;
			else
				expected_result2 = expected_result1;

			for (int gr = 0; gr < NR_GRPS; gr++) {
				job_res2.jobs[gr].loop_cnt_val = 0;
				if (basep_test_assemble_gpu_busy_loop_jobs_no_req_resource(
					    &job_res2.jobs[gr]))
					basep_test_enqueue_job_helper(groups2[gr].queue,
								      &job_res2.jobs[gr]);
			}

			/* This would make the kernel driver
			 * schedule the queue groups, otherwise
			 * the fake interrupt generated by
			 * the test module would not be handled
			 * by the kernel driver as it won't
			 * expect an interrupt from firmware
			 * for a queue group which hasn't been
			 * started/scheduled.
			 */
			for (int gr = 0; gr < NR_GRPS; gr++) {
				basep_gpu_queue_kick(groups1[gr].queue, false);
				basep_gpu_queue_kick(groups2[gr].queue, false);
			}

			/* Wait for kernel scheduler to program
			 * GPU command queue groups on CSG slot.
			 * This will allocate the GPU address
			 * space for groups in the context.
			 * After that, GPU fault worker could
			 * report GPU bus fault for groups.
			 */
			MALI_UTF_ASSERT_UINT_EQ_M(mali_tpi_sleep_ns(SCHED_WAIT_TIME_NS, false),
						  MALI_TPI_SLEEP_STATUS_OK, "Failed to wait sched");

			/* Send fault data to kernel space. */
			if (!send_fixture_values(suite)) {
				struct kutf_test_helpers_named_val action;

				/* Wait for a dummy fault event action to
				 * synchronize with the kernel side of the test.
				 */
				kutf_test_helpers_userdata_receive_check_val(
					&action, suite, FAULT_EVENT_ACTION,
					KUTF_TEST_HELPERS_VALTYPE_U64);

				/* Wait for user event callbacks. */
				MALI_UTF_ASSERT_UINT_EQ_M(
					mali_tpi_sleep_ns(FAULT_WAIT_TIME_NS, false),
					MALI_TPI_SLEEP_STATUS_OK, "Failed to wait sched");

				/* Compare callback data with expected
				 * result.
				 */
				for (int gr = 0; gr < NR_GRPS; gr++) {
					MALI_UTF_ASSERT_M(check_result(&cb_data1[gr],
								       &expected_result1),
							  "Callback not called");
					MALI_UTF_ASSERT_M(
						check_result(&cb_data2[gr], &expected_result2),
						jasid_valid ? "Unexpected callback called" :
								    "Callback not called");
				}
			}
			basep_test_csf_resource_terminate(ctx2, &job_res2, groups2, NR_GRPS);
		}
		basep_test_csf_resource_terminate(ctx1, &job_res1, groups1, NR_GRPS);
	}
}

static void cs_fault_event_test(struct mali_utf_suite *suite)
{
#if CSTD_OS_ANDROID
	CSTD_UNUSED(suite);
	mali_utf_test_skip_msg("Test skipped on Android platform");
#else
	fault_event_test_common(suite, FAULT_TEST_DEFAULT, false);
#endif
}

static void trace_info_fault_event_test(struct mali_utf_suite *suite)
{
#if CSTD_OS_ANDROID
	CSTD_UNUSED(suite);
	mali_utf_test_skip_msg("Test skipped on Android platform");
#else
	if (!base_is_debugfs_supported()) {
		mali_utf_test_skip_msg("Skip test as DEBUGFS is not enabled.");
		return;
	}
	struct kutf_test_helpers_named_val skip_trace_info_test;
	int error = kutf_test_helpers_userdata_receive_check_val(
		&skip_trace_info_test, suite, SKIP_TRACE_INFO_TEST, KUTF_TEST_HELPERS_VALTYPE_U64);

	if (error) {
		mali_utf_test_fail("Failed to retrieve SKIP_TRACE_INFO_TEST value");
		return;
	}

	if (skip_trace_info_test.u.val_u64)
		mali_utf_test_skip_msg("trace_info test skipped due to older GPU");
	else
		fault_event_test_common(suite, FAULT_TEST_DEFAULT, true);
#endif
}

/**
 * cs_fault_event_with_suspend_test - This test tries to verify the handling of
 * CSG suspend operation when a CS fault event is in flight.
 * @suite:   Test suite
 *
 * It reuses all the fixtures and code of "fault" test.
 * The test module enqueues a forceful group suspend operation immediately
 * after raising the fake interrupt for the CS fault event.
 * The dmesg output is checked for the presence of any kernel warning as
 * a criterion to decide whether the test passed or not. If there is a
 * warning then it could be an indication of something wrong on kernel
 * driver side which needs to be investigated.
 */
static void cs_fault_event_with_suspend_test(struct mali_utf_suite *suite)
{
#if CSTD_OS_ANDROID
	CSTD_UNUSED(suite);
	mali_utf_test_skip_msg("Test skipped on Android platform");
#else
	fault_event_test_common(suite, FAULT_TEST_SUSPEND, false);
#endif
}

/**
 * cs_fault_event_with_dof_test - This test tries to verify the dump on fault
 * @suite:   Test suite
 *
 * functionality in Kbase when a CS fault event occurs.
 * It reuses all the fixtures and code of "fault" test.
 * It opens the "csf_fault" debugfs file before injecting the fault and
 * wait for a signal from test module that a fault has been generated and
 * then reads the debugfs file to check for the error code and context ID
 * reported.
 */
static void cs_fault_event_with_dof_test(struct mali_utf_suite *suite)
{
#if CSTD_OS_ANDROID
	CSTD_UNUSED(suite);
	mali_utf_test_skip_msg("Test skipped on Android platform");
#else
	fault_event_test_common(suite, FAULT_TEST_DOF, false);
#endif
}

/**
 * gpu_fault_test - This test creates 2 contexts and GPU command queue groups per context.
 * @suite:   Test suite
 *
 * Then it injects a GPU bus fault (Address Space 1)
 * to exercise fault handling in kernel space.
 *
 * If the address space ID is valid then it's expected that groups of the
 * first context hit GPU bus fault whereas groups of the second context don't.
 * Thus the callback of groups should or shouldn't be called depending on the
 * fault hit.
 *
 * If the address space ID is invalid then it's expected that all groups of
 * all active contexts hit GPU bus fault. Thus the callback of all groups
 * should be called.
 */
static void gpu_fault_test(struct mali_utf_suite *suite)
{
#if CSTD_OS_ANDROID
	CSTD_UNUSED(suite);
	mali_utf_test_skip_msg("Test skipped on Android platform");
#else
	base_context ctx1;

	MALI_UTF_ASSERT_EX(base_context_init(&ctx1, BASE_CONTEXT_CSF_EVENT_THREAD));

	base_context ctx2;

	if (!base_context_init(&ctx2, BASE_CONTEXT_CSF_EVENT_THREAD))
		mali_utf_test_fail("Failed to create context2");
	else {
		gpu_fault_event_test_internal(suite, &ctx1, &ctx2);
		base_context_term(&ctx2);
	}
	base_context_term(&ctx1);
#endif
}

static void fw_cs_fault_error_cb(void *user_data, struct base_gpu_queue_group_error *error_data)
{
	struct base_gpu_queue_group_error_fatal_payload *cb_data = user_data;

	if (error_data->error_type != BASE_GPU_QUEUE_GROUP_ERROR_FATAL)
		mali_utf_test_warn("Unexpected error_type:%u is ignored", error_data->error_type);
	else
		*cb_data = error_data->payload.fatal_group;
}

static void check_fw_cs_fault_result(struct base_gpu_queue_group_error_fatal_payload *cb_data,
				     struct base_gpu_queue_group_error_fatal_payload *result,
				     const char *const err_string)
{
	MALI_UTF_ASSERT_UINT_EQ_M(cb_data->sideband, result->sideband, "%s", err_string);
	MALI_UTF_ASSERT_UINT_EQ_M(cb_data->status, result->status, "%s", err_string);
	MALI_UTF_ASSERT_UINT_EQ_M(cb_data->padding, result->padding, "%s", err_string);
}

struct test_thread_data {
	mali_utf_barrier *barrier;
	struct mali_utf_suite *suite;
	basep_test_single_cs_group *group;
};

static void *send_fault_thread(void *arg)
{
	struct test_thread_data *thread = arg;

	mali_utf_barrier_wait(thread->barrier);

	FILE *f = base_apitest_kmsg_open(0, SEEK_END);

	if (!f)
		return NULL;

	unsigned int ctx_id = base_get_context_id(thread->group->group->basep.ctx);

	/* Wait for basep_test_csf_resource_terminate to be executed and blocked */
	MALI_UTF_ASSERT_UINT_EQ_M(mali_tpi_sleep_ns(((uint64_t)500 * 1E6), false),
				  MALI_TPI_SLEEP_STATUS_OK, "Failed to wait sched");

	int err = send_values(thread->suite, ctx_id, 0, false);

	if (err)
		mali_utf_logerr("SEND CS_FW fatal error FAILED\n");

	/* Receive fault event action */
	struct kutf_test_helpers_named_val action;

	kutf_test_helpers_userdata_receive_check_val(&action, thread->suite, FAULT_EVENT_ACTION,
						     KUTF_TEST_HELPERS_VALTYPE_U64);

	struct kutf_fixture_data *const fix = thread->suite->fixture;
	const struct fault_test_fixture *fix_data = fix->extra_funcs_data;

	MALI_UTF_ASSERT_UINT_EQ(action.u.val_u64, fix_data->expected_action);
	verify_dmesg_output(f, fix_data, false);
	fclose(f);
	return NULL;
}

/**
 * fw_cs_fault_cs_unrecoverable_event_test_internal() - Internal test function to exercise
 *                                                      the handling of CS_UNRECOVERABLE
 *                                                      fatal event.
 * @suite:   Test suite
 * @ctx:    Base Context
 *
 * It creates GPU command queue groups on the context and injects a CS_UNRECOVERABLE
 * CS fatal error to exercise the error handling in Kbase.
 *
 * It's expected that the group is terminated and notified of the fatal fault.
 */
static void fw_cs_fault_cs_unrecoverable_event_test_internal(struct mali_utf_suite *const suite,
							     base_context *ctx)
{
	if (!read_write_register_supported()) {
		mali_utf_test_skip_msg(
			"Skip test because the register dumping interface isn't supported");
		return;
	}

	static const basep_test_gpu_queue_group_params param_ref = {
		.cs_min = CSF_NR_INTERFACES,
		.priority = BASE_QUEUE_GROUP_PRIORITY_MEDIUM,
		.tiler_mask = CSF_TILER_MASK,
		.fragment_mask = CSF_FRAGMENT_MASK,
		.compute_mask = CSF_COMPUTE_MASK,
		.tiler_max = CSF_TILER_MAX,
		.fragment_max = CSF_FRAGMENT_MAX,
		.compute_max = CSF_COMPUTE_MAX,
		.error_callback = NULL,
	};

	basep_test_gpu_queue_group_params param = param_ref;
	basep_test_single_cs_group group;
	basep_test_csf_job_resources job_res = { { { 0 } } };

	if (basep_test_csf_jobs_and_groups_init_multi_group_cfgs(ctx, &job_res, &param, &group, 1,
								 true)) {
		basep_test_csf_enqueue_loop_jobs_noreqres_flush(&job_res, &group, 1,
								INFINITE_LOOP_COUNT);

		basep_test_csf_job_data_wait_job_execution_start(&job_res.jobs[0]);

		if (select_register(GPU_CONTROL_REG(MCU_CONTROL)))
			mali_utf_test_fail("MCU_reset: select failed");
		else {
			if (write_register(MCU_CONTROL_REQ_DISABLE))
				mali_utf_test_fail("MCU_reset: write failed");
			else
				mali_utf_loginf(
					"Disabled MCU to make it unresponsive to CSG term req\n");
		}

		/* Start the thread to trigger the interrupt for CS_UNRECOVERABLE fatal error after
		 * basep_test_csf_resource_terminate() gets blocked in waiting for acknowledgment
		 * of the CSG termination request.
		 */
		mali_utf_barrier thread_barrier;

		MALI_UTF_ASSERT_EX_M(true == mali_utf_test_barrier_init(&thread_barrier, 2),
				     "Failed to create a barrier for thread synchronization");

		struct test_thread_data thread_data = { .barrier = &thread_barrier,
							.suite = suite,
							.group = &group };
		mali_tpi_thread thread;

		MALI_UTF_ASSERT_EX_M(true == mali_tpi_thread_create(&thread, send_fault_thread,
								    &thread_data),
				     "Failed to create a thread");

		/* Parent thread continues bellow */
		mali_utf_barrier_wait(&thread_barrier);
		basep_test_csf_resource_terminate(ctx, &job_res, &group, 1);
		mali_tpi_thread_wait(&thread, NULL);
	}
}

/**
 * fw_cs_fault_cs_unrecoverable_event_test() - Test to exercise handling of CS_UNRECOVERABLE
 *                                             fatal event
 *
 * It creates a context and calls an internal function to exercise error handling.
 *
 * @suite:   Test suite
 */
static void fw_cs_fault_cs_unrecoverable_event_test(struct mali_utf_suite *suite)
{
#if CSTD_OS_ANDROID
	CSTD_UNUSED(suite);
	mali_utf_test_skip_msg("Test skipped on Android platform");
#else
	base_context ctx;

	MALI_UTF_ASSERT_EX(base_context_init(&ctx, BASE_CONTEXT_CSF_EVENT_THREAD));

	fw_cs_fault_cs_unrecoverable_event_test_internal(suite, &ctx);

	base_context_term(&ctx);
#endif
}

#define NR_GRPS_FW_CS 2

/**
 * check_dump_on_fw_cs_fault() - Check if the expected context ID and error code was returned by
 *                               kbase for the firmware internal error event via the "csf_fault"
 *                               debugfs file.
 *
 * @suite:  Pointer to the test suite.
 * @dof_fd: File descriptor of the "csf_fault" debugfs file.
 * @ctx_id: ID of the context corresponding to the GPU queue for which the firmware
 *          internal error event was generated.
 * @cb_data1: Pointer to the structure containing unrecoverable fault error information
 *            associated with the group of 1st context.
 * @cb_data2: Pointer to the structure containing unrecoverable fault error information
 *            associated with the group of 2nd context.
 */
static void check_dump_on_fw_cs_fault(struct mali_utf_suite *suite, int dof_fd, unsigned int ctx_id,
				      struct base_gpu_queue_group_error_fatal_payload *cb_data1,
				      struct base_gpu_queue_group_error_fatal_payload *cb_data2)
{
	struct kutf_test_helpers_named_val dummy;
	int error = kutf_test_helpers_userdata_receive_check_val(&dummy, suite, DOF_CHECK_DONE,
								 KUTF_TEST_HELPERS_VALTYPE_U64);

	if (!error) {
		uint32_t tgid = 0, df_ctx_id = 0;
		uint32_t _err = 0;
		char buf[256];
		int ret;
		const ssize_t num_bytes = read(dof_fd, buf, sizeof(buf));

		if (num_bytes < 0) {
			mali_utf_test_fail("Failed to read fault info from csf_fault debugfs file");
			return;
		}

		ret = sscanf(buf, "%u_%u_%u", &tgid, &df_ctx_id, &_err);
		MALI_UTF_ASSERT_INT_EQ_M(ret, 3, "Unexpected data read from csf_fault file");
		MALI_UTF_ASSERT_UINT_EQ_M(ctx_id, df_ctx_id, "Unexpected ctx id");
		MALI_UTF_ASSERT_UINT_EQ_M(_err, DF_FW_INTERNAL_ERROR, "Unexpected error code");

		/* Wait for a while before checking for the callbacks. Until the dump completion
		 * is signaled, the error callbacks shall not be invoked.
		 */
		mali_tpi_sleep_ns(FAULT_WAIT_TIME_NS, false);

		MALI_UTF_ASSERT_UINT_EQ_M(cb_data1->status, UINT32_MAX,
					  "Group1 unexpectedly notified of FW internal error");
		MALI_UTF_ASSERT_UINT_EQ_M(cb_data2->status, UINT32_MAX,
					  "Group2 unexpectedly notified of FW internal error");

		write(dof_fd, "done", 4);
	}
}

/**
 * fw_cs_fault_event_test_internal() - Internal test function for
 *                                     firmware CS fatal error handling
 * @suite:   Test suite
 * @ctx1:    Base Context 1
 * @ctx2:    Base Context 2
 * @test_dof: Flag to indicate if dump on fault needs to be enabled.
 *
 * It creates GPU command queue groups per context and injects an internal
 * firmware CS fatal to exercise fault handling in kernel space.
 *
 * It's expected that all groups of all contexts are terminated
 * and notified of the fatal fault.
 */
static void fw_cs_fault_event_test_internal(struct mali_utf_suite *const suite, base_context *ctx1,
					    base_context *ctx2, bool test_dof)
{
	static const struct base_gpu_queue_group_error_fatal_payload cb_data = {
		.sideband = UINT64_MAX,
		.status = UINT32_MAX,
		.padding = UINT32_MAX,
	};

	static const basep_test_gpu_queue_group_params param = {
		.cs_min = CSF_NR_INTERFACES,
		.priority = BASE_QUEUE_GROUP_PRIORITY_MEDIUM,
		.tiler_mask = CSF_TILER_MASK,
		.fragment_mask = CSF_FRAGMENT_MASK,
		.compute_mask = CSF_COMPUTE_MASK,
		.tiler_max = CSF_TILER_MAX,
		.fragment_max = CSF_FRAGMENT_MAX,
		.compute_max = CSF_COMPUTE_MAX,
		.error_callback = fw_cs_fault_error_cb,
	};

	struct base_gpu_queue_group_error_fatal_payload cb_data1[NR_GRPS_FW_CS];
	basep_test_gpu_queue_group_params params1[NR_GRPS_FW_CS];

	for (int gr = 0; gr < NR_GRPS_FW_CS; gr++) {
		cb_data1[gr] = cb_data;
		params1[gr] = param;
		params1[gr].error_data = &cb_data1[gr];
	}

	basep_test_single_cs_group groups1[NR_GRPS_FW_CS];
	basep_test_csf_job_resources job_res1 = { { { 0 } } };
	int dof_fd = -1;

	if (test_dof) {
		char fname[256];

		snprintf(fname, sizeof(fname), KBASE_DEBUGFS_DIR_PREFIX "%u/csf_fault", 0);
		dof_fd = open(fname, O_RDWR);
		if (dof_fd < 0)
			mali_utf_test_fail("Failed to open %s", fname);
	}

	/* Create groups in ctx1 */
	if (basep_test_csf_jobs_and_groups_init_multi_group_cfgs(ctx1, &job_res1, params1, groups1,
								 NR_GRPS_FW_CS, true)) {
		struct base_gpu_queue_group_error_fatal_payload cb_data2[NR_GRPS_FW_CS];
		basep_test_gpu_queue_group_params params2[NR_GRPS_FW_CS];
		basep_test_csf_job_resources job_res2 = { { { 0 } } };

		for (int gr = 0; gr < NR_GRPS_FW_CS; gr++) {
			cb_data2[gr] = cb_data;
			params2[gr] = param;
			params2[gr].error_data = &cb_data2[gr];
		}

		for (int gr = 0; gr < NR_GRPS_FW_CS; gr++) {
			job_res1.jobs[gr].loop_cnt_val = 0;
			if (basep_test_assemble_gpu_busy_loop_jobs_no_req_resource(
				    &job_res1.jobs[gr]))
				basep_test_enqueue_job_helper(groups1[gr].queue,
							      &job_res1.jobs[gr]);
		}

		basep_test_single_cs_group groups2[NR_GRPS_FW_CS];

		/* Create single cs group2 in ctx2 */
		if (basep_test_csf_jobs_and_groups_init_multi_group_cfgs(
			    ctx2, &job_res2, params2, groups2, NR_GRPS_FW_CS, true)) {
			/* Setup expected results */
			struct base_gpu_queue_group_error_fatal_payload
				expected_result1 = {
				.sideband = 0,
				.status = GPU_EXCEPTION_TYPE_SW_FAULT_1,
			}, expected_result2;

			expected_result2 = expected_result1;

			for (int gr = 0; gr < NR_GRPS_FW_CS; gr++) {
				job_res2.jobs[gr].loop_cnt_val = 0;
				if (basep_test_assemble_gpu_busy_loop_jobs_no_req_resource(
					    &job_res2.jobs[gr]))
					basep_test_enqueue_job_helper(groups2[gr].queue,
								      &job_res2.jobs[gr]);
			}

			/* This would make the kernel driver
			 * schedule the queue groups, otherwise
			 * the fake interrupt generated by
			 * the test module would not be handled
			 * by the kernel driver as it won't
			 * expect an interrupt from firmware
			 * for a queue group which hasn't been
			 * started/scheduled.
			 */
			for (int gr = 0; gr < NR_GRPS_FW_CS; gr++) {
				basep_gpu_queue_kick(groups1[gr].queue, false);
				basep_gpu_queue_kick(groups2[gr].queue, false);
			}

			/* Wait for kernel scheduler to program
			 * GPU command queue groups on CSG slot.
			 * This will allocate the GPU address
			 * space for groups in the context.
			 * After that, GPU fault worker could
			 * report GPU bus fault for groups.
			 */
			MALI_UTF_ASSERT_UINT_EQ_M(mali_tpi_sleep_ns(SCHED_WAIT_TIME_NS, false),
						  MALI_TPI_SLEEP_STATUS_OK, "Failed to wait sched");

			base_mem_handle handle = groups1[0].queue->basep.buffer_h;
			uint64_t gpu_queue_va = base_mem_gpu_address(handle, 0);
			unsigned int ctx_id = base_get_context_id(ctx1);

			/* Send fault data to kernel space. */
			if (!send_values(suite, ctx_id, gpu_queue_va, false)) {
				struct kutf_test_helpers_named_val action;
				struct kutf_fixture_data *const fix = suite->fixture;
				const struct fault_test_fixture *fix_data = fix->extra_funcs_data;

				if (dof_fd >= 0) {
					check_dump_on_fw_cs_fault(suite, dof_fd, ctx_id, cb_data1,
								  cb_data2);
				}

				/* Receive fault event action */
				if (!kutf_test_helpers_userdata_receive_check_val(
					    &action, suite, FAULT_EVENT_ACTION,
					    KUTF_TEST_HELPERS_VALTYPE_U64)) {
					MALI_UTF_ASSERT_UINT_EQ(action.u.val_u64,
								fix_data->expected_action);
				}

				/* Wait for user event callbacks. */
				MALI_UTF_ASSERT_UINT_EQ_M(
					mali_tpi_sleep_ns(FAULT_WAIT_TIME_NS, false),
					MALI_TPI_SLEEP_STATUS_OK, "Failed to wait sched");

				/* Compare callback data with expected
				* result.
				*/
				for (int gr = 0; gr < NR_GRPS_FW_CS; gr++) {
					check_fw_cs_fault_result(&cb_data1[gr], &expected_result1,
								 "Callback not called");
					check_fw_cs_fault_result(&cb_data2[gr], &expected_result2,
								 "Callback not called");
				}
			}

			basep_test_csf_resource_terminate(ctx2, &job_res2, groups2, NR_GRPS_FW_CS);
		}
		basep_test_csf_resource_terminate(ctx1, &job_res1, groups1, NR_GRPS_FW_CS);
	}

	if (dof_fd >= 0)
		close(dof_fd);
}

static void fw_cs_fault_event_test_common(struct mali_utf_suite *suite, bool test_dof)
{
#if CSTD_OS_ANDROID
	CSTD_UNUSED(suite);
	mali_utf_test_skip_msg("Test skipped on Android platform");
#else
	base_context ctx1;

	MALI_UTF_ASSERT_EX(base_context_init(&ctx1, BASE_CONTEXT_CSF_EVENT_THREAD));

	base_context ctx2;
	if (!base_context_init(&ctx2, BASE_CONTEXT_CSF_EVENT_THREAD))
		mali_utf_test_fail("Failed to create context2");
	else {
		fw_cs_fault_event_test_internal(suite, &ctx1, &ctx2, test_dof);
		base_context_term(&ctx2);
	}

	base_context_term(&ctx1);
#endif
}

/**
 * fw_cs_fault_event_test() - Test for firmware CS fatal
 *
 * It creates two contexts and call internal function to exercise
 * error handling.
 *
 * @suite:   Test suite
 */
static void fw_cs_fault_event_test(struct mali_utf_suite *suite)
{
	fw_cs_fault_event_test_common(suite, false);
}

/**
 * fw_cs_fault_event_test_with_dof() - Test for firmware CS fatal when dump on fault
 *                                     is enabled.
 *
 * It creates two contexts and call internal function to exercise error handling.
 * It opens the "csf_fault" debugfs file before injecting the fault and
 * wait for a signal from test module that a fault has been generated and
 * then reads the debugfs file to check for the error code and context ID
 * reported and also confirms that the error callback has not been invoked.
 *
 * @suite:   Test suite
 */
static void fw_cs_fault_event_test_with_dof(struct mali_utf_suite *suite)
{
	fw_cs_fault_event_test_common(suite, true);
}

struct kutf_extra_func_spec kutf_fault_unit_test_extra_funcs[] = {
	{ CSF_FAULT_APP_NAME,
	  CS_FAULT_SUITE_NAME,
	  CS_FAULT_UNIT_TEST_0,
	  { cs_fault_event_pretest, cs_fault_event_test, NULL } },
	{ CSF_FAULT_APP_NAME,
	  CS_FAULT_SUITE_NAME,
	  CS_FAULT_UNIT_TEST_1,
	  { cs_fault_event_pretest, cs_fault_event_with_suspend_test, NULL } },
	{ CSF_FAULT_APP_NAME,
	  CS_FAULT_SUITE_NAME,
	  CS_FAULT_UNIT_TEST_2,
	  { cs_fault_event_pretest, cs_fault_event_with_dof_test, NULL } },
	{ CSF_FAULT_APP_NAME,
	  GPU_FAULT_SUITE_NAME,
	  GPU_FAULT_UNIT_TEST_0,
	  { gpu_fault_pretest, gpu_fault_test, NULL } },
	{ CSF_FAULT_APP_NAME,
	  FW_CS_FAULT_SUITE_NAME,
	  FW_CS_FAULT_UNIT_TEST_0,
	  { fw_cs_fault_event_pretest, fw_cs_fault_event_test, fw_cs_fault_event_posttest } },
	{ CSF_FAULT_APP_NAME,
	  FW_CS_FAULT_SUITE_NAME,
	  FW_CS_FAULT_UNIT_TEST_1,
	  { fw_cs_fault_cs_unrecoverable_event_pretest, fw_cs_fault_cs_unrecoverable_event_test,
	    fw_cs_fault_cs_unrecoverable_event_posttest } },
	{ CSF_FAULT_APP_NAME,
	  FW_CS_FAULT_SUITE_NAME,
	  FW_CS_FAULT_UNIT_TEST_2,
	  { fw_cs_fault_event_pretest, fw_cs_fault_event_test_with_dof,
	    fw_cs_fault_event_posttest } },
	{ CSF_FAULT_APP_NAME,
	  TRACE_INFO_FAULT_SUITE_NAME,
	  TRACE_INFO_FAULT_UNIT_TEST_0,
	  { trace_info_fault_event_pretest, trace_info_fault_event_test, NULL } },
	{ { 0 } } /* Marks the end of the list */
};
