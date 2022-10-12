// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/kallsyms.h>
#include "met_drv.h"
#include "interface.h"
#include "core_plf_init.h"
#include "met_kernel_symbol.h"
#include "met_api.h"

#undef	DEBUG

#ifdef MET_GPU
/*
 *   GPU
 */

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DEFINE
#include "met_gpu_api/met_gpu_api.h"
#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DEFINE
#include "met_gpu_adv_api/met_gpu_adv_api.h"
#endif /* MET_GPU */

long met_gpu_api_ready = 0;
EXPORT_SYMBOL(met_gpu_api_ready);
long met_gpu_adv_api_ready = 0;
EXPORT_SYMBOL(met_gpu_adv_api_ready);

#ifdef MET_VCOREDVFS
/*
 *   VCORE DVFS
 */
#include <dvfsrc-exp.h>

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DEFINE
#include "met_vcore_api/met_vcore_api.h"
#endif /* MET_VCOREDVFS */

long met_vcore_api_ready = 0;
EXPORT_SYMBOL(met_vcore_api_ready);

#ifdef MET_EMI

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DEFINE
#include "met_emi_api/met_emi_api.h"
#endif /* MET_EMI */

long met_emi_api_ready = 0;
EXPORT_SYMBOL(met_emi_api_ready);

#ifdef MET_BACKLIGHT

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DEFINE
#include "met_backlight_api/met_backlight_api.h"
#endif /* MET_BACKLIGHT */

long met_backlight_api_ready = 0;
EXPORT_SYMBOL(met_backlight_api_ready);

#ifdef MET_SSPM

#include <linux/scmi_protocol.h>
#include "sspm_reservedmem.h"
#ifndef __TINYSYS_SCMI_H__
#define __TINYSYS_SCMI_H__
#include "tinysys-scmi.h"
#endif

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DEFINE
#include "met_sspm_api/met_sspm_api.h"

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DEFINE
#include "met_scmi_api/met_scmi_api.h"

#endif /* MET_SSPM */

long met_scmi_api_ready = 0;
EXPORT_SYMBOL(met_scmi_api_ready);
long met_sspm_api_ready = 0;
EXPORT_SYMBOL(met_sspm_api_ready);

#ifdef MET_MCUPM

#include "mtk_tinysys_ipi.h"
#include "mcupm_driver.h"

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DEFINE
#include "met_mcupm_api/met_mcupm_api.h"

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DEFINE
#include "met_ipi_api/met_ipi_api.h"

#endif /* MET_MCUPM */

long met_ipi_api_ready = 0;
EXPORT_SYMBOL(met_ipi_api_ready);
long met_mcupm_api_ready = 0;
EXPORT_SYMBOL(met_mcupm_api_ready);

int core_plf_init(void)
{
#ifdef MET_GPU
#ifdef MET_GPU_LOAD_MONITOR
	met_register(&met_gpu);
#endif
#ifdef MET_GPU_DVFS_MONITOR
    met_register(&met_gpudvfs);
#endif
#ifdef MET_GPU_MEM_MONITOR
	met_register(&met_gpumem);
#endif
#ifdef MET_GPU_PWR_MONITOR
	met_register(&met_gpupwr);
#endif
#ifdef MET_GPU_PMU_MONITOR
	met_register(&met_gpu_pmu);
#endif
#ifdef MET_GPU_STALL_MONITOR
	met_register(&met_gpu_stall);
#endif
#ifdef MET_GPU_STALL_MONITOR
	met_register(&met_gpu_stall);
#endif
#endif

#ifdef MET_VCOREDVFS
	met_register(&met_vcoredvfs);
#endif

#ifdef MET_EMI
	met_register(&met_sspm_emi);
#endif

#ifdef MET_SMI
	met_register(&met_sspm_smi);
#endif

#ifdef MET_WALLTIME
	met_register(&met_wall_time);
#endif

#ifdef MET_SSPM
	met_register(&met_sspm_common);
#endif

#ifdef MET_MCUPM
	met_register(&met_mcupm_common);
#endif

#ifdef MET_CPUDSU
	met_register(&met_cpudsu);
#endif

#ifdef MET_BACKLIGHT
	met_register(&met_backlight);
#endif

#ifdef MET_THERMAL
	met_register(&met_thermal);
#endif

#ifdef MET_PTPOD
	met_register(&met_ptpod);
#endif

	return 0;
}

void core_plf_exit(void)
{
#ifdef MET_GPU
#ifdef MET_GPU_LOAD_MONITOR
	met_deregister(&met_gpu);
#endif
#ifdef MET_GPU_DVFS_MONITOR
    met_deregister(&met_gpudvfs);
#endif
#ifdef MET_GPU_MEM_MONITOR
	met_deregister(&met_gpumem);
#endif
#ifdef MET_GPU_PWR_MONITOR
	met_deregister(&met_gpupwr);
#endif
#ifdef MET_GPU_PMU_MONITOR
	met_deregister(&met_gpu_pmu);
#endif
#ifdef MET_GPU_STALL_MONITOR
	met_deregister(&met_gpu_stall);
#endif
#endif

#ifdef MET_VCOREDVFS
	met_deregister(&met_vcoredvfs);
#endif

#ifdef MET_EMI
	met_deregister(&met_sspm_emi);
#endif

#ifdef MET_SMI
	met_deregister(&met_sspm_smi);
#endif

#ifdef MET_WALLTIME
	met_deregister(&met_wall_time);
#endif

#ifdef MET_SSPM
	met_deregister(&met_sspm_common);
#endif

#ifdef MET_MCUPM
	met_deregister(&met_mcupm_common);
#endif

#ifdef MET_CPUDSU
	met_deregister(&met_cpudsu);
#endif

#ifdef MET_BACKLIGHT
	met_deregister(&met_backlight);
#endif

#ifdef MET_THERMAL
	met_deregister(&met_thermal);
#endif

#ifdef MET_PTPOD
	met_deregister(&met_ptpod);
#endif
}
