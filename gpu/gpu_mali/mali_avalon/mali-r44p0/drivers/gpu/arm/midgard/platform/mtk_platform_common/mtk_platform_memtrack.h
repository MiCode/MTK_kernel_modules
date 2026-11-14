// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_MEMTRACK_H__
#define __MTK_PLATFORM_MEMTRACK_H__

int mtk_memtrack_procfs_init(struct kbase_device *kbdev, struct proc_dir_entry *parent);
int mtk_memtrack_procfs_term(struct kbase_device *kbdev, struct proc_dir_entry *parent);
int mtk_memtrack_init(struct kbase_device *kbdev);
int mtk_memtrack_term(struct kbase_device *kbdev);

#endif /* __MTK_PLATFORM_MEMTRACK_H__ */
