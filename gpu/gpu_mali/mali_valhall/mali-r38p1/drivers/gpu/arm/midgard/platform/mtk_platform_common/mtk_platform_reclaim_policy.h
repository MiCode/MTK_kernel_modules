/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (c) 2022 MediaTek Inc.
*/

#ifndef __MTK_PLATFORM_RECLAIM_POLICY_H__
#define __MTK_PLATFORM_RECLAIM_POLICY_H__

int MTKGPU_reclaim_policy_init(struct kbase_device *kbdev);
void MTKGPU_reclaim_policy_destroy(struct kbase_device *kbdev);
extern void (*mtk_set_gpu_reclaim_policy_fp)(unsigned int val);
extern unsigned int (*mtk_get_gpu_reclaim_policy_fp)(void);


#endif /* __MTK_PLATFORM_RECLAIM_POLICY_H__ */

