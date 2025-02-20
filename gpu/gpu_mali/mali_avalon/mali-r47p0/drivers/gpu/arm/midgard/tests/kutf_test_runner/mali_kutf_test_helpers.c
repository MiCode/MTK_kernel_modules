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

#include <kutf/kutf_resultset.h>
#include "mali_kutf_test_runner.h"
#include "mali_kutf_test_helpers.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>

#define ERR_BUF_SZ 256

/*
 * Maximum length of a single userdata that will be read from the kernel.
 * Messages longer than this will be truncated in the kernel.
 */
#define KUTF_USERDATA_LEN (1024u)

void log_error_with_errno(char *msg, int errnum)
{
	char errbuf[ERR_BUF_SZ] = {
		0,
	};
	char *errstr;

#if ((defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L) || \
     (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 600)) &&        \
	!defined(_GNU_SOURCE)
	{
		int ret;

		ret = strerror_r(errnum, errbuf, ERR_BUF_SZ);
		if (!ret)
			errstr = errbuf;
		else
			errstr = "(failed to decode errno)";
	}
#else
	errstr = strerror_r(errnum, errbuf, ERR_BUF_SZ);
#endif

	mali_utf_logerr("Function failed with errno=%d '%s': %s\n", errnum, errstr, msg);
}

int kutf_test_helpers_userdata_send(struct mali_utf_suite *suite, const char *str)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	ssize_t ret;
	size_t str_len;
	char *newline_ptr;

	/* Check for newline character and strip remaining part of the string */
	newline_ptr = strchr(str, '\n');
	if (newline_ptr)
		str_len = newline_ptr - str;
	else
		str_len = strlen(str);

	ret = write(fix->fd, str, str_len);

	mali_utf_loginf(">%.*s\n", (int)str_len, str);

	if (ret != (ssize_t)str_len) {
		log_error_with_errno("write failed on userdata_send", errno);

		return 1;
	}

	return 0;
}

char *kutf_test_helpers_userdata_receive(struct mali_utf_suite *suite)
{
	struct kutf_fixture_data *const fix = suite->fixture;
	ssize_t read_bytes;
	char *str;

	/*
	 * This is bigger than needed, but avoids the need for a
	 * temporary buffer. The extra bytes are for the newline and null
	 * terminator
	 */
	str = mali_utf_test_alloc(KUTF_USERDATA_LEN + 2);
	if (!str)
		return NULL;

	do {
		/* The extra byte is for the newline ('\n') character */
		read_bytes = read(fix->fd, str, KUTF_USERDATA_LEN + 1);

		if (read_bytes < 0) {
			log_error_with_errno("userdata read failed", errno);
			return NULL;
		}
		if (read_bytes == 0) {
			/* End of file */
			mali_utf_logerr("userdata read failed with end-of-file\n");
			return NULL;
		}

		if (str[read_bytes - 1] == '\n') {
			/* Remove the trailing newline and NULL terminate */
			str[read_bytes - 1] = '\0';
		} else {
			/* NULL terminate the string */
			str[read_bytes] = '\0';
		}

		mali_utf_loginf("<%s\n", str);

#define KUTF_RESULT_PREFIX "KUTF_RESULT_"

		if (strncmp(str, KUTF_RESULT_PREFIX, strlen(KUTF_RESULT_PREFIX)) == 0) {
			/* Looks like a result, not user data */
			kutf_test_runner_add_result_external(str);
			/* Read the next line */
		} else {
			break;
		}
	} while (1);

	/* Return the user data */
	return str;
}

const char *valtype_names[] = {
	"INVALID",
	"U64",
	"STR",
};

const char *get_val_type_name(enum kutf_test_helpers_valtype valtype)
{
	/* enums can be signed or unsigned (implementation dependant), so
	 * enforce it to prevent:
	 * a) "<0 comparison on unsigned type" warning - if we did both upper
	 *    and lower bound check
	 * b) incorrect range checking if it was a signed type - if we did
	 *    upper bound check only
	 */
	unsigned int type_idx = (unsigned int)valtype;

	if (type_idx >= (unsigned int)KUTF_TEST_HELPERS_VALTYPE_COUNT)
		type_idx = (unsigned int)KUTF_TEST_HELPERS_VALTYPE_INVALID;

	return valtype_names[type_idx];
}

/* Check up to str_len chars of val_str to see if it's a valid value name:
 *
 * - Has between 1 and KUTF_TEST_HELPERS_MAX_VAL_NAME_LEN characters before the \0 terminator
 * - And, each char is in the character set [A-Z0-9_]
 */
static int validate_val_name(char *val_str, size_t str_len)
{
	int i = 0;

	for (i = 0; str_len && i <= KUTF_TEST_HELPERS_MAX_VAL_NAME_LEN && val_str[i] != '\0';
	     ++i, --str_len) {
		char val_chr = val_str[i];

		if (val_chr >= 'A' && val_chr <= 'Z')
			continue;
		if (val_chr >= '0' && val_chr <= '9')
			continue;
		if (val_chr == '_')
			continue;

		/* Character not in the set [A-Z0-9_] - report error */
		return 1;
	}

	/* Names of 0 length are not valid */
	if (i == 0)
		return 1;
	/* Length greater than KUTF_TEST_HELPERS_MAX_VAL_NAME_LEN not allowed */
	if (i > KUTF_TEST_HELPERS_MAX_VAL_NAME_LEN ||
	    (i == KUTF_TEST_HELPERS_MAX_VAL_NAME_LEN && val_str[i] != '\0'))
		return 1;

	return 0;
}

/* Find the length of the valid part of the string when it will be in quotes
 * e.g. "str"
 *
 * That is, before any '\\', '\n' or '"' characters. This is so we don't have
 * to escape the string
 */
static size_t find_quoted_string_valid_len(char *str)
{
	char *ptr;
	const char *check_chars = "\\\n\"";

	ptr = strpbrk(str, check_chars);
	if (ptr)
		return ptr - str;

	return strlen(str);
}

#define MAX_U64_HEX_LEN 16
/* (Name size) + ("=0x" size) + (64-bit hex value size) + (terminator) */
#define NAMED_U64_VAL_BUF_SZ (KUTF_TEST_HELPERS_MAX_VAL_NAME_LEN + 3 + MAX_U64_HEX_LEN + 1)

int kutf_test_helpers_userdata_send_named_u64(struct mali_utf_suite *suite, char *val_name,
					      uint64_t val)
{
	int ret = 1;
	char msgbuf[NAMED_U64_VAL_BUF_SZ];

	if (validate_val_name(val_name, KUTF_TEST_HELPERS_MAX_VAL_NAME_LEN + 1)) {
		mali_utf_test_fail("Failed to send uint64_t value named '%s': Invalid value name",
				   val_name);
		goto out_err;
	}

	ret = snprintf(msgbuf, NAMED_U64_VAL_BUF_SZ, "%s=0x%" PRIx64, val_name, val);
	if (ret >= NAMED_U64_VAL_BUF_SZ || ret < 0) {
		mali_utf_test_fail(
			"Failed to send uint64_t value named '%s': snprintf() problem buffer size==%d ret=%d",
			val_name, NAMED_U64_VAL_BUF_SZ, ret);
		goto out_err;
	}
	msgbuf[NAMED_U64_VAL_BUF_SZ - 1] = '\0';

	ret = kutf_test_helpers_userdata_send(suite, msgbuf);
	if (ret) {
		mali_utf_test_fail("Failed to send uint64_t value named '%s': send returned %d",
				   val_name, ret);
		goto out_err;
	}

out_err:
	return ret;
}

#define NAMED_VALUE_SEP "="
#define NAMED_STR_START_DELIM NAMED_VALUE_SEP "\""
#define NAMED_STR_END_DELIM "\""

int kutf_test_helpers_userdata_max_str_len_for_kern(char *val_name, int kern_buf_sz)
{
	int val_name_len = strlen(val_name);
	int start_delim_len = strlen(NAMED_STR_START_DELIM);
	int end_delim_len = strlen(NAMED_STR_END_DELIM);
	int max_str_len;

	max_str_len = kern_buf_sz - val_name_len - start_delim_len - end_delim_len;

	return max_str_len;
}

int kutf_test_helpers_userdata_send_named_str(struct mali_utf_suite *suite, char *val_name,
					      char *val_str)
{
	int val_str_len;
	int str_buf_sz;
	char *str_buf = NULL;
	int ret = 1;
	char *copy_ptr;
	int val_name_len;
	int start_delim_len = strlen(NAMED_STR_START_DELIM);
	int end_delim_len = strlen(NAMED_STR_END_DELIM);

	if (validate_val_name(val_name, KUTF_TEST_HELPERS_MAX_VAL_NAME_LEN + 1)) {
		mali_utf_test_fail("Failed to send str value named '%s': Invalid value name",
				   val_name);
		goto out_err;
	}
	val_name_len = strlen(val_name);

	val_str_len = find_quoted_string_valid_len(val_str);

	/* (name length) + ("=\"" length) + (val_str len) + ("\"" length) + terminator */
	str_buf_sz = val_name_len + start_delim_len + val_str_len + end_delim_len + 1;

	/* Using malloc() here instead of mempool since we know we need to free
	 * before we return
	 */
	str_buf = malloc(str_buf_sz);
	if (!str_buf) {
		mali_utf_test_fail(
			"Failed to send str value named '%s': malloc failed, str_buf_sz=%d",
			val_name, str_buf_sz);
		goto out_err;
	}
	copy_ptr = str_buf;

	/* Manually copy each string component instead of snprintf because
	 * val_str may need to end early, and less error path handling
	 */

	/* name */
	memcpy(copy_ptr, val_name, val_name_len);
	copy_ptr += val_name_len;

	/* str start delimiter */
	memcpy(copy_ptr, NAMED_STR_START_DELIM, start_delim_len);
	copy_ptr += start_delim_len;

	/* str value */
	memcpy(copy_ptr, val_str, val_str_len);
	copy_ptr += val_str_len;

	/* str end delimiter */
	memcpy(copy_ptr, NAMED_STR_END_DELIM, end_delim_len);
	copy_ptr += end_delim_len;

	/* Terminator */
	*copy_ptr = '\0';

	ret = kutf_test_helpers_userdata_send(suite, str_buf);
	if (ret) {
		mali_utf_test_fail("Failed to send str value named '%s': send returned %d",
				   val_name, ret);
		goto out_err;
	}

out_err:
	free(str_buf);
	return ret;
}

int kutf_test_helpers_userdata_receive_named_val(struct kutf_test_helpers_named_val *named_val,
						 struct mali_utf_suite *suite)
{
	char *recv_str;
	char *search_ptr;
	char *name_str = NULL;
	size_t name_len;
	size_t strval_len;
	enum kutf_test_helpers_valtype type = KUTF_TEST_HELPERS_VALTYPE_INVALID;
	char *strval = NULL;
	uint64_t u64val = 0;
	int err = KUTF_TEST_HELPERS_ERR_INVALID_VALUE;

	recv_str = kutf_test_helpers_userdata_receive(suite);
	if (!recv_str)
		return KUTF_TEST_HELPERS_ERR_RECV_FAULT;

	/* Find the '=', grab the name and validate it */
	search_ptr = strchr(recv_str, NAMED_VALUE_SEP[0]);
	if (search_ptr) {
		name_len = (size_t)(search_ptr - recv_str);
		if (!validate_val_name(recv_str, name_len)) {
			/* no need to reallocate - just modify string in place */
			name_str = recv_str;
			name_str[name_len] = '\0';

			/* Move until after the '=' */
			recv_str += name_len + 1;
		}
	}
	if (!name_str) {
		mali_utf_logerr("Invalid name part for received string '%s'\n", recv_str);
		return KUTF_TEST_HELPERS_ERR_INVALID_NAME;
	}

	/* detect value type */
	if (*recv_str == NAMED_STR_START_DELIM[1]) {
		/* string delimiter start*/
		++recv_str;

		/* Find end of string */
		search_ptr = strchr(recv_str, NAMED_STR_END_DELIM[0]);
		if (search_ptr) {
			strval_len = (size_t)(search_ptr - recv_str);
			/* Validate the string to ensure it contains no quotes */
			if (strval_len == find_quoted_string_valid_len(recv_str)) {
				/* no need to reallocate - just modify string in place */
				strval = recv_str;
				strval[strval_len] = '\0';

				/* Move until after the end delimiter */
				recv_str += (strval_len + 1);
				type = KUTF_TEST_HELPERS_VALTYPE_STR;
			} else {
				mali_utf_logerr(
					"String value contains invalid characters in rest of received string '%s'\n",
					recv_str);
				err = KUTF_TEST_HELPERS_ERR_CHARS_AFTER_VAL;
			}
		} else {
			/* No end-delimiter found. We don't know the kernel
			 * buffer size that caused the truncation, so just
			 * accept the string as being delimited by '\n'
			 */
			strval_len = strlen(recv_str);
			/* Validate the string to ensure it contains no quotes */
			if (strval_len == find_quoted_string_valid_len(recv_str)) {
				strval = recv_str;

				/* Move to the end of the string */
				recv_str += strval_len;
				type = KUTF_TEST_HELPERS_VALTYPE_STR;
			} else {
				mali_utf_logerr(
					"String value contains invalid characters in rest of received string '%s'\n",
					recv_str);
				err = KUTF_TEST_HELPERS_ERR_CHARS_AFTER_VAL;
			}
		}
	} else {
		errno = 0;
		/* possibly a number value - strtoull will parse it */
		u64val = strtoull(recv_str, &search_ptr, 0);
		if (search_ptr != recv_str && !errno) {
			type = KUTF_TEST_HELPERS_VALTYPE_U64;
			recv_str = search_ptr;
		} else {
			/* special case: not a number, report as such */
			mali_utf_logerr(
				"Rest of received string was not a numeric value or quoted string value: '%s', errno=%d\n",
				recv_str, errno);
			err = KUTF_TEST_HELPERS_ERR_INVALID_VALUE;
		}
	}

	if (type == KUTF_TEST_HELPERS_VALTYPE_INVALID)
		return err;

	/* Any remaining characters - error */
	if (strlen(recv_str) != 0) {
		mali_utf_logerr("Characters remain after value of type %s: '%s'\n",
				(type == KUTF_TEST_HELPERS_VALTYPE_U64) ? "u64" : "string",
				recv_str);
		return KUTF_TEST_HELPERS_ERR_CHARS_AFTER_VAL;
	}

	/* Success - write into the output structure */
	switch (type) {
	case KUTF_TEST_HELPERS_VALTYPE_U64:
		named_val->u.val_u64 = u64val;
		break;
	case KUTF_TEST_HELPERS_VALTYPE_STR:
		named_val->u.val_str = strval;
		break;
	default:
		mali_utf_logerr("Unreachable, fix userdata_receive_named_val\n");
		return 1;
	}

	named_val->val_name = name_str;
	named_val->type = type;

	return KUTF_TEST_HELPERS_ERR_NONE;
}

#define DUMMY_MSG "<placeholder due to test fail>"
int kutf_test_helpers_userdata_receive_check_val(struct kutf_test_helpers_named_val *named_val,
						 struct mali_utf_suite *suite,
						 char *expect_val_name,
						 enum kutf_test_helpers_valtype expect_val_type)
{
	int err;

	err = kutf_test_helpers_userdata_receive_named_val(named_val, suite);
	if (err < 0) {
		mali_utf_test_fail("Failed to receive value named '%s'", expect_val_name);
		return err;
	} else if (err > 0) {
		mali_utf_test_fail("Named-value parse error when expecting value named '%s'",
				   expect_val_name);
		goto out_fail_and_fixup;
	}

	if (strcmp(named_val->val_name, expect_val_name) != 0) {
		mali_utf_test_fail("Expecting to receive value named '%s' but got '%s'",
				   expect_val_name, named_val->val_name);
		goto out_fail_and_fixup;
	}

	if (named_val->type != expect_val_type) {
		mali_utf_test_fail("Expecting value named '%s' to be of type %s but got %s",
				   expect_val_name, get_val_type_name(expect_val_type),
				   get_val_type_name(named_val->type));
		goto out_fail_and_fixup;
	}

	return err;

out_fail_and_fixup:
	/* Produce a valid but incorrect value */
	switch (expect_val_type) {
	case KUTF_TEST_HELPERS_VALTYPE_U64:
		named_val->u.val_u64 = 0ull;
		break;
	case KUTF_TEST_HELPERS_VALTYPE_STR: {
		size_t len = sizeof(DUMMY_MSG);
		char *str = mali_utf_test_alloc(len);

		if (!str)
			return -1;

		strncpy(str, DUMMY_MSG, len);
		str[len - 1] = '\0';
		named_val->u.val_str = str;
		break;
	}
	default:
		break;
	}

	/* Indicate that this is invalid */
	named_val->type = KUTF_TEST_HELPERS_VALTYPE_INVALID;

	/* But at least allow the caller to continue in the test with failures */
	return 0;
}

void kutf_test_helpers_output_named_val(struct kutf_test_helpers_named_val *named_val)
{
	switch (named_val->type) {
	case KUTF_TEST_HELPERS_VALTYPE_U64:
		mali_utf_loginf("%s=0x%" PRIx64 "\n", named_val->val_name, named_val->u.val_u64);
		break;
	case KUTF_TEST_HELPERS_VALTYPE_STR:
		mali_utf_loginf("%s=\"%s\"\n", named_val->val_name, named_val->u.val_str);
		break;
	case KUTF_TEST_HELPERS_VALTYPE_INVALID:
		mali_utf_loginf("%s is invalid\n", named_val->val_name);
		break;
	default:
		mali_utf_loginf("%s has unknown type %d\n", named_val->val_name, named_val->type);
		break;
	}
}
