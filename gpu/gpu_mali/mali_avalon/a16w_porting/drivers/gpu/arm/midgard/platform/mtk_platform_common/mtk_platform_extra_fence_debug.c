// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include "mtk_platform_extra_fence_debug.h"

int extra_fence_debug_mode;

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_extra_fence_debug_show(struct seq_file *m, void *v)
{
	seq_printf(m, "extra_fence_debug_mode = %d\n",  extra_fence_debug_mode);

	return 0;
}

static int mtk_extra_fence_debug_open(struct inode *in, struct file *file)
{
	struct kbase_device *kbdev = in->i_private;
	file->private_data = kbdev;

	if (file->f_mode & FMODE_WRITE)
		return 0;

	return single_open(file, mtk_extra_fence_debug_show, in->i_private);
}

static int mtk_extra_fence_debug_release(struct inode *in, struct file *file)
{
	if (!(file->f_mode & FMODE_WRITE)) {
		struct seq_file *m = (struct seq_file *)file->private_data;

		if (m)
			seq_release(in, file);
	}

	return 0;
}

static ssize_t mtk_extra_fence_debug_write(struct file *file, const char __user *ubuf,
			size_t count, loff_t *ppos)
{
	struct kbase_device *kbdev = (struct kbase_device *)file->private_data;
	struct kbase_csf_scheduler *const scheduler = &kbdev->csf.scheduler;
	int ret = 0;
    int temp = 0;
	int i = 0;
    CSTD_UNUSED(ppos);

	ret = kstrtoint_from_user(ubuf, count, 0, &temp);
	if (ret)
		return ret;

	if (temp >= EXTRA_FENCE_DEBUG_MODE_NONE && temp < EXTRA_FENCE_DEBUG_MODE_COUNT)
		extra_fence_debug_mode = temp;
	else
		extra_fence_debug_mode = 0;

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"set extra_fence_debug = %d\n", extra_fence_debug_mode);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	dev_info(kbdev->dev, "set extra_fence_debug_mode = %d", extra_fence_debug_mode);

	return count;
}

static const struct file_operations mtk_extra_fence_debug_fops = {
	.open    = mtk_extra_fence_debug_open,
	.release = mtk_extra_fence_debug_release,
	.read    = seq_read,
	.write   = mtk_extra_fence_debug_write,
	.llseek  = seq_lseek
};

int mtk_extra_fence_debug_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("extra_fence_debug", 0444,
			kbdev->mali_debugfs_directory, kbdev,
			&mtk_extra_fence_debug_fops);
	return 0;
}
#else /* CONFIG_DEBUG_FS */
int mtk_extra_fence_debug_debugfs_init(struct kbase_device *kbdev)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */

int mtk_extra_fence_debug_mode(void)
{
	return extra_fence_debug_mode;
}

int mtk_extra_fence_debug_init(void)
{
    extra_fence_debug_mode = 0;
	return 0;
}
