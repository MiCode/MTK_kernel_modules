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
MET_VCOREDVFS_INC := $(srctree)/drivers/misc/mediatek/base/power/include/
MET_PTPOD_INC := $(srctree)/drivers/misc/mediatek/base/power/cpufreq_v2/src/mach/$(MTK_PLATFORM)/

################################################################################
# Feature Spec
################################################################################
SPMTWAM_VERSION := ap
SPMTWAM_IDLE_SIGNAL_SUPPORT := single

################################################################################
# Feature On/Off
################################################################################
FEATURE_SSPM_EMI := n

# ONDIEMET
FEATURE_ONDIEMET := n

# VCOREDVFS API VERSION 
VCOREDVFS_OLD_VER := y
