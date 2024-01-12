###############################################################################
# Necessary Check
###############################################################################
# Support GKI mixed build
ifeq ($(DEVICE_MODULES_PATH),)
DEVICE_MODULES_PATH = $(srctree)
else
LINUXINCLUDE := $(DEVCIE_MODULES_INCLUDE) $(LINUXINCLUDE)
TOP := $(srctree)/..
endif

ifneq ($(KERNEL_OUT),)
    ccflags-y += -imacros $(KERNEL_OUT)/include/generated/autoconf.h
endif

ifndef TOP
    TOP := $(srctree)/..
endif

# Force build fail on modpost warning
KBUILD_MODPOST_FAIL_ON_WARNINGS := y


###############################################################################
# GCC Options
###############################################################################
ccflags-y += -Wall
ccflags-y += -Werror


###############################################################################
# Compile Options
###############################################################################
ifneq ($(TARGET_BUILD_VARIANT), user)
    ccflags-y += -D CONNFEM_DBG=1
else
    ccflags-y += -D CONNFEM_DBG=0
endif


###############################################################################
# Include Paths
###############################################################################
KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

ccflags-y += -I$(KO_CODE_PATH)/include


###############################################################################
# ConnFem Module
###############################################################################
MODULE_NAME := connfem
obj-m += $(MODULE_NAME).o

$(MODULE_NAME)-objs += connfem_module.o
$(MODULE_NAME)-objs += connfem_api.o
$(MODULE_NAME)-objs += connfem_container.o
$(MODULE_NAME)-objs += connfem_dt_parser.o
$(MODULE_NAME)-objs += connfem_epaelna.o
$(MODULE_NAME)-objs += connfem_subsys_bt.o
$(MODULE_NAME)-objs += connfem_subsys_wifi.o
$(MODULE_NAME)-objs += connfem_subsys_common.o
$(MODULE_NAME)-objs += connfem_cfg.o

INTERNAL_PATH := $(wildcard $(TOP)/vendor/mediatek/internal/connfem_enable)

ifneq ($(INTERNAL_PATH),)
    $(info ConnFem: MTK internal load)
    $(MODULE_NAME)-objs += connfem_internal.o
else
    $(info ConnFem: Run time check load)
endif


###############################################################################
# Test
###############################################################################
CONNFEM_TEST_ENABLED = no

ifeq ($(CONNFEM_TEST_ENABLED), yes)
    ccflags-y += -D CONNFEM_TEST_ENABLED=1
    $(MODULE_NAME)-objs += connfem_test.o
else
    ccflags-y += -D CONNFEM_TEST_ENABLED=0
endif
