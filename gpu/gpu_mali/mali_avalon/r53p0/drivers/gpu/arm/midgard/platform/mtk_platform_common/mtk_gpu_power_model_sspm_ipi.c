// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#include "mali_kbase.h"
#include <linux/scmi_protocol.h>
#include <tinysys-scmi.h>
#include "mtk_gpu_power_model_sspm_ipi.h"

static int init_flag;
static bool ipi_register_flag;

#ifdef CONFIG_MALI_SCMI_ENABLE
static int gpu_pm_id;
static struct scmi_tinysys_info_st *_tinfo;
#endif

void gpu_send_enable_ipi(unsigned int type, unsigned int enable)
{
	int ret = 0;
	struct gpu_pm_ipi_cmds ipi_cmd;
	if (!ipi_register_flag) {
		pr_info("ipi_register_flag fail");
	}

	ipi_cmd.cmd = type;
	ipi_cmd.power_statue= enable;
#ifdef CONFIG_MALI_SCMI_ENABLE
	ret = scmi_tinysys_common_set(_tinfo->ph, gpu_pm_id,
			ipi_cmd.cmd, ipi_cmd.power_statue, 0, 0, 0);
#endif
	if (ret) {
		pr_info("gpu_send_enable_ipi %d send fail,ret=%d\n",
		ipi_cmd.cmd, ret);
	}
}

void MTK_GPU_Power_model_sspm_enable(void) {
	/*int pm_tool = MTK_get_mtk_pm();

	if (pm_tool == pm_non)
		MTKGPUPower_model_kbase_setup(pm_swpm, 0);

	MTKGPUPower_model_kbase_setup(pm_swpm, 0);
	*/
	gpu_send_enable_ipi(GPU_PM_SWITCH, 1);
	init_flag = gpm_sspm_side;
}

int MTK_GPU_Power_model_init(void) {
#ifdef CONFIG_MALI_SCMI_ENABLE
	_tinfo = get_scmi_tinysys_info();
	if(_tinfo){
		if (!of_property_read_u32(_tinfo->sdev->dev.of_node, "scmi-gpupm",&gpu_pm_id)){
			ipi_register_flag = true;
		}else{
			pr_info("get scmi-gpupm fail\n");
			ipi_register_flag = false;
		}
	}else{
		pr_info("get_scmi_tinysys_info fail\n");
	}
#endif
	mtk_swpm_gpu_pm_start_fp = MTK_GPU_Power_model_sspm_enable;

	return 0;
}

void MTK_GPU_Power_model_destroy(void) {
	mtk_swpm_gpu_pm_start_fp = NULL;
}

