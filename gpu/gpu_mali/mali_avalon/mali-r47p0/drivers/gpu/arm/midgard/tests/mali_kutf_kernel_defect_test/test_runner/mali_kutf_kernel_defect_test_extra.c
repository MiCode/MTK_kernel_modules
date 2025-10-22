/*
 * Copyright:
 * ----------------------------------------------------------------------------
 * This confidential and proprietary software may be used only as authorized
 * by a licensing agreement from ARM Limited.
 *      (C) COPYRIGHT 2021-2023 ARM Limited, ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorized copies and
 * copies may only be made to the extent permitted by a licensing agreement
 * from ARM Limited.
 * ----------------------------------------------------------------------------
 */

#include <utf/mali_utf.h>
#include <kutf/kutf_resultset.h>
#include "mali_kutf_test_runner.h"
#include "mali_kutf_kernel_defect_test_extra.h"


struct kutf_extra_func_spec defect_test_GPUCORE27226 = {
	KERNEL_DEFECT_TEST_APP_NAME, /* app_name */
	KERNEL_DEFECT_TEST_SUITE_NAME, /* suite_name */
	KERNEL_DEFECT_GPUCORE_27226, /* test_name */
	{
		NULL, /* pretest */
		NULL, /* midtest */
		NULL /* posttest */
	}
};

struct kutf_extra_func_spec defect_test_GPUCORE35490 = {
	KERNEL_DEFECT_TEST_APP_NAME, /* app_name */
	KERNEL_DEFECT_TEST_SUITE_NAME, /* suite_name */
	KERNEL_DEFECT_GPUCORE_35490, /* test_name */
	{
		NULL, /* pretest */
		GPUCORE35490, /* midtest */
		NULL /* posttest */
	}
};

struct kutf_extra_func_spec defect_test_GPUCORE37201 = {
	KERNEL_DEFECT_TEST_APP_NAME, /* app_name */
	KERNEL_DEFECT_TEST_SUITE_NAME, /* suite_name */
	KERNEL_DEFECT_GPUCORE_37201, /* test_name */
	{
		NULL, /* pretest */
		GPUCORE37201, /* midtest */
		NULL /* posttest */
	}
};

struct kutf_extra_func_spec defect_test_GPUCORE37465 = {
	KERNEL_DEFECT_TEST_APP_NAME, /* app_name */
	KERNEL_DEFECT_TEST_SUITE_NAME, /* suite_name */
	KERNEL_DEFECT_GPUCORE_37465, /* test_name */
	{
		NULL, /* pre-test */
		GPUCORE37465, /* mid-test */
		NULL /* post-test */
	}
};

struct kutf_extra_func_spec defect_test_GPUCORE39614 = {
	KERNEL_DEFECT_TEST_APP_NAME, /* app_name */
	KERNEL_DEFECT_TEST_SUITE_NAME, /* suite_name */
	KERNEL_DEFECT_GPUCORE_39614, /* test_name */
	{
		NULL, /* pretest */
		GPUCORE39614, /* midtest */
		NULL /* posttest */
	}
};
