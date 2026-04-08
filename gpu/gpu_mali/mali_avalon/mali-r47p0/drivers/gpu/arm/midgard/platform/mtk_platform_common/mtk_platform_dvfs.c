// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#if IS_ENABLED(CONFIG_PROC_FS)
#include <linux/proc_fs.h>
#endif
#include <mali_kbase.h>
#include <platform/mtk_platform_common.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <backend/gpu/mali_kbase_pm_defs.h>
#include <csf/ipa_control/mali_kbase_csf_ipa_control_ex.h>
#include <mtk_gpufreq.h>
#include <mtk_gpu_utility.h>
#include <platform/mtk_platform_common/mtk_platform_dvfs.h>

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && \
	IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY) && \
	IS_ENABLED(CONFIG_MALI_MTK_GPU_DVFS_HINT_26M_LOADING)
#include <platform/mtk_platform_common/mtk_platform_dvfs_hint_26m_perf_cnting.h>
#include "platform/mtk_platform_common/mtk_platform_dvfs_hint_26m_perf_cnting_ex.h"
#endif /* CONFIG_MALI_MIDGARD_DVFS && CONFIG_MALI_MTK_DVFS_POLICY && CONFIG_MALI_MTK_GPU_DVFS_HINT_26M_LOADING*/

#if IS_ENABLED(CONFIG_PROC_FS)
/* name of the proc entry */
#define	PROC_GPU_UTILIZATION "gpu_utilization"
static DEFINE_MUTEX(gpu_utilization_lock);
#endif

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
static unsigned int current_util_active;
static unsigned int current_util_3d;
static unsigned int current_util_ta;
static unsigned int current_util_compute;
static unsigned int current_util_iter;
static unsigned int current_util_mcu;
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE) && \
    IS_ENABLED(CONFIG_MALI_MTK_GPU_DVFS_ASYNC)
#define UTIL_ACTIVE_ID   0
#define UTIL_TA_ID       1
#define UTIL_COMPUTE_ID  2
#define UTIL_3D_ID       3
#define UTIL_ITER_ID     4
#define UTIL_MCU_ID      5
#define UTIL_IRQ_ID      6
#define UTIL_SC_COMP_ID  7
#define UTIL_l2ext_ID    8
#elif IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
#define UTIL_ACTIVE_ID   0
#define UTIL_TA_ID       1
#define UTIL_COMPUTE_ID  2
#define UTIL_3D_ID       3
#define UTIL_ITER_ID     4
#define UTIL_MCU_ID      5
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

void mtk_common_ged_dvfs_dual_commit(unsigned long gpuNewFreqID,
                                unsigned long stackNewFreqID,
                                int *pbCommited)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();
	int ret;

	if (!IS_ERR_OR_NULL(kbdev)) {
		if (kbdev->pm.backend.gpu_ready) {
			ret = mtk_common_gpufreq_dual_commit(gpuNewFreqID, stackNewFreqID);
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
	unsigned long long delta_time;

#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
	int utilisation[NUM_PERF_COUNTERS*2];
#else
	int utilisation[NUM_PERF_COUNTERS];
#endif /* CONFIG_MALI_MTK_DVFS_LOADING_MODE */

	struct kbasep_pm_metrics *diff;
	int index = 0;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	diff = &kbdev->pm.backend.metrics.dvfs_diff;

	kbase_pm_get_dvfs_metrics(kbdev, &kbdev->pm.backend.metrics.dvfs_last,
				  diff);

	delta_time = max(diff->time_busy[0] + diff->time_idle[0], 1u);
	for (index = 0; index < NUM_PERF_COUNTERS; index++) {
		// delta time should be the same for all PMU, so simply reuse it
		utilisation[index] = (100 * diff->time_busy[index]) / delta_time;
#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
		utilisation[index + NUM_PERF_COUNTERS] = diff->counterRaw[index];
#endif /* CONFIG_MALI_MTK_DVFS_LOADING_MODE */
	}

#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
	util_ex->util_active    = utilisation[UTIL_ACTIVE_ID];
	util_ex->util_ta        = utilisation[UTIL_TA_ID];
	util_ex->util_compute   = utilisation[UTIL_COMPUTE_ID];
	util_ex->util_3d        = utilisation[UTIL_3D_ID];
	util_ex->util_iter      = utilisation[UTIL_ITER_ID];
	util_ex->util_mcu       = utilisation[UTIL_MCU_ID];

	util_ex->util_active_raw    = utilisation[UTIL_ACTIVE_ID + NUM_PERF_COUNTERS];
	util_ex->util_iter_raw      = utilisation[UTIL_ITER_ID + NUM_PERF_COUNTERS];
	util_ex->util_mcu_raw       = utilisation[UTIL_MCU_ID + NUM_PERF_COUNTERS];
#if IS_ENABLED(CONFIG_MALI_MTK_GPU_DVFS_ASYNC)
	util_ex->util_irq_raw       = utilisation[UTIL_IRQ_ID + NUM_PERF_COUNTERS];
	util_ex->util_sc_comp_raw   = utilisation[UTIL_SC_COMP_ID + NUM_PERF_COUNTERS];
	util_ex->util_l2ext_raw     = utilisation[UTIL_l2ext_ID + NUM_PERF_COUNTERS];
#endif /* CONFIG_MALI_MTK_GPU_DVFS_ASYNC */

	util_ex->delta_time     = delta_time << 8;   // 8 = KBASE_PM_TIME_SHIFT
#if IS_ENABLED(CONFIG_MALI_MTK_GPU_DVFS_HINT_26M_LOADING)
	util_ex->util_iter_u_mcu = mtk_dvfs_hint_26m_cal_prfcnt_utilization(
          SELECT_UNION_ITER_MCU, gpu_power_status);
#endif /* CONFIG_MALI_MTK_GPU_DVFS_HINT_26M_LOADING */
#endif

	if (pui32Loading)
		*pui32Loading = utilisation[0];

	if (pui32Idle)
		*pui32Idle = 100 - utilisation[0];

	if (utilisation[0] < 0 || utilisation[1] < 0 ||
	    utilisation[2] < 0 || utilisation[3] < 0 ||
		utilisation[4] < 0 || utilisation[5] < 0) {
		utilisation[0] = 0;
		utilisation[1] = 0;
		utilisation[2] = 0;
		utilisation[3] = 0;
		utilisation[4] = 0;
		utilisation[5] = 0;
	} else {
		current_util_active  = utilisation[0];
		current_util_ta      = utilisation[1];
		current_util_compute = utilisation[2];
		current_util_3d      = utilisation[3];
		current_util_iter    = utilisation[4];
		current_util_mcu     = utilisation[5];
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
	unsigned long long delta_time;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	diff = &kbdev->pm.backend.metrics.dvfs_diff;

	kbase_pm_get_dvfs_metrics(kbdev, &kbdev->pm.backend.metrics.dvfs_last, diff);

	delta_time = max(diff->time_busy + diff->time_idle, 1u);
	utilisation = (100 * diff->time_busy) / delta_time;
	busy = max(diff->busy_gl + diff->busy_cl[0] + diff->busy_cl[1], 1u);
	util_gl_share = (100 * diff->busy_gl) / busy;
	util_cl_share[0] = (100 * diff->busy_cl[0]) / busy;
	util_cl_share[1] = (100 * diff->busy_cl[1]) / busy;

#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
	util_ex->util_active = utilisation;
	util_ex->util_3d = (100 * diff->busy_gl_plus[0]) / delta_time;
	util_ex->util_ta = (100 * (diff->busy_gl_plus[1]+diff->busy_gl_plus[2])) /
		delta_time;
	util_ex->util_compute = (100 * (diff->busy_cl[0]+diff->busy_cl[1])) /
		delta_time;
	util_ex->delta_time = delta_time << 8;   // 8 = KBASE_PM_TIME_SHIFT
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
		current_util_3d = (100 * diff->busy_gl_plus[0]) / delta_time;
		current_util_ta = (100 * (diff->busy_gl_plus[1]+diff->busy_gl_plus[2])) /
			delta_time;
		current_util_compute = (100 * (diff->busy_cl[0]+diff->busy_cl[1])) /
			delta_time;
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

/* only work if CSF exit */
void mtk_set_gpu_idle_time(unsigned int val){
#if MALI_USE_CSF && IS_ENABLED(CONFIG_MALI_MTK_GPU_IDLE_TEST)
		struct kbase_device *kbdev;

		kbdev = (struct kbase_device *)mtk_common_get_kbdev();
		if (IS_ERR_OR_NULL(kbdev)) {
			return;
		}

		kbase_csf_firmware_set_gpu_idle_hysteresis_time(kbdev, val);
#else
		return;
#endif

}

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

u64 mtk_common_get_system_timer(void)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();
	u64 system_time_tmp = 0;
	bool gpu_ready;
	unsigned long flags;

	/* need to lock for using function point */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);

	gpu_ready = kbdev->pm.backend.gpu_ready;

	if (gpu_ready)
		kbase_backend_get_gpu_time_norequest(kbdev, NULL, &system_time_tmp, NULL);

	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	return system_time_tmp;
}
#endif

#if IS_ENABLED(CONFIG_PROC_FS)
static int mtk_dvfs_gpu_utilization_show(struct seq_file *m, void *v)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	unsigned int cur_opp_idx;

	mutex_lock(&gpu_utilization_lock);

	mtk_common_update_gpu_utilization();

#if defined(CONFIG_MTK_GPUFREQ_V2)
	cur_opp_idx = mtk_common_gpufreq_bringup() ?
		0 : gpufreq_get_cur_oppidx(TARGET_DEFAULT);
#else
	cur_opp_idx = mtk_common_gpufreq_bringup() ?
		0 : mt_gpufreq_get_cur_freq_index();
#endif /* CONFIG_MTK_GPUFREQ_V2 */

	seq_printf(m, "ACTIVE=%u 3D/TA/COMPUTE=%u/%u/%u OPP_IDX=%u MFG_PWR=%d\n",
	           current_util_active, current_util_3d, current_util_ta,
	           current_util_compute, cur_opp_idx, mtk_common_pm_is_mfg_active());

	mutex_unlock(&gpu_utilization_lock);
#else
	seq_puts(m, "GPU DVFS doesn't be enabled\n");
#endif

	return 0;
}
DEFINE_PROC_SHOW_ATTRIBUTE(mtk_dvfs_gpu_utilization);

int mtk_dvfs_procfs_init(struct kbase_device *kbdev, struct proc_dir_entry *parent)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	proc_create(PROC_GPU_UTILIZATION, 0440, parent, &mtk_dvfs_gpu_utilization_proc_ops);

	return 0;
}

int mtk_dvfs_procfs_term(struct kbase_device *kbdev, struct proc_dir_entry *parent)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	remove_proc_entry(PROC_GPU_UTILIZATION, parent);

	return 0;
}
#else
int mtk_dvfs_procfs_init(struct kbase_device *kbdev, struct proc_dir_entry *parent)
{
	return 0;
}

int mtk_dvfs_procfs_term(struct kbase_device *kbdev, struct proc_dir_entry *parent)
{
	return 0;
}
#endif /* CONFIG_PROC_FS */

int mtk_dvfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
	ged_dvfs_cal_gpu_utilization_ex_fp = mtk_common_cal_gpu_utilization_ex;
	mtk_notify_gpu_freq_change_fp = MTKGPUFreq_change_notify;
#else
	ged_dvfs_cal_gpu_utilization_fp = mtk_common_cal_gpu_utilization;
#endif /* CONFIG_MALI_MTK_DVFS_LOADING_MODE */
	ged_dvfs_gpu_freq_commit_fp = mtk_common_ged_dvfs_commit;
	ged_dvfs_gpu_freq_dual_commit_fp = mtk_common_ged_dvfs_dual_commit;
	ged_dvfs_set_gpu_core_mask_fp = mtk_set_core_mask;
	mtk_get_system_timer_fp = mtk_common_get_system_timer;
#endif /* CONFIG_MALI_MIDGARD_DVFS && CONFIG_MALI_MTK_DVFS_POLICY */
	mtk_set_gpu_idle_fp = mtk_set_gpu_idle_time;
	return 0;
}

int mtk_dvfs_term(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_LOADING_MODE)
	ged_dvfs_cal_gpu_utilization_ex_fp = NULL;
	mtk_notify_gpu_freq_change_fp = NULL;
#else
	ged_dvfs_cal_gpu_utilization_fp = NULL;
#endif /* CONFIG_MALI_MTK_DVFS_LOADING_MODE */
	ged_dvfs_gpu_freq_commit_fp = NULL;
	ged_dvfs_gpu_freq_dual_commit_fp = NULL;
	ged_dvfs_set_gpu_core_mask_fp = NULL;
#endif /* CONFIG_MALI_MIDGARD_DVFS && CONFIG_MALI_MTK_DVFS_POLICY */
	mtk_set_gpu_idle_fp = NULL;
	return 0;
}

