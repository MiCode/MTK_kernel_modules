/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_WHITEBOX_SYNC_UPDATE_H__
#define __MTK_PLATFORM_WHITEBOX_SYNC_UPDATE_H__

int mtk_whitebox_sync_update_test_mode(void);
int mtk_whitebox_sync_update_test_debugfs_init(struct kbase_device *kbdev);
int mtk_whitebox_sync_update_test_init(void);

enum SYNC_UPDATE_TEST_MODE {
    SYNC_UPDATE_TEST_MODE_NONE = 0,
    SYNC_UPDATE_TEST_MODE_SIMPLE,
    SYNC_UPDATE_TEST_MODE_SYNC_UPDATE_AFTER_GPU_IDLE,
    SYNC_UPDATE_TEST_MODE_MANUAL,
    SYNC_UPDATE_TEST_MODE_COUNT,
};

#endif /* __MTK_PLATFORM_WHITEBOX_SYNC_UPDATE_H__ */
