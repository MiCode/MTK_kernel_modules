// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#if IS_ENABLED(CONFIG_MALI_MTK_KBASE_THREAD_DEBUG)
#include "mali_kbase_ctx_sched.h"
#endif /* CONFIG_MALI_MTK_KBASE_THREAD_DEBUG */

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_whitebox_fault_worker_enable_show(struct seq_file *m, void *v)
{
	seq_printf(m, "WHITEBOX fault worker is ready for user\n");

	return 0;
}

static int mtk_whitebox_fault_worker_enable_open(struct inode *in, struct file *file)
{
	struct kbase_device *kbdev = in->i_private;
	file->private_data = kbdev;

	if (file->f_mode & FMODE_WRITE)
		return 0;

	return single_open(file, mtk_whitebox_fault_worker_enable_show, in->i_private);
}

static int mtk_whitebox_fault_worker_enable_release(struct inode *in, struct file *file)
{
	if (!(file->f_mode & FMODE_WRITE)) {
		struct seq_file *m = (struct seq_file *)file->private_data;

		if (m)
			seq_release(in, file);
	}

	return 0;
}

static ssize_t mtk_whitebox_fault_worker_enable_write(struct file *file, const char __user *ubuf,
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
		struct kbase_as *as = &kbdev->as[1];

		pr_info("[WHITEBOX] Enqueue bus fault worker");
		WARN_ON(!queue_work(as->pf_wq, &as->work_busfault));
		atomic_inc(&kbdev->faults_pending);
		pr_info("[WHITEBOX] Enqueue page fault worker");

#if IS_ENABLED(CONFIG_MALI_MTK_WORKQUEUE_TO_KTHREAD_WORKER)
		bool ret = kthread_queue_work(kbdev->csf.scheduler.mmu_page_fault_worker, &as->work_pagefault);
		WARN_ON(!ret);
		if (ret) {
#if IS_ENABLED(CONFIG_MALI_MTK_KBASE_THREAD_DEBUG)
			struct kbase_context *kctx = kbase_ctx_sched_as_to_ctx(kbdev, as->number);
			if (kctx) {
				mali_kthread_event("queue work", kctx, "kbase_mmu_page_fault_worker");
			}
#endif /* CONFIG_MALI_MTK_KBASE_THREAD_DEBUG */
		}
#else
		WARN_ON(!queue_work(as->pf_wq, &as->work_pagefault));
#endif /* CONFIG_MALI_MTK_WORKQUEUE_TO_KTHREAD_WORKER */
		atomic_inc(&kbdev->faults_pending);
	}

	return count;
}

static const struct file_operations mtk_whitebox_fault_worker_enable_fops = {
	.open    = mtk_whitebox_fault_worker_enable_open,
	.release = mtk_whitebox_fault_worker_enable_release,
	.read    = seq_read,
	.write   = mtk_whitebox_fault_worker_enable_write,
	.llseek  = seq_lseek
};

int mtk_whitebox_fault_worker_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("whitebox_fault_worker", 0444,
			kbdev->mali_debugfs_directory, kbdev,
			&mtk_whitebox_fault_worker_enable_fops);
	return 0;
}
#else /* CONFIG_DEBUG_FS */
int mtk_whitebox_fault_worker_debugfs_init(struct kbase_device *kbdev)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */
