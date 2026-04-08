/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_WHITEBOX_MISSING_DOORBELL_H__
#define __MTK_PLATFORM_WHITEBOX_MISSING_DOORBELL_H__

bool mtk_whitebox_missing_doorbell_enable(void);
int mtk_whitebox_missing_doorbell_debugfs_init(struct kbase_device *kbdev);
int mtk_whitebox_missing_doorbell_init(void);


#endif /* __MTK_PLATFORM_WHITEBOX_MISSING_DOORBELL_H__ */