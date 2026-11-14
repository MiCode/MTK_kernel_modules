// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <platform/mtk_platform_common.h>
#include <platform/mtk_platform_common/mtk_platform_adaptive_power_policy.h>

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_FS)
static int mtk_debug_adaptive_power_policy(struct seq_file *file, void *data)
{
	struct kbase_device *kbdev = file->private;
	struct device_node *np;
	u32 adaptive_power_policy = 0;

	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	np = kbdev->dev->of_node;

	if (!of_property_read_u32(np, "adaptive-power-policy", &adaptive_power_policy))
		seq_printf(file, "Adaptive Power Policy: %d\n", adaptive_power_policy);
	else
		seq_printf(file, "Adaptive Power Policy: No dts property setting, default disabled\n");

	return 0;
}

static int mtk_adaptive_power_policy_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, mtk_debug_adaptive_power_policy,
	                   in->i_private);
}

static const struct file_operations mtk_adaptive_power_policy_debugfs_fops = {
	.open = mtk_adaptive_power_policy_debugfs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

int mtk_debug_adaptive_power_policy_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("adaptive_power_policy", 0440,
		kbdev->mali_debugfs_directory, kbdev,
		&mtk_adaptive_power_policy_debugfs_fops);

	return 0;
}

#endif /* CONFIG_MALI_MTK_DEBUG_FS */