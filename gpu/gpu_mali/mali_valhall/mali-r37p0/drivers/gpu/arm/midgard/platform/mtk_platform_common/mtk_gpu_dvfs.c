// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <platform/mtk_platform_common.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <backend/gpu/mali_kbase_pm_defs.h>
#include <csf/ipa_control/mali_kbase_csf_ipa_control_ex.h>

#include "mtk_gpu_dvfs.h"
#include <mtk_gpu_utility.h>


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
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();
	int ret;

	if (!IS_ERR_OR_NULL(kbdev)) {
		if (kbdev->pm.backend.gpu_ready) {
			ret = mtk_common_gpufreq_commit(ui32NewFreqID);
			if (pbCommited) {
				*pbCommited = (ret == 0) ? true : false;
			}
		}
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
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();
#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
	struct GpuUtilization_Ex *util_ex =
			(struct GpuUtilization_Ex *) Util_Ex;
#endif

	int utilisation[4];
	struct kbasep_pm_metrics *diff;
	int index = 0;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	diff = &kbdev->pm.backend.metrics.dvfs_diff;

	kbase_pm_get_dvfs_metrics(kbdev, &kbdev->pm.backend.metrics.dvfs_last,
				  diff);

	for (index = 0; index < 4; index++) {
		utilisation[index] = (100 * diff->time_busy[index]) /
			max(diff->time_busy[index]+ diff->time_idle[index], 1u);
	}

#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
	util_ex->util_active    = utilisation[0];
	util_ex->util_ta        = utilisation[1];
	util_ex->util_compute   = utilisation[2];
	util_ex->util_3d        = utilisation[3];
#endif

	if (pui32Loading)
		*pui32Loading = utilisation[0];

	if (pui32Idle)
		*pui32Idle = 100 - utilisation[0];

	if (utilisation[0] < 0 || utilisation[1] < 0 ||
	    utilisation[2] < 0 || utilisation[3] < 0) {
		utilisation[0] = 0;
		utilisation[1] = 0;
		utilisation[2] = 0;
		utilisation[3] = 0;
	} else {
		current_util_active  = utilisation[0];
		current_util_ta      = utilisation[1];
		current_util_compute = utilisation[2];
		current_util_3d      = utilisation[3];
	}
#else // MALI_USE_CSF

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

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && \
	IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
int (*mtk_common_rate_change_notify_fp)(struct kbase_device *kbdev,
					       u32 clk_index, u32 clk_rate_hz) = NULL;

EXPORT_SYMBOL(mtk_common_rate_change_notify_fp);

void MTKGPUFreq_change_notify(u32 clk_idx, u32 gpufreq)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();

	if (mtk_common_rate_change_notify_fp && !IS_ERR_OR_NULL(kbdev))
		mtk_common_rate_change_notify_fp(kbdev, clk_idx, gpufreq);
}
#endif
#endif

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
int mtk_set_core_mask(u64 core_mask)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();
	int ret = 0;

	kbase_devfreq_set_core_mask(kbdev, core_mask);

	/* TODO: need to check get_core_mask hang issue, to verity the scaling result */
	// current_mask = kbdev->pm.backend.ca_cores_enabled;

	return ret;
}
#endif
