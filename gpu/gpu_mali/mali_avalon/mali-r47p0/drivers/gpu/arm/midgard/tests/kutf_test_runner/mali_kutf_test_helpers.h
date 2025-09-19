/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2017-2023 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef _KUTF_TEST_HELPERS_H_
#define _KUTF_TEST_HELPERS_H_

#include <tpi/mali_tpi.h>
#include <utf/include/mali_utf_main.h>

#define KUTF_TEST_HELPERS_MAX_VAL_NAME_LEN 255

enum kutf_test_helpers_valtype {
	KUTF_TEST_HELPERS_VALTYPE_INVALID,
	KUTF_TEST_HELPERS_VALTYPE_U64,
	KUTF_TEST_HELPERS_VALTYPE_STR,

	KUTF_TEST_HELPERS_VALTYPE_COUNT /* Must be last */
};

struct kutf_test_helpers_named_val {
	enum kutf_test_helpers_valtype type;
	char *val_name;
	union {
		uint64_t val_u64;
		char *val_str;
	} u;
};

/* Extra error values for certain helpers when we want to distinguish between
 * Linux's own error values too.
 *
 * Negative values indicate a problem in accessing the 'run' file itself (are
 * generally unrecoverable).
 *
 * Positive values indicate correct access but invalid parsing (can be
 * recovered from assuming data in the future is correct)
 */
enum kutf_test_helpers_err {
	/* Error in receiving a raw line from the run file - must be negative */
	KUTF_TEST_HELPERS_ERR_RECV_FAULT = -1,
	/* No error - must be zero */
	KUTF_TEST_HELPERS_ERR_NONE = 0,
	/* Named value parsing encountered an invalid name */
	KUTF_TEST_HELPERS_ERR_INVALID_NAME,
	/* Named value parsing of string or uint64_t type encountered extra
	 * characters after the value (after the last digit for a uint64_t type or
	 * after the string end delimiter for string type)
	 */
	KUTF_TEST_HELPERS_ERR_CHARS_AFTER_VAL,
	/* Named value parsing of string type couldn't find the string end
	 * delimiter.
	 *
	 * This cannot be encountered when the NAME="value" message exceeds the
	 * textbuf's maximum line length, because such messages are not checked
	 * for an end string delimiter
	 */
	KUTF_TEST_HELPERS_ERR_NO_END_DELIMITER,
	/* Named value didn't parse as any of the known types */
	KUTF_TEST_HELPERS_ERR_INVALID_VALUE,
};

/* Userdata send message string to the run file
 *
 * This is assuming the kernel-side test is using the userdata helpers
 *
 * If str contains any '\n' characters, then the first of these will be used as
 * the string terminator
 *
 * Returns 0 on success, non-zero on failure
 */
int kutf_test_helpers_userdata_send(struct mali_utf_suite *suite, const char *str);

/* Userdata Receive message string from the run file
 *
 * This is assuming the kernel-side test is using the userdata helpers
 *
 * The string is allocated on the fixture's mempool
 *
 * One line (as defined by the kernel's implementation) is read. Any trailing
 * '\n' is removed.
 *
 * The string is always terminated with a '\0' character.
 *
 * Returns the string on success, or NULL on failure
 */
char *kutf_test_helpers_userdata_receive(struct mali_utf_suite *suite);

/* Userdata Send named NAME=value pair, uint64_t value
 *
 * NAME must match [A-Z0-9_]\+ and can be up to MAX_VAL_NAME_LEN characters long
 *
 * This is assuming the kernel-side test is using the userdata helpers
 *
 * Any failure will be logged on the suite's current test fixture
 *
 * Returns 0 on success, non-zero on failure
 */
int kutf_test_helpers_userdata_send_named_u64(struct mali_utf_suite *suite, char *val_name,
					      uint64_t val);

/* Get the maximum length of a string that can be represented as a particular
 * NAME="value" pair without string-value truncation in the kernel's buffer
 *
 * Given val_name and the kernel buffer's size, this can be used to determine
 * the maximum length of a string that can be sent as val_name="value" pair
 * without having the string value truncated. Any string longer than this will
 * be truncated at some point during communication to this size.
 *
 * The calculation is valid both for sending strings of val_str_len to kernel,
 * and for receiving a string that was originally val_str_len from the kernel.
 *
 * It is assumed that valname is a valid name for
 * kutf_test_helpers_userdata_send_named_str(), and no checking will be made to
 * ensure this.
 *
 * Returns the maximum string length that can be represented, or a negative
 * value if the NAME="value" encoding itself wouldn't fit in kern_buf_sz
 */
int kutf_test_helpers_userdata_max_str_len_for_kern(char *val_name, int kern_buf_sz);

/* Userdata Send named NAME="str" pair
 *
 * no escaping allowed in str. Any of the following characters will terminate
 * the string: '"' '\\' '\n'
 *
 * NAME must match [A-Z0-9_]\+ and can be up to MAX_VAL_NAME_LEN characters long
 *
 * This is assuming the kernel-side test is using the userdata helpers
 *
 * Any failure will be logged on the suite's current test fixture
 *
 * Returns 0 on success, non-zero on failure
 */
int kutf_test_helpers_userdata_send_named_str(struct mali_utf_suite *suite, char *val_name,
					      char *val_str);

/* Userdata Receive named NAME=value pair
 *
 * This can receive uint64_t and string values - check named_val->type
 *
 * If you are not planning on dynamic handling of the named value's name and
 * type, then kutf_test_helpers_userdata_receive_check_val() is more useful as a
 * convenience function.
 *
 * String members of named_val will come from memory allocated on the fixture's
 * mempool
 *
 * Returns 0 on success. Otherwise a value corresponding to an enum
 * kutf_helper_err value, which will be:
 * - a negative value indicating failure to receive from the 'run' file
 * - a positive value indicating correct reception of data but an error in
 *   parsing
 */
int kutf_test_helpers_userdata_receive_named_val(struct kutf_test_helpers_named_val *named_val,
						 struct mali_utf_suite *suite);

/* Userdata Receive and validate NAME=value pair
 *
 * As with kutf_test_helpers_userdata_receive_named_val, but validate that the
 * name and type are as expected, as a convenience for a common pattern found
 * in tests.
 *
 * NOTE: this only returns an error value if there was actually a problem
 * receiving data.
 *
 * NOTE: If the underlying data was received correctly, but:
 * - isn't of the expected name
 * - isn't the expected type
 * - isn't correctly parsed for the type
 * then the following happens:
 * - failure result is recorded
 * - named_val->type will be KUTF_TEST_HELPERS_VALTYPE_INVALID
 * - named_val->u will contain some default value that should be relatively
 *   harmless for the test, including being writable in the case of string
 *   values
 * - return value will be 0 to indicate success
 *
 * The rationale behind this is that we'd prefer to continue the rest of the
 * test with failures propagated, rather than hitting a timeout
 */
int kutf_test_helpers_userdata_receive_check_val(struct kutf_test_helpers_named_val *named_val,
						 struct mali_utf_suite *suite,
						 char *expect_val_name,
						 enum kutf_test_helpers_valtype expect_val_type);

/* Output a named value as an info message */
void kutf_test_helpers_output_named_val(struct kutf_test_helpers_named_val *named_val);

#endif /* _KUTF_TEST_HELPERS_H_ */
