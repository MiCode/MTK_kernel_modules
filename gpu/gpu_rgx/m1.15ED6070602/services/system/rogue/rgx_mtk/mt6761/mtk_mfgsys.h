/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc.
 */

#ifndef MTK_MFGSYS_H
#define MTK_MFGSYS_H

#include "servicesext.h"
#include "rgxdevice.h"
#include "ged_dvfs.h"

/* Control SW APM is enabled or not  */
#ifndef MTK_BRINGUP
#define MTK_PM_SUPPORT 1
#define MTCMOS_CONTROL 1
#else
#define MTK_PM_SUPPORT 0
#endif

void mtk_fdvfs_update_cur_freq(int ui32GPUFreq);
PVRSRV_ERROR MTKMFGSystemInit(void);
void MTKMFGSystemDeInit(void);
void MTKFWDump(void);

/* below register interface in RGX sysconfig.c */
PVRSRV_ERROR MTKSysPrePowerState(IMG_HANDLE hSysData,
				PVRSRV_SYS_POWER_STATE eNewPowerState,
				PVRSRV_SYS_POWER_STATE eCurrentPowerState,
				IMG_BOOL bForced);

PVRSRV_ERROR MTKSysPostPowerState(IMG_HANDLE hSysData,
				PVRSRV_SYS_POWER_STATE eNewPowerState,
				PVRSRV_SYS_POWER_STATE eCurrentPowerState,
				IMG_BOOL bForced);

PVRSRV_ERROR MTKSystemPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState);

PVRSRV_ERROR MTKSystemPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState);

int MTKRGXDeviceInit(PVRSRV_DEVICE_CONFIG *psDevConfig);
int MTKRGXDeviceDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig);

extern int spm_mtcmos_ctrl_mfg0(int state);
extern int spm_mtcmos_ctrl_mfg1(int state);
extern int spm_mtcmos_ctrl_mfg2(int state);
extern int spm_mtcmos_ctrl_mfg3(int state);

extern void switch_mfg_clk(int src);
extern int mtcmos_mfg_series_on(void);

extern unsigned int mtk_notify_sspm_fdvfs_gpu_pow_status(unsigned int enable);
extern void mfgsys_mtcmos_check(void);
extern unsigned int mfgsys_cg_check(void);

extern void ged_log_trace_counter(char *name, int count);

/* from gpu/ged/src/ged_dvfs.c */
extern void (*ged_dvfs_cal_gpu_utilization_fp)(unsigned int *pui32Loading,
					unsigned int *pui32Block,
					unsigned int *pui32Idle);
extern void (*ged_dvfs_gpu_freq_commit_fp)(unsigned long ui32NewFreqID,
					GED_DVFS_COMMIT_TYPE eCommitType,
					int *pbCommited);

#ifdef CONFIG_MTK_HIBERNATION
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
int gpu_pm_restore_noirq(struct device *device);
#endif

typedef void (*gpufreq_input_boost_notify)(unsigned int);
typedef void (*gpufreq_power_limit_notify)(unsigned int);

#define INPUT_BOOST_CB mt_gpufreq_input_boost_notify_registerCB
#define POWER_LIMIT_CB mt_gpufreq_power_limit_notify_registerCB
extern void INPUT_BOOST_CB(gpufreq_input_boost_notify pCB);
extern void POWER_LIMIT_CB(gpufreq_power_limit_notify pCB);

extern unsigned int (*mtk_get_gpu_loading_fp)(void);
extern unsigned int (*mtk_get_gpu_block_fp)(void);
extern unsigned int (*mtk_get_gpu_idle_fp)(void);
extern unsigned int (*mtk_get_gpu_power_loading_fp)(void);
extern void (*mtk_enable_gpu_dvfs_timer_fp)(bool bEnable);
extern void (*mtk_boost_gpu_freq_fp)(void);
extern void (*mtk_set_bottom_gpu_freq_fp)(unsigned int ui32FreqLevel);

extern unsigned int (*mtk_custom_get_gpu_freq_level_count_fp)(void);
extern void (*mtk_custom_boost_gpu_freq_fp)(unsigned int ui32FreqLevel);
extern void (*mtk_custom_upbound_gpu_freq_fp)(unsigned int ui32FreqLevel);
extern unsigned int (*mtk_get_custom_boost_gpu_freq_fp)(void);
extern unsigned int (*mtk_get_custom_upbound_gpu_freq_fp)(void);

extern int* (*mtk_get_gpu_cur_owner_fp)(void);

#ifdef SUPPORT_PDVFS
extern unsigned int mt_gpufreq_get_volt_by_idx(unsigned int idx);
#endif

#if defined(MODULE)
int mtk_mfg_async_init(void);
int mtk_mfg_2d_init(void);
#endif

#endif

