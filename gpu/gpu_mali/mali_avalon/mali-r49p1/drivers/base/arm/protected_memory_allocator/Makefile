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

ccflags-y = -I$(KO_CODE_PATH)/../../../gpu/arm/midgard/include \
            -I$(srctree)/drivers/dma-buf/heaps \
            -I$(srctree)/drivers/misc/mediatek/trusted_mem/public
subdir-ccflags-y += $(ccflags-y)

ifeq ($(CONFIG_MALI_PROTECTED_MEMORY_ALLOCATOR), y)
ifeq ($(MTK_PLATFORM_VERSION), mt6989)
CONFIG_MALI_MTK_GPU_IOMMU := y
CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP := n
endif
ifeq ($(MTK_PLATFORM_VERSION), mt6991)
CONFIG_MALI_MTK_GPU_IOMMU := y
CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP := y
CONFIG_MALI_MTK_GHPM_STAGE1_ENABLE := y
endif
ifeq ($(MTK_PLATFORM_VERSION), mt6899)
CONFIG_MALI_MTK_GPU_IOMMU := n
CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP := n
endif
ifeq ($(MTK_PLATFORM_VERSION), mt6768)
CONFIG_MALI_MTK_GPU_IOMMU := n
CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP := n
endif
ifeq ($(MTK_PLATFORM_VERSION), mt6877)
CONFIG_MALI_MTK_GPU_IOMMU := n
CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP := n
endif
ifeq ($(MTK_PLATFORM_VERSION), mt6893)
CONFIG_MALI_MTK_GPU_IOMMU := n
CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP := n
endif
ifeq ($(MTK_PLATFORM_VERSION), mt6833)
CONFIG_MALI_MTK_GPU_IOMMU := n
CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP := n
endif
ifeq ($(MTK_PLATFORM_VERSION), mt6853)
CONFIG_MALI_MTK_GPU_IOMMU := n
CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP := n
endif
ifeq ($(MTK_PLATFORM_VERSION), mt6781)
CONFIG_MALI_MTK_GPU_IOMMU := n
CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP := n
endif
CONFIG_MALI_MTK_GPU_IOMMU ?= n
CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP ?= n
CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP_2MB ?= n

ifeq ($(CONFIG_MALI_MTK_GPU_IOMMU), y)
ccflags-y += -DCONFIG_MALI_MTK_GPU_IOMMU
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpufreq
ifeq ($(CONFIG_MALI_MTK_GHPM_STAGE1_ENABLE), y)
ccflags-y += -DCONFIG_MALI_MTK_GHPM_STAGE1_ENABLE
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpueb/include
endif
endif

ifeq ($(CONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP), y)
ccflags-y += -DCONFIG_MALI_MTK_GPU_PMA_PAGE_HEAP
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/trusted_mem/public \
             -I$(DEVICE_MODULES_PATH)/drivers/dma-buf/heaps \
             -Wno-unused-function -Wno-unused-variable
endif

ccflags-y += -DCONFIG_MALI_MTK_GPU_PROTECTED_MEMORY_SUPPORT
obj-m += mali_prot_alloc_$(MTK_PLATFORM_VERSION)_r49.o
mali_prot_alloc_$(MTK_PLATFORM_VERSION)_r49-y := protected_memory_allocator.o
endif
