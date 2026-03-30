# SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
#
# (C) COPYRIGHT 2012-2021 ARM Limited. All rights reserved.
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

ifeq ($(BUILD_RULE),OOT)
    ifeq ($(MTK_PLATFORM_VERSION),mt6983)
        MTK_GPU_VERSION := $(LOCAL_MTK_GPU_VERSION)
        MTK_PLATFORM := $(MTK_PLATFORM_VERSION)
        CONFIG_MALI_CSF_SUPPORT := y
        CONFIG_MALI_REAL_HW := y
        CONFIG_MALI_DEBUG := y
        CONFIG_MALI_MIDGARD_DVFS := y
        CONFIG_MALI_DEVFREQ := y
        CONFIG_MALI_MTK_DEVFREQ := y
        CONFIG_MALI_PMU_LP4 := y
        CONFIG_MALI_SCMI_ENABLE := y
    endif
endif

# make $(src) as absolute path if it is not already, by prefixing $(srctree)
# This is to prevent any build issue due to wrong path.

# MTK modify for Google build.sh
# src:=$(if $(patsubst /%,,$(src)),$(srctree)/$(src),$(src))
KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

#
# Prevent misuse when Kernel configurations are not present by default
# in out-of-tree builds
#
ifneq ($(CONFIG_ANDROID),n)
ifeq ($(CONFIG_GPU_TRACEPOINTS),n)
    $(error CONFIG_GPU_TRACEPOINTS must be set in Kernel configuration)
endif
endif

ifeq ($(CONFIG_DMA_SHARED_BUFFER),n)
    $(error CONFIG_DMA_SHARED_BUFFER must be set in Kernel configuration)
endif

ifeq ($(CONFIG_PM_DEVFREQ),n)
    $(error CONFIG_PM_DEVFREQ must be set in Kernel configuration)
endif

ifeq ($(CONFIG_DEVFREQ_THERMAL),n)
    $(error CONFIG_DEVFREQ_THERMAL must be set in Kernel configuration)
endif

ifeq ($(CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND),n)
    $(error CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND must be set in Kernel configuration)
endif

ifeq ($(CONFIG_FW_LOADER), n)
    $(error CONFIG_FW_LOADER must be set in Kernel configuration)
endif

ifeq ($(CONFIG_MALI_PRFCNT_SET_SELECT_VIA_DEBUG_FS), y)
    ifneq ($(CONFIG_DEBUG_FS), y)
        $(error CONFIG_MALI_PRFCNT_SET_SELECT_VIA_DEBUG_FS depends on CONFIG_DEBUG_FS to be set in Kernel configuration)
    endif
endif

ifeq ($(CONFIG_MALI_FENCE_DEBUG), y)
    ifneq ($(CONFIG_SYNC), y)
        ifneq ($(CONFIG_SYNC_FILE), y)
            $(error CONFIG_MALI_FENCE_DEBUG depends on CONFIG_SYNC || CONFIG_SYNC_FILE to be set in Kernel configuration)
        endif
    endif
endif

#
# Configurations
#

# *** MTK ***
$(info *GPU [mtk-Kbuild] src = $(src))
$(info *GPU [mtk-Kbuild] srctree = $(srctree))
include $(KO_CODE_PATH)/Kbuild-mtk-custom-env
include $(KO_CODE_PATH)/platform/mtk_platform_common/Kbuild
ccflags-y += -I$(KO_CODE_PATH)/include
subdir-ccflags-y += -I$(KO_CODE_PATH)/include

# Driver version string which is returned to userspace via an ioctl
MALI_RELEASE_NAME ?= '"r37p0-00dev0"'
# Set up defaults if not defined by build system
ifeq ($(CONFIG_MALI_DEBUG), y)
    MALI_UNIT_TEST = 1
    MALI_CUSTOMER_RELEASE ?= 0
else
    MALI_UNIT_TEST ?= 0
    MALI_CUSTOMER_RELEASE ?= 1
endif
MALI_COVERAGE ?= 0

CONFIG_MALI_PLATFORM_NAME ?= "devicetree"

# Kconfig passes in the name with quotes for in-tree builds - remove them.
MALI_PLATFORM_DIR := $(shell echo $(CONFIG_MALI_PLATFORM_NAME))

include $(KO_CODE_PATH)/platform/$(MALI_PLATFORM_DIR)/Kbuild

ifeq ($(CONFIG_MALI_CSF_SUPPORT),y)
    MALI_JIT_PRESSURE_LIMIT_BASE = 0
    MALI_USE_CSF = 1
else
    MALI_JIT_PRESSURE_LIMIT_BASE ?= 1
    MALI_USE_CSF ?= 0
endif


ifneq ($(CONFIG_MALI_KUTF), n)
    MALI_KERNEL_TEST_API ?= 1
else
    MALI_KERNEL_TEST_API ?= 0
endif

# Experimental features (corresponding -D definition should be appended to
# ccflags-y below, e.g. for MALI_EXPERIMENTAL_FEATURE,
# -DMALI_EXPERIMENTAL_FEATURE=$(MALI_EXPERIMENTAL_FEATURE) should be appended)
#
# Experimental features must default to disabled, e.g.:
# MALI_EXPERIMENTAL_FEATURE ?= 0
MALI_INCREMENTAL_RENDERING ?= 0

#
# ccflags
#
ccflags-y += \
    -DMALI_CUSTOMER_RELEASE=$(MALI_CUSTOMER_RELEASE) \
    -DMALI_USE_CSF=$(MALI_USE_CSF) \
    -DMALI_KERNEL_TEST_API=$(MALI_KERNEL_TEST_API) \
    -DMALI_UNIT_TEST=$(MALI_UNIT_TEST) \
    -DMALI_COVERAGE=$(MALI_COVERAGE) \
    -DMALI_RELEASE_NAME=$(MALI_RELEASE_NAME) \
    -DMALI_JIT_PRESSURE_LIMIT_BASE=$(MALI_JIT_PRESSURE_LIMIT_BASE) \
    -DMALI_INCREMENTAL_RENDERING=$(MALI_INCREMENTAL_RENDERING) \
    -DMALI_KBASE_BUILD \
#    -DMALI_PLATFORM_DIR=$(MALI_PLATFORM_DIR)


ifeq ($(KBUILD_EXTMOD),)
# in-tree
    ccflags-y +=-DMALI_KBASE_PLATFORM_PATH=../../$(src)/platform/$(CONFIG_MALI_PLATFORM_NAME)
else
# out-of-tree
    ccflags-y +=-DMALI_KBASE_PLATFORM_PATH=$(src)/platform/$(CONFIG_MALI_PLATFORM_NAME)
endif

ccflags-y += \
    -I$(srctree)/include/linux \
    -I$(srctree)/drivers/staging/android \
    -I$(KO_CODE_PATH) \
    -I$(KO_CODE_PATH)/platform/$(MALI_PLATFORM_DIR) \
    -I$(KO_CODE_PATH)/../../../base \
    -I$(KO_CODE_PATH)/../../../../include

subdir-ccflags-y += $(ccflags-y)

#
# Kernel Modules
#
#obj-$(CONFIG_MALI_MIDGARD) += mali_kbase.o

# MTK: Tell the Linux build system from which .o file to create the kernel module
ifneq ($(CONFIG_MTK_GPU_VERSION),"")
    # LD1.0
    obj-$(CONFIG_MALI_MIDGARD) += mali_kbase.o
else ifneq ($(wildcard $(KO_CODE_PATH)/platform/$(MTK_PLATFORM)),)
    # LD2.0
    obj-$(CONFIG_MALI_MIDGARD) += mali_kbase_$(MTK_PLATFORM).o
    mali_kbase_$(MTK_PLATFORM)-y := mali_kbase.o
endif

ifeq ($(CONFIG_MTK_TRUSTED_MEMORY_SUBSYSTEM), y)
    include $(srctree)/drivers/misc/mediatek/trusted_mem/Makefile.include
    ccflags-y += -I$(srctree)/drivers/staging/android/ion
endif

obj-$(CONFIG_MALI_ARBITRATION) += arbitration/
obj-$(CONFIG_MALI_KUTF)    += tests/

mali_kbase-y += \
    mali_kbase_cache_policy.o \
    mali_kbase_ccswe.o \
    mali_kbase_mem.o \
    mali_kbase_mem_pool_group.o \
    mali_kbase_native_mgm.o \
    mali_kbase_ctx_sched.o \
    mali_kbase_gpuprops.o \
    mali_kbase_pm.o \
    mali_kbase_config.o \
    mali_kbase_kinstr_prfcnt.o \
    mali_kbase_vinstr.o \
    mali_kbase_hwcnt.o \
    mali_kbase_hwcnt_gpu.o \
    mali_kbase_hwcnt_gpu_narrow.o \
    mali_kbase_hwcnt_types.o \
    mali_kbase_hwcnt_virtualizer.o \
    mali_kbase_hwcnt_watchdog_if_timer.o \
    mali_kbase_softjobs.o \
    mali_kbase_hw.o \
    mali_kbase_debug.o \
    mali_kbase_gpu_memory_debugfs.o \
    mali_kbase_mem_linux.o \
    mali_kbase_core_linux.o \
    mali_kbase_mem_profile_debugfs.o \
    mali_kbase_disjoint_events.o \
    mali_kbase_debug_mem_view.o \
    mali_kbase_smc.o \
    mali_kbase_mem_pool.o \
    mali_kbase_mem_pool_debugfs.o \
    mali_kbase_debugfs_helper.o \
    mali_kbase_strings.o \
    mali_kbase_as_fault_debugfs.o \
    mali_kbase_regs_history_debugfs.o \
    mali_kbase_dvfs_debugfs.o \
    mali_power_gpu_frequency_trace.o \
    mali_kbase_trace_gpu_mem.o \
    mali_kbase_pbha.o

mali_kbase-$(CONFIG_DEBUG_FS) += mali_kbase_pbha_debugfs.o

mali_kbase-$(CONFIG_MALI_CINSTR_GWT) += mali_kbase_gwt.o

mali_kbase-$(CONFIG_SYNC) += \
    mali_kbase_sync_android.o \
    mali_kbase_sync_common.o

mali_kbase-$(CONFIG_SYNC_FILE) += \
    mali_kbase_fence_ops.o \
    mali_kbase_sync_file.o \
    mali_kbase_sync_common.o

ifeq ($(CONFIG_MALI_CSF_SUPPORT),y)
    mali_kbase-y += \
        mali_kbase_hwcnt_backend_csf.o \
        mali_kbase_hwcnt_backend_csf_if_fw.o
else
    mali_kbase-y += \
        mali_kbase_jm.o \
        mali_kbase_hwcnt_backend_jm.o \
        mali_kbase_hwcnt_backend_jm_watchdog.o \
        mali_kbase_dummy_job_wa.o \
        mali_kbase_debug_job_fault.o \
        mali_kbase_event.o \
        mali_kbase_jd.o \
        mali_kbase_jd_debugfs.o \
        mali_kbase_js.o \
        mali_kbase_js_ctx_attr.o \
        mali_kbase_kinstr_jm.o

    mali_kbase-$(CONFIG_MALI_DMA_FENCE) += \
        mali_kbase_fence_ops.o \
        mali_kbase_dma_fence.o \
        mali_kbase_fence.o

    mali_kbase-$(CONFIG_SYNC_FILE) += \
        mali_kbase_fence_ops.o \
        mali_kbase_fence.o
endif


INCLUDE_SUBDIR = \
    $(KO_CODE_PATH)/context/Kbuild \
    $(KO_CODE_PATH)/debug/Kbuild \
    $(KO_CODE_PATH)/device/Kbuild \
    $(KO_CODE_PATH)/backend/gpu/Kbuild \
    $(KO_CODE_PATH)/mmu/Kbuild \
    $(KO_CODE_PATH)/tl/Kbuild \
    $(KO_CODE_PATH)/gpu/Kbuild \
    $(KO_CODE_PATH)/thirdparty/Kbuild \
#    $(KO_CODE_PATH)/platform/$(MALI_PLATFORM_DIR)/Kbuild

ifeq ($(CONFIG_MALI_CSF_SUPPORT),y)
    INCLUDE_SUBDIR += $(KO_CODE_PATH)/csf/Kbuild
endif

ifeq ($(CONFIG_MALI_ARBITER_SUPPORT),y)
    INCLUDE_SUBDIR += $(KO_CODE_PATH)/arbiter/Kbuild
endif

ifeq ($(CONFIG_MALI_DEVFREQ),y)
    ifeq ($(CONFIG_DEVFREQ_THERMAL),y)
        INCLUDE_SUBDIR += $(KO_CODE_PATH)/ipa/Kbuild
    endif
endif

ifeq ($(KBUILD_EXTMOD),)
# in-tree
    -include $(INCLUDE_SUBDIR)
else
# out-of-tree
    include $(INCLUDE_SUBDIR)
endif
