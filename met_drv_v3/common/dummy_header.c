// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "interface.h"
#include "met_drv.h"
#include "mtk_typedefs.h"

static struct kobject *kobj_met_dummy;
static char header_str[PAGE_SIZE];
static int header_str_len;
struct metdevice met_dummy_header;

static ssize_t dummy_str_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t dummy_str_store(struct kobject *kobj,
			       struct kobj_attribute *attr, const char *buf, size_t n);
static struct kobj_attribute dummy_attr = __ATTR(dummy_str, 0664, dummy_str_show, dummy_str_store);


static ssize_t dummy_str_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int ret;

	ret = SNPRINTF(buf, PAGE_SIZE, "%s", header_str);

	return ret;
}

static ssize_t dummy_str_store(struct kobject *kobj,
			       struct kobj_attribute *attr, const char *buf, size_t n)
{
	int ret = 0;
	char *ptr = header_str;

	if ((header_str_len + strlen(buf)) < PAGE_SIZE) {
		ret = SNPRINTF(ptr + header_str_len, PAGE_SIZE - header_str_len, "%s\n", buf);
		header_str_len += ret;
	}
	met_dummy_header.mode = 1;

	return n;
}

static int dummy_reset(void)
{
	met_dummy_header.mode = 0;
	memset(header_str, 0x00, PAGE_SIZE);
	header_str_len = 0;

	return 0;
}

static int dummy_print_header(char *buf, int len)
{
	if (header_str_len > 0)
		len = SNPRINTF(buf, PAGE_SIZE, "%s", header_str);
	else
		len = SNPRINTF(buf, PAGE_SIZE, "# dummy header is empty\n");

	return len;
}

static int dummy_create(struct kobject *parent)
{
	int ret = 0;

	kobj_met_dummy = parent;
	ret = sysfs_create_file(kobj_met_dummy, &dummy_attr.attr);
	if (ret != 0) {
		pr_debug("Failed to create montype0 in sysfs\n");
		return ret;
	}

	return ret;
}

static void dummy_delete(void)
{
	sysfs_remove_file(kobj_met_dummy, &dummy_attr.attr);
	kobj_met_dummy = NULL;
}

struct metdevice met_dummy_header = {
	.name = "dummy_header",
	.type = MET_TYPE_MISC,
	.cpu_related = 0,
	.start = NULL,
	.stop = NULL,
	.reset = dummy_reset,
	.polling_interval = 0,
	.timed_polling = NULL,
	.print_help = NULL,
	.print_header = dummy_print_header,
	.create_subfs = dummy_create,
	.delete_subfs = dummy_delete,
};
