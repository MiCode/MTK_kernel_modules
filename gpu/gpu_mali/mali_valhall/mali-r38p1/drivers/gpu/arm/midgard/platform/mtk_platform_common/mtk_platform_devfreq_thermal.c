// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <platform/mtk_platform_common.h>
#include <platform/mtk_platform_common/mtk_platform_devfreq_thermal.h>
#include <mtk_gpufreq.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
int mtk_devfreq_thermal_get_real_power(struct devfreq *df,
										  u32 *power,
										  unsigned long freq /* Hz */,
										  unsigned long voltage /* mV */)
{
#if defined(CONFIG_MTK_GPUFREQ_V2)
	return mtk_common_gpufreq_bringup() ?
		0 : (gpufreq_get_leakage_power(TARGET_DEFAULT, voltage * 100) +
		gpufreq_get_dynamic_power(TARGET_DEFAULT, freq / 1000, voltage * 100));
#else
	(void)(voltage);
	return mtk_common_gpufreq_bringup() ?
		0 : (mt_gpufreq_get_leakage_mw() +
		mt_gpufreq_get_dyn_power(freq / 1000, voltage * 100));
#endif /* CONFIG_MTK_GPUFREQ_V2 */
}

struct devfreq_cooling_power mtk_devfreq_cooling_power_ops = {
	.get_real_power = &mtk_devfreq_thermal_get_real_power,
};

#else

unsigned long mtk_devfreq_thermal_get_static_power(struct devfreq *df,
                                                          unsigned long voltage /* mV */)
{
#if defined(CONFIG_MTK_GPUFREQ_V2)
	return mtk_common_gpufreq_bringup() ?
		0 : gpufreq_get_leakage_power(TARGET_DEFAULT, voltage * 100);
#else
	(void)(voltage);
	return mtk_common_gpufreq_bringup() ?
		0 : mt_gpufreq_get_leakage_mw();
#endif /* CONFIG_MTK_GPUFREQ_V2 */
}

unsigned long mtk_devfreq_thermal_get_dynamic_power(struct devfreq *df,
                                                           unsigned long freq /* Hz */,
                                                           unsigned long voltage /* mV */)
{
#if defined(CONFIG_MTK_GPUFREQ_V2)
	return mtk_common_gpufreq_bringup() ?
		0 : gpufreq_get_dynamic_power(TARGET_DEFAULT,
		freq / 1000, voltage * 100);
#else
	return mtk_common_gpufreq_bringup() ?
		0 : mt_gpufreq_get_dyn_power(freq / 1000, voltage * 100);
#endif /* CONFIG_MTK_GPUFREQ_V2 */
}

struct devfreq_cooling_power mtk_devfreq_cooling_power_ops = {
	.get_static_power  = &mtk_devfreq_thermal_get_static_power,
	.get_dynamic_power = &mtk_devfreq_thermal_get_dynamic_power,
};
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0) */
