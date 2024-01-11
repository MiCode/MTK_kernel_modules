// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_hwaccess_time.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <platform/mtk_platform_common.h>
#include <platform/mtk_platform_common/mtk_platform_reclaim_policy.h>

void MTKGPU_Set_reclaim_policy(unsigned int val){
#if IS_ENABLED(CONFIG_MALI_MTK_RECLAIM_POLICY)
		struct kbase_device *kbdev;

		kbdev = (struct kbase_device *)mtk_common_get_kbdev();
		if (IS_ERR_OR_NULL(kbdev)) {
			return;
		}

		kbdev->reclaim_policy = val;
#else
		return;
#endif

}

unsigned int MTKGPU_Get_reclaim_policy(void){
#if IS_ENABLED(CONFIG_MALI_MTK_RECLAIM_POLICY)
		struct kbase_device *kbdev;

		kbdev = (struct kbase_device *)mtk_common_get_kbdev();
		if (IS_ERR_OR_NULL(kbdev)) {
			return 0;
		}

		return kbdev->reclaim_policy;
#else
		return 0;
#endif

}


int MTKGPU_reclaim_policy_init(struct kbase_device *kbdev) {

	kbdev->reclaim_policy = 0;
	mtk_set_gpu_reclaim_policy_fp = MTKGPU_Set_reclaim_policy;
	mtk_get_gpu_reclaim_policy_fp = MTKGPU_Get_reclaim_policy;

	return 0;
}

void MTKGPU_reclaim_policy_destroy(struct kbase_device *kbdev) {
	mtk_set_gpu_reclaim_policy_fp = NULL;
	mtk_get_gpu_reclaim_policy_fp = NULL;

}

