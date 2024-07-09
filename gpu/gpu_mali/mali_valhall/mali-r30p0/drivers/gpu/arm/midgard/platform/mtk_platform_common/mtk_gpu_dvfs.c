// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <platform/mtk_platform_common.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <backend/gpu/mali_kbase_pm_defs.h>
#include "mtk_gpu_dvfs.h"

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
static unsigned int current_util_active;
static unsigned int current_util_3d;
static unsigned int current_util_ta;
static unsigned int current_util_compute;
#endif

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
void mtk_common_ged_dvfs_commit(unsigned long ui32NewFreqID,
                                GED_DVFS_COMMIT_TYPE eCommitType,
                                int *pbCommited)
{
	int ret = mtk_common_gpufreq_commit(ui32NewFreqID);
	if (pbCommited) {
		*pbCommited = (ret == 0) ? true : false;
	}
}
#endif

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
void mtk_common_update_gpu_utilization(void)
{
	unsigned int loading, block, idle;
#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
	struct GpuUtilization_Ex util_ex;
	mtk_common_cal_gpu_utilization_ex(&loading, &block, &idle, &util_ex);
#else
	mtk_common_cal_gpu_utilization(&loading, &block, &idle);
#endif
}

int mtk_common_get_util_active(void)
{
	return current_util_active;
}

int mtk_common_get_util_3d(void)
{
	return current_util_3d;
}

int mtk_common_get_util_ta(void)
{
	return current_util_ta;
}

int mtk_common_get_util_compute(void)
{
	return current_util_compute;
}

#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
void mtk_common_cal_gpu_utilization_ex(unsigned int *pui32Loading,
                                       unsigned int *pui32Block,
                                       unsigned int *pui32Idle,
                                       void *Util_Ex)
#else
void mtk_common_cal_gpu_utilization(unsigned int *pui32Loading,
                                    unsigned int *pui32Block,
                                    unsigned int *pui32Idle)
#endif
{
#if MALI_USE_CSF
	/* TODO: GPU DVFS */
	(void)pui32Loading;
	(void)pui32Block;
	(void)pui32Idle;
#else
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();
	int utilisation, util_gl_share;
	int util_cl_share[2];
	int busy;
	struct kbasep_pm_metrics *diff;
#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
	struct GpuUtilization_Ex *util_ex = (struct GpuUtilization_Ex *) Util_Ex;
#endif

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	diff = &kbdev->pm.backend.metrics.dvfs_diff;

	kbase_pm_get_dvfs_metrics(kbdev, &kbdev->pm.backend.metrics.dvfs_last, diff);

	utilisation = (100 * diff->time_busy) /
			max(diff->time_busy + diff->time_idle, 1u);

	busy = max(diff->busy_gl + diff->busy_cl[0] + diff->busy_cl[1], 1u);

	util_gl_share = (100 * diff->busy_gl) / busy;
	util_cl_share[0] = (100 * diff->busy_cl[0]) / busy;
	util_cl_share[1] = (100 * diff->busy_cl[1]) / busy;

#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
	util_ex->util_active = utilisation;
	util_ex->util_3d = (100 * diff->busy_gl_plus[0]) /
			max(diff->time_busy + diff->time_idle, 1u);
	util_ex->util_ta = (100 * (diff->busy_gl_plus[1]+diff->busy_gl_plus[2])) /
			max(diff->time_busy + diff->time_idle, 1u);
	util_ex->util_compute = (100 * (diff->busy_cl[0]+diff->busy_cl[1])) /
			max(diff->time_busy + diff->time_idle, 1u);
#endif

	if (pui32Loading)
		*pui32Loading = utilisation;

	if (pui32Idle)
		*pui32Idle = 100 - utilisation;

	if (utilisation < 0 || util_gl_share < 0 ||
	    util_cl_share[0] < 0 || util_cl_share[1] < 0) {
		utilisation = 0;
		util_gl_share = 0;
		util_cl_share[0] = 0;
		util_cl_share[1] = 0;
	} else {
		current_util_active = utilisation;
		current_util_3d = (100 * diff->busy_gl_plus[0]) /
				max(diff->time_busy + diff->time_idle, 1u);
		current_util_ta = (100 * (diff->busy_gl_plus[1]+diff->busy_gl_plus[2])) /
				max(diff->time_busy + diff->time_idle, 1u);
		current_util_compute = (100 * (diff->busy_cl[0]+diff->busy_cl[1])) /
				max(diff->time_busy + diff->time_idle, 1u);
	}
#endif /* MALI_USE_CSF */
}
#endif
