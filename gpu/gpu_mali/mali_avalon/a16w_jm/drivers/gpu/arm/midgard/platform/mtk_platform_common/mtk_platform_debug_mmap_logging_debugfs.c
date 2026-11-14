// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_sync.h>
#include <linux/delay.h>
#include <platform/mtk_platform_common.h>

#include "mtk_platform_debug.h"

//__attribute__((unused)) extern int mtk_debug_trylock(struct mutex *lock);

#define MTK_DEBUG_MMAP_LOGGING_MASK     0b001
#define MTK_DEBUG_MMAP_LOGGING_DISABLE      0
static int mmap_logging_mode = MTK_DEBUG_MMAP_LOGGING_DISABLE;

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_debug_mmap_logging_mode_show(struct seq_file *m, void *v)
{
    seq_printf(m, "mmap_logging_mode = %d\n", mmap_logging_mode);

    return 0;
}

static int mtk_debug_mmap_logging_mode_open(struct inode *in, struct file *file)
{
    if (file->f_mode & FMODE_WRITE)
        return 0;

    return single_open(file, mtk_debug_mmap_logging_mode_show, in->i_private);
}

static int mtk_debug_mmap_logging_mode_release(struct inode *in, struct file *file)
{
    if (!(file->f_mode & FMODE_WRITE)) {
        struct seq_file *m = (struct seq_file *)file->private_data;

        if (m)
            seq_release(in, file);
    }

    return 0;
}

static ssize_t mtk_debug_mmap_logging_mode_write(struct file *file, const char __user *ubuf,
            size_t count, loff_t *ppos)
{
    int ret = 0;

    CSTD_UNUSED(ppos);

    ret = kstrtoint_from_user(ubuf, count, 0, &mmap_logging_mode);
    if (ret)
        return ret;

    if (mmap_logging_mode < MTK_DEBUG_MMAP_LOGGING_DISABLE || mmap_logging_mode > MTK_DEBUG_MMAP_LOGGING_MASK)
        mmap_logging_mode = MTK_DEBUG_MMAP_LOGGING_DISABLE;

    return count;
}

static const struct file_operations mtk_debug_mmap_logging_mode_fops = {
    .open    = mtk_debug_mmap_logging_mode_open,
    .release = mtk_debug_mmap_logging_mode_release,
    .read    = seq_read,
    .write   = mtk_debug_mmap_logging_mode_write,
    .llseek  = seq_lseek
};

int mtk_debug_mmap_logging_debugfs_init(struct kbase_device *kbdev)
{
    if (IS_ERR_OR_NULL(kbdev))
        return -1;

    debugfs_create_file("mmap_logging_mode", 0644,
            kbdev->mali_debugfs_directory, kbdev,
            &mtk_debug_mmap_logging_mode_fops);

    return 0;
}

int mtk_debug_debugfs_mmap_logging_mode(void)
{
    return mmap_logging_mode;
}

#else
int mtk_debug_mmap_logging_debugfs_init(struct kbase_device *kbdev)
{
    return 0;
}
#endif /* CONFIG_DEBUG_FS */