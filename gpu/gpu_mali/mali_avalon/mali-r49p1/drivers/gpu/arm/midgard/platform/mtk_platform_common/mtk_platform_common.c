// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <platform/mtk_platform_common.h>
#include <mtk_gpufreq.h>
#include <ged_dvfs.h>
#include <ged_base.h>
#include <ged_type.h>

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#include <mt-plat/aee.h>
#endif /* CONFIG_MTK_AEE_FEATURE */

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG) || IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
#include <platform/mtk_platform_common/mtk_platform_debug.h>
#endif /* CONFIG_MALI_MTK_DEBUG || CONFIG_MALI_MTK_DEBUG_DUMP*/

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include <platform/mtk_platform_common/mtk_platform_logbuffer.h>
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

#if IS_ENABLED(CONFIG_MALI_MTK_DIAGNOSIS_MODE)
#include "mtk_platform_diagnosis_mode.h"

#if IS_ENABLED(CONFIG_MALI_MTK_KE_DUMP_FWLOG)
#include "csf/mali_kbase_csf_trace_buffer.h"
#endif /* CONFIG_MALI_MTK_KE_DUMP_FWLOG */

#if IS_ENABLED(CONFIG_MALI_MTK_CM7_TRACE)
#include <bus_tracer_v1.h>
#include "mtk_platform_cm7_trace.h"
#endif /* CONFIG_MALI_MTK_CM7_TRACE */
#endif /* CONFIG_MALI_MTK_DIAGNOSIS_MODE */

#if IS_ENABLED(CONFIG_MALI_MTK_MEMTRACK)
#include <platform/mtk_platform_common/mtk_platform_memtrack.h>
#endif /* CONFIG_MALI_MTK_MEMTRACK */

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
#include <platform/mtk_platform_common/mtk_platform_dvfs.h>
#if IS_ENABLED(CONFIG_MALI_MTK_GPU_DVFS_HINT_26M_LOADING)
#include <platform/mtk_platform_common/mtk_platform_dvfs_hint_26m_perf_cnting.h>
#endif /* CONFIG_MALI_MTK_GPU_DVFS_HINT_26M_LOADING */
#endif /* CONFIG_MALI_MIDGARD_DVFS && CONFIG_MALI_MTK_DVFS_POLICY */

#if IS_ENABLED(CONFIG_MALI_MTK_PROC_FS)
#include <linux/proc_fs.h>

#if IS_ENABLED(CONFIG_MALI_MTK_ADAPTIVE_POWER_POLICY)
#include <platform/mtk_platform_common/mtk_platform_adaptive_power_policy.h>
#endif /* CONFIG_MALI_MTK_ADAPTIVE_POWER_POLICY */

#if IS_ENABLED(CONFIG_MTK_GPU_SWPM_SUPPORT)
#include <platform/mtk_mfg_counter.h>
#include <mtk_ltr_pmu.h>
#include <mtk_gpu_power_model_sspm_ipi.h>
#endif /* CONFIG_MTK_GPU_SWPM_SUPPORT */

/* name of the proc root dir */
#define	PROC_ROOT "mtk_mali"
static struct proc_dir_entry *proc_root;
#endif /* CONFIG_MALI_MTK_PROC_FS */

#if IS_ENABLED(CONFIG_MALI_MTK_DEVFREQ_GOVERNOR)
#include <platform/mtk_platform_common/mtk_platform_devfreq_governor.h>
#endif /* CONFIG_MALI_MTK_DEVFREQ_GOVERNOR */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FORCE_HARD_RESET)
#include <platform/mtk_platform_common/mtk_platform_whitebox_force_hard_reset.h>
#endif /* CONFIG_MALI_MTK_WHITEBOX_FORCE_HARD_RESET */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_DIRECTLY_HARD_RESET)
#include <platform/mtk_platform_common/mtk_platform_whitebox_directly_hard_reset.h>
#endif /* CONFIG_MALI_MTK_WHITEBOX_DIRECTLY_HARD_RESET */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FORCE_TERMINATE_CSG)
#include <platform/mtk_platform_common/mtk_platform_whitebox_force_terminate_csg.h>
#endif /* CONFIG_MALI_MTK_WHITEBOX_FORCE_TERMINATE_CSG */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FAULT_WORKER)
#include <platform/mtk_platform_common/mtk_platform_whitebox_fault_worker.h>
#endif /* CONFIG_MALI_MTK_WHITEBOX_FAULT_WORKER */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_SCHEDULER)
#include <platform/mtk_platform_common/mtk_platform_whitebox_scheduler.h>
#endif /* CONFIG_MALI_MTK_WHITEBOX_SCHEDULER */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_SYNC_UPDATE)
#include <platform/mtk_platform_common/mtk_platform_whitebox_sync_update.h>
#endif /* CONFIG_MALI_MTK_WHITEBOX_SYNC_UPDATE */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_MEMORY_FOOTPRINT)
#include <platform/mtk_platform_common/mtk_platform_whitebox_memory_footprint.h>
#endif /* CONFIG_MALI_MTK_WHITEBOX_MEMORY_FOOTPRINT */

#if IS_ENABLED(CONFIG_MALI_MTK_MBRAIN_SUPPORT)
#include <ged_mali_event.h>
#include <platform/mtk_platform_common/mtk_platform_mali_event.h>
#endif /* CONFIG_MALI_MTK_MBRAIN_SUPPORT */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL)
#include <platform/mtk_platform_common/mtk_platform_whitebox_missing_doorbell.h>
#endif /* CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL */

static bool mfg_powered;
static DEFINE_MUTEX(mfg_pm_lock);
static DEFINE_MUTEX(common_debug_lock);

static struct kbase_device *mali_kbdev;

struct kbase_device *mtk_common_get_kbdev(void)
{
	return mali_kbdev;
}

bool mtk_common_pm_is_mfg_active(void)
{
	return mfg_powered;
}

void mtk_common_pm_mfg_active(void)
{
	mutex_lock(&mfg_pm_lock);
	mfg_powered = true;
	mutex_unlock(&mfg_pm_lock);
}

void mtk_common_pm_mfg_idle(void)
{
	mutex_lock(&mfg_pm_lock);
	mfg_powered = false;
	mutex_unlock(&mfg_pm_lock);
}

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FORCE_HARD_RESET)
bool mtk_common_whitebox_force_hard_reset_enable(void)
{
	return mtk_whitebox_force_hard_reset_enable();
}
#endif /* CONFIG_MALI_MTK_WHITEBOX_FORCE_HARD_RESET */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_DIRECTLY_HARD_RESET)
bool mtk_common_whitebox_directly_hard_reset_enable(void)
{
	return mtk_whitebox_directly_hard_reset_enable();
}
#endif /* CONFIG_MALI_MTK_WHITEBOX_DIRECTLY_HARD_RESET */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FORCE_TERMINATE_CSG)
bool mtk_common_whitebox_force_terminate_csg_enable(void)
{
	return mtk_whitebox_force_terminate_csg_enable();
}
#endif /* CONFIG_MALI_MTK_WHITEBOX_FORCE_TERMINATE_CSG */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_SYNC_UPDATE)
int mtk_common_whitebox_sync_update_test_mode(void)
{
	return mtk_whitebox_sync_update_test_mode();
}
#endif /* CONFIG_MALI_MTK_WHITEBOX_SYNC_UPDATE */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL)
bool mtk_common_whitebox_missing_doorbell_enable(void)
{
	return mtk_whitebox_missing_doorbell_enable();
}
#endif /* CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL */
#if IS_ENABLED(CONFIG_MALI_MTK_DIAGNOSIS_MODE)
#if IS_ENABLED(CONFIG_MALI_MTK_POWER_TRANSITION_TIMEOUT_DEBUG)
#define MAX_STATES_NUM 16
extern u8 mcu_state_array[MAX_STATES_NUM];
extern u8 l2_state_array[MAX_STATES_NUM];
extern int mcu_history_idx;
extern int l2_history_idx;
#endif /* CONFIG_MALI_MTK_POWER_TRANSITION_TIMEOUT_DEBUG */
#endif /* CONFIG_MALI_MTK_DIAGNOSIS_MODE */
void mtk_common_debug(enum mtk_common_debug_types type, struct kbase_context *kctx, u64 hook_point)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();
#if IS_ENABLED(CONFIG_MALI_MTK_DIAGNOSIS_MODE)
	u64 diagnosis_mode = 0;
	u64 diagnosis_dump_mask = 0;
#if IS_ENABLED(CONFIG_MALI_MTK_POWER_TRANSITION_TIMEOUT_DEBUG)
	u8 tmp_state_array[MAX_STATES_NUM];
	int tmp_idx = 0 ;
	int i = 0 ;
#endif
#endif /* CONFIG_MALI_MTK_DIAGNOSIS_MODE */

	if (IS_ERR_OR_NULL(kbdev))
		return;

	if (type == MTK_COMMON_DBG_DUMP_DB_BY_SETTING)
	{
#if IS_ENABLED(CONFIG_MALI_MTK_DIAGNOSIS_MODE)
		diagnosis_mode = mtk_diagnosis_mode_get_mode();
		diagnosis_dump_mask = mtk_diagnosis_mode_get_dump_mask();
#if IS_ENABLED(CONFIG_MALI_MTK_POWER_TRANSITION_TIMEOUT_DEBUG)
		tmp_idx = mcu_history_idx % MAX_STATES_NUM; // Ensure tmp_idx is within bounds
		for (i = 0; i < MAX_STATES_NUM; i++) {
			tmp_state_array[i] = mcu_state_array[tmp_idx];
			tmp_idx = (tmp_idx + 1) % MAX_STATES_NUM; // Wrap around to 0 when reaching MAX_STATES_NUM
		}
#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
		if (kbdev->is_reset_triggered_by_fence_timeout) {
			mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
				"mcu state back trace %hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu\n",
				tmp_state_array[0], tmp_state_array[1], tmp_state_array[2], tmp_state_array[3],
				tmp_state_array[4], tmp_state_array[5], tmp_state_array[6], tmp_state_array[7],
				tmp_state_array[8], tmp_state_array[9], tmp_state_array[10], tmp_state_array[11],
				tmp_state_array[12], tmp_state_array[13], tmp_state_array[14], tmp_state_array[15]);
		} else {
			dev_info(kbdev->dev, "mcu state back trace %hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu\n",
				tmp_state_array[0], tmp_state_array[1], tmp_state_array[2], tmp_state_array[3],
				tmp_state_array[4], tmp_state_array[5], tmp_state_array[6], tmp_state_array[7],
				tmp_state_array[8], tmp_state_array[9], tmp_state_array[10], tmp_state_array[11],
				tmp_state_array[12], tmp_state_array[13], tmp_state_array[14], tmp_state_array[15]);
		}
#else /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
		dev_info(kbdev->dev, "mcu state back trace %hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu\n",
			tmp_state_array[0], tmp_state_array[1], tmp_state_array[2], tmp_state_array[3],
			tmp_state_array[4], tmp_state_array[5], tmp_state_array[6], tmp_state_array[7],
			tmp_state_array[8], tmp_state_array[9], tmp_state_array[10], tmp_state_array[11],
			tmp_state_array[12], tmp_state_array[13], tmp_state_array[14], tmp_state_array[15]);
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"mcu state back trace %hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu\n",
			tmp_state_array[0], tmp_state_array[1], tmp_state_array[2], tmp_state_array[3],
			tmp_state_array[4], tmp_state_array[5], tmp_state_array[6], tmp_state_array[7],
			tmp_state_array[8], tmp_state_array[9], tmp_state_array[10], tmp_state_array[11],
			tmp_state_array[12], tmp_state_array[13], tmp_state_array[14], tmp_state_array[15]);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		tmp_idx = l2_history_idx % MAX_STATES_NUM;  // Ensure tmp_idx is within bounds
		for (i = 0; i < MAX_STATES_NUM; i++) {
			tmp_state_array[i] = l2_state_array[tmp_idx];
			tmp_idx = (tmp_idx + 1) % MAX_STATES_NUM;  // Increment and wrap around
		}
#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
		if (kbdev->is_reset_triggered_by_fence_timeout) {
			mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
				"l2 state back trace %hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu\n",
				tmp_state_array[0], tmp_state_array[1], tmp_state_array[2], tmp_state_array[3],
				tmp_state_array[4], tmp_state_array[5], tmp_state_array[6], tmp_state_array[7],
				tmp_state_array[8], tmp_state_array[9], tmp_state_array[10], tmp_state_array[11],
				tmp_state_array[12], tmp_state_array[13], tmp_state_array[14], tmp_state_array[15]);
		} else {
			dev_info(kbdev->dev, "l2 state back trace %hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu\n",
				tmp_state_array[0], tmp_state_array[1], tmp_state_array[2], tmp_state_array[3],
				tmp_state_array[4], tmp_state_array[5], tmp_state_array[6], tmp_state_array[7],
				tmp_state_array[8], tmp_state_array[9], tmp_state_array[10], tmp_state_array[11],
				tmp_state_array[12], tmp_state_array[13], tmp_state_array[14], tmp_state_array[15]);
		}
#else /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
		dev_info(kbdev->dev, "l2 state back trace %hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu\n",
			tmp_state_array[0], tmp_state_array[1], tmp_state_array[2], tmp_state_array[3],
			tmp_state_array[4], tmp_state_array[5], tmp_state_array[6], tmp_state_array[7],
			tmp_state_array[8], tmp_state_array[9], tmp_state_array[10], tmp_state_array[11],
			tmp_state_array[12], tmp_state_array[13], tmp_state_array[14], tmp_state_array[15]);
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"l2 state back trace %hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu->%hhu\n",
			tmp_state_array[0], tmp_state_array[1], tmp_state_array[2], tmp_state_array[3],
			tmp_state_array[4], tmp_state_array[5], tmp_state_array[6], tmp_state_array[7],
			tmp_state_array[8], tmp_state_array[9], tmp_state_array[10], tmp_state_array[11],
			tmp_state_array[12], tmp_state_array[13], tmp_state_array[14], tmp_state_array[15]);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
#endif /* CONFIG_MALI_MTK_POWER_TRANSITION_TIMEOUT_DEBUG */
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"diagnosis hook = 0x%08llx, mode = %llu, mask = 0x%08llx", hook_point, diagnosis_mode, diagnosis_dump_mask);
#else /* CONFIG_MALI_MTK_LOG_BUFFER */
		dev_info(kbdev->dev, "diagnosis hook = 0x%08llx, mode = %llu, mask = 0x%08llx", hook_point, diagnosis_mode, diagnosis_dump_mask);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		if (hook_point & diagnosis_dump_mask) {
			if (diagnosis_mode == 0) {
				return; // do nothing if diagnosis mode is not enabled
			} else if (diagnosis_mode == 1) {
				type = MTK_COMMON_DBG_TRIGGER_KERNEL_API;
			} else if (diagnosis_mode == 2) {
				type = MTK_COMMON_DBG_DUMP_FULL_DB;
			}
		} else {
			return; // do nothing if hook point is not matched
		}
#else /* CONFIG_MALI_MTK_DIAGNOSIS_MODE */
		return;
#endif /* CONFIG_MALI_MTK_DIAGNOSIS_MODE */
	}

	lockdep_off();
	do {
		if (!mutex_trylock(&common_debug_lock)) {
			pr_info("[%s]lock held, bypass debug dump", __func__);
			break;
		}
		switch (type) {
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
		case MTK_COMMON_DBG_CSF_DUMP_GROUPS_QUEUES:
			mtk_debug_csf_dump_groups_and_queues(kbdev, kctx);
			break;
		case MTK_COMMON_DBG_CSF_DUMP_ITER_HWIF:
			mtk_debug_csf_dump_iterator_hwif(kbdev);
			break;
		case MTK_COMMON_DBG_CSF_DUMP_ITER_HWIF_LOCKED:
			mtk_debug_csf_dump_iterator_hwif_locked(kbdev);
			break;
		case MTK_COMMON_DBG_DUMP_INFRA_STATUS:
			if (!mtk_common_gpufreq_bringup())
				mtk_debug_dump_infra_status(kbdev);
			break;
		case MTK_COMMON_DBG_DUMP_GIC_STATUS:
			mtk_debug_dump_gic_status(kbdev);
			break;
		case MTK_COMMON_DBG_DUMP_PM_STATUS:
			mtk_debug_dump_pm_status(kbdev);
			break;
		case MTK_COMMON_DBG_DUMP_ENOP_METADATA:
			mtk_debug_dump_enop_metadata(kbdev);
			break;
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP*/
#if IS_ENABLED(CONFIG_MALI_MTK_DIAGNOSIS_MODE)
		case MTK_COMMON_DBG_DUMP_FULL_DB:
			dev_info(kbdev->dev, "trigger gpu full DB dump");
#if IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
			if (diagnosis_dump_mask & MTK_DBG_COMMON_DUMP_ENABLE_GROUPS_QUEUES) {
				mtk_debug_csf_dump_groups_and_queues(kbdev, kctx);
			}
#endif /* CONFIG_MALI_MTK_FENCE_DEBUG */
#if IS_ENABLED(CONFIG_MALI_MTK_KE_DUMP_FWLOG)
			if (!(diagnosis_dump_mask & MTK_DBG_COMMON_DUMP_SKIP_FWLOG)) {
				mtk_kbase_csf_firmware_ke_dump_fwlog(kbdev); /* dump fwlog, reserve 1MB for fwlog */
			}
#endif /* CONFIG_MALI_MTK_KE_DUMP_FWLOG */
#if IS_ENABLED(CONFIG_MALI_MTK_CM7_TRACE)
#if IS_ENABLED(CONFIG_MTK_GPU_DIAGNOSIS_DEBUG)
			disable_etb_capture(); /* stop ETB capture before DFD trig */
#endif /* CONFIG_MTK_GPU_DIAGNOSIS_DEBUG */
#endif /* CONFIG_MALI_MTK_CM7_TRACE */
			BUG_ON(1);
			break;
		case MTK_COMMON_DBG_TRIGGER_KERNEL_API:
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
			aee_kernel_exception("GPU", "pid:%d", kctx->tgid);
#endif /* CONFIG_MTK_AEE_FEATURE */
			break;
#endif /* CONFIG_MALI_MTK_DIAGNOSIS_MODE */
		default:
			dev_info(kbdev->dev, "@%s: unsupported type (%d)", __func__, type);
			break;
		}
		mutex_unlock(&common_debug_lock);
	} while (false);
	lockdep_on();
}

int mtk_common_gpufreq_bringup(void)
{
	static int bringup = -1;

	if (bringup == -1) {
#if IS_ENABLED(CONFIG_MTK_GPUFREQ_V2)
		bringup = gpufreq_bringup();
#else
		bringup = mt_gpufreq_bringup();
#endif /* CONFIG_MTK_GPUFREQ_V2 */
	}

	return bringup;
}

int mtk_common_gpufreq_commit(int opp_idx)
{
	int ret = -1;

	mutex_lock(&mfg_pm_lock);
	if (opp_idx >= 0 && mtk_common_pm_is_mfg_active()) {
#if IS_ENABLED(CONFIG_MTK_GPUFREQ_V2)
		ret = mtk_common_gpufreq_bringup() ?
			-1 : gpufreq_commit(TARGET_DEFAULT, opp_idx);
#else
		ret = mtk_common_gpufreq_bringup() ?
			-1 : mt_gpufreq_target(opp_idx, KIR_POLICY);
#endif /* CONFIG_MTK_GPUFREQ_V2 */
	}
	mutex_unlock(&mfg_pm_lock);

	return ret;
}

int mtk_common_gpufreq_dual_commit(int gpu_oppidx, int stack_oppidx)
{
	int ret = -1;

	mutex_lock(&mfg_pm_lock);
	if (stack_oppidx >= 0 && mtk_common_pm_is_mfg_active()) {
#if IS_ENABLED(CONFIG_MTK_GPUFREQ_V2)
		ret = mtk_common_gpufreq_bringup() ?
			-1 : gpufreq_dual_commit(gpu_oppidx, stack_oppidx);
#else
		ret = mtk_common_gpufreq_bringup() ?
			-1 : mt_gpufreq_target(stack_oppidx, KIR_POLICY);
#endif /* CONFIG_MTK_GPUFREQ_V2 */
	}
	mutex_unlock(&mfg_pm_lock);

	return ret;
}

int mtk_common_ged_dvfs_get_last_commit_idx(void)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	return (int)ged_dvfs_get_last_commit_idx();
#else
	return -1;
#endif
}

int mtk_common_ged_dvfs_get_last_commit_top_idx(void)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	return (int)ged_dvfs_get_last_commit_top_idx();
#else
	return -1;
#endif
}

int mtk_common_ged_dvfs_get_last_commit_stack_idx(void)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	return (int)ged_dvfs_get_last_commit_stack_idx();
#else
	return -1;
#endif
}
unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_idx(void)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	return (unsigned long)ged_dvfs_write_sysram_last_commit_idx();
#else
	return -1;
#endif
}

unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_top_idx(void)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	return (unsigned long)ged_dvfs_write_sysram_last_commit_top_idx();
#else
	return -1;
#endif
}

unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_stack_idx(void)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	return (unsigned long)ged_dvfs_write_sysram_last_commit_stack_idx();
#else
	return -1;
#endif
}

unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_dual(void) {
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
		return (unsigned long)ged_dvfs_write_sysram_last_commit_dual();
#else
		return -1;
#endif
}

unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_idx_test(int commit_idx) {
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	return (unsigned long)ged_dvfs_write_sysram_last_commit_idx_test(commit_idx);
#else
	return -1;
#endif
}

unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_top_idx_test(int commit_idx)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	return (unsigned long)ged_dvfs_write_sysram_last_commit_top_idx_test(commit_idx);
#else
	return -1;
#endif
}

unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_stack_idx_test(int commit_idx)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	return (unsigned long)ged_dvfs_write_sysram_last_commit_stack_idx_test(commit_idx);
#else
	return -1;
#endif
}

unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_dual_test(int top_idx, int stack_idx)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	return (unsigned long)ged_dvfs_write_sysram_last_commit_dual_test(top_idx, stack_idx);
#else
	return -1;
#endif
}

int mtk_common_ged_dvfs_update_step_size(int low_step, int med_step, int high_step)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	return ged_dvfs_update_step_size(low_step, med_step, high_step);
#else
	return -1;
#endif
}

void mtk_common_get_system_timer_and_record(struct kbase_device *kbdev)
{
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && \
	IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY) && \
	IS_ENABLED(CONFIG_MALI_MTK_GPU_DVFS_READ_SOC_TIMER)
	u64 system_time_tmp = 0;
	bool gpu_ready;

	lockdep_assert_held(&kbdev->hwaccess_lock);

	gpu_ready = kbdev->pm.backend.gpu_ready;

	if (gpu_ready) {
		kbase_backend_get_gpu_time_norequest(kbdev, NULL, &system_time_tmp, NULL);
		ged_dvfs_record_soc_timer(system_time_tmp);
	}

	return;
#else
	return;
#endif

}

int mtk_common_ged_pwr_hint(int pwr_hint)
{
#if IS_ENABLED(CONFIG_MALI_MTK_PWR_HINT)
	return (int)ged_write_sysram_pwr_hint(pwr_hint);
#else
	return 0;
#endif
}

#if IS_ENABLED(CONFIG_PROC_FS)
static void mtk_common_procfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return;

  	proc_root = proc_mkdir(PROC_ROOT, NULL);
  	if (!proc_root) {
		dev_info(kbdev->dev, "@%s: Cann't create /proc/%s", __func__, PROC_ROOT);
  		return;
  	}

#if IS_ENABLED(CONFIG_MALI_MTK_MEMTRACK)
	mtk_memtrack_procfs_init(kbdev, proc_root);
#endif /* CONFIG_MALI_MTK_MEMTRACK */

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_procfs_init(kbdev, proc_root);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	mtk_dvfs_procfs_init(kbdev, proc_root);
#endif /* CONFIG_MALI_MIDGARD_DVFS && CONFIG_MALI_MTK_DVFS_POLICY */
}

static void mtk_common_procfs_term(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return;

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	mtk_dvfs_procfs_term(kbdev, proc_root);
#endif /* CONFIG_MALI_MIDGARD_DVFS && CONFIG_MALI_MTK_DVFS_POLICY */

#if IS_ENABLED(CONFIG_MALI_MTK_MEMTRACK)
	mtk_memtrack_procfs_term(kbdev, proc_root);
#endif /* CONFIG_MALI_MTK_MEMTRACK */

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_procfs_term(kbdev, proc_root);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

	proc_root = NULL;
	remove_proc_entry(PROC_ROOT, NULL);
}
#endif /* CONFIG_MALI_MTK_PROC_FS */

#if IS_ENABLED(CONFIG_MALI_MTK_SYSFS)
void mtk_common_sysfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return;

#if IS_ENABLED(CONFIG_MALI_MTK_DIAGNOSIS_MODE)
	mtk_diagnosis_mode_sysfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_DIAGNOSIS_MODE */

#if IS_ENABLED(CONFIG_MALI_MTK_CM7_TRACE)
        mtk_cm7_trace_sysfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_CM7_TRACE */
}

void mtk_common_sysfs_term(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return;

#if IS_ENABLED(CONFIG_MALI_MTK_CM7_TRACE)
        mtk_cm7_trace_sysfs_term(kbdev);
#endif /* CONFIG_MALI_MTK_CM7_TRACE */

#if IS_ENABLED(CONFIG_MALI_MTK_DIAGNOSIS_MODE)
	mtk_diagnosis_mode_sysfs_term(kbdev);
#endif /* CONFIG_MALI_MTK_DIAGNOSIS_MODE */
}
#endif /* CONFIG_MALI_MTK_SYSFS */

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_FS)
static int mtk_debug_sleep_mode(struct seq_file *file, void *data)
{
	struct kbase_device *kbdev = file->private;
	struct device_node *np;
	u32 sleep_mode_enable = 0;

	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	np = kbdev->dev->of_node;

	if (!of_property_read_u32(np, "sleep-mode-enable", &sleep_mode_enable))
		seq_printf(file, "Sleep mode: %s\n", (sleep_mode_enable) ? "enabled": "disabled");
	else
		seq_printf(file, "Sleep mode: No dts property setting, default disabled\n");

	return 0;
}

static int mtk_sleep_mode_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, mtk_debug_sleep_mode,
	                   in->i_private);
}

static const struct file_operations mtk_sleep_mode_debugfs_fops = {
	.open = mtk_sleep_mode_debugfs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

int mtk_debug_sleep_mode_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("sleep_mode", 0440,
		kbdev->mali_debugfs_directory, kbdev,
		&mtk_sleep_mode_debugfs_fops);

	return 0;
}

void mtk_common_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return;

	mtk_debug_sleep_mode_debugfs_init(kbdev);
#if IS_ENABLED(CONFIG_MALI_MTK_ADAPTIVE_POWER_POLICY)
	mtk_debug_adaptive_power_policy_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_ADAPTIVE_POWER_POLICY */
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
	mtk_debug_dump_enop_metadata_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */
#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FORCE_HARD_RESET)
	mtk_whitebox_force_hard_reset_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_WHITEBOX_FORCE_HARD_RESET */
#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_DIRECTLY_HARD_RESET)
	mtk_whitebox_directly_hard_reset_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_WHITEBOX_DIRECTLY_HARD_RESET */
#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FORCE_TERMINATE_CSG)
	mtk_whitebox_force_terminate_csg_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_WHITEBOX_FORCE_TERMINATE_CSG */
#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FAULT_WORKER)
	mtk_whitebox_fault_worker_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_WHITEBOX_FAULT_WORKER */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_SCHEDULER)
	mtk_whitebox_scheduler_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_WHITEBOX_SCHEDULER */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_SYNC_UPDATE)
	mtk_whitebox_sync_update_test_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_WHITEBOX_SYNC_UPDATE */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL)
	mtk_whitebox_missing_doorbell_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_MEMORY_FOOTPRINT)
	mtk_whitebox_memory_footprint_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_WHITEBOX_MEMORY_FOOTPRINT */

#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
	mtk_logbuffer_deferred_enable_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
}

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
void mtk_common_csf_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return;
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
	mtk_debug_csf_debugfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */
}
#endif /* CONFIG_MALI_CSF_SUPPORT */
#endif /* CONFIG_MALI_MTK_DEBUG_FS */

int mtk_common_device_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev)) {
		dev_info(kbdev->dev, "@%s: invalid kbdev", __func__);
		return -1;
	}

	mali_kbdev = kbdev;

	if (mtk_platform_pm_init(kbdev)) {
		dev_info(kbdev->dev, "@%s: Failed to init Platform PM", __func__);
		return -1;
	}

#if IS_ENABLED(CONFIG_MALI_MTK_MEMTRACK)
	mtk_memtrack_init(kbdev);
#endif /* CONFIG_MALI_MTK_MEMTRACK */

#if IS_ENABLED(CONFIG_MALI_MTK_DEVFREQ_GOVERNOR)
	mtk_devfreq_governor_init(kbdev);
#endif /* CONFIG_MALI_MTK_DEVFREQ_GOVERNOR */

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_init(kbdev);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

#if IS_ENABLED(CONFIG_MALI_MTK_PROC_FS)
	mtk_common_procfs_init(kbdev);
#endif /* CONFIG_MALI_MTK_PROC_FS */

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
	mtk_debug_dump_infra_status_init();
	mtk_debug_dump_enop_metadata_init(kbdev);
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */

#if IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
	mutex_init(&kbdev->register_check_lock);
	mutex_init(&kbdev->mmu_debug_info_lock);
	kbdev->mmu_debug_info_head = 0;
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */

#if IS_ENABLED(CONFIG_MALI_MTK_TRIGGER_KE)
	kbdev->exception_mask = (1u << EXCEPTION_RESET_FAILED);
	if (!of_property_read_u32(kbdev->dev->of_node, "exception-mask", &kbdev->exception_mask))
		dev_info(kbdev->dev, "@%s: exception_mask=0x%x",
				__func__, kbdev->exception_mask);
	else
		dev_info(kbdev->dev, "@%s: no dts property setting, default exception_mask=0x%x",
				__func__, kbdev->exception_mask);
#endif /* CONFIG_MALI_MTK_TRIGGER_KE */

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	mtk_dvfs_init(kbdev);
#endif /* CONFIG_MALI_MIDGARD_DVFS && CONFIG_MALI_MTK_DVFS_POLICY */

#if IS_ENABLED(CONFIG_MTK_GPU_SWPM_SUPPORT)
	mtk_mfg_counter_init();
	MTK_GPU_Power_model_init();
	MTK_LTR_gpu_pmu_init();
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FORCE_HARD_RESET)
	mtk_whitebox_force_hard_reset_init();
#endif /* CONFIG_MALI_MTK_WHITEBOX_FORCE_HARD_RESET */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_DIRECTLY_HARD_RESET)
	mtk_whitebox_directly_hard_reset_init();
#endif /* CONFIG_MALI_MTK_WHITEBOX_DIRECTLY_HARD_RESET */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FORCE_TERMINATE_CSG)
	mtk_whitebox_force_terminate_csg_init();
#endif /* CONFIG_MALI_MTK_WHITEBOX_FORCE_TERMINATE_CSG */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_SYNC_UPDATE)
	mtk_whitebox_sync_update_test_init();
#endif /* CONFIG_MALI_MTK_WHITEBOX_SYNC_UPDATE */

#if IS_ENABLED(CONFIG_MALI_MTK_MBRAIN_SUPPORT)
	ged_mali_event_init();
#endif /* CONFIG_MALI_MTK_MBRAIN_SUPPORT */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_MEMORY_FOOTPRINT)
	kbdev->mem_whitebox_debug = false;
#endif /* CONFIG_MALI_MTK_WHITEBOX_MEMORY_FOOTPRINT */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL)
	mtk_whitebox_missing_doorbell_init();
#endif /* CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL */

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_RESET_DEBUG)
	kbdev->reset_exception_mask = 0;
	if (!of_property_read_u32(kbdev->dev->of_node, "reset-exception-mask", &kbdev->reset_exception_mask))
		dev_info(kbdev->dev, "@%s: reset_exception_mask=0x%x",
				__func__, kbdev->reset_exception_mask);
	else
		dev_info(kbdev->dev, "@%s: no dts property setting, default reset_exception_mask=0x%x",
				__func__, kbdev->reset_exception_mask);
#endif /* CONFIG_MALI_MTK_GPU_RESET_DEBUG */

	return 0;
}

void mtk_common_device_term(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev)) {
		dev_info(kbdev->dev, "@%s: invalid kbdev", __func__);
		return;
	}

#if IS_ENABLED(CONFIG_MALI_MTK_MEMTRACK)
	mtk_memtrack_term(kbdev);
#endif /* CONFIG_MALI_MTK_MEMTRACK */

#if IS_ENABLED(CONFIG_MALI_MTK_DEVFREQ_GOVERNOR)
	mtk_devfreq_governor_term(kbdev);
#endif /* CONFIG_MALI_MTK_DEVFREQ_GOVERNOR */

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_term(kbdev);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

#if IS_ENABLED(CONFIG_MALI_MTK_PROC_FS)
	mtk_common_procfs_term(kbdev);
#endif /* CONFIG_MALI_MTK_PROC_FS */

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
	mtk_debug_dump_infra_status_term();
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	mtk_dvfs_term(kbdev);
#endif /* CONFIG_MALI_MIDGARD_DVFS && CONFIG_MALI_MTK_DVFS_POLICY */

#if IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
	mutex_destroy(&kbdev->register_check_lock);
	mutex_destroy(&kbdev->mmu_debug_info_lock);
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */

#if IS_ENABLED(CONFIG_MTK_GPU_SWPM_SUPPORT)
	mtk_mfg_counter_destroy();
	MTK_GPU_Power_model_destroy();
	MTK_LTR_gpu_pmu_destroy();
#endif


	mtk_platform_pm_term(kbdev);
}

#ifdef CONFIG_MALI_MTK_SHADER_PWR_CTL_WA
void mtk_set_gpufreq_clock_parking_lock(unsigned long *pFlags)
{
	_gpufreq_clock_parking_lock(pFlags);
}

void mtk_set_gpufreq_clock_parking_unlock(unsigned long *pFlags)
{
	_gpufreq_clock_parking_unlock(pFlags);
}

int mtk_set_gpufreq_clock_parking(int clksrc)
{
	/*
	 * This function will be called under the Interrupt-Handler,
	 * so can't implement any mutex-lock behaviors
	 * (that will result the sleep/schedule operations).
	 */

	int ret = 0;

	if (mtk_common_pm_is_mfg_active())
		ret = _gpufreq_clock_parking(clksrc);
	else
		pr_info("MALI: set clock parking at power off\n");

	return ret;
}
#endif /* CONFIG_MALI_MTK_SHADER_PWR_CTL_WA */
