/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023 MediaTek Inc.
 */

#ifndef C2PS_COMMON_INCLUDE_C2PS_SYSFS_H_
#define C2PS_COMMON_INCLUDE_C2PS_SYSFS_H_

#include <linux/kobject.h>

#define C2PS_SYSFS_MAX_BUFF_SIZE 2048

#define KOBJ_ATTR_RW(_name) \
	struct kobj_attribute kobj_attr_##_name =   \
		__ATTR(_name, 0660, \
		_name##_show, _name##_store)
#define KOBJ_ATTR_RO(_name) \
	struct kobj_attribute kobj_attr_##_name =   \
		__ATTR(_name, 0440, \
		_name##_show, NULL)
#define KOBJ_ATTR_WO(_name) \
	struct kobj_attribute kobj_attr_##_name =   \
	__ATTR(_name, 0660, \
	NULL, _name##_store)
#define KOBJ_ATTR_RWO(_name)     \
	struct kobj_attribute kobj_attr_##_name =       \
		__ATTR(_name, 0664,     \
		_name##_show, _name##_store)
#define KOBJ_ATTR_ROO(_name)    \
	struct kobj_attribute kobj_attr_##_name =   \
	__ATTR(_name, 0444, \
		_name##_show, NULL)

int c2ps_sysfs_create_dir(struct kobject *parent,
	const char *name, struct kobject **ppsKobj);
void c2ps_sysfs_remove_dir(struct kobject **ppsKobj);
void c2ps_sysfs_create_file(struct kobject *parent,
	struct kobj_attribute *kobj_attr);
void c2ps_sysfs_remove_file(struct kobject *parent,
	struct kobj_attribute *kobj_attr);
void c2ps_sysfs_init(void);
void c2ps_sysfs_exit(void);

extern struct kobject *kernel_kobj;

#endif  // C2PS_COMMON_INCLUDE_C2PS_SYSFS_H_
