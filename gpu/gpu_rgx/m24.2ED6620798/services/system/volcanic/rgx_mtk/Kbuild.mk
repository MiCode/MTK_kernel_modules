#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

PVRSRVKM_NAME = $(PVRSRV_MODNAME)

ccflags-y += \
	-Wno-error \

(PVRSRVKM_NAME)-y += \
	services/system/common/env/linux/interrupt_support.o \
	services/system/$(PVR_ARCH)/common/env/linux/dma_support.o \
	services/system/$(PVR_ARCH)/common/vmm_type_stub.o \
	services/server/common/vmm_pvz_client.o \
	services/server/common/vmm_pvz_server.o \
	services/server/common/vz_vmm_pvz.o \
	services/server/common/vz_vmm_vm.o \
	services/system/$(PVR_ARCH)/$(PVR_SYSTEM)/ion_support.o \
	services/system/$(PVR_ARCH)/$(PVR_SYSTEM)/sysconfig.o \
	services/system/$(PVR_ARCH)/$(PVR_SYSTEM)/$(MTK_PLATFORM)/mtk_mfgsys.o
$(info [GPU_DEBUG] srctree = $(srctree))
$(info [GPU_DEBUG] DEVICE_MODULES_PATH = $(DEVICE_MODULES_PATH))
ccflags-y += \
	-I$(TOP)/services/system/$(PVR_ARCH)/$(PVR_SYSTEM) \
	-I$(TOP)/services/system/$(PVR_ARCH)/$(PVR_SYSTEM)/$(MTK_PLATFORM) \
	-I$(TOP)/services/system/$(PVR_ARCH)/common/env/linux \
	-I$(TOP)/services/linux/include \
	-I$(TOP)/kernel/drivers/staging/imgtec \
	-I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat \
	-I$(DEVICE_MODULES_PATH)/drivers/staging/android/ion \
	-I$(DEVICE_MODULES_PATH)/drivers/staging/android/ion/mtk \
    -I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/ged/ \
    -I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/hal/ \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/ged/include \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/mt-plat \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/ged/ \
	-I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat/ \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/hal/ \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpu_bm \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpufreq/ \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpufreq/v2 \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpufreq/v2/include \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpufreq/v2/ \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpufreq/$(MTK_PLATFORM)
