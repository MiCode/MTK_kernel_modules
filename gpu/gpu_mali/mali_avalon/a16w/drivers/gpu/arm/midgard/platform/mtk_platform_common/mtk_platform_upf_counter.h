/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_UPF_COUNTER_H__
#define __MTK_PLATFORM_UPF_COUNTER_H__

void mtk_upf_counter_add(void);
int mtk_upf_counter_debugfs_init(struct kbase_device *kbdev);
int mtk_upf_counter_init(void);
unsigned long long mtk_upf_counter_get(void);
void mtk_upf_counter_reset(void);

#endif /* __MTK_PLATFORM_UPF_COUNTER_H__ */
