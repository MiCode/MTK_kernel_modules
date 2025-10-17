// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_COMMON_H__
#define __MTK_PLATFORM_COMMON_H__

#include <linux/platform_device.h>
#include <backend/gpu/mali_kbase_pm_defs.h>

enum mtk_common_debug_types {
	MTK_COMMON_DBG_DUMP_PM_STATUS,
	MTK_COMMON_DBG_DUMP_INFRA_STATUS,
	MTK_COMMON_DBG_CSF_DUMP_GROUPS_QUEUES,
	MTK_COMMON_DBG_CSF_DUMP_ITER_HWIF,
	MTK_COMMON_DBG_TRIGGER_KERNEL_API,
	MTK_COMMON_DBG_TRIGGER_WARN_ON,
	MTK_COMMON_DBG_TRIGGER_BUG_ON,
	MTK_COMMON_DBG_DUMP_FULL_DB,
	MTK_COMMON_DBG_DUMP_DB_BY_SETTING,
	MTK_COMMON_DBG_DUMP_ENOP_METADATA,
	MTK_COMMON_DBG_DUMP_GIC_STATUS,
};

#define MTK_DBG_HOOK_NA                                        ((u64)0x0)
#define MTK_DBG_HOOK_CSFATAL                                   ((u64)0x1<<0)
#define MTK_DBG_HOOK_CSFATAL_FWINTERNAL                        ((u64)0x1<<1)
#define MTK_DBG_HOOK_CSFATAL_QUEUENOTBOUND                     ((u64)0x1<<2)
#define MTK_DBG_HOOK_CSFAULT                                   ((u64)0x1<<9)
#define MTK_DBG_HOOK_MMU_UNHANDLEDPAGEFAULT                    ((u64)0x1<<12)
#define MTK_DBG_HOOK_MMU_UNEXPECTEDPAGEFAULT                   ((u64)0x1<<13)
#define MTK_DBG_HOOK_MMU_BUSFAULT                              ((u64)0x1<<14)
#define MTK_DBG_HOOK_FENCE_EXTERNAL_TIMEOUT                    ((u64)0x1<<16)
#define MTK_DBG_HOOK_FENCE_EXTERNAL_KCPU_TIMEOUT               ((u64)0x1<<17)
#define MTK_DBG_HOOK_FENCE_INTERNAL_TIMEOUT                    ((u64)0x1<<18)
#define MTK_DBG_HOOK_RESET                                     ((u64)0x1<<24)
#define MTK_DBG_HOOK_RESET_FAIL                                ((u64)0x1<<25)
#define MTK_DBG_HOOK_BITSTUCK_FAIL                             ((u64)0x1<<32)
#define MTK_DBG_HOOK_GLOBALREQUEST_TIMEOUT                     ((u64)0x1<<33)
#define MTK_DBG_HOOK_MCUPOWERON_FAIL                           ((u64)0x1<<34)
#define MTK_DBG_HOOK_FWRELOAD_FAIL                             ((u64)0x1<<35)
#define MTK_DBG_HOOK_GSG_TIMEOUT                               ((u64)0x1<<36)
#define MTK_DBG_HOOK_PM_TIMEOUT                                ((u64)0x1<<37)
#define MTK_DBG_HOOK_PM_RESET_FAIL                             ((u64)0x1<<38)
#define MTK_DBG_HOOK_FWBOOT_TIMEOUT                            ((u64)0x1<<39)
#define MTK_DBG_HOOK_MALI_FENCE_SIGNAL_TIMEOUT                 ((u64)0x1<<40)
// bit 56~63 to control dump in common dump flow
#define MTK_DBG_COMMON_DUMP_SKIP_ETB                           ((u64)0x1<<60)
#define MTK_DBG_COMMON_DUMP_SKIP_FWLOG                         ((u64)0x1<<61)
#define MTK_DBG_COMMON_DUMP_SKIP_COREDUMP                      ((u64)0x1<<62)
#define MTK_DBG_COMMON_DUMP_ENABLE_GROUPS_QUEUES               ((u64)0x1<<63)

struct kbase_device *mtk_common_get_kbdev(void);

bool mtk_common_pm_is_mfg_active(void);
void mtk_common_pm_mfg_active(void);
void mtk_common_pm_mfg_idle(void);

void mtk_common_debug(enum mtk_common_debug_types type, struct kbase_context *kctx, u64 hook_point);
int mtk_common_gpufreq_bringup(void);
int mtk_common_gpufreq_commit(int opp_idx);
int mtk_common_gpufreq_dual_commit(int gpu_oppidx, int stack_oppidx);
int mtk_common_ged_dvfs_get_last_commit_idx(void);
int mtk_common_ged_dvfs_get_last_commit_top_idx(void);
int mtk_common_ged_dvfs_get_last_commit_stack_idx(void);

unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_idx(void);
unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_top_idx(void);
unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_stack_idx(void);
unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_idx_test(int commit_idx);
unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_top_idx_test(int commit_idx);
unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_stack_idx_test(int commit_idx);
unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_dual(void);
unsigned long mtk_common_ged_dvfs_write_sysram_last_commit_dual_test(int top_idx, int stack_idx);
int mtk_common_ged_pwr_hint(int pwr_hint);
void mtk_common_get_system_timer_and_record(struct kbase_device *kbdev);

int mtk_common_ged_dvfs_update_step_size(int low_step, int med_step, int high_step);

int mtk_common_device_init(struct kbase_device *kbdev);
void mtk_common_device_term(struct kbase_device *kbdev);

#if IS_ENABLED(CONFIG_MALI_MTK_SYSFS)
void mtk_common_sysfs_init(struct kbase_device *kbdev);
void mtk_common_sysfs_term(struct kbase_device *kbdev);
#endif /* CONFIG_MALI_MTK_SYSFS */

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_FS)
void mtk_common_debugfs_init(struct kbase_device *kbdev);
void mtk_common_csf_debugfs_init(struct kbase_device *kbdev);
#endif /* CONFIG_MALI_MTK_DEBUG_FS */

int mtk_platform_pm_init(struct kbase_device *kbdev);
void mtk_platform_pm_term(struct kbase_device *kbdev);

#if IS_ENABLED(CONFIG_MALI_MTK_ACP_DSU_REQ) || IS_ENABLED(CONFIG_MALI_MTK_ACP_DSU_REQ_LEGACY)
#define REQ_DSU_POWER_ON (1)
#define REQ_DSU_POWER_OFF (0)
#define GPUACP_SMC_OP_CPUPM_PWR (1)
void mtk_platform_cpu_cache_request(struct kbase_device *kbdev, int request, enum kbase_l2_core_state l2_state);
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FORCE_HARD_RESET)
bool mtk_common_whitebox_force_hard_reset_enable(void);
#endif /* CONFIG_MALI_MTK_WHITEBOX_FORCE_HARD_RESET */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_FORCE_TERMINATE_CSG)
bool mtk_common_whitebox_force_terminate_csg_enable(void);
#endif /* CONFIG_MALI_MTK_WHITEBOX_FORCE_TERMINATE_CSG */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_SYNC_UPDATE)
int mtk_common_whitebox_sync_update_test_mode(void);
#endif /* CONFIG_MALI_MTK_WHITEBOX_SYNC_UPDATE */

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL)
bool mtk_common_whitebox_missing_doorbell_enable(void);
#endif /* CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL */

#endif /* __MTK_PLATFORM_COMMON_H__ */
