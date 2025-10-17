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
#if IS_ENABLED(CONFIG_MTK_GPU_SWPM_SUPPORT)
#include "platform/mtk_platform_common/mtk_ltr_pmu.h"
#endif
#if IS_ENABLED(CONFIG_MALI_MTK_AUTOSUSPEND_DELAY) || IS_ENABLED(CONFIG_MALI_MTK_ADAPTIVE_POWER_POLICY)
#include <ged_kpi.h>
#include <ged_notify_sw_vsync.h>
#endif
#include <mtk_gpufreq.h>
#include <mtk_gpu_utility.h>
#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
#include <mboot_params.h>
#endif /* CONFIG_MTK_AEE_IPANIC */

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && \
    IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY) && \
    IS_ENABLED(CONFIG_MALI_MTK_GPU_DVFS_HINT_26M_LOADING)
#include <platform/mtk_platform_common/mtk_platform_dvfs_hint_26m_perf_cnting.h>
#include "platform/mtk_platform_common/mtk_platform_dvfs_hint_26m_perf_cnting_ex.h"
#define TOP_BASE        (0x13FBF000)
#define DVFS_TOP_BASE       (0x13FBB000)
#endif /* CONFIG_MALI_MIDGARD_DVFS && CONFIG_MALI_MTK_DVFS_POLICY && CONFIG_MALI_MTK_GPU_DVFS_HINT_26M_LOADING*/

/* KBASE_PLATFORM_DEBUG_ENABLE, 1 for debug log enable, 0 for disable */
#define KBASE_PLATFORM_DEBUG_ENABLE  (0)

/* KBASE_PLATFORM_SUSPEND_DELAY, the ms for autosuspend timeout */
#define KBASE_PLATFORM_SUSPEND_DELAY (100) /* ms */
#if IS_ENABLED(CONFIG_MALI_MTK_AUTOSUSPEND_DELAY)
#define KBASE_PLATFORM_MAX_SUSPEND_DELAY (25) /* ms */
#define KBASE_PLATFORM_MIN_SUSPEND_DELAY (10) /* ms */
#endif
#if IS_ENABLED(CONFIG_MALI_MTK_AUTOSUSPEND_DELAY) || IS_ENABLED(CONFIG_MALI_MTK_ADAPTIVE_POWER_POLICY)
static int gInit_autosuspend_delay_ms = KBASE_PLATFORM_SUSPEND_DELAY;
static int gAutosuspend_delay_ms = 0;
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_ACP_DSU_REQ)
static int gIsDsuRequested = 0;
spinlock_t g_dsu_request_lock;
#endif /* CONFIG_MALI_MTK_ACP_DSU_REQ */

DEFINE_MUTEX(g_mfg_lock);

//FIXME
static int g_curFreqID;
static int g_is_suspend;

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
        return 1;
    }

    if (!gpufreq_power_ctrl_enable()) {
        mtk_common_pm_mfg_active();
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
        ged_dvfs_gpu_clock_switch_notify(GED_POWER_ON);
#endif
        return 1;
    }
#else /* CONFIG_MTK_GPUFREQ_V2 */
    if (mtk_common_gpufreq_bringup()) {
        mtk_common_pm_mfg_active();
        return 1;
    }

    if (!mt_gpufreq_power_ctl_en()) {
        mtk_common_pm_mfg_active();
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
        ged_dvfs_gpu_clock_switch_notify(GED_POWER_ON);
#endif
        return 1;
    }
#endif /* CONFIG_MTK_GPUFREQ_V2 */

    if (mtk_common_pm_is_mfg_active())
        return 0;

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_1);

    if (g_is_suspend == 1) {
        KBASE_PLATFORM_LOGI("%s, discard powering on since GPU is suspended", __func__);
        return 0;
    }

    /* on,off/ SWCG(BG3D)/ MTCMOS/ BUCK */
#if defined(CONFIG_MTK_GPUFREQ_V2)
    if (gpufreq_power_control(GPU_PWR_ON) < 0) {
        KBASE_PLATFORM_LOGI("%s, GPU fail to power on", __func__);
        return 0;
    }
#else
    mt_gpufreq_power_control(GPU_PWR_ON, CG_ON, MTCMOS_ON, BUCK_ON);
#endif /* CONFIG_MTK_GPUFREQ_V2 */

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_2);

#if defined(CONFIG_MTK_GPUFREQ_V2)
    // gpufreq_set_ocl_timestamp(); // k61: remove to power on flow
#else
    mt_gpufreq_set_timestamp();
#endif /* CONFIG_MTK_GPUFREQ_V2 */

    /* set a flag to enable GPU DVFS */
    mtk_common_pm_mfg_active();

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_3);

    /* resume frequency */
    mtk_common_gpufreq_commit(g_curFreqID);

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_4);

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
    ged_dvfs_gpu_clock_switch_notify(GED_POWER_ON);
#endif

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_5);

    return 1;
}

static void pm_callback_power_off_nolock(struct kbase_device *kbdev)
{
#if defined(CONFIG_MTK_GPUFREQ_V2)
    if (mtk_common_gpufreq_bringup())
        return;

    if (!gpufreq_power_ctrl_enable())
        return;
#else
    if (mtk_common_gpufreq_bringup())
        return;

    if (!mt_gpufreq_power_ctl_en())
        return;
#endif /* CONFIG_MTK_GPUFREQ_V2 */

    if (!mtk_common_pm_is_mfg_active())
        return;

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_6);

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
    ged_dvfs_gpu_clock_switch_notify(0);
#endif

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_7);

    /* set a flag to disable GPU DVFS */
    mtk_common_pm_mfg_idle();

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_8);

    /* suspend frequency */
    g_curFreqID = mtk_common_ged_dvfs_get_last_commit_idx();

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_9);

    /* check MFG bus if idle */
#if defined(CONFIG_MTK_GPUFREQ_V2)
    // gpufreq_check_bus_idle(); // k61: remove to power off flow
#else
    mt_gpufreq_check_bus_idle();
#endif /* CONFIG_MTK_GPUFREQ_V2 */

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_A);

    /* on,off/ SWCG(BG3D)/ MTCMOS/ BUCK */
#if defined(CONFIG_MTK_GPUFREQ_V2)
    if (gpufreq_power_control(GPU_PWR_OFF) < 0) {
        KBASE_PLATFORM_LOGE("Power Off Failed");
        return;
    }
#else
    mt_gpufreq_power_control(GPU_PWR_OFF, CG_OFF, MTCMOS_OFF, BUCK_OFF);
#endif /* CONFIG_MTK_GPUFREQ_V2 */

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_B);
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
    int ret = 1;

    mutex_lock(&g_mfg_lock);
    ret = pm_callback_power_on_nolock(kbdev);
    mtk_notify_gpu_power_change(1);
#if IS_ENABLED(CONFIG_MTK_GPU_SWPM_SUPPORT)
    MTK_LTR_gpu_pmu_resume();
#endif
    mutex_unlock(&g_mfg_lock);

    return ret;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
    mutex_lock(&g_mfg_lock);
    mtk_notify_gpu_power_change(0);
#if IS_ENABLED(CONFIG_MTK_GPU_SWPM_SUPPORT)
    MTK_LTR_gpu_pmu_suspend();
#endif
    pm_callback_power_off_nolock(kbdev);
    mutex_unlock(&g_mfg_lock);
}

static void pm_callback_power_suspend(struct kbase_device *kbdev)
{
    mutex_lock(&g_mfg_lock);

    if (mtk_common_pm_is_mfg_active()) {
        pm_callback_power_off_nolock(kbdev);
        KBASE_PLATFORM_LOGI("%s, force powering off GPU", __func__);
    }
    g_is_suspend = 1;
    KBASE_PLATFORM_LOGI("%s, GPU suspend", __func__);

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_E);

    mutex_unlock(&g_mfg_lock);
}

static void pm_callback_power_resume(struct kbase_device *kbdev)
{
    mutex_lock(&g_mfg_lock);

    g_is_suspend = 0;
    KBASE_PLATFORM_LOGI("%s, GPU resume", __func__);

    gpu_dvfs_status_footprint(GPU_DVFS_STATUS_STEP_F);

    mutex_unlock(&g_mfg_lock);
}

static void pm_callback_runtime_gpu_active(struct kbase_device *kbdev)
{
    unsigned long flags;
    int error;
#if IS_ENABLED(CONFIG_MALI_MTK_AUTOSUSPEND_DELAY)
    int target_fps = 0;
    int temp_autosuspend_delay_ms = 0;
#endif

    dev_dbg(kbdev->dev, "%s\n", __func__);

    lockdep_assert_held(&kbdev->pm.lock);

    mtk_common_ged_dvfs_write_sysram_last_commit_top_idx();
    mtk_common_ged_dvfs_write_sysram_last_commit_dual();

    spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
    WARN_ON(!kbdev->pm.backend.gpu_powered);
    WARN_ON(!kbdev->pm.active_count);
#if MALI_USE_CSF
    WARN_ON(kbdev->pm.runtime_active);
#endif
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
#if MALI_USE_CSF
    kbdev->pm.runtime_active = true;
#endif
#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
    ged_dvfs_gpu_clock_switch_notify(GED_POWER_ON);
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_AUTOSUSPEND_DELAY)
if (ged_gpu_apo_support() == APO_NORMAL_SUPPORT) {
    target_fps = ged_kpi_get_target_fps();

    if (target_fps < 0 || target_fps > 120)
        target_fps = ged_kpi_get_panel_refresh_rate();

    if (target_fps > 0 && target_fps <= 120) {
        if (target_fps >= 90)
            temp_autosuspend_delay_ms = KBASE_PLATFORM_MAX_SUSPEND_DELAY;
        else
            temp_autosuspend_delay_ms = KBASE_PLATFORM_MIN_SUSPEND_DELAY;
    } else
        temp_autosuspend_delay_ms = gInit_autosuspend_delay_ms;

    if (gAutosuspend_delay_ms != temp_autosuspend_delay_ms) {
        ged_gpu_autosuspend_timeout_notify(temp_autosuspend_delay_ms);
        pm_runtime_set_autosuspend_delay(kbdev->dev, temp_autosuspend_delay_ms);
        gAutosuspend_delay_ms = temp_autosuspend_delay_ms;
    }
}
#endif
}

static void pm_callback_runtime_gpu_idle(struct kbase_device *kbdev)
{
    unsigned long flags;
#if IS_ENABLED(CONFIG_MALI_MTK_ADAPTIVE_POWER_POLICY)
    int temp_autosuspend_delay_ms = 0;
#endif

    dev_dbg(kbdev->dev, "%s", __func__);

    lockdep_assert_held(&kbdev->pm.lock);

    mtk_common_ged_dvfs_write_sysram_last_commit_top_idx();
    mtk_common_ged_dvfs_write_sysram_last_commit_dual();

#if IS_ENABLED(CONFIG_MALI_MTK_ADAPTIVE_POWER_POLICY)
if (ged_gpu_apo_support() == APO_2_0_NORMAL_SUPPORT) {
    temp_autosuspend_delay_ms = kbdev->dev->power.autosuspend_delay;
    if (gAutosuspend_delay_ms != temp_autosuspend_delay_ms) {
        gAutosuspend_delay_ms = temp_autosuspend_delay_ms;
        ged_gpu_autosuspend_timeout_notify(temp_autosuspend_delay_ms);
        pm_runtime_set_autosuspend_delay(kbdev->dev, gAutosuspend_delay_ms);
    }
}
#endif /* CONFIG_MALI_MTK_ADAPTIVE_POWER_POLICY */

#if IS_ENABLED(CONFIG_MALI_MIDGARD_DVFS) && IS_ENABLED(CONFIG_MALI_MTK_DVFS_POLICY)
    ged_dvfs_gpu_clock_switch_notify(GED_SLEEP);
#endif

    spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
    WARN_ON(!kbdev->pm.backend.gpu_powered);
    WARN_ON(kbdev->pm.backend.l2_state != KBASE_L2_OFF);
    WARN_ON(kbdev->pm.active_count);
#if MALI_USE_CSF
    WARN_ON(!kbdev->pm.runtime_active);
#endif
    spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

    pm_runtime_mark_last_busy(kbdev->dev);
    pm_runtime_put_autosuspend(kbdev->dev);
#if MALI_USE_CSF
    kbdev->pm.runtime_active = false;
#endif
}

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


static int kbase_device_runtime_init(struct kbase_device *kbdev)
{
#if IS_ENABLED(CONFIG_MALI_MTK_AUTOSUSPEND_DELAY)
    struct device_node *np = kbdev->dev->of_node;
#endif
    int ret = 0;

    dev_dbg(kbdev->dev, "%s\n", __func__);

#if IS_ENABLED(CONFIG_MALI_MTK_AUTOSUSPEND_DELAY)
    if (!of_property_read_u32(np, "autosuspend-delay-ms", &gInit_autosuspend_delay_ms))
        dev_info(kbdev->dev, "AutoSuspend Delay: %dms", gInit_autosuspend_delay_ms);
    else {
        dev_info(kbdev->dev, "AutoSuspend Delay: No dts property setting, default %dms",
            gInit_autosuspend_delay_ms);
    }

    pm_runtime_set_autosuspend_delay(kbdev->dev, gInit_autosuspend_delay_ms);
    gAutosuspend_delay_ms = gInit_autosuspend_delay_ms;
#else
    pm_runtime_set_autosuspend_delay(kbdev->dev, KBASE_PLATFORM_SUSPEND_DELAY);
#endif

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

    dev_info(kbdev->dev, "GPU PM Callback - Initialize Done");

    return 0;
}

void mtk_platform_pm_term(struct kbase_device *kbdev)
{
    if (IS_ERR_OR_NULL(kbdev))
        return;
}
