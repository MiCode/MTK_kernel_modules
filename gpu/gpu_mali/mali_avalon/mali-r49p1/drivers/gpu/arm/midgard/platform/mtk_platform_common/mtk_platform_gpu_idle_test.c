// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <platform/mtk_platform_common.h>
#include <platform/mtk_platform_common/mtk_platform_gpu_idle_test.h>
#include <backend/gpu/mali_kbase_pm_internal.h>

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_FS)
static int mtk_debug_gpu_idle_test(struct seq_file *file, void *data)
{
	struct kbase_device *kbdev = file->private;
	struct device_node *np;
	unsigned long flags,flags2;

	if (IS_ERR_OR_NULL(kbdev))
		return -1;
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_csf_scheduler_spin_lock(kbdev,&flags2);
	kbase_csf_scheduler_process_gpu_idle_event(kbdev);
	kbase_csf_scheduler_spin_unlock(kbdev, flags2);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	return 0;
}

static int mtk_gpu_idle_test_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, mtk_debug_gpu_idle_test,
	                   in->i_private);
}

static const struct file_operations mtk_gpu_idle_test_debugfs_fops = {
	.open = mtk_gpu_idle_test_debugfs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

int mtk_debug_gpu_idle_test_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("gpu_idle_test", 0440,
		kbdev->mali_debugfs_directory, kbdev,
		&mtk_gpu_idle_test_debugfs_fops);

	return 0;
}

#endif /* CONFIG_MALI_MTK_DEBUG_FS */