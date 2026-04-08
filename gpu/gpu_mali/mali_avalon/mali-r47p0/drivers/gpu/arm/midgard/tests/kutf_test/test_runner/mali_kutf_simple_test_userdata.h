/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2017-2023 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef _KUTF_SIMPLE_TEST_USERDATA_H_
#define _KUTF_SIMPLE_TEST_USERDATA_H_

#include "mali_kutf_test_runner.h"

#define USERDATA_APP_NAME "test_app_userdata"
#define USERDATA_SUITE_NAME "simple_suite"

/* Extra test specs for userdata-based tests */
extern struct kutf_extra_func_spec kutf_simple_test_add_two_values;
extern struct kutf_extra_func_spec kutf_simple_test_increasing_str_sizes;
extern struct kutf_extra_func_spec kutf_simple_test_random_str_sizes;

#endif /* _KUTF_SIMPLE_TEST_USERDATA_H_ */
