// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>

/* RGX */
#include "interrupt_support.h"
#include "pvrsrv_device.h"
#include "rgxdevice.h"
#include "syscommon.h"
#include "physheap.h"
#if defined(SUPPORT_ION)
#include "ion_support.h"
#endif
/* MTK */
#include "mtk_mfgsys.h"
#include "sysinfo.h"

static RGX_TIMING_INFORMATION   gsRGXTimingInfo;
static RGX_DATA                 gsRGXData;
static PVRSRV_DEVICE_CONFIG     gsDevices[1];
static PHYS_HEAP_FUNCTIONS      gsPhysHeapFuncs;

#if defined(SUPPORT_TRUSTED_DEVICE)
static PHYS_HEAP_CONFIG         gsPhysHeapConfig[3];

extern int g_secgpu_fw_reservd_mem_pa;
extern int g_secgpu_fw_reservd_mem_size;

#define SECURE_FW_MEM_SIZE   (0x100000) /* 1MB */

#include <soc/mediatek/emi.h>
struct emimpu_region_t gpufw_rg_info;
void mfg_set_emi_mpu(phys_addr_t base, phys_addr_t size)
{
	gpufw_rg_info.rg_num = 28;
    gpufw_rg_info.start = base;
    gpufw_rg_info.end = base + size - 1;
}

#else
static PHYS_HEAP_CONFIG         gsPhysHeapConfig[1];
#endif

static struct platform_device   *gpsPVRCfgDev;

/* CPU to Device physcial address translation */
static void UMAPhysHeapCpuPAddrToDevPAddr(
	IMG_HANDLE hPrivData,
	IMG_UINT32 ui32NumOfAddr,
	IMG_DEV_PHYADDR *psDevPAddr,
	IMG_CPU_PHYADDR *psCpuPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);
	IMG_UINT32 ui32Idx;

	/* Optimise common case */
	psDevPAddr[0].uiAddr = psCpuPAddr[0].uiAddr;

	if (ui32NumOfAddr > 1) {
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
			psDevPAddr[ui32Idx].uiAddr = psCpuPAddr[ui32Idx].uiAddr;
	}
}

/* Device to CPU physcial address translation */
static void UMAPhysHeapDevPAddrToCpuPAddr(
	IMG_HANDLE hPrivData,
	IMG_UINT32 ui32NumOfAddr,
	IMG_CPU_PHYADDR *psCpuPAddr,
	IMG_DEV_PHYADDR *psDevPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);
	IMG_UINT32 ui32Idx;

	/* Optimise common case */
	psCpuPAddr[0].uiAddr = psDevPAddr[0].uiAddr;

	if (ui32NumOfAddr > 1) {
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
			psCpuPAddr[ui32Idx].uiAddr = psDevPAddr[ui32Idx].uiAddr;
	}
}

PVRSRV_ERROR SysDevInit(void *pvOSDevice, PVRSRV_DEVICE_CONFIG **ppsDevConfig)
{
	PVRSRV_ERROR err = PVRSRV_OK;
	
	int irq_res;
	struct resource *reg_res;

	gsPhysHeapFuncs.pfnCpuPAddrToDevPAddr = UMAPhysHeapCpuPAddrToDevPAddr;
	gsPhysHeapFuncs.pfnDevPAddrToCpuPAddr = UMAPhysHeapDevPAddrToCpuPAddr;

	gsPhysHeapConfig[0].uConfig.sUMA.pszPDumpMemspaceName = "SYSMEM";
	gsPhysHeapConfig[0].uConfig.sUMA.pszHeapName = "SYSMEM_UMA";
	gsPhysHeapConfig[0].eType = PHYS_HEAP_TYPE_UMA;
	gsPhysHeapConfig[0].uConfig.sUMA.psMemFuncs = &gsPhysHeapFuncs;
	gsPhysHeapConfig[0].uConfig.sUMA.hPrivData = (IMG_HANDLE)&gsDevices[0];
	gsPhysHeapConfig[0].ui32UsageFlags = PHYS_HEAP_USAGE_GPU_LOCAL;
#if defined(SUPPORT_DEDICATED_FW_MEMORY)
	MTK_LOGE("SUPPORT_DEDICATED_FW_MEMORY supported");
#else
	MTK_LOGE("SUPPORT_DEDICATED_FW_MEMORY NOT supported");
#endif
	
#if defined(SUPPORT_TRUSTED_DEVICE)
	int ret = secgpu_gpueb_init();
	MTK_LOGE("[SECGPU] init secgpu_gpueb_init (%d)", ret); //init ipi for secure gpu
	//init g_secgpu_fw_reservd_mem_pa
	if (unlikely(ret)) {
 		MTK_LOGE("[SECGPU] secgpu_gpueb_init fail");
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	{
		IMG_UINT64 ui64FwPrivateBase, ui64FwPrivateSize;
		IMG_UINT64 ui64GpuSecureBase, ui64GpuSecureSize;

		ui64FwPrivateSize = SECURE_FW_MEM_SIZE;
		ui64GpuSecureSize = g_secgpu_fw_reservd_mem_size - SECURE_FW_MEM_SIZE;

		ui64GpuSecureBase = g_secgpu_fw_reservd_mem_pa;
		ui64FwPrivateBase = ui64GpuSecureBase + ui64GpuSecureSize;

		/* GPU SECURE BUFFERS: TRUSTED */
		gsPhysHeapConfig[1].uConfig.sLMA.pszPDumpMemspaceName = "LMA";
		gsPhysHeapConfig[1].uConfig.sLMA.pszHeapName = "GPU_SECURE";
		gsPhysHeapConfig[1].eType = PHYS_HEAP_TYPE_LMA;
		gsPhysHeapConfig[1].uConfig.sLMA.psMemFuncs = &gsPhysHeapFuncs;
		gsPhysHeapConfig[1].ui32UsageFlags = PHYS_HEAP_USAGE_GPU_SECURE;
		gsPhysHeapConfig[1].uConfig.sLMA.sStartAddr.uiAddr = ui64GpuSecureBase;
		gsPhysHeapConfig[1].uConfig.sLMA.sCardBase.uiAddr = ui64GpuSecureBase;
		gsPhysHeapConfig[1].uConfig.sLMA.uiSize = ui64GpuSecureSize;
		gsPhysHeapConfig[1].uConfig.sLMA.hPrivData = NULL;

		/* Firmware Private heap: code & private Fw data: TRUSTED */
		gsPhysHeapConfig[2].uConfig.sLMA.pszPDumpMemspaceName = "LMA";
		gsPhysHeapConfig[2].uConfig.sLMA.pszHeapName = "FW_PRIVATE";
		gsPhysHeapConfig[2].eType = PHYS_HEAP_TYPE_LMA;
		gsPhysHeapConfig[2].uConfig.sLMA.psMemFuncs = &gsPhysHeapFuncs;
		gsPhysHeapConfig[2].ui32UsageFlags = PHYS_HEAP_USAGE_FW_PRIVATE;
		gsPhysHeapConfig[2].uConfig.sLMA.sStartAddr.uiAddr = ui64FwPrivateBase;
		gsPhysHeapConfig[2].uConfig.sLMA.sCardBase.uiAddr = ui64FwPrivateBase;
		gsPhysHeapConfig[2].uConfig.sLMA.uiSize = ui64FwPrivateSize;
		gsPhysHeapConfig[2].uConfig.sLMA.hPrivData = NULL;
	}

	MTK_LOGE("[SECGPU]: %s, Heap name: %s, base: 0x%llx, size: 0x%x\n", __func__,
			gsPhysHeapConfig[1].uConfig.sLMA.pszHeapName, gsPhysHeapConfig[1].uConfig.sLMA.sStartAddr.uiAddr , gsPhysHeapConfig[1].uConfig.sLMA.uiSize);
	MTK_LOGE("[SECGPU]: %s, Heap name: %s, base: 0x%llx, size: 0x%x\n", __func__,
			gsPhysHeapConfig[2].uConfig.sLMA.pszHeapName, gsPhysHeapConfig[2].uConfig.sLMA.sStartAddr.uiAddr , gsPhysHeapConfig[2].uConfig.sLMA.uiSize);
#endif // SUPPORT_TRUSTED_DEVICE

	gsDevices[0].pvOSDevice = pvOSDevice;
#if defined(SUPPORT_TRUSTED_DEVICE)
	gsDevices[0].pasPhysHeaps = gsPhysHeapConfig;

	gsDevices[0].ui32PhysHeapCount =
			sizeof(gsPhysHeapConfig) / sizeof(PHYS_HEAP_CONFIG);
#else
	gsDevices[0].pasPhysHeaps = &gsPhysHeapConfig[0];
	gsDevices[0].ui32PhysHeapCount = sizeof(gsPhysHeapConfig) / sizeof(PHYS_HEAP_CONFIG);
#endif

	/* set GPU init frequency */
	gsRGXTimingInfo.ui32CoreClockSpeed = RGX_HW_CORE_CLOCK_SPEED;

#if defined(MTK_PM_POLICY_AO)
	gsRGXTimingInfo.bEnableActivePM = false;
#else
	gsRGXTimingInfo.bEnableActivePM = true;
	gsRGXTimingInfo.ui32ActivePMLatencyms = SYS_RGX_ACTIVE_POWER_LATENCY_MS,
#endif /* MTK_PM_POLICY_AO */

#if defined(MTK_USE_HW_APM)
	gsRGXTimingInfo.bEnableRDPowIsland = true;
#else
	gsRGXTimingInfo.bEnableRDPowIsland = false;
#endif /* MTK_USE_HW_APM */

	/* set GPU PM info */
	gsRGXData.psRGXTimingInfo = &gsRGXTimingInfo;

	/* set RGX device */
	gsDevices[0].pszName = "RGX";
	gsDevices[0].pszVersion = NULL;

	gpsPVRCfgDev = to_platform_device((struct device *)pvOSDevice);

	/* setup IRQ from dts */
	irq_res = platform_get_irq(gpsPVRCfgDev, 0);
	if (irq_res >= 0) {
		gsDevices[0].ui32IRQ = irq_res;
		MTK_LOGE("[SECGPU]irq_res = 0x%llx", irq_res);
	} else {
		MTK_LOGE("irq_res = NULL");
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	/* setup register base from dts */
	reg_res = platform_get_resource(gpsPVRCfgDev, IORESOURCE_MEM, 0);
	if (reg_res) {
		gsDevices[0].sRegsCpuPBase.uiAddr = reg_res->start;
		gsDevices[0].ui32RegsSize = resource_size(reg_res);
		MTK_LOGE("[SECGPU]reg_res = 0x%llx, size = 0x%x", reg_res->start, resource_size(reg_res));
	} else {
		MTK_LOGE("reg_res = NULL");
		return PVRSRV_ERROR_INIT_FAILURE;
	}

#if defined(SUPPORT_ALT_REGBASE)
	gsDevices[0].sAltRegsGpuPBase.uiAddr = 0x7F000000;
#endif

	/* power management on HW system */
	gsDevices[0].pfnPrePowerState = MTKSysPrePowerState;
	gsDevices[0].pfnPostPowerState = MTKSysPostPowerState;

	/* clock frequency */
	gsDevices[0].pfnClockFreqGet = NULL;

	gsDevices[0].hDevData = &gsRGXData;
	gsDevices[0].eCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_NONE;
	gsDevices[0].eDefaultHeap = PVRSRV_PHYS_HEAP_GPU_LOCAL;
#if defined(SUPPORT_TRUSTED_DEVICE)
	/*  send FW image and FW boot time parameters to the trusted device. */
	gsDevices[0].pfnTDSendFWImage = MTKTDSendFWImage;

	/* send parameters needed in a power transition to the trusted device. */
	gsDevices[0].pfnTDSetPowerParams = MTKTDSetPowerParams;

	/*! Callbacks to ping the trusted device to securely run RGXStart/Stop() */
	gsDevices[0].pfnTDRGXStart = MTKTDRGXStart;
	gsDevices[0].pfnTDRGXStop = MTKTDRGXStop;

#endif /* defined(SUPPORT_TRUSTED_DEVICE) */

	/* Setup other system specific stuff */
#if defined(SUPPORT_ION)
	IonInit(NULL);
#endif

	gsDevices[0].pvOSDevice = pvOSDevice;
	*ppsDevConfig = &gsDevices[0];

	err = MTKRGXDeviceInit(gsDevices);

	return err;
}

void SysDevDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
#if defined(SUPPORT_ION)
	IonDeinit();
#endif

	MTKRGXDeviceDeInit(gsDevices);
	psDevConfig->pvOSDevice = NULL;
}

PVRSRV_ERROR SysInstallDeviceLISR(IMG_HANDLE hSysData,
				  IMG_UINT32 ui32IRQ,
				  const IMG_CHAR *pszName,
				  PFN_LISR pfnLISR,
				  void *pvData,
				  IMG_HANDLE *phLISRData)
{
	IMG_UINT32 ui32IRQFlags = SYS_IRQ_FLAG_TRIGGER_HIGH;

	PVR_UNREFERENCED_PARAMETER(hSysData);

#if defined(PVRSRV_GPUVIRT_MULTIDRV_MODEL)
	ui32IRQFlags |= SYS_IRQ_FLAG_SHARED;
#endif

	return OSInstallSystemLISR(phLISRData, ui32IRQ,
				   pszName, pfnLISR,
				   pvData,  ui32IRQFlags);
}

PVRSRV_ERROR SysUninstallDeviceLISR(IMG_HANDLE hLISRData)
{
	return OSUninstallSystemLISR(hLISRData);
}


PVRSRV_ERROR SysDebugInfo(PVRSRV_DEVICE_CONFIG *psDevConfig,
	DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf, void *pvDumpDebugFile)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);
	PVR_UNREFERENCED_PARAMETER(pfnDumpDebugPrintf);
	return PVRSRV_OK;
}
