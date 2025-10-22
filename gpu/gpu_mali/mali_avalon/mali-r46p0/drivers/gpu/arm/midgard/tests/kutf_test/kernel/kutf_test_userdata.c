// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2017-2023 ARM Limited. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/string.h>

#include <kutf/kutf_suite.h>
#include <kutf/kutf_utils.h>
#include <kutf/kutf_helpers.h>
#include <kutf/kutf_helpers_user.h>
#include "kutf_test_userdata.h"

#define VAL_A_NAME "A"
#define VAL_B_NAME "B"
#define VAL_C_NAME "C"
void userdata_add_two_values_test(struct kutf_context *context)
{
	struct kutf_helper_named_val val_a;
	struct kutf_helper_named_val val_b;
	u64 val_c_u64;
	int err;

	/* Receive A value */
	err = kutf_helper_receive_check_val(&val_a, context, VAL_A_NAME, KUTF_HELPER_VALTYPE_U64);
	if (err)
		return;

	/* Receive B value */
	err = kutf_helper_receive_check_val(&val_b, context, VAL_B_NAME, KUTF_HELPER_VALTYPE_U64);
	if (err)
		return;

	/* Add them together */
	val_c_u64 = val_a.u.val_u64 + val_b.u.val_u64;

	/* Send C value */
	err = kutf_helper_send_named_u64(context, VAL_C_NAME, val_c_u64);
	if (err)
		return;

	kutf_test_pass(context, NULL);
}

#define VAL_KERN_BUF_SZ_NAME "KERN_BUF_SZ"
#define VAL_NR_ITERS_NAME "NR_ITERS"
#define VAL_STR_NAME_U "USTR"
#define VAL_STR_NAME_K "KSTR"
void userdata_increasing_str_sizes_test(struct kutf_context *context)
{
	struct kutf_helper_named_val val_nr_iters;
	struct kutf_helper_named_val user_str_val;
	int err;
	int idx;
	int max_kern_str_len;

	/* Send KERN_BUF_SZ */
	err = kutf_helper_send_named_u64(context, VAL_KERN_BUF_SZ_NAME, KUTF_MAX_LINE_LENGTH);
	if (err)
		return;

	/* Maximum length of string being received */
	max_kern_str_len = kutf_helper_max_str_len_for_kern(VAL_STR_NAME_U, KUTF_MAX_LINE_LENGTH);

	/* Receive NR_ITERS */
	err = kutf_helper_receive_check_val(&val_nr_iters, context, VAL_NR_ITERS_NAME,
					    KUTF_HELPER_VALTYPE_U64);
	if (err || val_nr_iters.type == KUTF_HELPER_VALTYPE_INVALID)
		return;

	for (idx = 0; idx < val_nr_iters.u.val_u64; ++idx) {
		/* Receive USTR */
		err = kutf_helper_receive_check_val(&user_str_val, context, VAL_STR_NAME_U,
						    KUTF_HELPER_VALTYPE_STR);
		if (err)
			return;

		if (user_str_val.type != KUTF_HELPER_VALTYPE_INVALID) {
			int expected_str_len = idx;
			int user_str_len;

			if (expected_str_len > max_kern_str_len)
				expected_str_len = max_kern_str_len;

			/* Only thing we know in this test is the size of the string */
			user_str_len = strlen(user_str_val.u.val_str);
			if (user_str_len != expected_str_len) {
				const char *msg = kutf_dsprintf(
					&context->fixture_pool,
					"iter %d: Expected user string length was %d but got %d",
					idx, idx, user_str_len);
				kutf_test_fail(context, msg);
			}
		}

		/* Send KSTR */
		err = kutf_helper_send_named_str(context, VAL_STR_NAME_K, user_str_val.u.val_str);
		if (err)
			return;
	}

	kutf_test_pass(context, NULL);
}

#define VAL_EXPECT_LEN_NAME "EXPECT_LEN"
void userdata_random_str_sizes_test(struct kutf_context *context)
{
	struct kutf_helper_named_val val_nr_iters;
	struct kutf_helper_named_val user_str_val;
	int err;
	int idx;
	int max_kern_str_len;

	/* Send KERN_BUF_SZ */
	err = kutf_helper_send_named_u64(context, VAL_KERN_BUF_SZ_NAME, KUTF_MAX_LINE_LENGTH);
	if (err)
		return;

	/* Maximum length of string being received */
	max_kern_str_len = kutf_helper_max_str_len_for_kern(VAL_STR_NAME_U, KUTF_MAX_LINE_LENGTH);

	/* Receive NR_ITERS */
	err = kutf_helper_receive_check_val(&val_nr_iters, context, VAL_NR_ITERS_NAME,
					    KUTF_HELPER_VALTYPE_U64);
	if (err || val_nr_iters.type == KUTF_HELPER_VALTYPE_INVALID)
		return;

	for (idx = 0; idx < val_nr_iters.u.val_u64; ++idx) {
		int user_str_len;
		struct kutf_helper_named_val val_expect_len;
		int expected_str_len;

		/* Receive EXPECT_LEN */
		err = kutf_helper_receive_check_val(&val_expect_len, context, VAL_EXPECT_LEN_NAME,
						    KUTF_HELPER_VALTYPE_U64);
		if (err)
			return;

		expected_str_len = val_expect_len.u.val_u64;
		if (expected_str_len > max_kern_str_len)
			expected_str_len = max_kern_str_len;

		/* Receive USTR */
		err = kutf_helper_receive_check_val(&user_str_val, context, VAL_STR_NAME_U,
						    KUTF_HELPER_VALTYPE_STR);
		if (err)
			return;

		if (user_str_val.type != KUTF_HELPER_VALTYPE_INVALID) {
			/* Only thing we know in this test is the size of the string */
			user_str_len = strlen(user_str_val.u.val_str);
			if (user_str_len != expected_str_len) {
				const char *msg = kutf_dsprintf(
					&context->fixture_pool,
					"iter %d: Expected user string length was %d but got %d",
					idx, expected_str_len, user_str_len);
				kutf_test_fail(context, msg);
			}
		}

		/* Send KSTR */
		err = kutf_helper_send_named_str(context, VAL_STR_NAME_K, user_str_val.u.val_str);
		if (err)
			return;
	}

	kutf_test_pass(context, NULL);
}
