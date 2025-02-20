# SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
#
# (C) COPYRIGHT 2019-2021 ARM Limited. All rights reserved.
#
# This program is free software and is provided to you under the terms of the
# GNU General Public License version 2 as published by the Free Software
# Foundation, and any use by you of this program is subject to the terms
# of such GNU license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you can access it online at
# http://www.gnu.org/licenses/gpl-2.0.html.
#
#

KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

ccflags-y = -I$(KO_CODE_PATH)/../../../gpu/arm/midgard/include -I$(DEVICE_MODULES_PATH)/include/
subdir-ccflags-y += $(ccflags-y)

ifeq ($(CONFIG_MALI_MEMORY_GROUP_MANAGER), y)
obj-m += mali_mgm_$(MTK_PLATFORM_VERSION)_r49.o
mali_mgm_$(MTK_PLATFORM_VERSION)_r49-y := memory_group_manager.o

ccflags-y += -DCONFIG_MALI_MTK_PREVENT_PRINTK_TOO_MUCH

ifeq ($(MTK_PLATFORM_VERSION),mt6989)
ccflags-y += -DCONFIG_MALI_MTK_MGMM
CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE := y
endif
ifeq ($(MTK_PLATFORM_VERSION),mt6991)
ccflags-y += -DCONFIG_MALI_MTK_MGMM
CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE := y
endif
ifeq ($(MTK_PLATFORM_VERSION),mt6899)
ccflags-y += -DCONFIG_MALI_MTK_MGMM
endif
ifeq ($(MTK_PLATFORM_VERSION),mt6768)
ccflags-y += -DCONFIG_MALI_MTK_MGMM
CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE := y
endif
ifeq ($(MTK_PLATFORM_VERSION),mt6877)
ccflags-y += -DCONFIG_MALI_MTK_MGMM
CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE := y
endif
ifeq ($(MTK_PLATFORM_VERSION),mt6893)
ccflags-y += -DCONFIG_MALI_MTK_MGMM
CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE := n
endif
ifeq ($(MTK_PLATFORM_VERSION),mt6833)
ccflags-y += -DCONFIG_MALI_MTK_MGMM
CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE := n
endif
ifeq ($(MTK_PLATFORM_VERSION),mt6853)
ccflags-y += -DCONFIG_MALI_MTK_MGMM
CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE := n
endif
ifeq ($(MTK_PLATFORM_VERSION),mt6781)
ccflags-y += -DCONFIG_MALI_MTK_MGMM
CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE := n
endif
ifeq ($(MTK_PLATFORM_VERSION),mt6899)
CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE := y
endif

endif

ifeq ($(CONFIG_MALI_MTK_SLC_ALL_CACHE_MODE), y)
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/dma-buf/heaps -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/slbc
ccflags-y += -DCONFIG_MALI_MTK_SLC_ALL_CACHE_MODE
endif
