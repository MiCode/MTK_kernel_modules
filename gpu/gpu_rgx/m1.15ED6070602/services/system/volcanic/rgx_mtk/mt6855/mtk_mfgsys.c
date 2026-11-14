// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 */

/* LINUX */
#include <linux/module.h>
#include <linux/dma-mapping.h>
/* RGX */
#include "ospvr_gputrace.h"
#include "rgxdevice.h"
#include "osfunc.h"
#include "pvrsrv.h"
#include "rgxhwperf.h"
#include "rgxinit.h"
#include "pvr_dvfs.h"
/* MTK */
#include "mtk_mfgsys.h"
#include <mtk_gpufreq.h>
#include <mtk_gpu_utility.h>
#include <ged_log.h>

static GED_LOG_BUF_HANDLE ghMTKGEDLog;
static IMG_HANDLE ghRGXUtilUser;

static PVRSRV_DEVICE_NODE *MTKGetRGXDevNode(void)
{
	PVRSRV_DATA *psPVRSRVData = NULL;
	PVRSRV_DEVICE_NODE *psDeviceNode = NULL;
	IMG_INT32 i = 0;

	psPVRSRVData = PVRSRVGetPVRSRVData();
	if (psPVRSRVData) {
		for (i = 0; i < psPVRSRVData->ui32RegisteredDevices; i++) {
			psDeviceNode = &psPVRSRVData->psDeviceNodeList[i];
			/* find registerd device node with config */
			if (psDeviceNode && psDeviceNode->psDevConfig)
				return psDeviceNode;
		}
	}

	MTK_LOGE("fail to get PVRSRV data");
	return NULL;
}

static void MTKWriteBackFreqToRGX(PVRSRV_DEVICE_NODE *psDevNode, IMG_UINT32 ui32NewFreq)
{
	RGX_DATA *psRGXData = (RGX_DATA *)psDevNode->psDevConfig->hDevData;
	/* kHz to Hz */
	psRGXData->psRGXTimingInfo->ui32CoreClockSpeed = ui32NewFreq * 1000;
}

static void MTKCalGPULoading(unsigned int *pui32Loading,
	unsigned int *pui32Block, unsigned int *pui32Idle, void *Util_Ex)
{
	PVRSRV_DEVICE_NODE *psDevNode = NULL;
	PVRSRV_RGXDEV_INFO *psDevInfo = NULL;
	RGXFWIF_GPU_UTIL_STATS sGpuUtilStats = {};

	psDevNode = MTKGetRGXDevNode();
	if (!psDevNode) {
		MTK_LOGE("fail to get RGX device node");
		return;
	}

	psDevInfo = psDevNode->pvDevice;
	if (psDevInfo && psDevInfo->pfnGetGpuUtilStats) {
		psDevInfo->pfnGetGpuUtilStats(psDevInfo->psDeviceNode, ghRGXUtilUser, &sGpuUtilStats);
		if (sGpuUtilStats.bValid) {
#if defined(__arm64__) || defined(__aarch64__)
			*pui32Loading = (100*(sGpuUtilStats.ui64GpuStatActive)) /
				sGpuUtilStats.ui64GpuStatCumulative;
			*pui32Block = (100*(sGpuUtilStats.ui64GpuStatBlocked)) /
				sGpuUtilStats.ui64GpuStatCumulative;
			*pui32Idle = (100*(sGpuUtilStats.ui64GpuStatIdle)) /
				sGpuUtilStats.ui64GpuStatCumulative;
#else
			*pui32Loading = (unsigned long)(100*(sGpuUtilStats.ui64GpuStatActive)) /
				(unsigned long)sGpuUtilStats.ui64GpuStatCumulative;
			*pui32Block = (unsigned long)(100*(sGpuUtilStats.ui64GpuStatBlocked)) /
				(unsigned long)sGpuUtilStats.ui64GpuStatCumulative;
			*pui32Idle = (unsigned long)(100*(sGpuUtilStats.ui64GpuStatIdle)) /
				(unsigned long)sGpuUtilStats.ui64GpuStatCumulative;
#endif /* defined(__arm64__) || defined(__aarch64__) */
		}
	}
}

static void MTKDVFSCommit(unsigned long ui32OPPIdx,
	GED_DVFS_COMMIT_TYPE eCommitType, int *pbCommited)
{
	PVRSRV_DEV_POWER_STATE ePowerState = PVRSRV_DEV_POWER_STATE_DEFAULT;
	PVRSRV_DEVICE_NODE *psDevNode = NULL;
	PVRSRV_ERROR eResult = PVRSRV_OK;

	psDevNode = MTKGetRGXDevNode();
	if (!psDevNode) {
		MTK_LOGE("fail to get RGX device node");
		if (pbCommited)
			*pbCommited = IMG_FALSE;
		return;
	}

	/* pause GPU when DVFS transition */
	eResult = PVRSRVDevicePreClockSpeedChange(psDevNode, IMG_FALSE, (void *)NULL);
	if (eResult != PVRSRV_OK) {
		MTK_LOGE("fail to pause GPU when DVFS transition (%d)", eResult);
		return;
	}

	/* only DVFS when device is power-on */
	PVRSRVGetDevicePowerState(psDevNode, &ePowerState);
	if (ePowerState == PVRSRV_DEV_POWER_STATE_ON) {
		eResult = gpufreq_commit(TARGET_DEFAULT, (int)ui32OPPIdx);
		if (eResult) {
			MTK_LOGE("fail to commit OPP: %d (%d)", (int)ui32OPPIdx, eResult);
			if (pbCommited)
				*pbCommited = IMG_FALSE;
		} else {
			/* update freq change to driver */
			MTKWriteBackFreqToRGX(psDevNode, gpufreq_get_cur_freq(TARGET_DEFAULT));
			/* update GED log buffer */
			ged_log_buf_print2(ghMTKGEDLog, GED_LOG_ATTR_TIME,
				"OPP: %d, VGPU: %d, VSRAM: %d, FREQ: %d",
				gpufreq_get_cur_oppidx(TARGET_DEFAULT),
				gpufreq_get_cur_volt(TARGET_DEFAULT),
				gpufreq_get_cur_vsram(TARGET_DEFAULT),
				gpufreq_get_cur_freq(TARGET_DEFAULT));
			if (pbCommited)
				*pbCommited = IMG_TRUE;
		}
	}

	/* resume GPU after DVFS complete */
	PVRSRVDevicePostClockSpeedChange(psDevNode, IMG_FALSE, (void *)NULL);
}

PVRSRV_ERROR MTKSysPrePowerState(IMG_HANDLE hSysData,
	PVRSRV_SYS_POWER_STATE eNewPowerState,
	PVRSRV_SYS_POWER_STATE eCurrentPowerState,
	IMG_BOOL bForced)
{
	PVRSRV_ERROR eResult = PVRSRV_OK;

	PVR_UNREFERENCED_PARAMETER(hSysData);
	PVR_UNREFERENCED_PARAMETER(bForced);

	/* power off */
	if (eNewPowerState == PVRSRV_SYS_POWER_STATE_OFF &&
		eCurrentPowerState == PVRSRV_SYS_POWER_STATE_ON) {
		eResult = gpufreq_power_control(POWER_OFF);
		if (eResult < 0) {
			MTK_LOGE("fail to power off GPU (%d)", eResult);
			/* update GED log buffer */
			ged_log_buf_print2(ghMTKGEDLog, GED_LOG_ATTR_TIME, "POWER_OFF FAILED");
		} else {
			eResult = PVRSRV_OK;
			/* update GED log buffer */
			ged_log_buf_print2(ghMTKGEDLog, GED_LOG_ATTR_TIME, "POWER_OFF");
		}
	}

	return eResult;
}

PVRSRV_ERROR MTKSysPostPowerState(IMG_HANDLE hSysData,
	PVRSRV_SYS_POWER_STATE eNewPowerState,
	PVRSRV_SYS_POWER_STATE eCurrentPowerState,
	IMG_BOOL bForced)
{
	PVRSRV_ERROR eResult = PVRSRV_OK;

	PVR_UNREFERENCED_PARAMETER(hSysData);
	PVR_UNREFERENCED_PARAMETER(bForced);

	/* power on */
	if (eNewPowerState == PVRSRV_SYS_POWER_STATE_ON &&
		eCurrentPowerState == PVRSRV_SYS_POWER_STATE_OFF) {
		eResult = gpufreq_power_control(POWER_ON);
		if (eResult < 0) {
			MTK_LOGE("fail to power on GPU (%d)", eResult);
			/* update GED log buffer */
			ged_log_buf_print2(ghMTKGEDLog, GED_LOG_ATTR_TIME, "POWER_ON FAILED");
		} else {
			eResult = PVRSRV_OK;
			/* update GED log buffer */
			ged_log_buf_print2(ghMTKGEDLog, GED_LOG_ATTR_TIME, "POWER_ON");
		}
	}

	return eResult;
}

PVRSRV_ERROR MTKMFGSystemInit(void)
{
	return PVRSRV_OK;
}

void MTKMFGSystemDeInit(void)
{
	/* nothing */
}

PVRSRV_ERROR MTKRGXDeviceInit(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
#if !defined(CONFIG_MTK_ENABLE_GMO)
	ghMTKGEDLog = ged_log_buf_alloc(512, 512 * 64,
		GED_LOG_BUF_TYPE_RINGBUFFER, "PowerLog", "ppL");
#else
	ghMTKGEDLog = NULL;
#endif /* !CONFIG_MTK_ENABLE_GMO */

	/* register GED callback */
	ged_dvfs_cal_gpu_utilization_ex_fp = MTKCalGPULoading;
	ged_dvfs_gpu_freq_commit_fp = MTKDVFSCommit;

	/* user must be registered before monitor GPU utilization */
	if (ghRGXUtilUser == NULL)
		SORgxGpuUtilStatsRegister(&ghRGXUtilUser);

	/* set bit mask to 64-bit to pass "dma_capable" check */
	dma_set_mask(psDevConfig->pvOSDevice, DMA_BIT_MASK(64));
	dma_set_coherent_mask(psDevConfig->pvOSDevice, DMA_BIT_MASK(64));

	return PVRSRV_OK;
}

PVRSRV_ERROR MTKRGXDeviceDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	return PVRSRV_OK;
}

void MTKFWDump(void)
{
#if defined(MTK_DEBUG_PROC_PRINT)
	PVRSRV_DEVICE_NODE *psDevNode = NULL;

	psDevNode = MTKGetRGXDevNode();
	if (!psDevNode) {
		MTK_LOGE("fail to get RGX device node");
		return;
	}

	MTK_PVRSRVDebugRequestSetSilence(IMG_TRUE);
	PVRSRVDebugRequest(psDevNode, DEBUG_REQUEST_VERBOSITY_MAX, NULL, NULL);
	MTK_PVRSRVDebugRequestSetSilence(IMG_FALSE);
#endif /* MTK_DEBUG_PROC_PRINT */
}
EXPORT_SYMBOL(MTKFWDump);
