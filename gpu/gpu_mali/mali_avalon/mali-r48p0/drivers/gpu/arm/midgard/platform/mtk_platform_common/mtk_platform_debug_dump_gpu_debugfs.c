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

static int mem_dump_mode = MTK_DEBUG_MEM_DUMP_DISABLE;

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_debug_mem_dump_mode_show(struct seq_file *m, void *v)
{
    seq_printf(m, "mem_dump_mode = %d\n", mem_dump_mode);

    return 0;
}

static int mtk_debug_mem_dump_mode_open(struct inode *in, struct file *file)
{
    if (file->f_mode & FMODE_WRITE)
        return 0;

    return single_open(file, mtk_debug_mem_dump_mode_show, in->i_private);
}

static int mtk_debug_mem_dump_mode_release(struct inode *in, struct file *file)
{
    if (!(file->f_mode & FMODE_WRITE)) {
        struct seq_file *m = (struct seq_file *)file->private_data;

        if (m)
            seq_release(in, file);
    }

    return 0;
}

static ssize_t mtk_debug_mem_dump_mode_write(struct file *file, const char __user *ubuf,
            size_t count, loff_t *ppos)
{
    int ret = 0;

    CSTD_UNUSED(ppos);

    ret = kstrtoint_from_user(ubuf, count, 0, &mem_dump_mode);
    if (ret)
        return ret;

    if (mem_dump_mode < MTK_DEBUG_MEM_DUMP_DISABLE || mem_dump_mode > MTK_DEBUG_MEM_DUMP_MASK)
        mem_dump_mode = MTK_DEBUG_MEM_DUMP_DISABLE;

    return count;
}

static const struct file_operations mtk_debug_mem_dump_mode_fops = {
    .open    = mtk_debug_mem_dump_mode_open,
    .release = mtk_debug_mem_dump_mode_release,
    .read    = seq_read,
    .write   = mtk_debug_mem_dump_mode_write,
    .llseek  = seq_lseek
};

int mtk_debug_csf_debugfs_init(struct kbase_device *kbdev)
{
    if (IS_ERR_OR_NULL(kbdev))
        return -1;

    debugfs_create_file("mem_dump_mode", 0644,
            kbdev->mali_debugfs_directory, kbdev,
            &mtk_debug_mem_dump_mode_fops);

    return 0;
}

int mtk_debug_csf_debugfs_dump_mode(void)
{
    return mem_dump_mode;
}

#else
int mtk_debug_csf_debugfs_init(struct kbase_device *kbdev)
{
    return 0;
}
#endif /* CONFIG_DEBUG_FS */