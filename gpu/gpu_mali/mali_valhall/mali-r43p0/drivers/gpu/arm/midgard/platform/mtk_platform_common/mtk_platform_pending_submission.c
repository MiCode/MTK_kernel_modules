// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <platform/mtk_platform_common.h>
#include <platform/mtk_platform_common/mtk_platform_pending_submission.h>

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_FS)
static int mtk_debug_pending_submission_mode(struct seq_file *file, void *data)
{
	struct kbase_device *kbdev = file->private;
	struct device_node *np;
	u32 pending_submission_mode = 0;

	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	np = kbdev->dev->of_node;

	if (!of_property_read_u32(np, "pending-submission-mode", &pending_submission_mode))
		seq_printf(file, "Pending submission mode: %s\n",
			(pending_submission_mode == GPU_PENDING_SUBMISSION_KWORKER)? "Kworker":
			((pending_submission_mode == GPU_PENDING_SUBMISSION_KTHREAD)? "Kthread": "Undefined"));
	else
		seq_printf(file, "Pending submission mode: No dts property setting, default disabled\n");

	return 0;
}

static int mtk_pending_submission_mode_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, mtk_debug_pending_submission_mode,
	                   in->i_private);
}

static const struct file_operations mtk_pending_submission_mode_debugfs_fops = {
	.open = mtk_pending_submission_mode_debugfs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

int mtk_debug_pending_submission_mode_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("pending_submission_mode", 0440,
		kbdev->mali_debugfs_directory, kbdev,
		&mtk_pending_submission_mode_debugfs_fops);

	return 0;
}

#endif /* CONFIG_MALI_MTK_DEBUG_FS */