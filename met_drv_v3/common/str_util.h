
/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _SRC_STR_UTIL_H_
#define _SRC_STR_UTIL_H_


struct met_str_array{
	char **str_ptr_array;
	int str_ptr_array_length;
	char *target_str;
};

struct met_str_array * met_util_str_split(const char *input_str, int delim);
void met_util_str_array_clean(struct met_str_array *str_array_obj);
int met_util_in_str_array(const char *input_str, int compare_char_count, struct met_str_array *str_array_obj);

#endif				/* _SRC_STR_UTIL_H_ */

