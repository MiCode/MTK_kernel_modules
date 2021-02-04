/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>

#define MET_USER_EVENT_SUPPORT
#include "met_drv.h"
#include "trace.h"

static int met_backlight_enable;
static DEFINE_SPINLOCK(met_backlight_lock);
static struct kobject *kobj_met_backlight;

static ssize_t bl_tag_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t bl_tag_enable_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf, size_t n);
static struct kobj_attribute bl_tag_enable_attr =
__ATTR(backlight_tag_enable, 0664, bl_tag_enable_show, bl_tag_enable_store);

int enable_met_backlight_tag(void)
{
	return met_backlight_enable;
}

int output_met_backlight_tag(int level)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&met_backlight_lock, flags);
#ifdef CONFIG_MET_MODULE
	ret = met_tag_oneshot_real(33880, "_MM_BL_", level);
#else
	ret = met_tag_oneshot(33880, "_MM_BL_", level);
#endif
	spin_unlock_irqrestore(&met_backlight_lock, flags);

	return ret;
}

static ssize_t bl_tag_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", met_backlight_enable);

	return ret;
}

static ssize_t bl_tag_enable_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf, size_t n)
{
	int value;

	if ((n == 0) || (buf == NULL))
		return -EINVAL;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;

	if (value < 0)
		return -EINVAL;

	met_backlight_enable = value;

	return n;
}

static int met_backlight_create(struct kobject *parent)
{
	int ret = 0;

	kobj_met_backlight = parent;

	ret = sysfs_create_file(kobj_met_backlight, &bl_tag_enable_attr.attr);
	if (ret != 0) {
		pr_debug("Failed to create montype0 in sysfs\n");
		return ret;
	}

	return ret;
}

static void met_backlight_delete(void)
{
	sysfs_remove_file(kobj_met_backlight, &bl_tag_enable_attr.attr);
	kobj_met_backlight = NULL;
}

struct metdevice met_backlight = {
	.name = "backlight",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.create_subfs = met_backlight_create,
	.delete_subfs = met_backlight_delete,
	.cpu_related = 0,
};
EXPORT_SYMBOL(met_backlight);
