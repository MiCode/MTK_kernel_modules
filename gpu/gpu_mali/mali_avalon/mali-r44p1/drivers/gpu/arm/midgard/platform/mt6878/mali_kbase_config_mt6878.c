// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/pm_qos.h>
#include <linux/pm_runtime.h>
#include <linux/arm-smccc.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <device/mali_kbase_device.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include "mali_kbase_config_platform.h"
#include <platform/mtk_platform_utils.h>
#include <platform/mtk_platform_common.h>
#include <platform/mtk_platform_common/mtk_platform_debug.h>
#include <ged_dvfs.h>
#include <mtk_gpufreq.h>
#include <mtk_gpu_utility.h>
#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
#include <mboot_params.h>
#endif
#include <asm/ptrace.h>

/* KBASE_PLATFORM_DEBUG_ENABLE, 1 for debug log enable, 0 for disable */
#define KBASE_PLATFORM_DEBUG_ENABLE  (0)

/* KBASE_PLATFORM_SUSPEND_DELAY, the ms for autosuspend timeout */
#define KBASE_PLATFORM_SUSPEND_DELAY (100) /* ms */

#if IS_ENABLED(CONFIG_MALI_MTK_ACP_DSU_REQ) || IS_ENABLED(CONFIG_MALI_MTK_ACP_DSU_REQ_LEGACY)
static int gIsDsuRequested = 0;
spinlock_t g_dsu_request_lock;
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
#define MTK_DVFS_STEP_LOW (3)
#define MTK_DVFS_STEP_MED (3)
#define MTK_DVFS_STEP_HIGH (3)
#endif /* CONFIG_MALI_MTK_DVFS_POLICY */

DEFINE_MUTEX(g_mfg_lock);

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
	dev_dbg(kbdev->dev, "%s\n", __func__);

	if (mtk_common_gpufreq_bringup()) {
		mtk_common_pm_mfg_active();
		return 0;
	}

	if (!gpufreq_power_ctrl_enable()) {
		mtk_common_pm_mfg_active();
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
		ged_dvfs_gpu_clock_switch_notify(GED_POWER_ON);
#endif
		return 0;
	}

	if (mtk_common_pm_is_mfg_active())
		return 0;

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_1);

	/* update required frequency from GED to sysram */
	mtk_common_ged_dvfs_write_sysram_last_commit_top_idx();
	mtk_common_ged_dvfs_write_sysram_last_commit_stack_idx();

	/* on,off/ SWCG(BG3D)/ MTCMOS/ BUCK */
	if (gpufreq_power_control(GPU_PWR_ON) < 0) {
		KBASE_PLATFORM_LOGE("Power On Failed");
		return 1;
	}

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_2);

	/* set a flag to enable GPU DVFS */
	mtk_common_pm_mfg_active();

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_3);

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	ged_dvfs_gpu_clock_switch_notify(GED_POWER_ON);
#endif

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_4);

	return 0;
}

static void pm_callback_power_off_nolock(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "%s\n", __func__);

	if (mtk_common_gpufreq_bringup())
		return;

	if (!gpufreq_power_ctrl_enable())
		return;

	if (!mtk_common_pm_is_mfg_active())
		return;

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_5);

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	ged_dvfs_gpu_clock_switch_notify(GED_POWER_OFF);
#endif

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_6);

	/* set a flag to disable GPU DVFS */
	mtk_common_pm_mfg_idle();

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_7);

	/* on,off/ SWCG(BG3D)/ MTCMOS/ BUCK */
	if (gpufreq_power_control(GPU_PWR_OFF) < 0) {
		KBASE_PLATFORM_LOGE("Power Off Failed");
		return;
	}

	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_8);
}


static int pm_callback_power_on(struct kbase_device *kbdev)
{
	int ret = 1;
	unsigned long flags;

	dev_dbg(kbdev->dev, "%s %pK\n", __func__, (void *)kbdev->dev->pm_domain);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	WARN_ON(kbdev->pm.backend.gpu_powered);

	if (likely(kbdev->csf.firmware_inited)) {
		WARN_ON(!kbdev->pm.active_count);
		WARN_ON(kbdev->pm.runtime_active);
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

#if IS_ENABLED(CONFIG_MALI_MTK_ACP_DSU_REQ_LEGACY)
	mtk_platform_cpu_cache_request(kbdev, REQ_DSU_POWER_ON);
#endif

	mutex_lock(&g_mfg_lock);
	ret = pm_callback_power_on_nolock(kbdev);
	mtk_notify_gpu_power_change(1);
	mutex_unlock(&g_mfg_lock);

	return ret;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	unsigned long flags;
	struct arm_smccc_res res;

	dev_dbg(kbdev->dev, "%s\n", __func__);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	WARN_ON(kbdev->pm.backend.gpu_powered);

	if (likely(kbdev->csf.firmware_inited)) {
		WARN_ON(kbase_csf_scheduler_get_nr_active_csgs(kbdev));
		WARN_ON(kbdev->pm.backend.mcu_state != KBASE_MCU_OFF);
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	/* Power down the GPU immediately */
	mutex_lock(&g_mfg_lock);
	mtk_notify_gpu_power_change(0);
	pm_callback_power_off_nolock(kbdev);
	mutex_unlock(&g_mfg_lock);
#if IS_ENABLED(CONFIG_MALI_MTK_ACP_DSU_REQ_LEGACY)
	mtk_platform_cpu_cache_request(kbdev, REQ_DSU_POWER_OFF);
#endif
}

static void pm_callback_runtime_gpu_active(struct kbase_device *kbdev)
{
	unsigned long flags;
	int error;
	dev_dbg(kbdev->dev, "%s\n", __func__);

	lockdep_assert_held(&kbdev->pm.lock);

	mtk_common_ged_dvfs_write_sysram_last_commit_top_idx();
	mtk_common_ged_dvfs_write_sysram_last_commit_stack_idx();

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	WARN_ON(!kbdev->pm.backend.gpu_powered);
	WARN_ON(!kbdev->pm.active_count);
	WARN_ON(kbdev->pm.runtime_active);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	if (pm_runtime_status_suspended(kbdev->dev)) {
		error = pm_runtime_get_sync(kbdev->dev);
		KBASE_PLATFORM_LOGD("pm_runtime_get_sync returned %d", error);
	} else {
		/* Call the async version here, otherwise there could be
		 * a deadlock if the runtime suspend operation is ongoing.
		 * Caller would have taken the kbdev->pm.lock and/or the
		 * scheduler lock, and the runtime suspend callback function
		 * will also try to acquire the same lock(s).
		 */
		error = pm_runtime_get(kbdev->dev);
		KBASE_PLATFORM_LOGD("pm_runtime_get returned %d", error);
	}

	kbdev->pm.runtime_active = true;

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	ged_dvfs_gpu_clock_switch_notify(GED_POWER_ON);
#endif

}

static void pm_callback_runtime_gpu_idle(struct kbase_device *kbdev)
{
	unsigned long flags;

	dev_dbg(kbdev->dev, "%s", __func__);

	lockdep_assert_held(&kbdev->pm.lock);

	mtk_common_ged_dvfs_write_sysram_last_commit_top_idx();
	mtk_common_ged_dvfs_write_sysram_last_commit_stack_idx();

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	ged_dvfs_gpu_clock_switch_notify(GED_SLEEP);
#endif

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	WARN_ON(!kbdev->pm.backend.gpu_powered);
	WARN_ON(kbdev->pm.backend.l2_state != KBASE_L2_OFF);
	WARN_ON(kbdev->pm.active_count);
	WARN_ON(!kbdev->pm.runtime_active);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	pm_runtime_mark_last_busy(kbdev->dev);
	pm_runtime_put_autosuspend(kbdev->dev);
	kbdev->pm.runtime_active = false;
}

static int kbase_device_runtime_init(struct kbase_device *kbdev)
{
	int ret = 0;

	dev_dbg(kbdev->dev, "%s\n", __func__);

	pm_runtime_set_autosuspend_delay(kbdev->dev, KBASE_PLATFORM_SUSPEND_DELAY);
	pm_runtime_use_autosuspend(kbdev->dev);

	pm_runtime_set_active(kbdev->dev);
	pm_runtime_enable(kbdev->dev);

	if (!pm_runtime_enabled(kbdev->dev)) {
		KBASE_PLATFORM_LOGE("pm_runtime not enabled");
		ret = -EINVAL;
	} else if (atomic_read(&kbdev->dev->power.usage_count)) {
		KBASE_PLATFORM_LOGE("%s: Device runtime usage count unexpectedly non zero %d",
			__func__, atomic_read(&kbdev->dev->power.usage_count));
		ret = -EINVAL;
	}

	return ret;
}

static void kbase_device_runtime_disable(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "%s\n", __func__);

	if (atomic_read(&kbdev->dev->power.usage_count))
		KBASE_PLATFORM_LOGE("%s: Device runtime usage count unexpectedly non zero %d",
			__func__, atomic_read(&kbdev->dev->power.usage_count));

	pm_runtime_disable(kbdev->dev);
}

static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "%s\n", __func__);
	return 0;
}

static void pm_callback_runtime_off(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "%s\n", __func__);
}

static void pm_callback_resume(struct kbase_device *kbdev)
{
	KBASE_PLATFORM_LOGI("%s", __func__);
	mutex_lock(&g_mfg_lock);
	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_F);
	mutex_unlock(&g_mfg_lock);
}

static void pm_callback_suspend(struct kbase_device *kbdev)
{
	KBASE_PLATFORM_LOGI("%s", __func__);
	mutex_lock(&g_mfg_lock);
	gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_E);
	mutex_unlock(&g_mfg_lock);
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback = pm_callback_suspend,
	.power_resume_callback = pm_callback_resume,
	.power_runtime_init_callback = NULL,
	.power_runtime_term_callback = NULL,
	.power_runtime_on_callback = NULL,
	.power_runtime_off_callback = NULL,
	.power_runtime_gpu_idle_callback = NULL,
	.power_runtime_gpu_active_callback = NULL,
};

int mtk_platform_pm_init(struct kbase_device *kbdev)
{
	struct device_node *np = kbdev->dev->of_node;
	u32 sleep_mode_enable = 0;

	if (IS_ERR_OR_NULL(kbdev))
		return -1;

#if IS_ENABLED(CONFIG_MALI_MTK_ACP_DSU_REQ) || IS_ENABLED(CONFIG_MALI_MTK_ACP_DSU_REQ_LEGACY)
	spin_lock_init(&g_dsu_request_lock);
#endif /* CONFIG_MALI_MTK_ACP_DSU_REQ || CONFIG_MALI_MTK_ACP_DSU_REQ_LEGACY */

	if (!of_property_read_u32(np, "sleep-mode-enable", &sleep_mode_enable)) {
		dev_info(kbdev->dev, "Sleep mode %s", (sleep_mode_enable)? "enabled": "disabled");

		if (sleep_mode_enable == 1) {
			pm_callbacks.power_runtime_init_callback = kbase_device_runtime_init;
			pm_callbacks.power_runtime_term_callback = kbase_device_runtime_disable;
			pm_callbacks.power_runtime_on_callback = pm_callback_runtime_on;
			pm_callbacks.power_runtime_off_callback = pm_callback_runtime_off;
			pm_callbacks.power_runtime_gpu_idle_callback = pm_callback_runtime_gpu_idle;
			pm_callbacks.power_runtime_gpu_active_callback = pm_callback_runtime_gpu_active;
		}
	} else
		dev_info(kbdev->dev, "Sleep mode: No dts property setting, default disabled");

	gpu_dvfs_status_reset_footprint();

#if IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
	// update dvfs policy step depends on platform
	if (mtk_common_ged_dvfs_update_step_size(
			MTK_DVFS_STEP_LOW, MTK_DVFS_STEP_MED, MTK_DVFS_STEP_HIGH) == 0)
		dev_info(kbdev->dev, "[GPU DVFS] update step size by platform: low(%d) med(%d) high(%d)",
				MTK_DVFS_STEP_LOW, MTK_DVFS_STEP_MED, MTK_DVFS_STEP_HIGH);
	else
		dev_info(kbdev->dev, "[GPU DVFS] update_step_size FAIL");
#endif /* CONFIG_MALI_MTK_DVFS_POLICY */

	dev_info(kbdev->dev, "GPU PM Callback - Initialize Done");

	return 0;
}


void mtk_platform_pm_term(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return;
}
#if IS_ENABLED(CONFIG_MALI_MTK_ACP_DSU_REQ) || IS_ENABLED(CONFIG_MALI_MTK_ACP_DSU_REQ_LEGACY)
void mtk_platform_cpu_cache_request(struct kbase_device *kbdev, int request)
{
	struct arm_smccc_res res;
	unsigned long flags;

	spin_lock_irqsave(&g_dsu_request_lock, flags);
	if (request == REQ_DSU_POWER_ON)
	{
		if (gIsDsuRequested == 0 && (kbdev->gpu_props.props.raw_props.coherency_mode == COHERENCY_ACE_LITE))
		{
			/* Call smc into security mode */
			/* Check result in trusted zone */
			arm_smccc_smc(
				MTK_SIP_KERNEL_GPUEB_CONTROL,  /* a0 */
				GPUACP_SMC_OP_CPUPM_PWR,       /* a1 */
				REQ_DSU_POWER_ON,	       /* a2 */
				0, 0, 0, 0, 0, &res);
			gIsDsuRequested++;
		}
		else
		{
			KBASE_PLATFORM_LOGE("%s Duplicated request to DSU power on\n", __func__);
		}
	}
	else if (request == REQ_DSU_POWER_OFF)
	{
		if (gIsDsuRequested != 0 && (kbdev->gpu_props.props.raw_props.coherency_mode == COHERENCY_ACE_LITE))
		{
			/* Call smc into security mode */
			/* Check result in trusted zone */
			arm_smccc_smc(
				MTK_SIP_KERNEL_GPUEB_CONTROL,  /* a0 */
				GPUACP_SMC_OP_CPUPM_PWR,       /* a1 */
				REQ_DSU_POWER_OFF,	       /* a2 */
				0, 0, 0, 0, 0, &res);
			gIsDsuRequested--;
		}
		else
		{
			KBASE_PLATFORM_LOGE("%s Duplicated request to DSU power off\n", __func__);
		}
	}
	else
		KBASE_PLATFORM_LOGE("%s Unsupported request %d or bad ref cnt: %d\n",
			__func__, request, gIsDsuRequested);
	spin_unlock_irqrestore(&g_dsu_request_lock, flags);
}
#endif
