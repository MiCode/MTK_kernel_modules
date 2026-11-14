// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_LTR_GPU_PMU_H__
#define __MTK_LTR_GPU_PMU_H__
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

enum {
	gpm_off,
	gpm_kernel_side,
	gpm_sspm_side
};

void MTK_LTR_gpu_pmu_stop(void);
void MTK_LTR_gpu_pmu_start(unsigned int interval_ns);
void MTK_LTR_gpu_pmu_start_swpm(unsigned int interval_ns);
void MTK_LTR_gpu_pmu_suspend(void);
void MTK_LTR_gpu_pmu_resume(void);
int MTK_LTR_gpu_pmu_init(void);
void MTK_LTR_gpu_pmu_destroy(void);

extern void (*mtk_ltr_gpu_pmu_start_fp)(unsigned int interval_ns);
extern void (*mtk_ltr_gpu_pmu_stop_fp)(void);

#endif

