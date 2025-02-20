/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_WHITEBOX_FORCE_HARD_RESET_H__
#define __MTK_PLATFORM_WHITEBOX_FORCE_HARD_RESET_H__

bool mtk_whitebox_force_hard_reset_enable(void);
int mtk_whitebox_force_hard_reset_debugfs_init(struct kbase_device *kbdev);
int mtk_whitebox_force_hard_reset_init(void);

#endif /* __MTK_PLATFORM_WHITEBOX_FORCE_HARD_RESET_H__ */
