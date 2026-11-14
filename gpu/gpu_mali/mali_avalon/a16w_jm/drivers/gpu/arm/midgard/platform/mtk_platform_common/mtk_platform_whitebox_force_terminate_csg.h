/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_WHITEBOX_FORCE_TERMINATE_CSG_H__
#define __MTK_PLATFORM_WHITEBOX_FORCE_TERMINATE_CSG_H__

bool mtk_whitebox_force_terminate_csg_enable(void);
int mtk_whitebox_force_terminate_csg_debugfs_init(struct kbase_device *kbdev);
int mtk_whitebox_force_terminate_csg_init(void);

#endif /* __MTK_PLATFORM_WHITEBOX_FORCE_TERMINATE_CSG_H__ */
