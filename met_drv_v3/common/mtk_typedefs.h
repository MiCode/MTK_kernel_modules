/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MT_TYPEDEFS_H__

#include "interface.h"

/*
 *  KOBJ ATTR Manipulations Macros
 */

#define KOBJ_ITEM_LIST(args...)		args

/*
 * Declaring KOBJ attributes
 */
#define DECLARE_KOBJ_ATTR(attr_name) \
	static struct kobj_attribute attr_name##_attr = \
		__ATTR(attr_name, 0664, attr_name##_show, attr_name##_store)

#define DECLARE_KOBJ_ATTR_RO(attr_name) \
	static struct kobj_attribute attr_name##_attr = \
		__ATTR_RO(attr_name)

/*
 * Declaring KOBJ attributes with integer variable
 */
/* normal version */
#define DECLARE_KOBJ_ATTR_SHOW_INT(attr_name, var_name) \
	static ssize_t attr_name##_show( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		char *buf) \
	{ \
		return snprintf(buf, PAGE_SIZE, "%d\n", var_name); \
	}
#define DECLARE_KOBJ_ATTR_STORE_INT(attr_name, var_name) \
	static ssize_t attr_name##_store( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		const char *buf, \
		size_t n) \
	{ \
		int	val = 0; \
		if (kstrtoint(buf, 0, &val) != 0) { \
			return -EINVAL; \
		} \
		var_name = val; \
		return n; \
	}
#define DECLARE_KOBJ_ATTR_INT(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_SHOW_INT(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_STORE_INT(attr_name, var_name) \
	DECLARE_KOBJ_ATTR(attr_name)
#define DECLARE_KOBJ_ATTR_RO_INT(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_SHOW_INT(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_RO(attr_name)

/* cond-check version */
#define DECLARE_KOBJ_ATTR_STORE_INT_CHECK(attr_name, var_name, cond) \
	static ssize_t attr_name##_store( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		const char *buf, \
		size_t n) \
	{ \
		int	var_name##temp = var_name; \
		if (kstrtoint(buf, 0, &var_name) != 0) { \
			var_name = var_name##temp; \
			return -EINVAL; \
		} \
		if (cond) { \
			return n; \
		} else { \
			var_name = var_name##temp; \
			return -EINVAL; \
		} \
	}
/* Note: the name of val in cond can NOT be the same as var_name */
#define DECLARE_KOBJ_ATTR_INT_CHECK(attr_name, var_name, cond) \
	DECLARE_KOBJ_ATTR_SHOW_INT(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_STORE_INT_CHECK(attr_name, var_name, cond) \
	DECLARE_KOBJ_ATTR(attr_name)

/* helper procedure version */
#define DECLARE_KOBJ_ATTR_SHOW_INT_PROC(attr_name, var_name, func) \
	static ssize_t attr_name##_show( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		char *buf) \
	{ \
		return func(kobj, attr, buf, var_name); \
	}
#define DECLARE_KOBJ_ATTR_STORE_INT_PROC(attr_name, var_name, func) \
	static ssize_t attr_name##_store( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		const char *buf, \
		size_t n) \
	{ \
		return func(kobj, attr, buf, n, &(var_name)); \
	}
#define DECLARE_KOBJ_ATTR_INT_PROC(attr_name, var_name, show, store) \
	DECLARE_KOBJ_ATTR_SHOW_INT_PROC(attr_name, var_name, show) \
	DECLARE_KOBJ_ATTR_STORE_INT_PROC(attr_name, var_name, store) \
	DECLARE_KOBJ_ATTR(attr_name)

/*
 * Declaring KOBJ attributes with integer(hex) variable
 */
/* normal version */
#define DECLARE_KOBJ_ATTR_SHOW_HEX(attr_name, var_name) \
	static ssize_t attr_name##_show( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		char *buf) \
	{ \
		return snprintf(buf, PAGE_SIZE, "%x\n", var_name); \
	}
#define DECLARE_KOBJ_ATTR_STORE_HEX(attr_name, var_name) \
	static ssize_t attr_name##_store( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		const char *buf, \
		size_t n) \
	{ \
		unsigned int	val = 0; \
		if (kstrtouint(buf, 0, &val) != 0) { \
			return -EINVAL; \
		} \
		var_name = val; \
		return n; \
	}
#define DECLARE_KOBJ_ATTR_HEX(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_SHOW_HEX(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_STORE_HEX(attr_name, var_name) \
	DECLARE_KOBJ_ATTR(attr_name)
#define DECLARE_KOBJ_ATTR_RO_HEX(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_SHOW_HEX(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_RO(attr_name)

/* cond-check version */
#define DECLARE_KOBJ_ATTR_STORE_HEX_CHECK(attr_name, var_name, cond) \
	static ssize_t attr_name##_store( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		const char *buf, \
		size_t n) \
	{ \
		unsigned int	var_name##temp = var_name; \
		if (kstrtouint(buf, 0, &var_name) != 0) { \
			var_name = var_name##temp; \
			return -EINVAL; \
		} \
		if (cond) { \
			return n; \
		} else { \
			var_name = var_name##temp; \
			return -EINVAL; \
		} \
	}
/* Note: the name of val in cond can NOT be the same as var_name */
#define DECLARE_KOBJ_ATTR_HEX_CHECK(attr_name, var_name, cond) \
	DECLARE_KOBJ_ATTR_SHOW_HEX(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_STORE_HEX_CHECK(attr_name, var_name, cond) \
	DECLARE_KOBJ_ATTR(attr_name)

/* helper procedure version */
#define DECLARE_KOBJ_ATTR_SHOW_HEX_PROC(attr_name, var_name, func) \
	static ssize_t attr_name##_show( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		char *buf) \
	{ \
		return func(kobj, attr, buf, var_name); \
	}
#define DECLARE_KOBJ_ATTR_STORE_HEX_PROC(attr_name, var_name, func) \
	static ssize_t attr_name##_store( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		const char *buf, \
		size_t n) \
	{ \
		return func(kobj, attr, buf, n, &(var_name)); \
	}
#define DECLARE_KOBJ_ATTR_HEX_PROC(attr_name, var_name, show, store) \
	DECLARE_KOBJ_ATTR_SHOW_HEX_PROC(attr_name, var_name, show) \
	DECLARE_KOBJ_ATTR_STORE_HEX_PROC(attr_name, var_name, store) \
	DECLARE_KOBJ_ATTR(attr_name)

/*
 * Declaring KOBJ attributes with string variable
 */
#define DECLARE_KOBJ_ATTR_SHOW_STR(attr_name, var_name) \
	static ssize_t attr_name##_show( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		char *buf) \
	{ \
		return snprintf(buf, PAGE_SIZE, "%s", var_name); \
	}

#define DECLARE_KOBJ_ATTR_RO_STR(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_SHOW_STR(attr_name, var_name) \
	DECLARE_KOBJ_ATTR_RO(attr_name)

/*
 * Declaring KOBJ attributes with integer list variable
 */
#define DECLARE_KOBJ_ATTR_INT_LIST_ITEM(list_name, list) \
	static struct list_name##_list_item_t { \
		int	key; \
		int	val; \
	} const list_name##_list_item[] = { list }
#define DECLARE_KOBJ_ATTR_SHOW_INT_LIST(attr_name, var_name, list_name) \
	static ssize_t attr_name##_show( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		char *buf) \
	{ \
		int	i; \
		for (i = 0; i < ARRAY_SIZE(list_name##_list_item); i++) { \
			if (var_name == list_name##_list_item[i].key) { \
				return snprintf(buf, \
						PAGE_SIZE, \
						"%d\n", \
						list_name##_list_item[i].val); \
			} \
		} \
		return snprintf(buf, PAGE_SIZE, "%d\n", -1); \
	}
#define DECLARE_KOBJ_ATTR_STORE_INT_LIST(attr_name, var_name, list_name) \
	static ssize_t attr_name##_store( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		const char *buf, \
		size_t n) \
	{ \
		int	value; \
		int	i; \
		if (kstrtoint(buf, 10, &value) != 0) \
			return -EINVAL; \
		for (i = 0; i < ARRAY_SIZE(list_name##_list_item); i++) { \
			if (value == list_name##_list_item[i].val) { \
				var_name = list_name##_list_item[i].key; \
				return n; \
			} \
		} \
		return -EINVAL; \
	}
#define DECLARE_KOBJ_ATTR_INT_LIST(attr_name, var_name, list_name) \
	DECLARE_KOBJ_ATTR_SHOW_INT_LIST(attr_name, var_name, list_name) \
	DECLARE_KOBJ_ATTR_STORE_INT_LIST(attr_name, var_name, list_name) \
	DECLARE_KOBJ_ATTR(attr_name)

/*
 * Declaring KOBJ attributes with string list variable
 */
#define DECLARE_KOBJ_ATTR_STR_LIST_ITEM(list_name, list) \
	static struct list_name##_list_item_t { \
		int	key; \
		char	*val; \
	} const list_name##_list_item[] = { list }
#define DECLARE_KOBJ_ATTR_SHOW_STR_LIST(attr_name, var_name, list_name) \
	static ssize_t attr_name##_show( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		char *buf) \
	{ \
		int	i; \
		for (i = 0; i < ARRAY_SIZE(list_name##_list_item); i++) { \
			if (var_name == list_name##_list_item[i].key) { \
				return snprintf(buf, \
						PAGE_SIZE, \
						"%s\n", \
						list_name##_list_item[i].val); \
			} \
		} \
		return snprintf(buf, PAGE_SIZE, "%s\n", "ERR"); \
	}
#define DECLARE_KOBJ_ATTR_STORE_STR_LIST(attr_name, var_name, list_name) \
	static ssize_t attr_name##_store( \
		struct kobject *kobj, \
		struct kobj_attribute *attr, \
		const char *buf, \
		size_t n) \
	{ \
		int	i; \
		for (i = 0; i < ARRAY_SIZE(list_name##_list_item); i++) { \
			if (strncasecmp(buf, \
					list_name##_list_item[i].val, \
					strlen(list_name##_list_item[i].val)) == 0) { \
				var_name = list_name##_list_item[i].key; \
				return n; \
			} \
		} \
		return -EINVAL; \
	}
#define DECLARE_KOBJ_ATTR_STR_LIST(attr_name, var_name, list_name) \
	DECLARE_KOBJ_ATTR_SHOW_STR_LIST(attr_name, var_name, list_name) \
	DECLARE_KOBJ_ATTR_STORE_STR_LIST(attr_name, var_name, list_name) \
	DECLARE_KOBJ_ATTR(attr_name)

/*
 *  MET Debug Message
 */
#define METINFO(format, ...)	pr_debug("[MET]%s: "format, __func__, ##__VA_ARGS__)
#define METERROR(format, ...)	pr_debug("[MET][ERR]%s: "format, __func__, ##__VA_ARGS__)

#define SNPRINTF(str, size, format, ...) ({\
       int _r_e_t_; \
       _r_e_t_ = snprintf(str, size, format, ##__VA_ARGS__); \
       if (_r_e_t_ < 0 || _r_e_t_ >= size) \
               PR_BOOTMSG("!!ERROR: SNPRINTF fail!!\n"); \
       _r_e_t_; \
       })

#define SPRINTF(str, format, ...) ({\
       int _r_e_t_; \
       _r_e_t_ = sprintf(str, format, ##__VA_ARGS__); \
       if (_r_e_t_ < 0) \
               PR_BOOTMSG("!!ERROR: SPRINTF fail!!\n"); \
       _r_e_t_; \
       })

#endif	/* _MT_TYPEDEFS_H__ */
