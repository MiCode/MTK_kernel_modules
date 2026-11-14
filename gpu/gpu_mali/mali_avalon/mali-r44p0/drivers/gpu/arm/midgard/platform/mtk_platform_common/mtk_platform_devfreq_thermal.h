// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __MTK_GPU_IPA_H__
#define __MTK_GPU_IPA_H__

#include <linux/devfreq.h>
#include <linux/devfreq_cooling.h>

extern struct devfreq_cooling_power mtk_devfreq_cooling_power_ops;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
int mtk_devfreq_thermal_get_real_power(struct devfreq *df,
                                       u32 *power,
                                       unsigned long freq /* Hz */,
                                       unsigned long voltage /* mV */);

#else

unsigned long mtk_devfreq_thermal_get_static_power(struct devfreq *devfreq,
                                                   unsigned long voltage_mv);
unsigned long mtk_devfreq_thermal_get_dynamic_power(struct devfreq *devfreq,
                                                    unsigned long freqHz,
                                                    unsigned long voltage_mv);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0) */

#endif /* __MTK_GPU_IPA_H__ */
