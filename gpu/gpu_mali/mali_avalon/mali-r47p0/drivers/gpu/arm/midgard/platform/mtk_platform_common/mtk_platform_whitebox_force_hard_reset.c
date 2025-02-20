// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>

bool force_hard_reset_enable;

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_whitebox_force_hard_reset_enable_show(struct seq_file *m, void *v)
{
	seq_printf(m, "force_hard_reset_enable = %d\n", force_hard_reset_enable);

	return 0;
}

static int mtk_whitebox_force_hard_reset_enable_open(struct inode *in, struct file *file)
{
	struct kbase_device *kbdev = in->i_private;
	file->private_data = kbdev;

	if (file->f_mode & FMODE_WRITE)
		return 0;

	return single_open(file, mtk_whitebox_force_hard_reset_enable_show, in->i_private);
}

static int mtk_whitebox_force_hard_reset_enable_release(struct inode *in, struct file *file)
{
	if (!(file->f_mode & FMODE_WRITE)) {
		struct seq_file *m = (struct seq_file *)file->private_data;

		if (m)
			seq_release(in, file);
	}

	return 0;
}

static ssize_t mtk_whitebox_force_hard_reset_enable_write(struct file *file, const char __user *ubuf,
			size_t count, loff_t *ppos)
{
	struct kbase_device *kbdev = (struct kbase_device *)file->private_data;
	int ret = 0;
    int temp = 0;
    CSTD_UNUSED(ppos);

	ret = kstrtoint_from_user(ubuf, count, 0, &temp);
	if (ret)
		return ret;

	if (temp == 1)
		force_hard_reset_enable = 1;
	else
		force_hard_reset_enable = 0;

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"[WHITEBOX] force_hard_reset_enable = %d\n", force_hard_reset_enable);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	dev_info(kbdev->dev, "[WHITEBOX] force_hard_reset_enable = %d", force_hard_reset_enable);

	return count;
}

static const struct file_operations mtk_whitebox_force_hard_reset_enable_fops = {
	.open    = mtk_whitebox_force_hard_reset_enable_open,
	.release = mtk_whitebox_force_hard_reset_enable_release,
	.read    = seq_read,
	.write   = mtk_whitebox_force_hard_reset_enable_write,
	.llseek  = seq_lseek
};

int mtk_whitebox_force_hard_reset_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("force_hard_reset_enable", 0444,
			kbdev->mali_debugfs_directory, kbdev,
			&mtk_whitebox_force_hard_reset_enable_fops);
	return 0;
}
#else /* CONFIG_DEBUG_FS */
int mtk_whitebox_force_hard_reset_debugfs_init(struct kbase_device *kbdev)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */

bool mtk_whitebox_force_hard_reset_enable(void)
{
	return force_hard_reset_enable;
}

int mtk_whitebox_force_hard_reset_init(void)
{
    force_hard_reset_enable = false;
	return 0;
}
