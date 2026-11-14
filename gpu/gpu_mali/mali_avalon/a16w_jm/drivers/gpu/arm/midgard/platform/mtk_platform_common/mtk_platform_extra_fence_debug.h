/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_EXTRA_FENCE_DEBUG_H__
#define __MTK_PLATFORM_EXTRA_FENCE_DEBUG_H__

int mtk_extra_fence_debug_mode(void);
int mtk_extra_fence_debug_debugfs_init(struct kbase_device *kbdev);
int mtk_extra_fence_debug_init(void);

enum EXTRA_FENCE_DEBUG_MODE {
    EXTRA_FENCE_DEBUG_MODE_NONE = 0,
    EXTRA_FENCE_DEBUG_MODE_INFO,
    EXTRA_FENCE_DEBUG_MODE_VERBOSE,
    EXTRA_FENCE_DEBUG_MODE_COUNT,
};

#endif /* __MTK_PLATFORM_EXTRA_FENCE_DEBUG_H__ */
