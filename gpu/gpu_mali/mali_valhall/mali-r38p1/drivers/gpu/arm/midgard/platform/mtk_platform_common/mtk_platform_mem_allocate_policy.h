/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (c) 2022 MediaTek Inc.
*/

#ifndef __MTK_PLATFORM_MEM_ALLOCATE_POLICY_H__
#define __MTK_PLATFORM_MEM_ALLOCATE_POLICY_H__

int MTKGPU_mem_allocate_policy_init(struct kbase_device *kbdev);
void MTKGPU_mem_allocate_policy_destroy(struct kbase_device *kbdev);
extern void (*mtk_set_mem_allocate_policy_fp)(unsigned int val);
extern unsigned int (*mtk_get_mem_allocate_policy_fp)(void);


#endif /* __MTK_PLATFORM_MEM_ALLOCATE_POLICY_H__ */
