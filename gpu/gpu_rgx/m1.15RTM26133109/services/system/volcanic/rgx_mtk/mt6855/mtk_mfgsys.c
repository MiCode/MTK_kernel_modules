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
#include <ged_gpu_bm.h>
#if defined(SUPPORT_TRUSTED_DEVICE)
#include <linux/soc/mediatek/mtk_tinysys_ipi.h>
#include <gpueb_ipi.h>
#include <gpueb_reserved_mem.h>
#endif

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
	struct GpuUtilization_Ex *util_ex = (struct GpuUtilization_Ex *) Util_Ex;

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
			util_ex->util_active = *pui32Loading;
		}
	}
}

static void MTKDVFSCommit(unsigned long ui32OPPIdx,
	GED_DVFS_COMMIT_TYPE eCommitType, int *pbCommited)
{
	PVRSRV_DEV_POWER_STATE ePowerState = PVRSRV_DEV_POWER_STATE_DEFAULT;
	PVRSRV_DEVICE_NODE *psDevNode = NULL;
	PVRSRV_ERROR eResult = PVRSRV_OK;

	*pbCommited = IMG_FALSE;

	psDevNode = MTKGetRGXDevNode();
	if (!psDevNode) {
		MTK_LOGE("fail to get RGX device node");
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
		}
		else {
			/* update GED log buffer */
			ged_log_buf_print2(ghMTKGEDLog, GED_LOG_ATTR_TIME,
				"OPP: %d, VGPU: %d, VSRAM: %d, FREQ: %d",
				gpufreq_get_cur_oppidx(TARGET_DEFAULT),
				gpufreq_get_cur_volt(TARGET_DEFAULT),
				gpufreq_get_cur_vsram(TARGET_DEFAULT),
				gpufreq_get_cur_freq(TARGET_DEFAULT));
			*pbCommited = IMG_TRUE;
		}
	}

	/* resume GPU after DVFS complete */
	PVRSRVDevicePostClockSpeedChange(psDevNode, IMG_FALSE, (void *)NULL);
}

static void MTKGPUFreqChangeNotify(unsigned int clk_idx, unsigned int freq)
{
	PVRSRV_DEVICE_NODE *psDevNode = NULL;

	psDevNode = MTKGetRGXDevNode();
	if (clk_idx == 0 || clk_idx == 1)
		MTKWriteBackFreqToRGX(psDevNode, freq);
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
#if defined(ENABLE_COMMON_DVFS)
		ged_dvfs_gpu_clock_switch_notify(0);
		mtk_notify_gpu_power_change(0);
#endif /* ENABLE_COMMON_DVFS */
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
#if defined(ENABLE_COMMON_DVFS)
		ged_dvfs_gpu_clock_switch_notify(1);
		mtk_notify_gpu_power_change(1);
#endif /* ENABLE_COMMON_DVFS */
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
	mtk_notify_gpu_freq_change_fp = MTKGPUFreqChangeNotify;

	/* user must be registered before monitor GPU utilization */
	if (ghRGXUtilUser == NULL)
		SORgxGpuUtilStatsRegister(&ghRGXUtilUser);

	/* set bit mask to 64-bit to pass "dma_capable" check */
	dma_set_mask(psDevConfig->pvOSDevice, DMA_BIT_MASK(64));
	dma_set_coherent_mask(psDevConfig->pvOSDevice, DMA_BIT_MASK(64));

	/* init gpu bandwidth monitor */
	mtk_bandwidth_resource_init();

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

#if defined(SUPPORT_TRUSTED_DEVICE)
#include "rgxstartstop.h"
#include "rgxfwimageutils.h"
#include "rgxinit.h"
#include "rgxfwutils.h"
#include "rgxlayer_impl.h"

static int g_secgpu_ipi_channel;
static phys_addr_t g_secgpu_ipi_shared_mem_va;
int g_secgpu_ipi_shared_mem_pa;
int g_secgpu_ipi_shared_mem_size;
static phys_addr_t g_secgpu_fw_reservd_mem_va;
int g_secgpu_fw_reservd_mem_pa;
int g_secgpu_fw_reservd_mem_size;

static struct secgpu_ipi_data g_secgpu_recv_msg;

static int secgpu_ipi_to_gpueb(struct secgpu_ipi_data data)
{
	int ret = SECGPU_SUCCESS;
	int timeout = IPI_TIMEOUT_MS;

	if(data.cmd_id == CMD_SECGPU_INIT_SHARED_MEM){
		timeout = IPI_TIMEOUT_MS*10;
	}
	ret = mtk_ipi_send_compl(get_gpueb_ipidev(), g_secgpu_ipi_channel, IPI_SEND_POLLING,
		(void *)&data, SECGPU_IPI_DATA_LEN, timeout);

	if (unlikely(ret != IPI_ACTION_DONE)) {
		MTK_LOGE("[SECGPU] fail to send IPI command: cmd id %d (ret %d)", data.cmd_id, ret);
		return SECGPU_EINVAL;
	}

	if (g_secgpu_recv_msg.cmd_id == CMD_SECGPU_INIT_SHARED_MEM)
	{

		phys_addr_t shared_mem_pa = 0, shared_mem_va = 0, shared_mem_size = 0;
		IMG_CPU_PHYADDR sPhyAddr;
		sPhyAddr.uiAddr = g_secgpu_recv_msg.reserved_fw_mem_base;
		g_secgpu_fw_reservd_mem_pa = g_secgpu_recv_msg.reserved_fw_mem_base;
		g_secgpu_fw_reservd_mem_size = g_secgpu_recv_msg.reserved_fw_mem_size;
		g_secgpu_fw_reservd_mem_va = 0;
		//MTK_LOGI("[SECGPU] got reserved memory for fw phy_addr: 0x%x, virt_addr: 0x%llx, size: 0x%x",
		//			g_secgpu_fw_reservd_mem_pa, g_secgpu_fw_reservd_mem_va, g_secgpu_fw_reservd_mem_size);
	}
	return SECGPU_SUCCESS;
}

int secgpu_gpueb_init(void)
{
	struct secgpu_ipi_data send_msg = {};

	phys_addr_t shared_mem_va = 0;
	unsigned int shared_mem_pa = 0, shared_mem_size = 0;
	int ret = SECGPU_SUCCESS;

	// init ipi channel
	g_secgpu_ipi_channel = gpueb_get_send_PIN_ID_by_name("IPI_ID_SECURE_GPU");
	if (unlikely(g_secgpu_ipi_channel < 0)) {
		MTK_LOGE("[SECGPU] fail to get secgpu IPI channel id (ENOENT)");
		ret = SECGPU_EINVAL;
		return ret;
	}
	mtk_ipi_register(get_gpueb_ipidev(), g_secgpu_ipi_channel, NULL, NULL, (void *)&g_secgpu_recv_msg);

	// init shared memory
	shared_mem_va = gpueb_get_reserve_mem_virt_by_name("MEM_ID_SECGPU");
	MTK_LOGI("[SECGPU] status shared memory virt_addr: 0x%llx shared_mem_va", shared_mem_va);
	if (!shared_mem_va) {
		MTK_LOGE("[SECGPU] fail to get secgpu reserved memory virtual addr (ENOENT)");
		ret = SECGPU_EINVAL;
		return ret;
	}
	shared_mem_pa = gpueb_get_reserve_mem_phys_by_name("MEM_ID_SECGPU");
	if (!shared_mem_pa) {
		MTK_LOGE("[SECGPU] fail to get secgpu reserved memory physical addr (ENOENT)");
		ret = SECGPU_EINVAL;
		return ret;
	}
	shared_mem_size = gpueb_get_reserve_mem_size_by_name("MEM_ID_SECGPU");
	if (!shared_mem_size) {
		MTK_LOGE("[SECGPU] fail to get secgpu reserved memory size (ENOENT)");
		ret = SECGPU_EINVAL;
		return ret;
	}

	g_secgpu_ipi_shared_mem_va = shared_mem_va;
	//MTK_LOGI("[SECGPU] status shared memory virt_addr: 0x%llx g_secgpu_ipi_shared_mem_va", g_secgpu_ipi_shared_mem_va);

	send_msg.cmd_id = CMD_SECGPU_INIT_SHARED_MEM;
	send_msg.return_value = 0;
	send_msg.reserved_ipi_shm_base = shared_mem_pa;
	send_msg.reserved_ipi_shm_size = (uint32_t)shared_mem_size;
	g_secgpu_ipi_shared_mem_va = shared_mem_va;
	g_secgpu_ipi_shared_mem_pa = shared_mem_pa;
	g_secgpu_ipi_shared_mem_size = shared_mem_size;
	g_secgpu_fw_reservd_mem_va = NULL;
	g_secgpu_fw_reservd_mem_pa = NULL;

	ret = secgpu_ipi_to_gpueb(send_msg); // if (g_gpueb_support)
	if (unlikely(ret)) {
		MTK_LOGE("[SECGPU] fail to init secgpu shared memory (%d)", ret);
	}

	return ret;
}

static void setPowerParams()
{
	int i;
	PVRSRV_DEVICE_NODE *psDevNode = MTKGetRGXDevNode();
	if (!psDevNode) {
		MTK_LOGE("fail to get RGX device node");
		return;
	}
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *)psDevNode->pvDevice;
	if (!psDevInfo) {
		MTK_LOGE("fail to get RGX device info");
		return;
	}
	MTK_RGX_LAYER_PARAMS *pPowerParam = (MTK_RGX_LAYER_PARAMS *) (g_secgpu_ipi_shared_mem_va + SECGPU_SHM_POWERPAR_OFFSET);
	//pPowerParam->sPCAddr = psTDPowerParams->sPCAddr; //RGXAcquireKernelMMUPC
	pPowerParam->sDevFeatureCfg.ui64ErnsBrns = psDevInfo->sDevFeatureCfg.ui64ErnsBrns;
	pPowerParam->sDevFeatureCfg.ui64Features = psDevInfo->sDevFeatureCfg.ui64Features;
	for(i=0;i<RGX_FEATURE_WITH_VALUES_MAX_IDX;i++){
		pPowerParam->sDevFeatureCfg.ui32FeaturesValues[i] = psDevInfo->sDevFeatureCfg.ui32FeaturesValues[i];
	}
	pPowerParam->sDevFeatureCfg.ui32MAXDustCount = psDevInfo->sDevFeatureCfg.ui32MAXDMCount;

	for (i = 0; i < ARRAY_SIZE(psDevInfo->aui64MMUPageSizeRangeValue); ++i)
	{
		pPowerParam->aui64MMUPageSizeRangeValue[i] = psDevInfo->aui64MMUPageSizeRangeValue[i];
	}
}

PVRSRV_ERROR MTKTDSendFWImage(IMG_HANDLE hSysData, PVRSRV_TD_FW_PARAMS *psTDFWParams)
{
	int ret ;

	MTK_PVRSRV_DEVICE_FEATURE_CONFIG mtkDevCfg;
	struct secgpu_ipi_data send_msg;
	MTK_TD_FW_MEM fw_mem;
	MTK_RGX_FW_BOOT_PARAMS bootParam;
	PVRSRV_DEVICE_NODE *psDevNode = MTKGetRGXDevNode();
	if (!psDevNode) {
		MTK_LOGE("fail to get RGX device node");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *)psDevNode->pvDevice;
	if (!psDevInfo) {
		MTK_LOGE("fail to get RGX device info");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	IMG_DEV_PHYADDR sPhyAddr;
	PMR *psFWCodePMR, *psFWDataPMR, *psFWCoreCodePMR, *psFWCoreDataPMR;
	IMG_BOOL bValid;
	PVR_UNREFERENCED_PARAMETER(hSysData);

	if (g_secgpu_ipi_shared_mem_va == NULL) {
		MTK_LOGE("[SECGPU] g_secgpu_ipi_shared_mem_va is null ");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	bootParam.sMeta.sFWCodeDevVAddr.uiAddr = psTDFWParams->uFWP.sMeta.sFWCodeDevVAddr.uiAddr;
	bootParam.sMeta.sFWDataDevVAddr.uiAddr = psTDFWParams->uFWP.sMeta.sFWDataDevVAddr.uiAddr;
	bootParam.sMeta.sFWCorememCodeDevVAddr.uiAddr = psTDFWParams->uFWP.sMeta.sFWCorememCodeDevVAddr.uiAddr;
	bootParam.sMeta.sFWCorememCodeFWAddr.ui32Addr = psTDFWParams->uFWP.sMeta.sFWCorememCodeFWAddr.ui32Addr;
	bootParam.sMeta.uiFWCorememCodeSize = psTDFWParams->uFWP.sMeta.uiFWCorememCodeSize;
	bootParam.sMeta.sFWCorememDataDevVAddr.uiAddr = psTDFWParams->uFWP.sMeta.sFWCorememDataDevVAddr.uiAddr;
	bootParam.sMeta.sFWCorememDataFWAddr.ui32Addr = psTDFWParams->uFWP.sMeta.sFWCorememDataFWAddr.ui32Addr;
	bootParam.sMeta.ui32NumThreads = psTDFWParams->uFWP.sMeta.ui32NumThreads;
	memcpy(g_secgpu_ipi_shared_mem_va + SECGPU_SHM_BOOTPAR_OFFSET, &bootParam, sizeof(MTK_RGX_FW_BOOT_PARAMS));

	psFWCodePMR = (PMR *)(psDevInfo->psRGXFWCodeMemDesc->psImport->hPMR);
	psFWDataPMR = (PMR *)(psDevInfo->psRGXFWDataMemDesc->psImport->hPMR);
	sPhyAddr.uiAddr = 0;
	RGXGetPhyAddr(psFWCodePMR, &sPhyAddr, 0, OSGetPageShift(), 1, &bValid);
	fw_mem.code_addr_lo = (unsigned int)sPhyAddr.uiAddr;
	fw_mem.code_addr_hi = (unsigned int)(sPhyAddr.uiAddr >> 32);
	RGXGetPhyAddr(psFWDataPMR, &sPhyAddr, 0, OSGetPageShift(), 1, &bValid);
	fw_mem.data_addr_lo = (unsigned int)sPhyAddr.uiAddr;
	fw_mem.data_addr_hi = (unsigned int)(sPhyAddr.uiAddr >> 32);
	psFWCoreCodePMR = (PMR *)(psDevInfo->psRGXFWCorememCodeMemDesc->psImport->hPMR);
	psFWCoreDataPMR = (PMR *)(psDevInfo->psRGXFWIfCorememDataStoreMemDesc->psImport->hPMR);
	RGXGetPhyAddr(psFWCoreCodePMR, &sPhyAddr, 0, OSGetPageShift(), 1, &bValid);
	fw_mem.core_code_addr_lo = (unsigned int)sPhyAddr.uiAddr;
	fw_mem.core_code_addr_hi = (unsigned int)(sPhyAddr.uiAddr >> 32);
	RGXGetPhyAddr(psFWCoreDataPMR, &sPhyAddr, 0, OSGetPageShift(), 1, &bValid);
	fw_mem.core_data_addr_lo = (unsigned int)sPhyAddr.uiAddr;
	fw_mem.core_data_addr_hi = (unsigned int)(sPhyAddr.uiAddr >> 32);
	memcpy(g_secgpu_ipi_shared_mem_va + SECGPU_SHM_FW_MEM_OFFSET , &fw_mem, sizeof(MTK_TD_FW_MEM));

	IMG_UINT32 *pui32FirmwareSize = (IMG_UINT32 *) (g_secgpu_ipi_shared_mem_va + SECGPU_SHM_FW_BIN_OFFSET);
	*pui32FirmwareSize = psTDFWParams->ui32FirmwareSize;
	memcpy(g_secgpu_ipi_shared_mem_va + SECGPU_SHM_FW_BIN_OFFSET + sizeof(IMG_UINT32), (psTDFWParams->pvFirmware), psTDFWParams->ui32FirmwareSize);

	setPowerParams();

	send_msg.cmd_id = CMD_SECGPU_SET_FWIMG;
	send_msg.return_value = 0;

	ret = secgpu_ipi_to_gpueb(send_msg);
	if (unlikely(ret)) {
		MTK_LOGE("[SECGPU] secgpu_ipi_to_gpueb fail (cmd id: %d)", send_msg.cmd_id);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	return PVRSRV_OK;
}

PVRSRV_ERROR MTKTDSetPowerParams(IMG_HANDLE hSysData, PVRSRV_TD_POWER_PARAMS *psTDPowerParams)
{
	if (g_secgpu_ipi_shared_mem_va == NULL) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	struct secgpu_ipi_data send_msg;
	MTK_RGX_LAYER_PARAMS *pPowerParam = (MTK_RGX_LAYER_PARAMS *) (g_secgpu_ipi_shared_mem_va + SECGPU_SHM_POWERPAR_OFFSET);
	PVRSRV_DEVICE_NODE *psDevNode = MTKGetRGXDevNode();
	if (!psDevNode) {
		MTK_LOGE("fail to get RGX device node");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *)psDevNode->pvDevice;
	if (!psDevInfo) {
		MTK_LOGE("fail to get RGX device info");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	int i;
	PVR_UNREFERENCED_PARAMETER(hSysData);

	pPowerParam->sPCAddr = psTDPowerParams->sPCAddr;
	pPowerParam->sBootRemapAddr = psTDPowerParams->sBootRemapAddr;
	pPowerParam->sCodeRemapAddr = psTDPowerParams->sCodeRemapAddr;
	pPowerParam->sDataRemapAddr = psTDPowerParams->sDataRemapAddr;
	pPowerParam->sDevFeatureCfg.ui64ErnsBrns = psDevInfo->sDevFeatureCfg.ui64ErnsBrns;
	pPowerParam->sDevFeatureCfg.ui64Features = psDevInfo->sDevFeatureCfg.ui64Features;
	for(i=0;i<RGX_FEATURE_WITH_VALUES_MAX_IDX;i++){
		pPowerParam->sDevFeatureCfg.ui32FeaturesValues[i] = psDevInfo->sDevFeatureCfg.ui32FeaturesValues[i];
	}
	pPowerParam->sDevFeatureCfg.ui32MAXDustCount = psDevInfo->sDevFeatureCfg.ui32MAXDMCount;

	send_msg.cmd_id = CMD_SECGPU_SET_POWER;
	send_msg.return_value = 0;
	int ret = secgpu_ipi_to_gpueb(send_msg);
	if (unlikely(ret)) {
		MTK_LOGE("[SECGPU] secgpu_ipi_to_gpueb fail (cmd id: %d)", send_msg.cmd_id);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	return PVRSRV_OK;
}

PVRSRV_ERROR MTKTDRGXStart(IMG_HANDLE hSysData)
{
	if (g_secgpu_ipi_shared_mem_va == NULL) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	struct secgpu_ipi_data send_msg;
	PVR_UNREFERENCED_PARAMETER(hSysData);

	send_msg.cmd_id = CMD_SECGPU_SET_START;
	send_msg.return_value = 0;
	int ret = secgpu_ipi_to_gpueb(send_msg);
	if (unlikely(ret)) {
		MTK_LOGE("[SECGPU] secgpu_ipi_to_gpueb fail (cmd id: %d)", send_msg.cmd_id);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	return PVRSRV_OK;
}

PVRSRV_ERROR MTKTDRGXStop(IMG_HANDLE hSysData)
{
	if (g_secgpu_ipi_shared_mem_va == NULL) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	struct secgpu_ipi_data send_msg;
	PVR_UNREFERENCED_PARAMETER(hSysData);

	send_msg.cmd_id = CMD_SECGPU_SET_STOP;
	send_msg.return_value = 0;
	int ret = secgpu_ipi_to_gpueb(send_msg);
	if (unlikely(ret)) {
		MTK_LOGE("[SECGPU] secgpu_ipi_to_gpueb fail (cmd id: %d)", send_msg.cmd_id);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	return PVRSRV_OK;
}

#endif /* defined(SUPPORT_TRUSTED_DEVICE) */
