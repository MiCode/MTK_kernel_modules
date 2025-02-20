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
 * @file GPUCORE-37201.c
 *
 * Defect Test for GPUCORE-37201
 */

#include <base/mali_base.h>
#include <base/mali_base_kernel.h>
#include <base/mali_base_context.h>
#include <base/mali_base_ioctl.h>
#include <base/src/mali_base_kbase.h>
#include <base/tests/common/mali_base_user_common.h>
#include <base/tests/internal/api_tests/helpers/mali_base_helpers_kmsg.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utf/mali_utf.h>
#include "mali_kutf_test_helpers.h"

#define WARNING_TEXT "must first be scheduled out to flush GPU caches+tlbs before tearing"

#if !MALI_USE_CSF
/**
 * Main test function for the defect test associated with GPUCORE-37201.
 *
 * With the assistance of a KUTF kernel function, this test does two main things:
 *   - creates a new kbase context that fails it in the kbase_region_tracker_init
 *     function
 *   - checks dmesg if warning is detected in context init sequence
 *
 * Code flow is more or less as follows:
 *   - open /dev/kmsg file
 *   - signal kernel that userspace finished preparation
 *   - create a new failing kbase context
 *   - unblock userspace and check if warning is detected
 *   - clean up
 *
 * Both synchronization (at each step) and data exchange are done via the KUTF
 * data mechanism.
 */
static void GPUCORE37201_test_func(mali_utf_suite *suite)
{
	int err;
	struct kutf_test_helpers_named_val named_val;

	FILE *f_kmsg = base_apitest_kmsg_open(0, SEEK_END);

	if (!f_kmsg)
		mali_utf_test_fail("Failed to open kmsg");

	MALI_UTF_ASSERT_INT_EQ(
		kutf_test_helpers_userdata_send_named_u64(suite, "GPUCORE37201_USERSPACE_READY", 0),
		0);

	/* Waits for kernel space to signal completion */
	err = kutf_test_helpers_userdata_receive_check_val(
		&named_val, suite, "GPUCORE37201_KERNEL_READY", KUTF_TEST_HELPERS_VALTYPE_U64);
	if (err)
		mali_utf_test_fail("Failed to wait for kernel function to complete");

	if (f_kmsg) {
		if (base_apitest_kmsg_find_msg(f_kmsg, WARNING_TEXT))
			mali_utf_test_fail("kernel warning detected");
		(void)fclose(f_kmsg);
	}
}
#endif /* MALI_USE_CSF */

void GPUCORE37201(mali_utf_suite *suite)
{
#if !MALI_USE_CSF
	GPUCORE37201_test_func(suite);
#else
	CSTD_UNUSED(suite);
	mali_utf_test_skip_msg("GPUCORE-37201 defect test only available for JM GPUs.");
#endif /* MALI_USE_CSF */
}
