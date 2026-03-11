// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <platform/mtk_platform_common.h>
#include <mtk_gpufreq.h>
#include <ged_dvfs.h>
#if IS_ENABLED(CONFIG_PROC_FS)
#include <linux/proc_fs.h>
#if IS_ENABLED(CONFIG_MALI_MTK_MEM_TRACK)
#include <device/mali_kbase_device.h>
#endif
#endif
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
#include "mtk_platform_debug.h"
#endif
#if IS_ENABLED(CONFIG_MALI_MTK_MEM_TRACK)
extern unsigned int (*mtk_get_gpu_memory_usage_fp)(void);
#endif

static bool mfg_powered;
static DEFINE_MUTEX(mfg_pm_lock);
static struct kbase_device *mali_kbdev;
#if IS_ENABLED(CONFIG_PROC_FS)
static struct proc_dir_entry *mtk_mali_root;
#endif

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

void mtk_common_debug_dump(void)
{
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
	mtk_common_debug_dump_status();
#endif
}

int mtk_common_gpufreq_bringup(void)
{
	static int bringup = -1;

	if (bringup == -1) {
#if defined(CONFIG_MTK_GPUFREQ_V2)
		bringup = gpufreq_bringup();
#else
		bringup = mt_gpufreq_bringup();
#endif
	}

	return bringup;
}

int mtk_common_gpufreq_commit(int opp_idx)
{
	int ret = -1;

	mutex_lock(&mfg_pm_lock);
	if (opp_idx >= 0 && mtk_common_pm_is_mfg_active()) {
#if defined(CONFIG_MTK_GPUFREQ_V2)
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

int mtk_common_ged_dvfs_get_last_commit_idx(void)
{
	return -1;
}

#if IS_ENABLED(CONFIG_MALI_MTK_MEM_TRACK)
static unsigned int mtk_common_gpu_memory_usage(void)
{
	unsigned int used_pages = atomic_read(&(mali_kbdev->memdev.used_pages));
	return used_pages * 4096;
}
#endif

#if IS_ENABLED(CONFIG_PROC_FS)
void mtk_common_procfs_init(void)
{
  	mtk_mali_root = proc_mkdir("mtk_mali", NULL);
  	if (!mtk_mali_root) {
  		pr_info("cannot create /proc/%s\n", "mtk_mali");
  		return;
  	}
}

void mtk_common_procfs_exit(void)
{
	mtk_mali_root = NULL;
}
#endif


int mtk_common_device_init(struct kbase_device *kbdev)
{
	if (!kbdev) {
		pr_info("@%s: kbdev is NULL\n", __func__);
		return -1;
	}


	return 0;
}

void mtk_common_device_term(struct kbase_device *kbdev)
{

}
