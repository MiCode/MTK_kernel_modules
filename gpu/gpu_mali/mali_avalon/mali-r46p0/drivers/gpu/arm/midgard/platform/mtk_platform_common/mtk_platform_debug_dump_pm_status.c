// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include "mtk_platform_logbuffer.h"
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

static void pm_status_print(struct kbase_device *kbdev, const char *fmt, ...)
{
	va_list args;
	uint8_t buffer[MTK_LOG_BUFFER_ENTRY_SIZE];

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	dev_info(kbdev->dev, "%s", buffer);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "%s", buffer);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
}

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
static const char *mtk_debug_mcu_state_to_string(enum kbase_mcu_state state)
{
	const char *const strings[] = {
#define KBASEP_MCU_STATE(n) #n,
#include "mali_kbase_pm_mcu_states.h"
#undef KBASEP_MCU_STATE
	};
	if ((size_t)state >= ARRAY_SIZE(strings))
		return "Bad MCU state";
	else
		return strings[state];
}
#else
static const char *mtk_debug_core_state_to_string(enum kbase_shader_core_state state)
{
	const char *const strings[] = {
#define KBASEP_SHADER_STATE(n) #n,
#include "mali_kbase_pm_shader_states.h"
#undef KBASEP_SHADER_STATE
	};
	if ((size_t)state >= ARRAY_SIZE(strings))
		return "Bad shader core state";
	else
		return strings[state];
}
#endif

static const char *mtk_debug_l2_core_state_to_string(enum kbase_l2_core_state state)
{
	const char *const strings[] = {
#define KBASEP_L2_STATE(n) #n,
#include "mali_kbase_pm_l2_states.h"
#undef KBASEP_L2_STATE
	};
	if ((size_t)state >= ARRAY_SIZE(strings))
		return "Bad level 2 cache state";
	else
		return strings[state];
}

void mtk_debug_dump_pm_status(struct kbase_device *kbdev)
{
#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
	pm_status_print(kbdev,
		"[CSF] firmware_inited=%d firmware_reloaded=%d firmware_reload_needed=%d interrupt_received=%d\n",
			 kbdev->csf.firmware_inited,
			 kbdev->csf.firmware_reloaded,
			 kbdev->csf.firmware_reload_needed,
			 kbdev->csf.interrupt_received);
	pm_status_print(kbdev,
		"[CSF] firmware_hctl_core_pwr=%d glb_init_request_pending=%d scheduler.pm_active_count=%d\n",
			 kbdev->csf.firmware_hctl_core_pwr,
			 kbdev->csf.glb_init_request_pending,
			 kbdev->csf.scheduler.pm_active_count);
	pm_status_print(kbdev,
		"[PM] in_reset=%d reset_done=%d gpu_powered=%d gpu_ready=%d mcu_state=%s l2_state=%s mcu_desired=%d l2_desired=%d l2_always_on=%d\n",
			 kbdev->pm.backend.in_reset,
			 kbdev->pm.backend.reset_done,
			 kbdev->pm.backend.gpu_powered,
			 kbdev->pm.backend.gpu_ready,
			 mtk_debug_mcu_state_to_string(kbdev->pm.backend.mcu_state),
			 mtk_debug_l2_core_state_to_string(kbdev->pm.backend.l2_state),
			 kbdev->pm.backend.mcu_desired,
			 kbdev->pm.backend.l2_desired,
			 kbdev->pm.backend.l2_always_on);
	pm_status_print(kbdev,
		"[PM] hwcnt_desired=%d hwcnt_disabled=%d poweroff_wait_in_progress=%d invoke_poweroff_wait_wq_when_l2_off=%d poweron_required=%d\n",
			 kbdev->pm.backend.hwcnt_desired,
			 kbdev->pm.backend.hwcnt_disabled,
			 kbdev->pm.backend.poweroff_wait_in_progress,
			 kbdev->pm.backend.invoke_poweroff_wait_wq_when_l2_off,
			 kbdev->pm.backend.poweron_required);
#else /* CONFIG_MALI_CSF_SUPPORT */
	pm_status_print(kbdev,
		"[PM] in_reset=%d reset_done=%d gpu_powered=%d gpu_ready=%d shaders_state=%s l2_state=%s shaders_desired=%d l2_desired=%d l2_always_on=%d",
			 kbdev->pm.backend.in_reset,
			 kbdev->pm.backend.reset_done,
			 kbdev->pm.backend.gpu_powered,
			 kbdev->pm.backend.gpu_ready,
			 mtk_debug_core_state_to_string(kbdev->pm.backend.shaders_state),
			 mtk_debug_l2_core_state_to_string(kbdev->pm.backend.l2_state),
			 kbdev->pm.backend.shaders_desired,
			 kbdev->pm.backend.l2_desired,
			 kbdev->pm.backend.l2_always_on);
	pm_status_print(kbdev,
		"[PM] hwcnt_desired=%d hwcnt_disabled=%d poweroff_wait_in_progress=%d invoke_poweroff_wait_wq_when_l2_off=%d poweron_required=%d",
			 kbdev->pm.backend.hwcnt_desired,
			 kbdev->pm.backend.hwcnt_disabled,
			 kbdev->pm.backend.poweroff_wait_in_progress,
			 kbdev->pm.backend.invoke_poweroff_wait_wq_when_l2_off,
			 kbdev->pm.backend.poweron_required);
#endif /* CONFIG_MALI_CSF_SUPPORT */
}
