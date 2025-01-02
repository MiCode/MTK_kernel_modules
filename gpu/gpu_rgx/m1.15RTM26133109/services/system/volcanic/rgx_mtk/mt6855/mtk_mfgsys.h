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
#define SYS_RGX_ACTIVE_POWER_LATENCY_MS     (15)

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
extern void (*mtk_notify_gpu_freq_change_fp)(u32 clk_idx, u32 gpufreq);



#if defined(SUPPORT_TRUSTED_DEVICE)

enum secgpu_ipi_cmd {
	/* Common */
	CMD_SECGPU_INIT_SHARED_MEM      = 0,
	CMD_SECGPU_SET_FWIMG            = 1,
	CMD_SECGPU_SET_POWER            = 2,
	CMD_SECGPU_SET_START            = 3,
	CMD_SECGPU_SET_STOP             = 4,
};
#define SECGPU_SUCCESS 0
#define SECGPU_EINVAL -1
#define SECGPU_STATUS_MEM_SZ 10
#define SECGPU_IPI_DATA_LEN \
	DIV_ROUND_UP(sizeof(struct secgpu_ipi_data), MBOX_SLOT_SIZE)

#define PK_ALIGN4 __attribute__((packed, aligned(4)))
struct secgpu_ipi_data {
	unsigned int cmd_id;
	int return_value;
	unsigned int reserved_fw_mem_base;
	unsigned int reserved_fw_mem_size;
	unsigned int reserved_ipi_shm_base;
	unsigned int reserved_ipi_shm_size;
} PK_ALIGN4;

typedef struct PK_ALIGN4 _MTK_PVRSRV_DEVICE_FEATURE_CONFIG_
{
	IMG_UINT64 ui64ErnsBrns;
	IMG_UINT64 ui64Features;
	IMG_UINT32 ui32FeaturesValues[RGX_FEATURE_WITH_VALUES_MAX_IDX];
	IMG_UINT32 ui32MAXDustCount;
}MTK_PVRSRV_DEVICE_FEATURE_CONFIG;

typedef struct PK_ALIGN4 _MTK_TD_FW_MEM_
{
	IMG_UINT32 code_addr_lo;
	IMG_UINT32 code_addr_hi;
	IMG_UINT32 data_addr_lo;
	IMG_UINT32 data_addr_hi;
	/* only for META FW with RGX_FEATURE_META_DMA */
	IMG_UINT32 core_code_addr_lo;
	IMG_UINT32 core_code_addr_hi;
	IMG_UINT32 core_data_addr_lo;
	IMG_UINT32 core_data_addr_hi;
}MTK_TD_FW_MEM;

#define RGX_MAX_NUM_MMU_PAGE_SIZE_RANGES 4
typedef struct PK_ALIGN4 _MTK_RGX_LAYER_PARAMS_
{
	IMG_DEV_PHYADDR sPCAddr;
	/* MIPS-only fields */
	IMG_DEV_PHYADDR sGPURegAddr;
	IMG_DEV_PHYADDR sBootRemapAddr;
	IMG_DEV_PHYADDR sCodeRemapAddr;
	IMG_DEV_PHYADDR sDataRemapAddr;
	IMG_DEV_PHYADDR sTrampolineRemapAddr;
	IMG_BOOL bDevicePA0IsValid;
	IMG_UINT64 aui64MMUPageSizeRangeValue[RGX_MAX_NUM_MMU_PAGE_SIZE_RANGES];
	MTK_PVRSRV_DEVICE_FEATURE_CONFIG sDevFeatureCfg;
} MTK_RGX_LAYER_PARAMS;

typedef union PK_ALIGN4 _MTK_RGX_FW_BOOT_PARAMS_
{
	struct PK_ALIGN4
	{
		IMG_DEV_VIRTADDR sFWCodeDevVAddr;
		IMG_DEV_VIRTADDR sFWDataDevVAddr;
		IMG_DEV_VIRTADDR sFWCorememCodeDevVAddr;
		RGXFWIF_DEV_VIRTADDR sFWCorememCodeFWAddr;
		IMG_DEVMEM_SIZE_T uiFWCorememCodeSize;
		IMG_DEV_VIRTADDR sFWCorememDataDevVAddr;
		RGXFWIF_DEV_VIRTADDR sFWCorememDataFWAddr;
		IMG_UINT32 ui32NumThreads;
	} sMeta;
} MTK_RGX_FW_BOOT_PARAMS;

#define SECGPU_SHM_BOOTPAR_OFFSET     (0U)
#define SECGPU_SHM_POWERPAR_OFFSET    ((unsigned int) sizeof(MTK_RGX_FW_BOOT_PARAMS))
#define SECGPU_SHM_FW_MEM_OFFSET      (SECGPU_SHM_POWERPAR_OFFSET + (unsigned int) sizeof(MTK_RGX_LAYER_PARAMS))
#define SECGPU_SHM_FW_BIN_OFFSET      (SECGPU_SHM_FW_MEM_OFFSET   + (unsigned int) sizeof(MTK_TD_FW_MEM))
#define RESERVE_FOR_REE                0

PVRSRV_ERROR MTKTDSendFWImage(IMG_HANDLE hSysData, PVRSRV_TD_FW_PARAMS *psTDFWParams);
PVRSRV_ERROR MTKTDSetPowerParams(IMG_HANDLE hSysData,
				PVRSRV_TD_POWER_PARAMS *psTDPowerParams);
PVRSRV_ERROR MTKTDRGXStart(IMG_HANDLE hSysData);
PVRSRV_ERROR MTKTDRGXStop(IMG_HANDLE hSysData);
void dumpFWMem(IMG_UINT64 shift_of_g_pvFWKM);
int secgpu_gpueb_init(void);
#endif

#endif /* __MTK_MFGSYS_H__ */
