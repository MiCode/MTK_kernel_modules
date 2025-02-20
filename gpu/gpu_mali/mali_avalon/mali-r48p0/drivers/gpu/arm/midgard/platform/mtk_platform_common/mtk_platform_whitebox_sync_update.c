// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <csf/mali_kbase_csf_scheduler.h>
#include "mtk_platform_whitebox_sync_update.h"

int sync_update_test_mode;

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_whitebox_sync_update_test_show(struct seq_file *m, void *v)
{
	seq_printf(m, "sync_update_test_mode = %d\n",  sync_update_test_mode);

	return 0;
}

static int mtk_whitebox_sync_update_test_open(struct inode *in, struct file *file)
{
	struct kbase_device *kbdev = in->i_private;
	file->private_data = kbdev;

	if (file->f_mode & FMODE_WRITE)
		return 0;

	return single_open(file, mtk_whitebox_sync_update_test_show, in->i_private);
}

static int mtk_whitebox_sync_update_test_release(struct inode *in, struct file *file)
{
	if (!(file->f_mode & FMODE_WRITE)) {
		struct seq_file *m = (struct seq_file *)file->private_data;

		if (m)
			seq_release(in, file);
	}

	return 0;
}

static ssize_t mtk_whitebox_sync_update_test_write(struct file *file, const char __user *ubuf,
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

	if (temp >= SYNC_UPDATE_TEST_MODE_NONE && temp < SYNC_UPDATE_TEST_MODE_COUNT)
		sync_update_test_mode = temp;
	else
		sync_update_test_mode = 0;

	// trigger sync update worker manually
	if (sync_update_test_mode == SYNC_UPDATE_TEST_MODE_MANUAL) {
		// for loop to find valid kctx
		for (i = 0 ; i < BASE_MAX_NR_AS; ++i) {
			if (kbdev->as_to_kctx[i]) {
				mutex_lock(&scheduler->lock);
				dev_info(kbdev->dev, "[WHITEBOX] enqueue sync update work for kctx = %d", i);
				kbase_csf_scheduler_enqueue_sync_update_work(kbdev->as_to_kctx[i]);
				mutex_unlock(&scheduler->lock);
				break;
			}
		}
	}

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"[WHITEBOX] set sync_update_test = %d\n", sync_update_test_mode);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	dev_info(kbdev->dev, "[WHITEBOX] sync_update_test_mode = %d", sync_update_test_mode);

	return count;
}

static const struct file_operations mtk_whitebox_sync_update_test_fops = {
	.open    = mtk_whitebox_sync_update_test_open,
	.release = mtk_whitebox_sync_update_test_release,
	.read    = seq_read,
	.write   = mtk_whitebox_sync_update_test_write,
	.llseek  = seq_lseek
};

int mtk_whitebox_sync_update_test_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("whitebox_sync_update_test", 0444,
			kbdev->mali_debugfs_directory, kbdev,
			&mtk_whitebox_sync_update_test_fops);
	return 0;
}
#else /* CONFIG_DEBUG_FS */
int mtk_whitebox_sync_update_test_debugfs_init(struct kbase_device *kbdev)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */

int mtk_whitebox_sync_update_test_mode(void)
{
	return sync_update_test_mode;
}

int mtk_whitebox_sync_update_test_init(void)
{
    sync_update_test_mode = 0;
	return 0;
}
