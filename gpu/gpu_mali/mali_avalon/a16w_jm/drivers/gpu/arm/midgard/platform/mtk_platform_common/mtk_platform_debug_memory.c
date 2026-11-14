// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <linux/delay.h>
#include <backend/gpu/mali_kbase_pm_internal.h>

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_memory_debug_enable_show(struct seq_file *m, void *v)
{
	seq_printf(m, "WHITEBOX fault worker is ready for user\n");

	return 0;
}

static int mtk_memory_debug_enable_open(struct inode *in, struct file *file)
{
	struct kbase_device *kbdev = in->i_private;
	file->private_data = kbdev;

	if (file->f_mode & FMODE_WRITE)
		return 0;

	return single_open(file, mtk_memory_debug_enable_show, in->i_private);
}

static int mtk_memory_debug_enable_release(struct inode *in, struct file *file)
{
	if (!(file->f_mode & FMODE_WRITE)) {
		struct seq_file *m = (struct seq_file *)file->private_data;

		if (m)
			seq_release(in, file);
	}

	return 0;
}

static ssize_t mtk_memory_debug_enable_write(struct file *file, const char __user *ubuf,
			size_t count, loff_t *ppos)
{
	struct kbase_device *kbdev = (struct kbase_device *)file->private_data;
	int ret = 0;
	int temp = 0;
	CSTD_UNUSED(ppos);

	if (!kbdev)
		return -ENODEV;

	ret = kstrtoint_from_user(ubuf, count, 0, &temp);
	if (ret)
		return ret;

	kbdev->memory_debug_mode = temp;
    pr_info("[mali-debug] mtk_memory_debug_mode %d %d", kbdev->memory_debug_mode, kbdev->memory_debug_mode & 2);

	return count;
}

static const struct file_operations mtk_memory_debug_enable_fops = {
	.open    = mtk_memory_debug_enable_open,
	.release = mtk_memory_debug_enable_release,
	.read    = seq_read,
	.write   = mtk_memory_debug_enable_write,
	.llseek  = seq_lseek
};

int mtk_memory_debug_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("memory_debug_mode", 0444,
			kbdev->mali_debugfs_directory, kbdev,
			&mtk_memory_debug_enable_fops);
	return 0;
}
#else /* CONFIG_DEBUG_FS */
int mtk_memory_debug_debugfs_init(struct kbase_device *kbdev)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */
