// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define BUILD_WITH_MET
#define MET_USER_EVENT_SUPPORT

#ifdef MET_USER_EVENT_SUPPORT

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fs.h>
/* #include <asm/uaccess.h> */
#include <linux/uaccess.h>

#include "met_drv.h"
#include "met_tag.h"
#include "interface.h"
#include "switch.h"
#include "mtk_typedefs.h"

struct bltable_t bltab;

static int dump_buffer_size;
static int dump_data_size;
static int dump_overrun;
static int dump_overrun_size;
static int dump_seq_no;
static void *dump_buffer;

#define OPFLAG_OVERWRITE	0x1
static unsigned int options_flag;

#define DEVICE_NAME		"met_tag"

/* #define ERRF_ENABLE */
/* #define DEBF_ENABLE */

#ifdef ERRF_ENABLE
#define MSG_ERR			"Error:["DEVICE_NAME"]"
#define ERRF(args...)	pr_debug(MSG_ERR args)
#else
#define ERRF(args...)
#endif

#ifdef DEBF_ENABLE
#define MSG_IFO			"Info :["DEVICE_NAME"]"
#define DEBF(args...)	pr_debug(MSG_IFO args)
#else
#define DEBF(args...)
#endif

static int is_enabled(unsigned int class_id)
{
	int i;

	if (bltab.flag == 0)
		return 1;
	if (bltab.flag & MET_CLASS_ALL)
		return 0;

	for (i = 0; i < MAX_EVENT_CLASS; i++) {
		if ((bltab.flag & (1 << i)) && (bltab.class_id[i] == class_id))
			return 0;
	}

	return 1;
}

noinline int tracing_mark_write(int type, unsigned int class_id,
				       const char *name, unsigned int value,
				       unsigned int value2, unsigned int value3)
{
	if (type == TYPE_MET_SUSPEND) {
		MET_TRACE("C|0|MET_SUSPEND|1");
		return 0;
	}
	if (type == TYPE_MET_RESUME) {
		MET_TRACE("C|0|MET_SUSPEND|0");
		return 0;
	}
	if (!is_enabled(class_id))
		return 0;
	switch (type) {
	case TYPE_START:
		MET_TRACE("B|%d|%s\n", class_id, name);
		break;
	case TYPE_END:
		MET_TRACE("E|%s\n", name);
		break;
	case TYPE_ONESHOT:
		MET_TRACE("C|%d|%s|%d\n", class_id, name, value);
		break;
	case TYPE_ASYNC_START:
		MET_TRACE("S|%d|%s|%d\n", class_id, name, value);
		break;
	case TYPE_ASYNC_END:
		MET_TRACE("F|%d|%s|%d\n", class_id, name, value);
		break;
	case TYPE_DUMP:
		MET_TRACE("D|%d|%s|%d|%d|%d\n", class_id, name, value, value2, value3);
		break;
	default:
		return -1;
	}
	return 0;
}

int met_tag_init(void)
{
	memset(&bltab, 0, sizeof(struct bltable_t));
	bltab.flag = MET_CLASS_ALL;
	mutex_init(&bltab.mlock);
	return 0;
}

int met_tag_uninit(void)
{
	met_set_dump_buffer_real(0);
	return 0;
}

int met_tag_start_real(unsigned int class_id, const char *name)
{
	int ret;

	ret = tracing_mark_write(TYPE_START, class_id, name, 0, 0, 0);
#if 0
	if ((met_switch.mode & MT_SWITCH_TAGPOLLING)) {
		/* tag polling only enable when MT_SWITCH_TAGPOLLING is config */
		force_sample(NULL);
	}
#endif
	return ret;
}
EXPORT_SYMBOL(met_tag_start_real);

int met_tag_end_real(unsigned int class_id, const char *name)
{
	int ret;
#if 0
	if ((met_switch.mode & MT_SWITCH_TAGPOLLING)) {
		/* tag polling only enable when MT_SWITCH_TAGPOLLING is config */
		force_sample(NULL);
	}
#endif
	ret = tracing_mark_write(TYPE_END, class_id, name, 0, 0, 0);

	return ret;
}
EXPORT_SYMBOL(met_tag_end_real);

int met_tag_async_start_real(unsigned int class_id, const char *name, unsigned int cookie)
{
	int ret;

	ret = tracing_mark_write(TYPE_ASYNC_START, class_id, name, cookie, 0, 0);
#if 0
	if ((met_switch.mode & MT_SWITCH_TAGPOLLING)) {
		/* tag polling only enable when MT_SWITCH_TAGPOLLING is config */
		force_sample(NULL);
	}
#endif
	return ret;
}

int met_tag_async_end_real(unsigned int class_id, const char *name, unsigned int cookie)
{
	int ret;

#if 0
	if ((met_switch.mode & MT_SWITCH_TAGPOLLING)) {
		/* tag polling only enable when MT_SWITCH_TAGPOLLING is config */
		force_sample(NULL);
	}
#endif
	ret = tracing_mark_write(TYPE_ASYNC_END, class_id, name, cookie, 0, 0);
	return ret;
}

int met_tag_oneshot_real(unsigned int class_id, const char *name, unsigned int value)
{
	int ret;

	ret = tracing_mark_write(TYPE_ONESHOT, class_id, name, value, 0, 0);
#if 0
	if ((met_switch.mode & MT_SWITCH_TAGPOLLING)) {
		/* tag polling only enable when MT_SWITCH_TAGPOLLING is config */
		force_sample(NULL);
	}
#endif
	return ret;
}
EXPORT_SYMBOL(met_tag_oneshot_real);

int met_tag_userdata_real(char *pData)
{
	MET_TRACE("%s\n", pData);
	return 0;
}

int met_tag_dump_real(unsigned int class_id, const char *name, void *data, unsigned int length)
{
	int ret;

	if ((dump_data_size + length + sizeof(int)) > dump_buffer_size) {
		if (options_flag & OPFLAG_OVERWRITE) {
			dump_overrun_size = dump_data_size;
			dump_overrun++;
			memcpy(dump_buffer, &dump_seq_no, sizeof(int));
			memcpy(dump_buffer + sizeof(int), data, length);
			ret = tracing_mark_write(TYPE_DUMP, class_id, name,
						 dump_seq_no++, 0, length + sizeof(int));
			dump_data_size = length + sizeof(int);
		} else {
			ret = tracing_mark_write(TYPE_DUMP, class_id, name, dump_seq_no++, 0, 0);
		}
	} else {
		memcpy(dump_buffer + dump_data_size, &dump_seq_no, sizeof(int));
		memcpy(dump_buffer + dump_data_size + sizeof(int), data, length);
		ret = tracing_mark_write(TYPE_DUMP, class_id, name,
					 dump_seq_no++, dump_data_size, length + sizeof(int));
		dump_data_size += length + sizeof(int);
	}
	return ret;
}

int met_tag_disable_real(unsigned int class_id)
{
	int i;

	mutex_lock(&bltab.mlock);

	if (class_id == MET_CLASS_ALL) {
		bltab.flag |= MET_CLASS_ALL;
		mutex_unlock(&bltab.mlock);
		return 0;
	}

	for (i = 0; i < MAX_EVENT_CLASS; i++) {
		if ((bltab.flag & (1 << i)) == 0) {
			bltab.class_id[i] = class_id;
			bltab.flag |= (1 << i);
			mutex_unlock(&bltab.mlock);
			return 0;
		}
	}

	mutex_unlock(&bltab.mlock);
	return -1;
}

int met_tag_enable_real(unsigned int class_id)
{
	int i;

	mutex_lock(&bltab.mlock);

	if (class_id == MET_CLASS_ALL) {
		bltab.flag &= (~MET_CLASS_ALL);
		mutex_unlock(&bltab.mlock);
		return 0;
	}

	for (i = 0; i < MAX_EVENT_CLASS; i++) {
		if ((bltab.flag & (1 << i)) && (bltab.class_id[i] == class_id)) {
			bltab.flag &= (~(1 << i));
			bltab.class_id[i] = 0;
			mutex_unlock(&bltab.mlock);
			return 0;
		}
	}

	mutex_unlock(&bltab.mlock);
	return -1;
}

int met_set_dump_buffer_real(int size)
{
	if (dump_buffer_size && dump_buffer) {
		free_pages((unsigned long)dump_buffer, get_order(dump_buffer_size));
		dump_data_size = 0;
		dump_overrun = 0;
		dump_overrun_size = 0;
		dump_seq_no = 0;
		dump_buffer_size = 0;
	}
	/* size is 0 means free dump buffer */
	if (size == 0)
		return 0;

	if (size < 0)
		return -1;

	size = (size + (PAGE_SIZE - 1)) & (~(PAGE_SIZE - 1));
	dump_buffer = (void *)__get_free_pages(GFP_KERNEL, get_order(size));
	if (dump_buffer == NULL) {
		ERRF("can not allocate buffer to copy\n");
		return -ENOMEM;
	}

	dump_buffer_size = size;
	return dump_buffer_size;
}

#if 0
int met_save_dump_buffer_real(const char *pathname)
{
	int size, ret = 0;
	struct file *outfp = NULL;
	mm_segment_t oldfs;

	if (dump_data_size == 0)
		return 0;

	if (dump_data_size < 0 || dump_overrun_size < 0)
		return -1;

	if (dump_buffer == NULL || dump_buffer_size <= 0)
		return -1;

	if (dump_overrun)
		size = dump_overrun_size;
	else
		size = dump_data_size;

	if (size >= dump_buffer_size)
		return -1;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	outfp = filp_open(pathname, O_WRONLY | O_TRUNC | O_CREAT, 0664);
	if (unlikely(outfp == NULL)) {
		ERRF("can not open saved file for write\n");
		return -EIO;
	}

	ret = vfs_write(outfp, dump_buffer, size, &(outfp->f_pos));
	if (ret < 0)
		ERRF("can not write to dump file\n");
	else {
		dump_data_size = 0;
		dump_overrun = 0;
		dump_overrun_size = 0;
		dump_seq_no = 0;
	}

	set_fs(oldfs);

	if (outfp != NULL)
		filp_close(outfp, NULL);

	return 0;
}

int met_save_log_real(const char *pathname)
{
	int len, ret = 0;
	struct file *infp = NULL;
	struct file *outfp = NULL;
	void *ptr = NULL;
	mm_segment_t oldfs;

	infp = filp_open("/sys/kernel/debug/tracing/trace", O_RDONLY, 0);
	if (unlikely(infp == NULL)) {
		ERRF("can not open trace file for read\n");
		ret = -1;
		goto save_out;
	}

	outfp = filp_open(pathname, O_WRONLY | O_TRUNC | O_CREAT, 0664);
	if (unlikely(outfp == NULL)) {
		ERRF("can not open saved file for write\n");
		ret = -2;
		goto save_out;
	}

	ptr = (void *)__get_free_pages(GFP_KERNEL, 2);
	if (ptr == NULL) {
		ERRF("can not allocate buffer to copy\n");
		ret = -3;
		goto save_out;
	}

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	while (1) {
		len = vfs_read(infp, ptr, PAGE_SIZE << 2, &(infp->f_pos));
		if (len < 0) {
			ERRF("can not read from trace file\n");
			ret = -3;
			break;
		} else if (len == 0) {
			break;
		}

		ret = vfs_write(outfp, ptr, len, &(outfp->f_pos));
		if (ret < 0) {
			ERRF("can not write to saved file\n");
			break;
		}
	}

	set_fs(oldfs);

save_out:
	if (ptr != NULL)
		free_pages((unsigned long)ptr, 2);
	if (infp != NULL)
		filp_close(infp, NULL);
	if (outfp != NULL)
		filp_close(outfp, NULL);

	return ret;
}
#endif

#ifdef BUILD_WITH_MET
#include <linux/module.h>
#include <linux/uaccess.h>
/* =========================================================================== */
/* misc file nodes */
/* =========================================================================== */
static ssize_t enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i;

	i = SNPRINTF(buf, PAGE_SIZE, "%d\n", (bltab.flag >> 31) ? 0 : 1);
	return i;
}

static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
			    size_t n)
{
	int value;

	if ((n == 0) || (buf == NULL))
		return -EINVAL;
	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;

	mutex_lock(&bltab.mlock);

	if (value == 1)
		bltab.flag &= (~MET_CLASS_ALL);
	else
		bltab.flag |= MET_CLASS_ALL;

	mutex_unlock(&bltab.mlock);

	return n;
}

static struct kobject *kobj_tag;
static struct kobj_attribute enable_attr = __ATTR(enable, 0664, enable_show, enable_store);

static ssize_t dump_buffer_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i, size;

	if (dump_overrun)
		size = dump_overrun_size;
	else
		size = dump_data_size;

	i = SNPRINTF(buf, PAGE_SIZE, "Buffer Size (KB)=%d\nData Size (KB)=%d\nOverrun=%d\n",
		     dump_buffer_size >> 10, size >> 10, dump_overrun);
	return i;
}

static ssize_t dump_buffer_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
				 size_t n)
{
	int ret, value;

	if ((n == 0) || (buf == NULL))
		return -EINVAL;
	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;

	ret = met_set_dump_buffer_real(value << 10);

	if (ret < 0)
		return ret;

	return n;
}

static struct kobj_attribute dump_buffer_attr =
__ATTR(dump_buffer_kb, 0664, dump_buffer_show, dump_buffer_store);

static ssize_t options_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i = 0;

	buf[0] = 0;

	if (options_flag == 0) {
		strncat(buf, "none\n", PAGE_SIZE - 1 - i);
		i += 5;
	}

	if (options_flag & OPFLAG_OVERWRITE) {
		strncat(buf, "overwrite\n", PAGE_SIZE - 1 - i);
		i += 10;
	}

	return i;
}

static ssize_t options_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
			     size_t n)
{
	if ((n == 0) || (buf == NULL))
		return -EINVAL;

	if ((n == 1) && (buf[0] == 0xA)) {
		options_flag = 0;
		return n;
	}

	if (strncmp(buf, "overwrite", 9) == 0)
		options_flag |= OPFLAG_OVERWRITE;
	else
		return -EINVAL;

	return n;
}

static struct kobj_attribute options_attr = __ATTR(options, 0664, options_show, options_store);

int tag_reg(struct file_operations *const fops, struct kobject *kobj)
{
	int ret;

	kobj_tag = kobject_create_and_add("tag", kobj);
	if (kobj_tag == NULL) {
		ERRF("can not create kobject: kobj_bus\n");
		return -1;
	}

	ret = sysfs_create_file(kobj_tag, &enable_attr.attr);
	if (ret != 0) {
		ERRF("Failed to create enable in sysfs\n");
		kobject_del(kobj_tag);
		kobject_put(kobj_tag);
		kobj_tag = NULL;
		return ret;
	}

	ret = sysfs_create_file(kobj_tag, &dump_buffer_attr.attr);
	if (ret != 0) {
		ERRF("Failed to create dump_buffer in sysfs\n");
		sysfs_remove_file(kobj_tag, &enable_attr.attr);
		kobject_del(kobj_tag);
		kobject_put(kobj_tag);
		kobj_tag = NULL;
		return ret;
	}

	ret = sysfs_create_file(kobj_tag, &options_attr.attr);
	if (ret != 0) {
		ERRF("Failed to create options in sysfs\n");
		sysfs_remove_file(kobj_tag, &enable_attr.attr);
		sysfs_remove_file(kobj_tag, &dump_buffer_attr.attr);
		kobject_del(kobj_tag);
		kobject_put(kobj_tag);
		kobj_tag = NULL;
		return ret;
	}

	met_tag_init();

	return 0;
}

int tag_unreg(void)
{
	met_tag_uninit();
	sysfs_remove_file(kobj_tag, &enable_attr.attr);
	sysfs_remove_file(kobj_tag, &dump_buffer_attr.attr);
	sysfs_remove_file(kobj_tag, &options_attr.attr);
	kobject_del(kobj_tag);
	kobject_put(kobj_tag);
	kobj_tag = NULL;

	return 0;
}

#endif				/* BUILD_WITH_MET */

#else				/* not MET_USER_EVENT_SUPPORT */

#ifdef BUILD_WITH_MET
int tag_reg(void *p, void *q)
{
	return 0;
}

int tag_unreg(void)
{
	return 0;
}
#endif

#endif				/* MET_USER_EVENT_SUPPORT */
