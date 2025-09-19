// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_PENDING_SUBMISSION_H__
#define __MTK_PLATFORM_PENDING_SUBMISSION_H__

enum gpu_pending_submission_mode {
	GPU_PENDING_SUBMISSION_KWORKER = 0,
	GPU_PENDING_SUBMISSION_KTHREAD,
};

int mtk_debug_pending_submission_mode_debugfs_init(struct kbase_device *kbdev);

#endif /* __MTK_PLATFORM_PENDING_SUBMISSION_H__ */
