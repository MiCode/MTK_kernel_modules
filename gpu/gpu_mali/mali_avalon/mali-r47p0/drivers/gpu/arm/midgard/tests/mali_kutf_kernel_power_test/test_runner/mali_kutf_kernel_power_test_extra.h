/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
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

#ifndef _KUTF_KERNEL_POWER_TEST_EXTRA_H_
#define _KUTF_KERNEL_POWER_TEST_EXTRA_H_

#include <utf/mali_utf.h>
#include <base/mali_base.h>
#include "mali_kutf_test_runner.h"
#include "../mali_kutf_kernel_power_test.h"
#include <base/tests/internal/api_tests/helpers/mali_base_helpers.h>

#include <base/tests/internal/api_tests/helpers/mali_base_helpers_csf.h>
#include <base/tests/internal/api_tests/csf/helpers/mali_base_csf_scheduler_helpers.h>
#include <base/tests/internal/api_tests/csf/mali_base_csf_user.h>

#define PM_BUFFER_SIZE 256

extern struct kutf_extra_func_spec power_test_default_power_cycle;
extern struct kutf_extra_func_spec power_test_default_power_cycle_whilst_wait_enqueued;
extern struct kutf_extra_func_spec power_test_default_power_core_availability_cg0;
extern struct kutf_extra_func_spec power_test_default_power_policy_unit;
extern struct kutf_extra_func_spec power_test_instant_poweroff_power_cycle;
extern struct kutf_extra_func_spec power_test_instant_poweroff_power_cycle_whilst_wait_enqueued;
extern struct kutf_extra_func_spec power_test_instant_poweroff_power_core_availability_cg0;
extern struct kutf_extra_func_spec power_test_instant_poweroff_power_policy_unit;

struct kutf_userspace_power_fixture_data {
	base_context ctx;
	basep_test_single_cs_group group;
	basep_test_csf_job_resources jobs_res;
	char old_pm_poweroff[PM_BUFFER_SIZE];
};

int mali_kutf_kernel_power_test_default_pretest(mali_utf_suite *test_suite);
int mali_kutf_kernel_power_test_instant_poweroff_pretest(mali_utf_suite *test_suite);
int mali_kutf_kernel_power_test_default_poweroff_no_csf_pretest(mali_utf_suite *test_suite);
int mali_kutf_kernel_power_test_instant_poweroff_no_csf_pretest(mali_utf_suite *test_suite);
void mali_kutf_kernel_power_test_teardown(mali_utf_suite *test_suite);
void mali_kutf_kernel_power_test_no_csf_teardown(mali_utf_suite *test_suite);

#endif /* _KUTF_KERNEL_POWER_TEST_EXTRA_H_ */
