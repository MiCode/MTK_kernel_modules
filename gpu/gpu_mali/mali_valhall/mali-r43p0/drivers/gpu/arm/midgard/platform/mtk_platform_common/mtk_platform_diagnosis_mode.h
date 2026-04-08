/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (c) 2022 MediaTek Inc.
*/

#ifndef __MTK_PLATFORM_DIAGNOSIS_MODE_H__
#define __MTK_PLATFORM_DIAGNOSIS_MODE_H__

int mtk_diagnosis_mode_sysfs_init(struct kbase_device *kbdev);
int mtk_diagnosis_mode_sysfs_term(struct kbase_device *kbdev);
u64 mtk_diagnosis_mode_get_mode(void);
u64 mtk_diagnosis_mode_get_dump_mask(void);


#endif /* __MTK_PLATFORM_DIAGNOSIS_MODE_H__ */
