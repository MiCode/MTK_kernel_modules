################################################################################
# Copyright (C) 2019 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
################################################################################

################################################################################
# Include Path
################################################################################
MET_VCOREDVFS_INC := $(srctree)/drivers/misc/mediatek/base/power/include/vcorefs_v3
MET_PTPOD_INC := $(srctree)/drivers/misc/mediatek/base/power/cpufreq_v1/src/mach/$(MTK_PLATFORM)/
GPUFREQ_INC := $(srctree)/drivers/misc/mediatek/base/power/include/gpufreq_v1

################################################################################
# Feature Spec
# CPUPMU_VERSION: V8_0/V8_2
# EMI_SEDA_VERSION: SEDA2/SEDA3/SEDA3_5/SEDA3_6
# SPMTWAM_VERSION: ap/sspm
# SPMTWAM_IDLE_SIGNAL_SUPPORT: single/multiple
# GPU_STALL_CNT_TYPE := single/multiple
# SSPM_VERSION: v1/v2
################################################################################
CPUPMU_VERSION := V8_2
EMI_SEDA_VERSION := SEDA3_6
SPMTWAM_VERSION := sspm
SPMTWAM_IDLE_SIGNAL_SUPPORT := multiple
GPU_STALL_CNT_TYPE := single
SSPM_VERSION := v2
DYNAMIC_ALLOC_ODM_BUF_SIZE := 0x800000

################################################################################
# Feature On/Off
################################################################################
FEATURE_EVENT_POWER := n
FEATURE_SPMTWAM := n
FEATURE_ONDIEMET := y
