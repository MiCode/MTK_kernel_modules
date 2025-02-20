// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2020-2023 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#include <linux/delay.h>
#include <linux/math64.h>
#include <linux/module.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#endif

#include "mali_kbase.h"
#include <mali_kbase_reset_gpu.h>

#include <kutf/kutf_suite.h>

#include "backend/gpu/mali_kbase_clk_rate_trace_mgr.h"
#include "csf/ipa_control/mali_kbase_csf_ipa_control.h"
#include "../mali_kutf_csf_ipa_control_unit_test.h"
#include "backend/gpu/mali_kbase_pm_internal.h"

/**
 * NUM_TEST_SESSIONS - Number of sessions to create for tests with more than one
 * session
 */
#define NUM_TEST_SESSIONS ((size_t)2)

/**
 * NUM_OUT_OF_ORDER_UNREGISTER_TEST_SESSIONS - Number of sessions to create for
 * active session iteration test
 */
#define NUM_OUT_OF_ORDER_UNREGISTER_TEST_SESSIONS ((size_t)4)

/**
 * OUT_OF_ORDER_UNREGISTER_TEST_UNREGISTER_INDEX - Index of session to be
 * unregistered for active session iteration test
 */
#define OUT_OF_ORDER_UNREGISTER_TEST_UNREGISTER_INDEX 2

/**
 * QUERY_NUM_ITERS - Number of iterations for query tests
 */
#define QUERY_NUM_ITERS (10)

/*
 * Constants used as performance counters indices.
 */
#define IDX_4 ((u64)4)
#define IDX_5 ((u64)5)
#define IDX_6 ((u64)6)
#define IDX_7 ((u64)7)

#ifndef CONFIG_ANDROID

/**
 * STATUS_RESET - Status flags from the STATUS register of the IPA Control interface.
 */
#define STATUS_RESET ((u32)1 << 9)

#endif /* CONFIG_ANDROID */

#if MALI_UNIT_TEST

/**
 * PERF_COUNTERS_NUM - Number of performance counters to configure
 */
#define PERF_COUNTERS_NUM ((size_t)5)

/**
 * EXCESSIVE_PERF_COUNTERS_NUM - Excessive number of performance counters for a
 * given core type
 */
#define EXCESSIVE_PERF_COUNTERS_NUM (KBASE_IPA_CONTROL_NUM_BLOCK_COUNTERS + 1)

/**
 * EXCESSIVE_NUM_SESSIONS - Excessive number of sessions for kbase_ipa_control
 */
#define EXCESSIVE_NUM_SESSIONS (KBASE_IPA_CONTROL_MAX_SESSIONS + 1)

/**
 * RATE_CHANGE_PERF_COUNTERS_NUM - Number of performance counters for GPU rate
 * change tests
 */
#define RATE_CHANGE_PERF_COUNTERS_NUM ((size_t)2)

/**
 * RATE_CHANGE_NUM_ITERS - Number of iterations for GPU rate change tests
 */
#define RATE_CHANGE_NUM_ITERS (10)

/**
 * RATE_CHANGE_THRESHOLD_NUM - Numerator of the threshold used to compare
 * normalized and raw values in GPU rate change tests.
 *
 * We expect the absolute value of the difference between normalized
 * and raw values to be less than a given threshold:
 *
 * abs(normalized_value - raw_value) / raw_value <= NUM/div
 */
#define RATE_CHANGE_THRESHOLD_NUM ((u64)1)

/**
 * RATE_CHANGE_THRESHOLD_DIV - Divisor of the threshold used to compare
 * normalized and raw values in GPU rate change tests.
 *
 * We expect the absolute value of the difference between normalized
 * and raw values to be less than a given threshold:
 *
 * abs(normalized_value - raw_value) / raw_value <= num/DIV
 */
#define RATE_CHANGE_THRESHOLD_DIV ((u64)100)

/*
 * Constants used as performance counters indices.
 */
#define IDX_8 ((u64)8)
#define IDX_9 ((u64)9)
#define IDX_10 ((u64)10)
#define IDX_11 ((u64)11)
#define IDX_12 ((u64)12)
#define IDX_13 ((u64)13)
#define IDX_15 ((u64)15)
#define IDX_16 ((u64)16)

/*
 * Constants used as GPU rates.
 */
#define GPU_RATE_1KHZ ((u32)1 * 1000)
#define GPU_RATE_10KHZ ((u32)10 * 1000)
#define GPU_RATE_100KHZ ((u32)100 * 1000)
#define GPU_RATE_1MHZ ((u32)1 * 1000 * 1000)
#define GPU_RATE_10MHZ ((u32)10 * 1000 * 1000)
#define GPU_RATE_50MHZ ((u32)50 * 1000 * 1000)
#define GPU_RATE_100MHZ ((u32)100 * 1000 * 1000)
#define GPU_RATE_200MHZ ((u32)200 * 1000 * 1000)
#define GPU_RATE_500MHZ ((u32)500 * 1000 * 1000)
#define GPU_RATE_1GHZ ((u32)1 * 1000 * 1000 * 1000)

#endif /* MALI_UNIT_TEST */

/**
 * PERF_COUNTER_DEF - Macro used to define a performance counter in test
 * configuration
 * @scaler:     Scaling factor by which the counter's value shall be multiplied.
 * @norm:       Indicating whether counter values shall be normalized by GPU
 *              frequency.
 * @block_type: Type of counter block for performance counter.
 * @cnt_idx:    Index of the performance counter inside the block.
 */
#define PERF_COUNTER_DEF(scaler, norm, cnt_idx, block_type)                                     \
	{                                                                                       \
		.scaling_factor = scaler, .gpu_norm = norm, .type = block_type, .idx = cnt_idx, \
	}

/**
 * struct kutf_ipa_control_fixture_data - Fixture data for the test functions.
 * @kbdev:	kbase device for the GPU.
 */
struct kutf_ipa_control_fixture_data {
	struct kbase_device *kbdev;
};

/* KUTF test application pointer for this test */
static struct kutf_application *ipa_control_app;

/**
 * mali_kutf_ipa_control_unit_create_fixture() - Creates the fixture data
 *                          required for all the tests in the fault suite.
 * @context:	KUTF context.
 *
 * Return: Fixture data created on success or NULL on failure
 */
static void *mali_kutf_ipa_control_unit_create_fixture(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data;
	struct kbase_device *kbdev = NULL;

	pr_debug("Creating fixture\n");
	data = kutf_mempool_alloc(&context->fixture_pool, sizeof(*data));
	if (!data)
		return NULL;

	*data = (const struct kutf_ipa_control_fixture_data){ NULL };

	/* Acquire the kbase device */
	pr_debug("Finding device\n");
	kbdev = kbase_find_device(-1);
	if (kbdev == NULL) {
		kutf_test_fail(context, "Failed to find kbase device");
		return NULL;
	}

	kbase_pm_context_active(kbdev);
	data->kbdev = kbdev;

	pr_debug("Created fixture\n");

	return data;
}

/**
 * mali_kutf_ipa_control_unit_remove_fixture() - Destroy fixture data previously
 * created by mali_kutf_ipa_control_unit_create_fixture().
 *
 * @context:             KUTF context.
 */
static void mali_kutf_ipa_control_unit_remove_fixture(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;

	pr_debug("Destroying fixture\n");
	kbase_pm_context_idle(kbdev);
	kbase_release_device(kbdev);
	pr_debug("Destroyed fixture\n");
}

#if MALI_UNIT_TEST
static u64 read_config(struct kbase_device *kbdev, u8 type)
{
	switch (type) {
	case KBASE_IPA_CORE_TYPE_CSHW:
		return kbase_reg_read64(kbdev, IPA_CONTROL_ENUM(SELECT_CSHW));
	case KBASE_IPA_CORE_TYPE_MEMSYS:
		return kbase_reg_read64(kbdev, IPA_CONTROL_ENUM(SELECT_MEMSYS));
	case KBASE_IPA_CORE_TYPE_TILER:
		return kbase_reg_read64(kbdev, IPA_CONTROL_ENUM(SELECT_TILER));
	case KBASE_IPA_CORE_TYPE_SHADER:
		return kbase_reg_read64(kbdev, IPA_CONTROL_ENUM(SELECT_SHADER));
	default:
		WARN(1, "Unknown core type: %u\n", type);
		return 0;
	}
}

static bool counter_enabled(struct kbase_device *kbdev, u8 idx, u8 type)
{
	int i;
	const u64 config = read_config(kbdev, type);

	for (i = 0; i < KBASE_IPA_CONTROL_NUM_BLOCK_COUNTERS; i++) {
		u8 enabled_event_index = (config >> (i * 8)) & 0xFF;

		if (idx == enabled_event_index)
			return true;
	}

	return false;
}
#endif

static u64 read_value_cnt(struct kbase_device *kbdev, u8 type, u8 select_idx)
{
	switch (type) {
	case KBASE_IPA_CORE_TYPE_CSHW:
		return kbase_reg_read64(kbdev, IPA_VALUE_CSHW_OFFSET(select_idx));

	case KBASE_IPA_CORE_TYPE_MEMSYS:
		return kbase_reg_read64(kbdev, IPA_VALUE_MEMSYS_OFFSET(select_idx));

	case KBASE_IPA_CORE_TYPE_TILER:
		return kbase_reg_read64(kbdev, IPA_VALUE_TILER_OFFSET(select_idx));

	case KBASE_IPA_CORE_TYPE_SHADER:
		return kbase_reg_read64(kbdev, IPA_VALUE_SHADER_OFFSET(select_idx));

	default:
		WARN(1, "Unknown core type: %u\n", type);
		return 0;
	}
}

#if MALI_UNIT_TEST
/**
 * mali_kutf_1_client_N_counters_register_test() - Test for 1 client
 *                                                 configuring N counters
 *
 * @context: KUTF context
 *
 * One client subscribes to a set of N performance counters.
 * Verify that SELECT registers in the IPA_CONTROL interface are configured
 * as expected.
 */
static void mali_kutf_1_client_N_counters_register_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_client = NULL;
	int i, ret;
	const struct kbase_ipa_control_perf_counter perf_counters[PERF_COUNTERS_NUM] = {
		PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1, false, IDX_5, KBASE_IPA_CORE_TYPE_MEMSYS),
		PERF_COUNTER_DEF(1, false, IDX_6, KBASE_IPA_CORE_TYPE_TILER),
		PERF_COUNTER_DEF(1, false, IDX_7, KBASE_IPA_CORE_TYPE_SHADER),
		PERF_COUNTER_DEF(1, false, IDX_15, KBASE_IPA_CORE_TYPE_CSHW),
	};

	ret = kbase_ipa_control_register(kbdev, &perf_counters[0], PERF_COUNTERS_NUM, &ipa_client);

	if (ret) {
		kutf_test_fail(context, "failed to register");
	} else {
		if (!ipa_client)
			kutf_test_fail(context, "NULL ipa_client");
	}

	for (i = 0; i < PERF_COUNTERS_NUM; i++) {
		if (!counter_enabled(kbdev, perf_counters[i].idx, perf_counters[i].type))
			kutf_test_fail(context, "Wrong configuration");
	}

	if (!ret) {
		ret = kbase_ipa_control_unregister(kbdev, ipa_client);
		if (ret)
			kutf_test_fail(context, "failed to unregister");
	}
}

/**
 * mali_kutf_N_clients_same_counter_register_test() - Test for N clients configuring
 *                                                    the same counter
 *
 * @context: KUTF context
 *
 * Multiple clients subscribe to the same performance counter.
 * Verify that the SELECT register in the IPA_CONTROL interface is configured
 * as expected and that kbase_ipa_control allows concurrent sessions
 * with different handles.
 */
static void mali_kutf_N_clients_same_counter_register_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_clients[NUM_TEST_SESSIONS] = { NULL };
	int i, ret;
	const struct kbase_ipa_control_perf_counter perf_counter =
		PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW);

	for (i = 0; i < NUM_TEST_SESSIONS; i++) {
		ret = kbase_ipa_control_register(kbdev, &perf_counter, 1, &ipa_clients[i]);
		if (ret) {
			kutf_test_fail(context, "Failed to register");
		} else {
			if (!ipa_clients[i]) {
				kutf_test_fail(context, "NULL ipa_client");
			} else {
				int j;

				for (j = 0; j < i; j++) {
					if (ipa_clients[i] == ipa_clients[j])
						kutf_test_fail(context, "duplicate ipa_client");
				}
			}
		}
	}

	if (!counter_enabled(kbdev, perf_counter.idx, perf_counter.type))
		kutf_test_fail(context, "Wrong configuration");

	for (i = 0; i < NUM_TEST_SESSIONS; i++) {
		ret = kbase_ipa_control_unregister(kbdev, ipa_clients[i]);
		if (ret)
			kutf_test_fail(context, "Failed to unregister");
	}
}

/**
 * mali_kutf_N_clients_M_counters_register_test - Test for N clients
 *                                                configuring multiple counters
 *
 * @context: KUTF context
 *
 * Multiple clients subscribe to multiple performance counters, some of which
 * are common two clients.
 * Verify that SELECT registers in the IPA_CONTROL interface are configured
 * as expected and that kbase_ipa_control allows concurrent sessions
 * with different handles.
 */
static void mali_kutf_N_clients_M_counters_register_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_clients[NUM_TEST_SESSIONS] = { NULL };
	int i, ret;
	const struct kbase_ipa_control_perf_counter
		perf_counters[NUM_TEST_SESSIONS][PERF_COUNTERS_NUM] = {
			{
				PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
				PERF_COUNTER_DEF(1, false, IDX_5, KBASE_IPA_CORE_TYPE_MEMSYS),
				PERF_COUNTER_DEF(1, false, IDX_6, KBASE_IPA_CORE_TYPE_TILER),
				PERF_COUNTER_DEF(1, false, IDX_7, KBASE_IPA_CORE_TYPE_SHADER),
				PERF_COUNTER_DEF(1, false, IDX_15, KBASE_IPA_CORE_TYPE_CSHW),
			},
			{
				PERF_COUNTER_DEF(1, false, IDX_8, KBASE_IPA_CORE_TYPE_CSHW),
				PERF_COUNTER_DEF(1, false, IDX_16, KBASE_IPA_CORE_TYPE_MEMSYS),
				PERF_COUNTER_DEF(1, false, IDX_6, KBASE_IPA_CORE_TYPE_TILER),
				PERF_COUNTER_DEF(1, false, IDX_16, KBASE_IPA_CORE_TYPE_MEMSYS),
				PERF_COUNTER_DEF(1, false, IDX_16, KBASE_IPA_CORE_TYPE_SHADER),
			}
		};

	for (i = 0; i < NUM_TEST_SESSIONS; i++) {
		ret = kbase_ipa_control_register(kbdev, &perf_counters[i][0], PERF_COUNTERS_NUM,
						 &ipa_clients[i]);
		if (ret) {
			kutf_test_fail(context, "Failed to register");
		} else {
			if (!ipa_clients[i]) {
				kutf_test_fail(context, "NULL ipa_client");
			} else {
				int j;

				for (j = 0; j < i; j++) {
					if (ipa_clients[i] == ipa_clients[j])
						kutf_test_fail(context, "duplicate ipa_client");
				}
			}
		}
	}

	for (i = 0; i < NUM_TEST_SESSIONS; i++) {
		int j;

		for (j = 0; j < PERF_COUNTERS_NUM; j++) {
			if (!counter_enabled(kbdev, perf_counters[i][j].idx,
					     perf_counters[i][j].type))
				kutf_test_fail(context, "Wrong configuration");
		}
	}

	for (i = 0; i < NUM_TEST_SESSIONS; i++) {
		ret = kbase_ipa_control_unregister(kbdev, ipa_clients[i]);
		if (ret)
			kutf_test_fail(context, "Failed to unregister");
	}
}

/**
 * mali_kutf_1_client_too_many_counters_register_test() - Test for 1 client configuring
 *                                                        too many counters
 *
 * @context: KUTF context
 *
 * One clients tries to subscribe to more performance counters than available
 * for one particular type of core.
 * Verify that the request is rejected.
 */
static void mali_kutf_1_client_too_many_counters_register_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_client = NULL;
	int ret;
	const struct kbase_ipa_control_perf_counter perf_counters[EXCESSIVE_PERF_COUNTERS_NUM] = {
		PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1, false, IDX_5, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1, false, IDX_6, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1, false, IDX_7, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1, false, IDX_8, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1, false, IDX_9, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1, false, IDX_10, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1, false, IDX_11, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1, false, IDX_12, KBASE_IPA_CORE_TYPE_CSHW),
	};

	ret = kbase_ipa_control_register(kbdev, &perf_counters[0], EXCESSIVE_PERF_COUNTERS_NUM,
					 &ipa_client);

	if (!ret) {
		kutf_test_fail(context, "unexpected success");
		ret = kbase_ipa_control_unregister(kbdev, ipa_client);
		if (ret)
			kutf_test_fail(context, "Failed to unregister");
	}
}

/**
 * mali_kutf_N_clients_incompatible_counters_register_test() - Test for N clients configuring
 *                                                             incompatible counters
 *
 * @context: KUTF context
 *
 * Multiple clients try to subscribe to some performance counters.
 * Their requests are valid if taken in isolation, but are not valid
 * when combined together because they exceed the number of available
 * performance counters.
 * Verify that the last request is rejected, while previous requests succeed.
 */
static void mali_kutf_N_clients_incompatible_counters_register_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_clients[NUM_TEST_SESSIONS] = { NULL };
	int i, ret;
	const struct kbase_ipa_control_perf_counter
		perf_counters[NUM_TEST_SESSIONS][PERF_COUNTERS_NUM] = {
			{
				PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
				PERF_COUNTER_DEF(1, false, IDX_5, KBASE_IPA_CORE_TYPE_CSHW),
				PERF_COUNTER_DEF(1, false, IDX_6, KBASE_IPA_CORE_TYPE_CSHW),
				PERF_COUNTER_DEF(1, false, IDX_7, KBASE_IPA_CORE_TYPE_CSHW),
				PERF_COUNTER_DEF(1, false, IDX_8, KBASE_IPA_CORE_TYPE_CSHW),
			},
			{
				PERF_COUNTER_DEF(1, false, IDX_9, KBASE_IPA_CORE_TYPE_CSHW),
				PERF_COUNTER_DEF(1, false, IDX_10, KBASE_IPA_CORE_TYPE_CSHW),
				PERF_COUNTER_DEF(1, false, IDX_11, KBASE_IPA_CORE_TYPE_CSHW),
				PERF_COUNTER_DEF(1, false, IDX_12, KBASE_IPA_CORE_TYPE_CSHW),
				PERF_COUNTER_DEF(1, false, IDX_13, KBASE_IPA_CORE_TYPE_CSHW),
			}
		};

	for (i = 0; i < NUM_TEST_SESSIONS; i++) {
		ret = kbase_ipa_control_register(kbdev, &perf_counters[i][0], PERF_COUNTERS_NUM,
						 &ipa_clients[i]);

		if (i != (NUM_TEST_SESSIONS - 1)) {
			if (ret) {
				kutf_test_fail(context, "Failed to register");
			} else {
				if (!ipa_clients[i])
					kutf_test_fail(context, "NULL ipa_client");
			}
		} else {
			if (!ret)
				kutf_test_fail(context, "Unexpected success");
		}
	}

	for (i = 0; i < PERF_COUNTERS_NUM; i++) {
		if (!counter_enabled(kbdev, perf_counters[0][i].idx, perf_counters[0][i].type))
			kutf_test_fail(context, "Wrong configuration");
	}

	for (i = 0; i < NUM_TEST_SESSIONS - 1; i++) {
		ret = kbase_ipa_control_unregister(kbdev, ipa_clients[i]);
		if (ret)
			kutf_test_fail(context, "Failed to unregister");
	}
}

/**
 * mali_kutf_too_many_clients_register_test() - Test for too many clients
 *
 * @context: KUTF context
 *
 * Too many clients try to subscribe.
 * Verify that only a valid number of clients subscribe successfully
 * and that only their performance counters are configured.
 */
static void mali_kutf_too_many_clients_register_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_clients[EXCESSIVE_NUM_SESSIONS] = { NULL };
	int i, ret, num_sessions;
	const struct kbase_ipa_control_perf_counter perf_counters[EXCESSIVE_NUM_SESSIONS] = {
		PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1, false, IDX_5, KBASE_IPA_CORE_TYPE_MEMSYS),
		PERF_COUNTER_DEF(1, false, IDX_6, KBASE_IPA_CORE_TYPE_TILER),
		PERF_COUNTER_DEF(1, false, IDX_7, KBASE_IPA_CORE_TYPE_SHADER),
		PERF_COUNTER_DEF(1, false, IDX_8, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1, false, IDX_9, KBASE_IPA_CORE_TYPE_TILER),
		PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_SHADER),
		PERF_COUNTER_DEF(1, false, IDX_11, KBASE_IPA_CORE_TYPE_MEMSYS),
		PERF_COUNTER_DEF(1, false, IDX_5, KBASE_IPA_CORE_TYPE_TILER),
	};

	for (i = 0, num_sessions = 0; i < EXCESSIVE_NUM_SESSIONS; i++) {
		ret = kbase_ipa_control_register(kbdev, &perf_counters[i], 1, &ipa_clients[i]);
		if (!ret)
			num_sessions++;
	}

	if (num_sessions > KBASE_IPA_CONTROL_MAX_SESSIONS)
		kutf_test_fail(context, "Too many sessions succeeded");

	for (i = 0; i < EXCESSIVE_NUM_SESSIONS; i++) {
		if (i < num_sessions) {
			if (!counter_enabled(kbdev, perf_counters[i].idx, perf_counters[i].type))
				kutf_test_fail(context, "Wrong configuration");
		}
	}

	for (i = 0; i < num_sessions; i++)
		ret = kbase_ipa_control_unregister(kbdev, ipa_clients[i]);
}

static void init_register_suite(struct kutf_suite *suite)
{
	unsigned int const filters = suite->suite_default_flags;

	kutf_add_test_with_filters(suite, 0x0, CSF_IPA_CONTROL_REGISTER_UNIT_TEST_0,
				   mali_kutf_1_client_N_counters_register_test, filters);
	kutf_add_test_with_filters(suite, 0x1, CSF_IPA_CONTROL_REGISTER_UNIT_TEST_1,
				   mali_kutf_N_clients_same_counter_register_test, filters);
	kutf_add_test_with_filters(suite, 0x2, CSF_IPA_CONTROL_REGISTER_UNIT_TEST_2,
				   mali_kutf_N_clients_M_counters_register_test, filters);
	kutf_add_test_with_filters(suite, 0x3, CSF_IPA_CONTROL_REGISTER_UNIT_TEST_3,
				   mali_kutf_1_client_too_many_counters_register_test, filters);
	kutf_add_test_with_filters(suite, 0x4, CSF_IPA_CONTROL_REGISTER_UNIT_TEST_4,
				   mali_kutf_N_clients_incompatible_counters_register_test,
				   filters);
	kutf_add_test_with_filters(suite, 0x5, CSF_IPA_CONTROL_REGISTER_UNIT_TEST_5,
				   mali_kutf_too_many_clients_register_test, filters);
}
#endif

/**
 * test_query_values() - Test whether values returned by a query are plausible.
 *
 * @kbdev:         Pointer to kbase device.
 * @context:       KUTF context.
 * @ipa_client:    Session handle for a client of kbase_ipa_control.
 * @perf_counters: Array of performance counters to query.
 * @raw_before:    Array of raw values read before the last query.
 * @raw_after:     Array of raw values read after the last query.
 * @num_values:    Number of values returned by the query.
 *
 * This function performs a single query on behalf of a client and verifies
 * that values returned by the query are plausible.
 *
 * The function can read raw values directly from the IPA Control interface,
 * but that alone isn't sufficient to predict the result of a query since
 * performance counters may be incrementing continuously over time.
 * Moreover, for each performance counter the kbase_ipa_control returns the
 * difference since the last query rather than the actual value read from the
 * counter.
 *
 * The function performs 3 operations:
 *
 * 1) Read raw counter before query
 * 2) Query
 * 3) Read raw counter after query
 *
 * The raw values from the previous query are given as input parameters:
 * they are raw_before (r0) and raw_after (r1). The function performs
 * two more read: one before (r2) and one after (r3) the query (d1).
 *
 *       t0 (time flows downwards)
 *        |
 *        |
 *  r0 -->|
 *        |
 *        |<-- d0
 *        |\
 *  r1 -->| \
 *        |  \
 *        |   }- query result
 *        |  /
 *  r2 -->| /
 *        |/
 *        |<-- d1
 *        |
 *  r3 -->|
 *        |
 *        |
 *        \/
 *        t1
 *
 * The query result obtained at instant d1 represents the difference in the
 * performance counter value between d1 and d0. The raw values provide
 * an upper and a lower limit to that result: the d1-d0 value must be comprised
 * between max = r3 - r0 and a min = r2 - r1.
 *
 * The two raw values read before (r2) and after (r3) the query are output
 * via the output parameters raw_before and raw_after, and can be passed
 * to the function to perform the next iteration.
 */
static void test_query_values(struct kbase_device *kbdev, struct kutf_context *context,
			      void *ipa_client,
			      const struct kbase_ipa_control_perf_counter *perf_counters,
			      u64 *raw_before, u64 *raw_after, size_t num_values)
{
	int i, ret;
	u64 query_result[KBASE_IPA_CONTROL_MAX_COUNTERS],
		raw_before_new_query[KBASE_IPA_CONTROL_MAX_COUNTERS],
		raw_after_new_query[KBASE_IPA_CONTROL_MAX_COUNTERS], prot_time;
	struct kbase_ipa_control_session *session;
	struct kbase_ipa_control_prfcnt *prfcnts;
	struct kbase_ipa_control *ipa_ctrl;

	ipa_ctrl = &kbdev->csf.ipa_control;
	session = (struct kbase_ipa_control_session *)ipa_client;
	prfcnts = session->prfcnts;

	for (i = 0; i < num_values; i++) {
		raw_before_new_query[i] =
			read_value_cnt(kbdev, prfcnts[i].type, prfcnts[i].select_idx);
	}
	msleep(50);

	ret = kbase_ipa_control_query(kbdev, ipa_client, &query_result[0], 1, &prot_time);
	if (ret < 0)
		kutf_test_fail(context, "Failed to query");
	msleep(50);

	for (i = 0; i < num_values; i++) {
		raw_after_new_query[i] =
			read_value_cnt(kbdev, prfcnts[i].type, prfcnts[i].select_idx);
	}
	msleep(50);

	for (i = 0; i < num_values; i++) {
		u64 r0, r1, r2, r3, diff, min, max;

		r0 = raw_before[i];
		r1 = raw_after[i];
		r2 = raw_before_new_query[i];
		if (perf_counters[i].gpu_norm)
			query_result[i] *= ipa_ctrl->cur_gpu_rate;
		diff = div64_u64(query_result[i], perf_counters[i].scaling_factor);
		r3 = raw_after_new_query[i];

		min = r2 - r1;
		max = r3 - r0;

		if ((diff < min) || (diff > max))
			kutf_test_fail(context, "Query result beyond limits");

		raw_before[i] = r2;
		raw_after[i] = r3;
	}
}

/**
 * mali_kutf_1_client_1_counter_query_test() - Test for 1 client querying
 *                                             1 counter
 *
 * @context: KUTF context
 *
 * One client queries a counter multiple times.
 * Verify that the values reported by kbase_ipa_control are plausible.
 */
static void mali_kutf_1_client_1_counter_query_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_client;
	int i, ret;
	u64 raw_before, raw_after;
	const struct kbase_ipa_control_perf_counter perf_counter =
		PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW);

	raw_before = 0;

	ret = kbase_ipa_control_register(kbdev, &perf_counter, 1, &ipa_client);

	if (ret) {
		kutf_test_fail(context, "failed to register");
	} else {
		if (!ipa_client)
			kutf_test_fail(context, "NULL ipa_client");
	}
	msleep(50);

	raw_after = read_value_cnt(kbdev, perf_counter.type, 0);
	msleep(50);

	if (!ret) {
		for (i = 0; i < QUERY_NUM_ITERS; i++)
			test_query_values(kbdev, context, ipa_client, &perf_counter, &raw_before,
					  &raw_after, 1);
	}

	ret = kbase_ipa_control_unregister(kbdev, ipa_client);
	if (ret)
		kutf_test_fail(context, "Failed to unregister");
}

/**
 * mali_kutf_N_clients_same_counter_query_test() - Test for N clients querying
 *                                                 the same counter
 *
 * @context: KUTF context
 *
 * Multiple clients query the same performance counter multiple times at
 * different times.
 * Verify that the values reported by kbase_ipa_control are plausible.
 */
static void mali_kutf_N_clients_same_counter_query_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_clients[NUM_TEST_SESSIONS];
	int i, ret;
	u64 raw_before[NUM_TEST_SESSIONS], raw_after[NUM_TEST_SESSIONS];
	const struct kbase_ipa_control_perf_counter perf_counters[NUM_TEST_SESSIONS] = {
		PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1000000, true, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
	};

	for (i = 0; i < NUM_TEST_SESSIONS; i++) {
		raw_before[i] = 0;

		ret = kbase_ipa_control_register(kbdev, &perf_counters[i], 1, &ipa_clients[i]);

		if (ret) {
			kutf_test_fail(context, "failed to register");
		} else {
			if (!ipa_clients[i])
				kutf_test_fail(context, "NULL ipa_client");
		}
		msleep(50);

		raw_after[i] = read_value_cnt(kbdev, perf_counters[i].type, 0);
		msleep(50);
	}

	for (i = 0; i < QUERY_NUM_ITERS; i++) {
		int j;

		for (j = 0; j < NUM_TEST_SESSIONS; j++) {
			test_query_values(kbdev, context, ipa_clients[j], &perf_counters[j],
					  &raw_before[j], &raw_after[j], 1);
		}
	}

	for (i = 0; i < NUM_TEST_SESSIONS; i++) {
		ret = kbase_ipa_control_unregister(kbdev, ipa_clients[i]);
		if (ret)
			kutf_test_fail(context, "Failed to unregister");
	}
}

/**
 * mali_kutf_query_with_gpu_power_cycle_test() - Test for a Client querying
 *                                               the counter during the GPU
 *                                               power cycle.
 *
 * @context: KUTF context
 *
 * A client makes multiple queries for the same performance counter at
 * different times, which includes querying when the GPU is powered off and
 * after GPU has been powered up again.
 * Verify that the values reported by kbase_ipa_control are plausible.
 */
static void mali_kutf_query_with_gpu_power_cycle_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_client = NULL;
	int i, ret;
	u64 query_result = 0, second_query_result = U64_MAX, prot_time;
	u64 raw_before, raw_after;
	const struct kbase_ipa_control_perf_counter perf_counter =
		PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW);

#ifdef CONFIG_OF
	if (of_machine_is_compatible("arm,juno") <= 0) {
		kutf_test_skip_msg(context, "Test is supported only on Juno");
		return;
	}
#endif
	ret = kbase_ipa_control_register(kbdev, &perf_counter, 1, &ipa_client);
	if (ret) {
		kutf_test_fail(context, "failed to register");
		return;
	}

	if (!ipa_client) {
		kutf_test_fail(context, "NULL ipa_client");
		return;
	}

	raw_before = read_value_cnt(kbdev, perf_counter.type, 0);
	/* Wait for some time while GPU is active */
	msleep(50);
	raw_after = read_value_cnt(kbdev, perf_counter.type, 0);

	/* To release the pm active reference taken in the fixture create
	 * function.
	 */
	kbase_pm_context_idle(kbdev);
	/* Turn off the GPU now */
	kbase_csf_scheduler_pm_suspend(kbdev);
	/* Wait for the GPU to get powered down */
	kbase_pm_wait_for_gpu_power_down(kbdev);

	/* Juno platform doesn't actually turn down the GPU power unlike
	 * customer's real platform. This might result in the retention of
	 * IPA registers, which is undesirable for this test.
	 *
	 * Juno has the sysctl register for pin GPU reset.
	 */
	if (kbase_prepare_to_reset_gpu(kbdev, RESET_FLAGS_NONE)) {

		kbase_reset_gpu(kbdev);

		if (kbase_reset_gpu_wait(kbdev))
			kutf_test_fail(context, "GPU reset failed");
	} else
		kutf_test_fail(context, "GPU reset failed");

	ret = kbase_ipa_control_query(kbdev, ipa_client, &query_result, 1, &prot_time);
	if (ret)
		kutf_test_fail(context, "1st query after power down failed");

	if (query_result < (raw_after - raw_before))
		kutf_test_fail(context, "Unexpected result for 1st query after power down");

	/* Wait for some time while GPU is powered down before the 2nd query */
	msleep(50);

	if (!ret) {
		ret = kbase_ipa_control_query(kbdev, ipa_client, &second_query_result, 1,
					      &prot_time);
		if (ret)
			kutf_test_fail(context, "2nd query after power down failed");
	}

	if (!ret) {
		if (second_query_result)
			kutf_test_fail(context,
				       "Unexpected non zero result for 2nd query after power down");
	}

	/* Turn on the GPU now */
	kbase_csf_scheduler_pm_resume(kbdev);
	/* Wait for the GPU to get powered up */
	kbase_pm_wait_for_desired_state(kbdev);

	/* To re-acquire the pm active reference for the fixture destroy
	 * function.
	 */
	kbase_pm_context_active(kbdev);

	if (!ret) {
		/* The first call to test_query_values() will use the raw
		 * value from the 2nd call to kbase_ipa_control_query() to
		 * calculate the difference and raw_after was initialized
		 * before that call. So update of raw_after is needed here.
		 */
		raw_after = read_value_cnt(kbdev, perf_counter.type, 0);

		for (i = 0; i < QUERY_NUM_ITERS; i++)
			test_query_values(kbdev, context, ipa_client, &perf_counter, &raw_before,
					  &raw_after, 1);
	}

	if (ipa_client)
		kbase_ipa_control_unregister(kbdev, ipa_client);
}

/**
 * mali_kutf_query_out_of_order_unregister_test() - Test for counter querying
 *                                               when clients are unregistered
 *                                               out of order
 *
 * @context: KUTF context
 *
 * Multiple clients are registered, and one client which is not the last registered client
 * is unregistered, resulting a client not at the end of the client array being inactive.
 * Attempt to query counter value and unregister of clients by iterating over the entire array.
 * Verify that querying and unregistering of inactive clients are handled properly.
 */
static void mali_kutf_query_out_of_order_unregister_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_clients[NUM_OUT_OF_ORDER_UNREGISTER_TEST_SESSIONS];
	int i, ret;
	u64 query_result[NUM_OUT_OF_ORDER_UNREGISTER_TEST_SESSIONS];
	const struct kbase_ipa_control_perf_counter
		perf_counters[NUM_OUT_OF_ORDER_UNREGISTER_TEST_SESSIONS] = {
			PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
			PERF_COUNTER_DEF(1, false, IDX_5, KBASE_IPA_CORE_TYPE_CSHW),
			PERF_COUNTER_DEF(1, false, IDX_6, KBASE_IPA_CORE_TYPE_CSHW),
			PERF_COUNTER_DEF(1, false, IDX_7, KBASE_IPA_CORE_TYPE_CSHW),
		};
	bool has_failed = false;

	for (i = 0; i < NUM_OUT_OF_ORDER_UNREGISTER_TEST_SESSIONS; i++) {
		ret = kbase_ipa_control_register(kbdev, &perf_counters[i], 1, &ipa_clients[i]);

		if (ret) {
			kutf_test_fail(context, "Failed to register session");
			return;
		}

		if (!ipa_clients[i]) {
			kutf_test_fail(context, "NULL ipa_client");
			return;
		}

		/* Wait for some time while GPU is active */
		msleep(50);
	}

	/* Out-of-order unregister of session */
	ret = kbase_ipa_control_unregister(
		kbdev, ipa_clients[OUT_OF_ORDER_UNREGISTER_TEST_UNREGISTER_INDEX]);
	if (ret)
		kutf_test_fail(context, "Failed to unregister");

	for (i = 0; i < NUM_OUT_OF_ORDER_UNREGISTER_TEST_SESSIONS; i++) {
		ret = kbase_ipa_control_query(kbdev, ipa_clients[i], &query_result[i], 1, NULL);
		if (i == OUT_OF_ORDER_UNREGISTER_TEST_UNREGISTER_INDEX) {
			if (!ret) {
				/* Break from further query as test has already failed */
				kutf_test_fail(context, "Query of inactive session should fail");
				has_failed = true;
			}
		} else {
			if (ret) {
				kutf_test_fail(context, "Query of active session should succeed");
				has_failed = true;
			}
		}

		/* Skip rest of queries if at least one test has failed */
		if (has_failed)
			break;
	}

	for (i = 0; i < NUM_OUT_OF_ORDER_UNREGISTER_TEST_SESSIONS; i++) {
		ret = kbase_ipa_control_unregister(kbdev, ipa_clients[i]);
		if (i == OUT_OF_ORDER_UNREGISTER_TEST_UNREGISTER_INDEX) {
			if (!ret && !has_failed) {
				/* Continue attempt to unregister all sessions even if test has failed */
				kutf_test_fail(context,
					       "Unregistration of inactive session should fail");
				has_failed = true;
			}
		} else {
			if (ret && !has_failed) {
				kutf_test_fail(context,
					       "Unregistration of active session should succeed");
				has_failed = true;
			}
		}
	}
}

static void init_query_suite(struct kutf_suite *suite)
{
	unsigned int const filters = suite->suite_default_flags;

	kutf_add_test_with_filters(suite, 0x0, CSF_IPA_CONTROL_QUERY_UNIT_TEST_0,
				   mali_kutf_1_client_1_counter_query_test, filters);
	kutf_add_test_with_filters(suite, 0x1, CSF_IPA_CONTROL_QUERY_UNIT_TEST_1,
				   mali_kutf_N_clients_same_counter_query_test, filters);
	kutf_add_test_with_filters(suite, 0x2, CSF_IPA_CONTROL_QUERY_UNIT_TEST_2,
				   mali_kutf_query_with_gpu_power_cycle_test, filters);
	kutf_add_test_with_filters(suite, 0x3, CSF_IPA_CONTROL_QUERY_UNIT_TEST_3,
				   mali_kutf_query_out_of_order_unregister_test, filters);
}

#ifndef CONFIG_ANDROID
/**
 * mali_kutf_manual_sampling_before_reset_test() - Test for execution of manual
 *                                                 sampling before, during and
 *                                                 after a GPU reset.
 *
 * @context: KUTF context
 *
 * Capture the raw counter values and signal the IPA control register
 * a GPU reset will occur. Check the manual sampling has been performed before
 * the reset by submitting queries. The first one should show progress,
 * the second one not. Signal the post GPU reset to IPA control.
 * No real GPU reset is required for checking the manual sampling.
 */
static void mali_kutf_manual_sampling_before_reset_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_client;
	int i, ret;
	u64 raw_before, raw_after;
	u64 first_query_result, second_query_result;
	const struct kbase_ipa_control_perf_counter perf_counter =
		PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW);
	unsigned long flags;

	ret = kbase_ipa_control_register(kbdev, &perf_counter, 1, &ipa_client);

	if (ret) {
		kutf_test_fail(context, "failed to register");
	} else {
		if (!ipa_client)
			kutf_test_fail(context, "NULL ipa_client");
	}

	raw_before = read_value_cnt(kbdev, perf_counter.type, 0);
	msleep(50);

	raw_after = read_value_cnt(kbdev, perf_counter.type, 0);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_ipa_control_handle_gpu_reset_pre(kbdev);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	kbase_ipa_control_query(kbdev, ipa_client, &first_query_result, 1, NULL);
	if (first_query_result < (raw_after - raw_before))
		kutf_test_fail(context,
			       "Unexpected result for 1st query after the pre GPU reset event");

	kbase_ipa_control_query(kbdev, ipa_client, &second_query_result, 1, NULL);
	if (second_query_result)
		kutf_test_fail(
			context,
			"Unexpected non zero result for 2nd query after pre GPU reset event");

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_ipa_control_handle_gpu_reset_post(kbdev);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	raw_after = read_value_cnt(kbdev, perf_counter.type, 0);

	for (i = 0; i < QUERY_NUM_ITERS; i++) {
		test_query_values(kbdev, context, ipa_client, &perf_counter, &raw_before,
				  &raw_after, 1);
	}

	ret = kbase_ipa_control_unregister(kbdev, ipa_client);
	if (ret)
		kutf_test_fail(context, "Failed to unregister");
}

/**
 * mali_kutf_STATUS_RESET_unset_after_reset_test() - Test for acknowledge of
 *                                                   GPU reset to IPA control.
 *
 * @context: KUTF context
 *
 * Signal the IPA control a GPU reset will occur, reset the GPU and
 * acknowledge the GPU reset to IPA control. Then check the STATUS RESET bit is
 * unset. This proves the acknowledge of GPU reset is effective.
 */
static void mali_kutf_STATUS_RESET_unset_after_reset_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_client = NULL;
	int ret;
	u32 status;
	const struct kbase_ipa_control_perf_counter perf_counter =
		PERF_COUNTER_DEF(1, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW);

	ret = kbase_ipa_control_register(kbdev, &perf_counter, 1, &ipa_client);

	if (ret) {
		kutf_test_fail(context, "failed to register");
		return;
	}

	if (!ipa_client) {
		kutf_test_fail(context, "NULL ipa_client");
		return;
	}

	/* Reset the GPU */
	if (kbase_prepare_to_reset_gpu(kbdev, RESET_FLAGS_NONE)) {
		kbase_reset_gpu(kbdev);
	} else {
		kutf_test_fail(context, "GPU reset failed");
		return;
	}

	kbase_reset_gpu_wait(kbdev);

	/* Check STATUS RESET is unset */
	status = kbase_reg_read32(kbdev, IPA_CONTROL_ENUM(STATUS));
	if (status & STATUS_RESET)
		kutf_test_fail(context, "STATUS RESET is not unset");

	if (ipa_client)
		kbase_ipa_control_unregister(kbdev, ipa_client);
}

/**
 * mali_kutf_1_client_1_counter_query_with_reset_test() - Test the query
 *                                                        to IPA control is
 *                                                        progessing before,
 *                                                        during and after
 *                                                        a GPU reset.
 *
 * @context: KUTF context
 *
 * Reset the GPU and issue sequential queries to IPA control before,
 * during and after a GPU reset.
 * Check the counter value is progressing.
 * Repeat the sequence many times.
 */
static void mali_kutf_1_client_1_counter_query_with_reset_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	int i;

	for (i = 0; i < 20; i++) {
		/* Reset the GPU */
		if (kbase_prepare_to_reset_gpu(kbdev, RESET_FLAGS_NONE)) {
			kbase_reset_gpu(kbdev);
		} else {
			kutf_test_fail(context, "GPU reset failed");
			break;
		}

		msleep(50);

		mali_kutf_1_client_1_counter_query_test(context);

		kbase_reset_gpu_wait(kbdev);
	}
}

/**
 * mali_kutf_N_clients_same_counter_query_after_reset_test() - Test the query
 *                                                             to IPA control is
 *                                                             progessing after
 *                                                             a GPU reset.
 *
 * @context: KUTF context
 *
 * After a GPU reset, issue multiple queries to IPA control.
 * Check the counter value is still progressing.
 */
static void mali_kutf_N_clients_same_counter_query_after_reset_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;

	/* Reset the GPU */
	if (kbase_prepare_to_reset_gpu(kbdev, RESET_FLAGS_NONE)) {
		kbase_reset_gpu(kbdev);
	} else {
		kutf_test_fail(context, "GPU reset failed");
		return;
	}

	kbase_reset_gpu_wait(kbdev);

	mali_kutf_N_clients_same_counter_query_test(context);
}

static void init_reset_suite(struct kutf_suite *suite)
{
	unsigned int const filters = suite->suite_default_flags;

	kutf_add_test_with_filters(suite, 0x0, CSF_IPA_CONTROL_RESET_UNIT_TEST_0,
				   mali_kutf_manual_sampling_before_reset_test, filters);
	kutf_add_test_with_filters(suite, 0x1, CSF_IPA_CONTROL_RESET_UNIT_TEST_1,
				   mali_kutf_STATUS_RESET_unset_after_reset_test, filters);
	kutf_add_test_with_filters(suite, 0x2, CSF_IPA_CONTROL_RESET_UNIT_TEST_2,
				   mali_kutf_1_client_1_counter_query_with_reset_test, filters);
	kutf_add_test_with_filters(suite, 0x3, CSF_IPA_CONTROL_RESET_UNIT_TEST_3,
				   mali_kutf_N_clients_same_counter_query_after_reset_test,
				   filters);
}
#endif /* ifndef CONFIG_ANDROID */

#if MALI_UNIT_TEST
/**
 * mali_kutf_1_client_1_counter_rate_change_fixed_test - Test for 1 client
 *                                                       querying 1 counter
 *                                                       after each GPU rate
 *                                                       change
 *
 * @context: KUTF context
 *
 * The objective of the test is to verify that normalized values take into
 * account the new GPU rate. This test does not try to validate values
 * that incorporate one or more GPU rate changes.
 *
 * One client register to read the same performance counter both with and
 * without GPU normalization, and queries it multiple times.
 * The GPU rate changes multiple times, and for every GPU rate change
 * the client queries the counter a couple of times to verify that the
 * normalized value over an interval of time during which the GPU rate
 * is fixed is equal to the raw value divided by the new GPU frequency.
 */
static void mali_kutf_1_client_1_counter_rate_change_fixed_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_client = NULL;
	int i, ret;

	const struct kbase_ipa_control_perf_counter perf_counters[RATE_CHANGE_PERF_COUNTERS_NUM] = {
		PERF_COUNTER_DEF(1000000, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1000000, true, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
	};
	const int RAW_IDX = 0, NORM_IDX = 1;
	const u32 gpu_rates[RATE_CHANGE_NUM_ITERS] = {
		GPU_RATE_1KHZ,	GPU_RATE_10KHZ,	 GPU_RATE_100KHZ, GPU_RATE_1MHZ,   GPU_RATE_10MHZ,
		GPU_RATE_50MHZ, GPU_RATE_100MHZ, GPU_RATE_200MHZ, GPU_RATE_500MHZ, GPU_RATE_1GHZ,
	};

	ret = kbase_ipa_control_register(kbdev, &perf_counters[0], RATE_CHANGE_PERF_COUNTERS_NUM,
					 &ipa_client);

	if (ret) {
		kutf_test_fail(context, "failed to register");
		return;
	}

	if (!ipa_client) {
		kutf_test_fail(context, "NULL ipa_client");
		return;
	}

	/*
	 * The testing strategy is:
	 * 1) Change GPU rate
	 * 2) Query values. This reading will be ignored, as the GPU rate changed.
	 *    This is the beginning of the interval of time to be tested.
	 * 3) Wait.
	 * 4) Query values. This is the end of the interval of time to be tested.
	 * 5) Verify that values are plausible by comparing the normalized value
	 *    with the raw value and the GPU rate that was stable during the whole
	 *    interval of time under test.
	 */
	for (i = 0; i < RATE_CHANGE_NUM_ITERS; i++) {
		u64 values[RATE_CHANGE_PERF_COUNTERS_NUM], normalized_raw_value, test_delta;

		kbase_ipa_control_rate_change_notify_test(kbdev, KBASE_CLOCK_DOMAIN_TOP,
							  gpu_rates[i]);
		ret = kbase_ipa_control_query(kbdev, ipa_client, &values[0],
					      RATE_CHANGE_PERF_COUNTERS_NUM, NULL);
		if (ret)
			kutf_test_fail(context, "failed to query");
		msleep(2000);
		ret = kbase_ipa_control_query(kbdev, ipa_client, &values[0],
					      RATE_CHANGE_PERF_COUNTERS_NUM, NULL);
		if (ret)
			kutf_test_fail(context, "failed to query");

		normalized_raw_value = div_u64(values[RAW_IDX], gpu_rates[i]);
		if (normalized_raw_value > values[NORM_IDX])
			test_delta = normalized_raw_value - values[NORM_IDX];
		else
			test_delta = values[NORM_IDX] - normalized_raw_value;

		if (test_delta * RATE_CHANGE_THRESHOLD_DIV >
		    values[NORM_IDX] * RATE_CHANGE_THRESHOLD_NUM) {
			kutf_test_fail(
				context,
				"Difference between raw and normalized values is greater than expected");
		}
	}

	ret = kbase_ipa_control_unregister(kbdev, ipa_client);
	if (ret)
		kutf_test_fail(context, "failed to unregister");
}

/**
 * mali_kutf_2_clients_1_counter_rate_change_variable_test - Test for 2 clients
 *                                                           querying 1 counter
 *                                                           after many GPU
 *                                                           rate changes
 *
 * @context: KUTF context
 *
 * The objective of the test is to verify that normalized values take into
 * account the new GPU rate. This test does not try to validate values
 * that incorporate one or more GPU rate changes.
 *
 * One client register to read the raw value of a performance counter,
 * while another client registers to read the same performance counter
 * as a normalized value.
 * The GPU rate changes multiple times at regular intervals of time.
 * The first client reads the raw value at every time interval, while
 * the second client only reads the normalized value at the end.
 * Verify that the normalized value is compatible with the sum of raw values.
 */
static void mali_kutf_2_clients_1_counter_rate_change_variable_test(struct kutf_context *context)
{
	struct kutf_ipa_control_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	void *ipa_clients[2];
	int i, ret;
	u64 raw_values[RATE_CHANGE_NUM_ITERS], norm_value, test_delta, normalized_raw_value = 0;
	const struct kbase_ipa_control_perf_counter perf_counters[2] = {
		PERF_COUNTER_DEF(1000000, false, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
		PERF_COUNTER_DEF(1000000, true, IDX_4, KBASE_IPA_CORE_TYPE_CSHW),
	};
	const int RAW_IDX = 0, NORM_IDX = 1;
	const u32 gpu_rates[RATE_CHANGE_NUM_ITERS] = {
		GPU_RATE_1KHZ,	GPU_RATE_10KHZ,	 GPU_RATE_100KHZ, GPU_RATE_1MHZ,   GPU_RATE_10MHZ,
		GPU_RATE_50MHZ, GPU_RATE_100MHZ, GPU_RATE_200MHZ, GPU_RATE_500MHZ, GPU_RATE_1GHZ,
	};

	for (i = 0; i < 2; i++) {
		ret = kbase_ipa_control_register(kbdev, &perf_counters[i], 1, &ipa_clients[i]);

		if (ret) {
			kutf_test_fail(context, "failed to register");
		} else {
			if (!ipa_clients[i])
				kutf_test_fail(context, "NULL ipa_client");
		}
	}

	/*
	 * The testing strategy is:
	 * 1) Change GPU rate
	 * 2) Query raw value and divide by current GPU rate, then add it
	 *    to a temporary sum
	 * At the end of the loop, query the normalized value and compare
	 * with the sum of "normalized" raw values.
	 */
	for (i = 0; i < RATE_CHANGE_NUM_ITERS; i++) {
		kbase_ipa_control_rate_change_notify_test(kbdev, KBASE_CLOCK_DOMAIN_TOP,
							  gpu_rates[i]);
		msleep(2000);
		ret = kbase_ipa_control_query(kbdev, ipa_clients[RAW_IDX], &raw_values[i], 1, NULL);
		if (ret)
			kutf_test_fail(context, "failed to query");

		normalized_raw_value += div_u64(raw_values[i], gpu_rates[i]);
	}

	ret = kbase_ipa_control_query(kbdev, ipa_clients[NORM_IDX], &norm_value, 1, NULL);
	if (ret)
		kutf_test_fail(context, "failed to query");

	if (normalized_raw_value > norm_value)
		test_delta = normalized_raw_value - norm_value;
	else
		test_delta = norm_value - normalized_raw_value;

	if (test_delta * RATE_CHANGE_THRESHOLD_DIV > norm_value * RATE_CHANGE_THRESHOLD_NUM) {
		kutf_test_fail(
			context,
			"Difference between raw and normalized values is greater than expected");
	}

	for (i = 0; i < 2; i++) {
		ret = kbase_ipa_control_unregister(kbdev, ipa_clients[i]);
		if (ret)
			kutf_test_fail(context, "failed to unregister");
	}
}

static void init_rate_change_suite(struct kutf_suite *suite)
{
	unsigned int const filters = suite->suite_default_flags;

	kutf_add_test_with_filters(suite, 0x0, CSF_IPA_CONTROL_RATE_CHANGE_UNIT_TEST_0,
				   mali_kutf_1_client_1_counter_rate_change_fixed_test, filters);
	kutf_add_test_with_filters(suite, 0x1, CSF_IPA_CONTROL_RATE_CHANGE_UNIT_TEST_1,
				   mali_kutf_2_clients_1_counter_rate_change_variable_test,
				   filters);
}
#endif

static const struct suite_list {
	const char *name;
	unsigned int fixture_count;
	void (*init)(struct kutf_suite *suite);
} suite_list[] = {
#if MALI_UNIT_TEST
	{ CSF_IPA_CONTROL_REGISTER_SUITE_NAME, 1, init_register_suite },
#endif
	{ CSF_IPA_CONTROL_QUERY_SUITE_NAME, 1, init_query_suite },
#ifndef CONFIG_ANDROID
	{ CSF_IPA_CONTROL_RESET_SUITE_NAME, 1, init_reset_suite },
#endif
#if MALI_UNIT_TEST
	{ CSF_IPA_CONTROL_RATE_CHANGE_SUITE_NAME, 1, init_rate_change_suite },
#endif
};

/**
 * mali_kutf_ipa_control_unit_test_main_init() - Entry point for test module.
 *
 * Return: 0 on success.
 */
static int __init mali_kutf_ipa_control_unit_test_main_init(void)
{
	unsigned int i;
	union kutf_callback_data suite_data = { NULL };

	pr_debug("Creating app\n");
	ipa_control_app = kutf_create_application(CSF_IPA_CONTROL_APP_NAME);

	if (!ipa_control_app) {
		pr_warn("Creation of app " CSF_IPA_CONTROL_APP_NAME " failed!\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(suite_list); i++) {
		struct kutf_suite *suite;

		pr_debug("Create suite %s\n", suite_list[i].name);
		suite = kutf_create_suite_with_filters_and_data(
			ipa_control_app, suite_list[i].name, suite_list[i].fixture_count,
			mali_kutf_ipa_control_unit_create_fixture,
			mali_kutf_ipa_control_unit_remove_fixture, KUTF_F_TEST_GENERIC, suite_data);

		if (!suite) {
			pr_warn("Creation of suite %s for app " CSF_IPA_CONTROL_APP_NAME
				" failed!\n",
				suite_list[i].name);
			return -ENOMEM;
		}

		suite_list[i].init(suite);
	}

	pr_debug("Init complete\n");
	return 0;
}

/**
 * mali_kutf_ipa_control_unit_test_main_exit() - Module exit point for this test.
 */
static void __exit mali_kutf_ipa_control_unit_test_main_exit(void)
{
	pr_debug("Exit start\n");
	kutf_destroy_application(ipa_control_app);
	pr_debug("Exit complete\n");
}

module_init(mali_kutf_ipa_control_unit_test_main_init);
module_exit(mali_kutf_ipa_control_unit_test_main_exit);

MODULE_LICENSE("GPL");
