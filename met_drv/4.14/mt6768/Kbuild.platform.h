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
MET_SSPM_INC := $(srctree)/drivers/misc/mediatek/sspm/v1

################################################################################
# Feature Spec
# CPUPMU_VERSION: V8_0/V8_2
# EMI_SEDA_VERSION: SEDA2/SEDA3/SEDA3_5
# SPMTWAM_VERSION: ap/sspm
# SPMTWAM_IDLE_SIGNAL_SUPPORT: single/mutilple
################################################################################
CPUPMU_VERSION := V8_2
EMI_SEDA_VERSION := SEDA3
SPMTWAM_VERSION := sspm
SPMTWAM_IDLE_SIGNAL_SUPPORT := multiple

################################################################################
# Feature On/Off
################################################################################
