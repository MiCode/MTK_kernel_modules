/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc.
 */

#ifndef __MTK_MFGSYS_H__
#define __MTK_MFGSYS_H__

/* RGX */
#include "img_types.h"
#include "img_defs.h"
#include "pvrsrv_error.h"
#include "pvrsrv_device.h"
#include "servicesext.h"
/* MTK */
#include <ged_dvfs.h>
#include <ged_type.h>

/**************************************************
 * Platform Config
 **************************************************/
#define SYS_RGX_OF_COMPATIBLE               "mediatek,rgx"
#define RGX_HW_CORE_CLOCK_SPEED             (480000000)
#define SYS_RGX_ACTIVE_POWER_LATENCY_MS     (3)

/**************************************************
 * Function Declaration
 **************************************************/
PVRSRV_ERROR MTKRGXDeviceInit(PVRSRV_DEVICE_CONFIG *psDevConfig);
PVRSRV_ERROR MTKRGXDeviceDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig);
PVRSRV_ERROR MTKSysPrePowerState(IMG_HANDLE hSysData,
	PVRSRV_SYS_POWER_STATE eNewPowerState,
	PVRSRV_SYS_POWER_STATE eCurrentPowerState,
	IMG_BOOL bForced);
PVRSRV_ERROR MTKSysPostPowerState(IMG_HANDLE hSysData,
	PVRSRV_SYS_POWER_STATE eNewPowerState,
	PVRSRV_SYS_POWER_STATE eCurrentPowerState,
	IMG_BOOL bForced);
PVRSRV_ERROR MTKMFGSystemInit(void);
void MTKMFGSystemDeInit(void);
void MTKFWDump(void);

extern void (*ged_dvfs_cal_gpu_utilization_ex_fp)(unsigned int *pui32Loading,
	unsigned int *pui32Block, unsigned int *pui32Idle, void *Util_Ex);
extern void (*ged_dvfs_gpu_freq_commit_fp)(unsigned long ui32NewFreqID,
	GED_DVFS_COMMIT_TYPE eCommitType, int *pbCommited);

#endif /* __MTK_MFGSYS_H__ */
