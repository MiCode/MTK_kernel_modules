// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include "mali_kbase_config_platform.h"
#include "platform/mtk_platform_common.h"
#include <ged_dvfs.h>
#include <mtk_gpufreq.h>
#include <mtk_gpu_utility.h>
#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
#include <mboot_params.h>
#endif
#if IS_ENABLED(CONFIG_MTK_GPU_SWPM_SUPPORT)
#include <mtk_gpu_power_sspm_ipi.h>
#endif

DEFINE_MUTEX(g_mfg_lock);
static int g_cur_opp_idx;

enum gpu_dvfs_status_step {
	GPU_DVFS_STATUS_STEP_1 = 0x1,
	GPU_DVFS_STATUS_STEP_2 = 0x2,
	GPU_DVFS_STATUS_STEP_3 = 0x3,
	GPU_DVFS_STATUS_STEP_4 = 0x4,
	GPU_DVFS_STATUS_STEP_5 = 0x5,
	GPU_DVFS_STATUS_STEP_6 = 0x6,
	GPU_DVFS_STATUS_STEP_7 = 0x7,
	GPU_DVFS_STATUS_STEP_8 = 0x8,
	GPU_DVFS_STATUS_STEP_9 = 0x9,
	GPU_DVFS_STATUS_STEP_A = 0xA,
	GPU_DVFS_STATUS_STEP_B = 0xB,
	GPU_DVFS_STATUS_STEP_C = 0xC,
	GPU_DVFS_STATUS_STEP_D = 0xD,
	GPU_DVFS_STATUS_STEP_E = 0xE,
	GPU_DVFS_STATUS_STEP_F = 0xF,
};

static inline void gpu_dvfs_status_footprint(enum gpu_dvfs_status_step step)
{
#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
	aee_rr_rec_gpu_dvfs_status(step |
				(aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif
}

static inline void gpu_dvfs_status_reset_footprint(void)
{
#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
	aee_rr_rec_gpu_dvfs_status(0);
#endif
}

static int pm_callback_power_on_nolock(struct kbase_device *kbdev)
{
#if defined(CONFIG_MTK_GPUFREQ_V2)
	if (mtk_common_gpufreq_bringup()) {
		mtk_common_pm_mfg_active();
		return 0;
	}

	if (!gpufreq_power_ctrl_enable()) {
		mtk_common_pm_mfg_active();
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
		ged_dvfs_gpu_clock_switch_notify(1);
#endif
		return 0;
	}
#endif /* CONFIG_MTK_GPUFREQ_V2 */

	if (mtk_common_pm_is_mfg_active())
		return 0;

	dev_vdbg(kbdev->dev, "GPU PM Callback - Active");

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_1);

	/* on,off/ SWCG(BG3D)/ MTCMOS/ BUCK */
#if defined(CONFIG_MTK_GPUFREQ_V2)
	if (gpufreq_power_control(POWER_ON) < 0) {
		dev_info(kbdev->dev, "GPU PM Callback - Power On Failed");
		return 1;
	}
#endif /* CONFIG_MTK_GPUFREQ_V2 */

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_2);

#if defined(CONFIG_MTK_GPUFREQ_V2)
	gpufreq_set_timestamp();
#endif /* CONFIG_MTK_GPUFREQ_V2 */

	/* set a flag to enable GPU DVFS */
	mtk_common_pm_mfg_active();

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_3);

	/* resume frequency */
	mtk_common_gpufreq_commit(g_cur_opp_idx);

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_4);

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	ged_dvfs_gpu_clock_switch_notify(1);
#endif

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_5);

	return 0;
}

static void pm_callback_power_off_nolock(struct kbase_device *kbdev)
{
#if defined(CONFIG_MTK_GPUFREQ_V2)
	if (mtk_common_gpufreq_bringup())
		return;

	if (!gpufreq_power_ctrl_enable())
		return;
#endif /* CONFIG_MTK_GPUFREQ_V2 */

	if (!mtk_common_pm_is_mfg_active())
		return;

	dev_vdbg(kbdev->dev, "GPU PM Callback - Idle");

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_6);

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	ged_dvfs_gpu_clock_switch_notify(0);
#endif

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_7);

	/* set a flag to disable GPU DVFS */
	mtk_common_pm_mfg_idle();

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_8);

	/* suspend frequency */
	g_cur_opp_idx = mtk_common_ged_dvfs_get_last_commit_idx();

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_9);

	/* check MFG bus if idle */
#if defined(CONFIG_MTK_GPUFREQ_V2)
	gpufreq_check_bus_idle();
#endif /* CONFIG_MTK_GPUFREQ_V2 */

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_A);

	/* on,off/ SWCG(BG3D)/ MTCMOS/ BUCK */
#if defined(CONFIG_MTK_GPUFREQ_V2)
	if (gpufreq_power_control(POWER_OFF) < 0) {
		dev_info(kbdev->dev, "GPU PM Callback - Power Off Failed");
		return;
	}
#endif /* CONFIG_MTK_GPUFREQ_V2 */

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_B);
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	int ret = 0;

	mutex_lock(&g_mfg_lock);
	ret = pm_callback_power_on_nolock(kbdev);
#if IS_ENABLED(CONFIG_MTK_GPU_SWPM_SUPPORT)
	MTKGPUPower_model_resume();
#endif
	mtk_notify_gpu_power_change(1);
	mutex_unlock(&g_mfg_lock);

	return ret;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	mutex_lock(&g_mfg_lock);
	mtk_notify_gpu_power_change(0);
#if IS_ENABLED(CONFIG_MTK_GPU_SWPM_SUPPORT)
	MTKGPUPower_model_suspend();
#endif
	pm_callback_power_off_nolock(kbdev);
	mutex_unlock(&g_mfg_lock);
}

static void pm_callback_power_suspend(struct kbase_device *kbdev)
{
	mutex_lock(&g_mfg_lock);
	dev_vdbg(kbdev->dev, "GPU PM Callback - Suspend");
	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_E);
	mutex_unlock(&g_mfg_lock);
}

static void pm_callback_power_resume(struct kbase_device *kbdev)
{
	mutex_lock(&g_mfg_lock);
	dev_vdbg(kbdev->dev, "GPU PM Callback - Resume");
	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_F);
	mutex_unlock(&g_mfg_lock);
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = pm_callback_power_suspend,
	.power_resume_callback = pm_callback_power_resume,
};

#ifndef CONFIG_OF
static struct kbase_io_resources io_resources = {
	.job_irq_number = 68,
	.mmu_irq_number = 69,
	.gpu_irq_number = 70,
	.io_memory_region = {
	.start = 0xFC010000,
	.end = 0xFC010000 + (4096 * 4) - 1
	}
};
#endif /* CONFIG_OF */

static struct kbase_platform_config versatile_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &versatile_platform_config;
}

int mtk_platform_device_init(struct kbase_device *kbdev)
{

	if (!kbdev) {
		dev_info(kbdev->dev, "@%s: kbdev is NULL", __func__);
		return -1;
	}

	gpu_dvfs_status_reset_footprint();
	dev_info(kbdev->dev, "GPU PM Callback - Initialize Done");

	return 0;
}

void mtk_platform_device_term(struct kbase_device *kbdev) { }
