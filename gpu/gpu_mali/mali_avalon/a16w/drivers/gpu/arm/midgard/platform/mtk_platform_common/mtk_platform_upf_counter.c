// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>

unsigned long long upf_counter;

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_upf_counter_show(struct seq_file *m, void *v)
{
	seq_printf(m, "upf_counter = %lld\n", upf_counter);

	return 0;
}

static int mtk_upf_counter_open(struct inode *in, struct file *file)
{
	struct kbase_device *kbdev = in->i_private;
	file->private_data = kbdev;

	if (file->f_mode & FMODE_WRITE)
		return 0;

	return single_open(file, mtk_upf_counter_show, in->i_private);
}

static int mtk_upf_counter_release(struct inode *in, struct file *file)
{
	if (!(file->f_mode & FMODE_WRITE)) {
		struct seq_file *m = (struct seq_file *)file->private_data;

		if (m)
			seq_release(in, file);
	}

	return 0;
}

static ssize_t mtk_upf_counter_write(struct file *file, const char __user *ubuf,
			size_t count, loff_t *ppos)
{
	struct kbase_device *kbdev = (struct kbase_device *)file->private_data;
	int ret = 0;
    int temp = 0;
    CSTD_UNUSED(ppos);

	ret = kstrtoint_from_user(ubuf, count, 0, &temp);
	if (ret)
		return ret;

	/* Reset the upf counter if user write 0 */
	if (temp == 0)
		upf_counter = 0;

	dev_info(kbdev->dev, "upf_counter reset to = %lld", upf_counter);

	return count;
}

static const struct file_operations mtk_upf_counter_fops = {
	.open    = mtk_upf_counter_open,
	.release = mtk_upf_counter_release,
	.read    = seq_read,
	.write   = mtk_upf_counter_write,
	.llseek  = seq_lseek
};

int mtk_upf_counter_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("upf_counter", 0444,
			kbdev->mali_debugfs_directory, kbdev,
			&mtk_upf_counter_fops);
	return 0;
}
#else /* CONFIG_DEBUG_FS */
int mtk_upf_counter_debugfs_init(struct kbase_device *kbdev)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */

unsigned long long mtk_upf_counter_get(void)
{
	return upf_counter;
}

void mtk_upf_counter_reset(void)
{
	upf_counter = 0;
}

void mtk_upf_counter_add(void)
{
	upf_counter ++;
}

int mtk_upf_counter_init(void)
{
    upf_counter = 0;
	return 0;
}
