/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __CORE_PLF_INIT_H__
#define __CORE_PLF_INIT_H__

extern struct miscdevice met_device;

/*
 *   MET External Symbol
 */

#ifdef MET_GPU
#include <mtk_gpu_utility.h>

#include "met_gpu_api/met_gpu_api.h"
#include "met_gpu_adv_api/met_gpu_adv_api.h"

#ifdef MET_GPU_LOAD_MONITOR
extern struct metdevice met_gpu;
#endif

#ifdef MET_GPU_MEM_MONITOR
extern struct metdevice met_gpumem;
#endif

#ifdef MET_GPU_PMU_MONITOR
extern struct metdevice met_gpu_pmu;
#endif

#ifdef MET_GPU_DVFS_MONITOR
extern struct metdevice met_gpudvfs;
#endif

#ifdef MET_GPU_PWR_MONITOR
extern struct metdevice met_gpupwr;
#endif

#ifdef MET_GPU_STALL_MONITOR
extern struct metdevice met_gpu_stall;
#endif
#endif /* MET_GPU */

extern long met_gpu_api_ready;
extern long met_gpu_adv_api_ready;

#ifdef MET_VCOREDVFS
#include "met_vcore_api/met_vcore_api.h"
extern struct metdevice met_vcoredvfs;
#endif /* MET_VCOREDVFS */

extern long met_vcore_api_ready;

#ifdef MET_EMI
#include "met_emi_api/met_emi_api.h"
extern struct metdevice met_sspm_emi;
#endif /* MET_EMI */

extern long met_emi_api_ready;

#ifdef MET_SMI
extern struct metdevice met_sspm_smi;
#endif

#ifdef MET_WALLTIME
extern struct metdevice met_wall_time;
#endif

#ifdef MET_SSPM
extern struct metdevice met_sspm_common;
#endif

#ifdef MET_MCUPM
extern struct metdevice met_mcupm_common;
#endif

#ifdef MET_CPUDSU
extern struct metdevice met_cpudsu;
#endif

#ifdef MET_THERMAL
extern struct metdevice met_thermal;
#endif

#ifdef MET_BACKLIGHT
#include "met_backlight_api/met_backlight_api.h"

extern struct metdevice met_backlight;
#endif

extern long met_backlight_api_ready;

#ifdef MET_PTPOD
extern struct metdevice met_ptpod;
#endif

#ifdef MET_SSPM

#include <linux/scmi_protocol.h>
#include "sspm_reservedmem.h"
#ifndef __TINYSYS_SCMI_H__
#define __TINYSYS_SCMI_H__
#include "tinysys-scmi.h"
#endif

#include "met_scmi_api/met_scmi_api.h"

#include "met_sspm_api/met_sspm_api.h"

#endif /* MET_SSPM */

extern long met_scmi_api_ready;
extern long met_sspm_api_ready;

#ifdef MET_MCUPM

#include "mtk_tinysys_ipi.h"
#include "mcupm_driver.h"
#include "mcupm_ipi_id.h"

#include "met_mcupm_api/met_mcupm_api.h"

#include "met_ipi_api/met_ipi_api.h"

#endif /* MET_MCUPM */

extern long met_ipi_api_ready;
extern long met_mcupm_api_ready;

#endif /*__CORE_PLF_INIT_H__*/
