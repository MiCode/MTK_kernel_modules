// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <mtk_gpu_utility.h>
#include <linux/uaccess.h>
#include <mali_kbase_gator_api.h>
#include <platform/mtk_mfg_counter.h>
#include <linux/sched.h>
#include <linux/mutex.h>

#include "mtk_ltr_pmu.h"
#include "mtk_gpu_power_model_sspm_ipi.h"

#include "mali_kbase.h"
#include "mali_kbase_vinstr.h"
#include <linux/scmi_protocol.h>
#include <linux/module.h>
#include "platform/mtk_platform_common.h"


#if MALI_USE_CSF
#include "csf/mali_kbase_csf_firmware.h"
#endif


static int init_flag;
struct kbase_device *pm_kbdev;

static DEFINE_MUTEX(gpu_pmu_info_lock);
static void MTK_LTR_gpu_pmu_kbase_setup(int flag, unsigned int interval_ns) {
	struct kbase_ioctl_hwcnt_reader_setup setup;
	int ret;

	//Get the first device - it doesn't matter in this case
	pm_kbdev = kbase_find_device(-1);
	if (!pm_kbdev)
		return;

	//Default doesn't enable all HWC
	setup.fe_bm = 0x16;
	setup.shader_bm = 0x5EC6;
	setup.tiler_bm = 0x6;
	setup.mmu_l2_bm = 0x1FC0;
	setup.buffer_count = 1;
	MTK_update_mtk_pm(flag);
	ret = MTK_kbase_vinstr_hwcnt_reader_setup(pm_kbdev->vinstr_ctx, &setup);
	//1ms = 1000000ns
	MTK_kbasep_vinstr_hwcnt_set_interval(interval_ns);
}



void MTK_LTR_gpu_pmu_start(unsigned int interval_ns) {

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_LOW_POWER)
	return;
#else
	int pm_tool = MTK_get_mtk_pm();
	mutex_lock(&gpu_pmu_info_lock);
	if (pm_tool == pm_swpm) {
		//gpu stall counter on
		//mtk_gpu_stall_create_subfs();
		//mtk_gpu_stall_start();
		MTK_kbasep_vinstr_hwcnt_set_interval(0);
		MTK_update_mtk_pm(pm_ltr);
		MTK_kbasep_vinstr_hwcnt_set_interval(interval_ns);
	} else {
		//gpu stall counter on
		//mtk_gpu_stall_create_subfs();
		//mtk_gpu_stall_start();
		MTK_LTR_gpu_pmu_kbase_setup(pm_ltr, interval_ns);
	}
	if (init_flag != gpm_sspm_side)
		init_flag = gpm_kernel_side;

	mutex_unlock(&gpu_pmu_info_lock);
#endif

}
EXPORT_SYMBOL(MTK_LTR_gpu_pmu_start);

void MTK_LTR_gpu_pmu_start_swpm(unsigned int interval_ns){
	int pm_tool = MTK_get_mtk_pm();

	mutex_lock(&gpu_pmu_info_lock);
	//Get the first device - it doesn't matter in this case
	pm_kbdev = kbase_find_device(-1);
	if (!pm_kbdev) {
		return;
	}

	if (init_flag == gpm_sspm_side) {
		gpu_send_enable_ipi(GPU_PM_SWITCH, 0);
		init_flag = gpm_off;
	}

	if (init_flag == gpm_off && pm_tool == pm_non) {
		MTK_LTR_gpu_pmu_kbase_setup(pm_swpm, interval_ns);
	}
	else if(pm_tool != pm_non){
		MTK_kbasep_vinstr_hwcnt_set_interval(0);
		MTK_update_mtk_pm(pm_swpm);
		MTK_kbasep_vinstr_hwcnt_set_interval(interval_ns);
	}
	gpu_send_enable_ipi(GPU_PM_SWITCH, 0);
	init_flag = gpm_kernel_side;
	mutex_unlock(&gpu_pmu_info_lock);
}
EXPORT_SYMBOL(MTK_LTR_gpu_pmu_start_swpm);

void MTK_LTR_gpu_pmu_stop(void){

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_LOW_POWER)
	return;
#else
	mutex_lock(&gpu_pmu_info_lock);
	if (init_flag != gpm_off) {
		if (init_flag == gpm_sspm_side) {
			gpu_send_enable_ipi(GPU_PM_SWITCH, 0);
			gpu_send_enable_ipi(GPU_PM_POWER_STATUE, 0);
		} else {
			MTK_kbasep_vinstr_hwcnt_release();
			//mtk_gpu_stall_stop();
			//mtk_gpu_stall_delete_subfs();
		}
		MTK_update_mtk_pm(pm_non);
		init_flag = gpm_off;
	}
	mutex_unlock(&gpu_pmu_info_lock);
#endif

}
EXPORT_SYMBOL(MTK_LTR_gpu_pmu_stop);

void MTK_LTR_gpu_pmu_suspend(void){
	if (init_flag != gpm_kernel_side) {
		return;
	}
	kbase_vinstr_suspend(pm_kbdev->vinstr_ctx);
}
EXPORT_SYMBOL(MTK_LTR_gpu_pmu_suspend);

void MTK_LTR_gpu_pmu_resume(void){
	if (init_flag != gpm_kernel_side) {
		return;
	}
	kbase_vinstr_resume(pm_kbdev->vinstr_ctx);
}
EXPORT_SYMBOL(MTK_LTR_gpu_pmu_resume);


int MTK_LTR_gpu_pmu_init(void) {
	mtk_ltr_gpu_pmu_start_fp = MTK_LTR_gpu_pmu_start;
	mtk_ltr_gpu_pmu_stop_fp = MTK_LTR_gpu_pmu_stop;

	return 0;
}

void MTK_LTR_gpu_pmu_destroy(void) {
	mtk_ltr_gpu_pmu_start_fp = NULL;
	mtk_ltr_gpu_pmu_stop_fp = NULL;
}

