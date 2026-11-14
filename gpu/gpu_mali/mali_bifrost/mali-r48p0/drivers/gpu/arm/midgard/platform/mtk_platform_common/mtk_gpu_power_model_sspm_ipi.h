// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_GPU_POWER_MODEL_H__
#define __MTK_GPU_POWER_MODEL_H__
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

enum {
	GPU_PM_POWER_STATUE,
	GPU_PM_SWITCH,	// 0 : API 1 : SSPM
	GPU_PM_LAST,
	GPU_PM_LAST2
};

struct gpu_pm_ipi_cmds {
	unsigned int cmd;
	unsigned int power_statue;
};

enum {
       gpm_off,
       gpm_kernel_side,
       gpm_sspm_side
};

void MTK_GPU_Power_model_sspm_enable(void);
int MTK_GPU_Power_model_init(void);
void MTK_GPU_Power_model_destroy(void);
void gpu_send_enable_ipi(unsigned int type, unsigned int enable);

extern void (*mtk_swpm_gpu_pm_start_fp)(void);

#endif

