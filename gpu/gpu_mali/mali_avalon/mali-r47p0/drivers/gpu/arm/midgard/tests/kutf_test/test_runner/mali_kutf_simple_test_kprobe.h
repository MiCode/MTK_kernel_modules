/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2023 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef _KUTF_SIMPLE_TEST_KPROBE_H_
#define _KUTF_SIMPLE_TEST_KPROBE_H_

#include "mali_kutf_test_runner.h"

#define KPROBE_APP_NAME "kprobe"
#define KPROBE_SUITE_NAME "kprobe_suite"
#define KPROBE_TEST_NAME "kprobe_test"

/* Extra test specs for kprobe tests */
extern struct kutf_extra_func_spec kutf_simple_test_kprobe;

#endif /* _KUTF_SIMPLE_TEST_KPROBE_H_ */
