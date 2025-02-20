// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 MediaTek Inc.
 */

#include "c2ps_sysfs.h"
#include "c2ps_common.h"

#define C2PS_SYSFS_DIR_NAME "c2ps"

static struct kobject *c2ps_kobj;

int c2ps_sysfs_create_dir(struct kobject *parent,
	const char *name, struct kobject **ppsKobj)
{
	struct kobject *psKobj = NULL;

	if (name == NULL || ppsKobj == NULL) {
		C2PS_LOGE("Failed to create sysfs directory %p %p\n",
				name, ppsKobj);
		return -1;
	}

	parent = (parent != NULL) ? parent : c2ps_kobj;
	psKobj = kobject_create_and_add(name, parent);
	if (!psKobj) {
		C2PS_LOGE("Failed to create '%s' sysfs directory\n",
				name);
		return -1;
	}
	*ppsKobj = psKobj;

	return 0;
}

void c2ps_sysfs_remove_dir(struct kobject **ppsKobj)
{
	if (ppsKobj == NULL)
		return;
	kobject_put(*ppsKobj);
	*ppsKobj = NULL;
}

void c2ps_sysfs_create_file(struct kobject *parent,
	struct kobj_attribute *kobj_attr)
{
	if (kobj_attr == NULL) {
		C2PS_LOGE("Failed to create '%s' sysfs file kobj_attr=NULL\n",
				  C2PS_SYSFS_DIR_NAME);
		return;
	}

	parent = (parent != NULL) ? parent : c2ps_kobj;
	if (sysfs_create_file(parent, &(kobj_attr->attr))) {
		C2PS_LOGE("Failed to create sysfs file\n");
		return;
	}
}

void c2ps_sysfs_remove_file(struct kobject *parent,
	struct kobj_attribute *kobj_attr)
{
	if (kobj_attr == NULL)
		return;
	parent = (parent != NULL) ? parent : c2ps_kobj;
	sysfs_remove_file(parent, &(kobj_attr->attr));
}

void c2ps_sysfs_init(void)
{
	c2ps_kobj = kobject_create_and_add(C2PS_SYSFS_DIR_NAME,
			kernel_kobj);
	if (!c2ps_kobj) {
		C2PS_LOGE("Failed to create '%s' sysfs root directory\n",
				C2PS_SYSFS_DIR_NAME);
	}

}

void c2ps_sysfs_exit(void)
{
	kobject_put(c2ps_kobj);
	c2ps_kobj = NULL;
}
