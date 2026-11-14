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

#ifndef _KUTF_KERNEL_DEFECT_TEST_EXTRA_H_
#define _KUTF_KERNEL_DEFECT_TEST_EXTRA_H_

#include "../mali_kutf_kernel_defect_test.h"
#include "mali_kutf_test_runner.h"
#include <utf/mali_utf.h>

extern struct kutf_extra_func_spec defect_test_GPUCORE27226;
extern struct kutf_extra_func_spec defect_test_GPUCORE35490;
extern struct kutf_extra_func_spec defect_test_GPUCORE37201;
extern struct kutf_extra_func_spec defect_test_GPUCORE37465;
extern struct kutf_extra_func_spec defect_test_GPUCORE39614;

void GPUCORE35490(struct mali_utf_suite *suite);
void GPUCORE37201(struct mali_utf_suite *suite);
void GPUCORE37465(struct mali_utf_suite *suite);
void GPUCORE39614(struct mali_utf_suite *suite);

#endif /* _KUTF_KERNEL_DEFECT_TEST_EXTRA_H_ */
