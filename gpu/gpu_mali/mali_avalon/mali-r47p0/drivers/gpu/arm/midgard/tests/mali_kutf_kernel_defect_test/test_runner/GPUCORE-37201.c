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


void GPUCORE37201(mali_utf_suite *suite)
{
	CSTD_UNUSED(suite);
	mali_utf_test_skip_msg("GPUCORE-37201 defect test only available for JM GPUs.");
}
