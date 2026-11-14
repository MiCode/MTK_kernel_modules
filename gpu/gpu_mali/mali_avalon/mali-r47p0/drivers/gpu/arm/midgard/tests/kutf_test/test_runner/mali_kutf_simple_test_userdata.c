/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2017-2023 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include <tpi/mali_tpi.h>
#include <utf/include/mali_utf_suite.h>
#include <utf/include/mali_utf_mem.h>
#include <utf/include/mali_utf_resultset.h>
#include <utf/include/mali_utf_helpers.h>
#include <utf/include/mali_utf_main.h>
#include "mali_kutf_simple_test_userdata.h"
#include "mali_kutf_test_helpers.h"

#include <string.h>

struct userdata_fixture {
	mali_tpi_rand_state rand_state;
};

/* All of these tests share the same userdata fixture */
static int userdata_setup_fixture(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	struct userdata_fixture *u_fix;
	/* Low bits of current time in microseconds give us the seed */
	uint32_t rand_seed = (uint32_t)(mali_tpi_get_time_ns() / 1000ull);

	/* Initialize our own fixture */
	u_fix = mali_utf_test_alloc(sizeof(*u_fix));
	if (!u_fix) {
		mali_utf_logerr("Failed to allocate userdata test fixture\n");
		return 1;
	}

	/* Initialize Random Number Generator */
	mali_utf_loginf("Random seed: 0x%.8x\n", rand_seed);
	mali_tpi_rand_init(&u_fix->rand_state, rand_seed, 0);

	fix->extra_funcs_data = u_fix;

	return 0;
}

#define VAL_A_NAME "A"
#define VAL_B_NAME "B"
#define VAL_C_NAME "C"
static void userdata_add_two_values(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	struct userdata_fixture *u_fix = fix->extra_funcs_data;
	int err;
	/* Attempt val_a to fill some of the upper bits of the uint64_t, but not all
	 * of it to avoid wraparound
	 */
	uint64_t val_a = mali_tpi_rand_gen_uint64(&u_fix->rand_state, UINT32_MAX,
						  ((uint64_t)UINT32_MAX) << 24);
	uint64_t val_b = mali_tpi_rand_gen_uint64(&u_fix->rand_state, 0, UINT32_MAX);
	struct kutf_test_helpers_named_val val_c;
	uint64_t expected_val;

	/* Send A value */
	mali_utf_loginf(VAL_A_NAME "=0x%.16" PRIx64 "\n", val_a);
	err = kutf_test_helpers_userdata_send_named_u64(suite, VAL_A_NAME, val_a);
	if (err)
		goto out;

	/* Send B value */
	mali_utf_loginf(VAL_B_NAME "=0x%.16" PRIx64 "\n", val_b);
	err = kutf_test_helpers_userdata_send_named_u64(suite, VAL_B_NAME, val_b);
	if (err)
		goto out;

	/* Calculate expected value for adding them together */
	expected_val = val_a + val_b;
	mali_utf_loginf("Expecting " VAL_C_NAME "=0x%.16" PRIx64 ", actual value follows\n",
			expected_val);

	/* Receive C value */
	err = kutf_test_helpers_userdata_receive_check_val(&val_c, suite, VAL_C_NAME,
							   KUTF_TEST_HELPERS_VALTYPE_U64);
	if (err)
		goto out;
	kutf_test_helpers_output_named_val(&val_c);
	MALI_UTF_ASSERT_UINT_EQ(val_c.u.val_u64, expected_val);

out:
	return;
}

struct kutf_extra_func_spec kutf_simple_test_add_two_values = {
	USERDATA_APP_NAME, /* app_name */
	USERDATA_SUITE_NAME, /* suite_name */
	"add_two_values", /* test_name */
	{
		userdata_setup_fixture, /* pretest */
		userdata_add_two_values, /* midtest */
		NULL /* posttest */
	}
};

#define VAL_KERN_BUF_SZ_NAME "KERN_BUF_SZ"
#define VAL_NR_ITERS_NAME "NR_ITERS"
#define VAL_STR_NAME_U "USTR"
#define VAL_STR_NAME_K "KSTR"
#define VAL_STR_MAX_INFO_LEN 270

static void userdata_increasing_str_sizes(struct mali_utf_suite *suite)
{
	int err;
	int idx;
	struct kutf_test_helpers_named_val test_info_buf_sz;
	int kern_buf_sz;
	int max_kern_str_len_stru;
	int max_kern_str_len_strk;
	int max_kern_str_len;

	/* Sample string to send back and forth. Aim to be about twice as large
	 * as kernel-side's TEXTBUF_LINE_SZ. Test character codes in the
	 * range 32--127 excluding '"'
	 */
	const char test_str[] =
		"The quick brown fox jumps over the lazy dog. "
		"0 1 2 3 4 5 6 7 8 9 10 !#$%&'()*+,-./:;<=>?@[]^_`{|}~ "
		"If the callback object was set up as non-direct "
		"any pending callbacks will be flushed first. "
		"Note that to avoid a deadlock the lock callbacks needs "
		"can't be held when a callback object is terminated. "
		"If the resource is being used or waited -EBUSY is returned. "
		"The caller should NOT try to terminate a resource that could still have clients. "
		"After the function returns the resource is no longer known by . "
		"This is the end of the message.";

	int nr_iters = sizeof(test_str);
	char buf[sizeof(test_str)] = {
		0,
	};

	/* Receive KERN_BUF_SZ */
	err = kutf_test_helpers_userdata_receive_check_val(
		&test_info_buf_sz, suite, VAL_KERN_BUF_SZ_NAME, KUTF_TEST_HELPERS_VALTYPE_U64);
	if (err || test_info_buf_sz.type == KUTF_TEST_HELPERS_VALTYPE_INVALID)
		goto out;
	kutf_test_helpers_output_named_val(&test_info_buf_sz);
	kern_buf_sz = test_info_buf_sz.u.val_u64;

	/* Maximum length of string being sent to kernel and back */
	max_kern_str_len_stru =
		kutf_test_helpers_userdata_max_str_len_for_kern(VAL_STR_NAME_U, kern_buf_sz);
	max_kern_str_len_strk =
		kutf_test_helpers_userdata_max_str_len_for_kern(VAL_STR_NAME_K, kern_buf_sz);
	max_kern_str_len = max_kern_str_len_strk;
	if (max_kern_str_len > max_kern_str_len_stru)
		max_kern_str_len = max_kern_str_len_stru;

	/* Send NR_ITERS */
	err = kutf_test_helpers_userdata_send_named_u64(suite, VAL_NR_ITERS_NAME, nr_iters);
	if (err)
		goto out;

	/* Copy successive characters into buf. Buf is already filled with 0s
	 * so is always 0-terminated
	 *
	 * Start off with buf being the string ""
	 */
	for (idx = 0; idx < nr_iters; ++idx) {
		struct kutf_test_helpers_named_val kern_str_val;
		int expected_str_len = idx;

		if (expected_str_len > max_kern_str_len)
			expected_str_len = max_kern_str_len;

		/* Send USTR */
		err = kutf_test_helpers_userdata_send_named_str(suite, VAL_STR_NAME_U, buf);
		if (err)
			goto out;

		/* Receive KSTR */
		err = kutf_test_helpers_userdata_receive_check_val(
			&kern_str_val, suite, VAL_STR_NAME_K, KUTF_TEST_HELPERS_VALTYPE_STR);
		if (err)
			goto out;

		/* Print out most, but not all the iters */
		if (idx <= VAL_STR_MAX_INFO_LEN) {
			mali_utf_loginf("%.3d:", idx);
			kutf_test_helpers_output_named_val(&kern_str_val);
		}

		/* Check KSTR */
		/* String gets truncated when kernel side, so only check up to expected_str_len */
		MALI_UTF_ASSERT_INT_EQ(strlen(kern_str_val.u.val_str), expected_str_len);
		MALI_UTF_ASSERT(strncmp(kern_str_val.u.val_str, buf, expected_str_len) == 0);

		/* Next iteration will have one more character in the string */
		buf[idx] = test_str[idx];
	}

out:
	return;
}

struct kutf_extra_func_spec kutf_simple_test_increasing_str_sizes = {
	USERDATA_APP_NAME, /* app_name */
	USERDATA_SUITE_NAME, /* suite_name */
	"increasing_str_sizes", /* test_name */
	{
		userdata_setup_fixture, /* pretest */
		userdata_increasing_str_sizes, /* midtest */
		NULL /* posttest */
	}
};

static int make_random_str(struct mali_utf_suite *suite, char *buf, int max_size)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	struct userdata_fixture *u_fix = fix->extra_funcs_data;
	int str_len;
	int idx;

	str_len = mali_tpi_rand_gen_int32(&u_fix->rand_state, 0, max_size - 1);

	for (idx = 0; idx < str_len; ++idx) {
		char chr = 0;
		/* Generate non-special characters */
		while (!chr || chr == '"' || chr == '\\')
			chr = mali_tpi_rand_gen_int8(&u_fix->rand_state, 32, 126);

		buf[idx] = chr;
	}
	buf[idx] = '\0';

	return str_len;
}

/*
 * Aim to be just larger than the kernel-side's maximum line length, to test
 * the maximum size, but avoid too many error lines being produced.
 */
#define RAND_SIZES_MAX_STR_SZ 1040
/* Twice as many iters to have high likelihood of encountering all sizes */
#define RAND_SIZES_NR_ITERS (RAND_SIZES_MAX_STR_SZ * 2)
#define VAL_EXPECT_LEN_NAME "EXPECT_LEN"

static void userdata_random_str_sizes(struct mali_utf_suite *suite)
{
	int err;
	int idx;
	int nr_iters = RAND_SIZES_NR_ITERS;
	struct kutf_test_helpers_named_val test_info_buf_sz;
	int kern_buf_sz;
	int max_kern_str_len_stru;
	int max_kern_str_len_strk;
	int max_kern_str_len;

	/* Receive KERN_BUF_SZ */
	err = kutf_test_helpers_userdata_receive_check_val(
		&test_info_buf_sz, suite, VAL_KERN_BUF_SZ_NAME, KUTF_TEST_HELPERS_VALTYPE_U64);
	if (err || test_info_buf_sz.type == KUTF_TEST_HELPERS_VALTYPE_INVALID)
		goto out;
	kutf_test_helpers_output_named_val(&test_info_buf_sz);
	kern_buf_sz = test_info_buf_sz.u.val_u64;

	/* Maximum length of string being sent to kernel and back */
	max_kern_str_len_stru =
		kutf_test_helpers_userdata_max_str_len_for_kern(VAL_STR_NAME_U, kern_buf_sz);
	max_kern_str_len_strk =
		kutf_test_helpers_userdata_max_str_len_for_kern(VAL_STR_NAME_K, kern_buf_sz);
	max_kern_str_len = max_kern_str_len_strk;
	if (max_kern_str_len > max_kern_str_len_stru)
		max_kern_str_len = max_kern_str_len_stru;

	/* Send NR_ITERS */
	err = kutf_test_helpers_userdata_send_named_u64(suite, VAL_NR_ITERS_NAME, nr_iters);
	if (err)
		goto out;

	for (idx = 0; idx < nr_iters; ++idx) {
		char buf[RAND_SIZES_MAX_STR_SZ];
		struct kutf_test_helpers_named_val kern_str_val;
		int str_len;
		int expected_str_len;

		str_len = make_random_str(suite, buf, RAND_SIZES_MAX_STR_SZ);
		expected_str_len = str_len;

		/* Send EXPECT_LEN */
		err = kutf_test_helpers_userdata_send_named_u64(suite, VAL_EXPECT_LEN_NAME,
								str_len);
		if (err)
			goto out;

		if (expected_str_len > max_kern_str_len) {
			/* Should be too long for the kernel */
			/* Check that the suite hasn't already failed */
			if (suite->status != MALI_UTF_RESULT_UNKNOWN)
				goto out;
			mali_utf_test_expect_fail();
		}

		/* Send USTR */
		err = kutf_test_helpers_userdata_send_named_str(suite, VAL_STR_NAME_U, buf);
		if (err) {
			if (expected_str_len > max_kern_str_len) {
				/* Remove the failure */
				suite->status = MALI_UTF_RESULT_UNKNOWN;
				mali_utf_test_expect_pass();
				/* Failed as expected, send a shorter string */
				buf[max_kern_str_len] = 0;
				expected_str_len = max_kern_str_len;

				err = kutf_test_helpers_userdata_send_named_str(
					suite, VAL_STR_NAME_U, buf);
				if (err)
					goto out;
			} else {
				goto out;
			}
		}

		/* Receive KSTR */
		err = kutf_test_helpers_userdata_receive_check_val(
			&kern_str_val, suite, VAL_STR_NAME_K, KUTF_TEST_HELPERS_VALTYPE_STR);
		if (err)
			goto out;

		/* Print out most, but not all the iters */
		if (str_len <= VAL_STR_MAX_INFO_LEN) {
			mali_utf_loginf("%.4d:orig_len=%d,", idx, str_len);
			kutf_test_helpers_output_named_val(&kern_str_val);
		}

		/* Check KSTR */
		/* String gets truncated when kernel side, so only check up to expected_str_len */
		MALI_UTF_ASSERT_INT_EQ(strlen(kern_str_val.u.val_str), expected_str_len);
		MALI_UTF_ASSERT(strncmp(kern_str_val.u.val_str, buf, expected_str_len) == 0);
	}

out:
	return;
}

struct kutf_extra_func_spec kutf_simple_test_random_str_sizes = {
	USERDATA_APP_NAME, /* app_name */
	USERDATA_SUITE_NAME, /* suite_name */
	"random_str_sizes", /* test_name */
	{
		userdata_setup_fixture, /* pretest */
		userdata_random_str_sizes, /* midtest */
		NULL /* posttest */
	}
};
