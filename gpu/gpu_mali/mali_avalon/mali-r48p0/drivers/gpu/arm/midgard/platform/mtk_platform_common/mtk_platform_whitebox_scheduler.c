// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <linux/delay.h>
#include <backend/gpu/mali_kbase_pm_internal.h>

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_whitebox_scheduler_enable_show(struct seq_file *m, void *v)
{
	seq_printf(m, "WHITEBOX fault worker is ready for user\n");

	return 0;
}

static int mtk_whitebox_scheduler_enable_open(struct inode *in, struct file *file)
{
	struct kbase_device *kbdev = in->i_private;
	file->private_data = kbdev;

	if (file->f_mode & FMODE_WRITE)
		return 0;

	return single_open(file, mtk_whitebox_scheduler_enable_show, in->i_private);
}

static int mtk_whitebox_scheduler_enable_release(struct inode *in, struct file *file)
{
	if (!(file->f_mode & FMODE_WRITE)) {
		struct seq_file *m = (struct seq_file *)file->private_data;

		if (m)
			seq_release(in, file);
	}

	return 0;
}

static ssize_t mtk_whitebox_scheduler_enable_write(struct file *file, const char __user *ubuf,
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

	if (temp == 1) {
		pr_info("[WHITEBOX] kbase_csf_scheduler_invoke_tick");
		kbase_csf_scheduler_invoke_tick(kbdev);
	}
	else if (temp == 2) {
		pr_info("[WHITEBOX] kbase_csf_scheduler_invoke_tock");
		kbase_csf_scheduler_invoke_tock(kbdev);
	}
	else if (temp == 3) {
		const struct kbase_pm_policy *current_policy, *backup_policy;
		const struct kbase_pm_policy *const *policy_list;
		int policy_count;
		int n,i,retry;
		uint64_t shader_ready;
		uint64_t shader_present_bitmap;
		bool correct_power_state;
		int ret_val;

		pr_info("[WHITEBOX] gpu_idle_worker");
		for (i = 0; i < 10000; ++i) {
			/* Get current policy */
			current_policy = kbase_pm_get_policy(kbdev);
			policy_count = kbase_pm_list_policies(kbdev, &policy_list);

			shader_present_bitmap = kbdev->gpu_props.shader_present;

			/* Go through all supported power policies*/
			for (n = 0; n < policy_count; ++n) {
				const struct kbase_pm_policy *pm_policy = policy_list[n];
				/* Set the policy */
				kbase_pm_set_policy(kbdev, pm_policy);
				ret_val = kbase_pm_wait_for_desired_state(kbdev);
				if (ret_val) {
					pr_info("[WHITEBOX] Wait for pm state change failed on synchronous power off");
				}
				shader_ready = kbase_reg_read64(kbdev, GPU_CONTROL_ENUM(SHADER_READY));
				correct_power_state = (shader_ready & shader_present_bitmap) != 0;
				if (!correct_power_state && pm_policy->id != 0) {

					for (retry = 0; retry < 5; ++retry)
					{
					    /* Wait for the power state to take effect */
						msleep(1);
						shader_ready = kbase_reg_read64(kbdev, GPU_CONTROL_ENUM(SHADER_READY));
						if (shader_ready != 0) {
							correct_power_state = true;
							//pr_info("[WHITEBOX] correct_power_state = true");
							break;
						}
					}
					if (!correct_power_state) {
						pr_info("[WHITEBOX] Cores mis-match between present and ready mask %llu", shader_ready);
						return count;
					}
				}
			}
			/* Restore the original power and core availability policies */
			kbase_pm_set_policy(kbdev, current_policy);
                }
		pr_info("[WHITEBOX] Complete");
	}

	return count;
}

static const struct file_operations mtk_whitebox_scheduler_enable_fops = {
	.open    = mtk_whitebox_scheduler_enable_open,
	.release = mtk_whitebox_scheduler_enable_release,
	.read    = seq_read,
	.write   = mtk_whitebox_scheduler_enable_write,
	.llseek  = seq_lseek
};

int mtk_whitebox_scheduler_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("whitebox_scheduler", 0444,
			kbdev->mali_debugfs_directory, kbdev,
			&mtk_whitebox_scheduler_enable_fops);
	return 0;
}
#else /* CONFIG_DEBUG_FS */
int mtk_whitebox_scheduler_debugfs_init(struct kbase_device *kbdev)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */
