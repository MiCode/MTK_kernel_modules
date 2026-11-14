/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (c) 2022 MediaTek Inc.
*/

#ifndef __MTK_PLATFORM_CM7_TRACE_H__
#define __MTK_PLATFORM_CM7_TRACE_H__

int mtk_cm7_trace_sysfs_init(struct kbase_device *kbdev);
int mtk_cm7_trace_sysfs_term(struct kbase_device *kbdev);

#endif /* __MTK_PLATFORM_CM7_TRACE_H__ */
