###############################################################################
# Support GKI mixed build
ifeq ($(DEVICE_MODULES_PATH),)
DEVICE_MODULES_PATH = $(srctree)
else
LINUXINCLUDE := $(DEVCIE_MODULES_INCLUDE) $(LINUXINCLUDE)
endif

# Necessary Check

ifneq ($(KERNEL_OUT),)
    ccflags-y += -imacros $(KERNEL_OUT)/include/generated/autoconf.h
endif

ifndef TOP
    TOP := $(srctree)/..
endif

# Force build fail on modpost warning
KBUILD_MODPOST_FAIL_ON_WARNINGS := y

###############################################################################
# Option for some ALPS specific feature, ex: AEE.
ifeq ($(CONFIG_ARCH_MEDIATEK),y)
ccflags-y += -D CONNINFRA_PLAT_ALPS=1
ccflags-y += -D MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE=1
else
ccflags-y += -D CONNINFRA_PLAT_ALPS=0
ccflags-y += -D MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE=0
endif

ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/base/power/include/clkbuf_v1
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/clkbuf/src/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/connectivity/common
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/connectivity/power_throttling
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/pmic/include/
ccflags-y += -I$(DEVICE_MODULES_PATH)/include/linux/soc/mediatek/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/gpu/drm/mediatek/mediatek_v2
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/memory/mediatek/

###############################################################################
ccflags-y += -Werror
ccflags-y += -Wno-error=format
ccflags-y += -Wno-error=format-extra-args

###############################################################################
KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

MODULE_NAME := conninfra
ifeq ($(CONFIG_WLAN_DRV_BUILD_IN),y)
$(warning $(MODULE_NAME) build-in boot.img)
obj-y += $(MODULE_NAME).o
PATH_TO_CONNINFRA_DRV := $(srctree)/$(src)
else
$(warning $(MODULE_NAME) is kernel module)
obj-m += $(MODULE_NAME).o
PATH_TO_CONNINFRA_DRV := $(KO_CODE_PATH)
endif

$(info [conninfra_drv] $$PATH_TO_CONNINFRA_DRV is [${PATH_TO_CONNINFRA_DRV}])
$(info [conninfra_drv] $$KO_CODE_PATH is [${KO_CODE_PATH}])
$(info [conninfra_drv] $$KBUILD_EXTRA_SYMBOLS is [${KBUILD_EXTRA_SYMBOLS}])

ifeq ($(CFG_CONNINFRA_COCLOCK_SUPPORT),y)
    $(info [conninfra_drv][Kbuild] co-clock flag=1)
    PATH_TO_MD_FSM ?= $(PATH_TO_CONNINFRA_DRV)/../../wwan/tmi3
    $(info [conninfra_drv] $$PATH_TO_MD_FSM is [${PATH_TO_MD_FSM}])
    ifneq ($(wildcard $(PATH_TO_MD_FSM)),)
        ccflags-y += -I$(PATH_TO_MD_FSM)
        ccflags-y += -I$(PATH_TO_MD_FSM)/common
    else
        $(info [conninfra_drv] $$PATH_TO_MD_FSM not found)
    endif
else
    $(info [conninfra_drv] $$CFG_CONNINFRA_COCLOCK_SUPPORT not support)
endif

###############################################################################
# Common_main
###############################################################################
ccflags-y += -I$(KO_CODE_PATH)/include
ccflags-y += -I$(KO_CODE_PATH)/base/include
ccflags-y += -I$(KO_CODE_PATH)/conf/include

# adaptor
ccflags-y += -I$(KO_CODE_PATH)/adaptor/connsyslog
ccflags-y += -I$(KO_CODE_PATH)/adaptor/coredump
ccflags-y += -I$(KO_CODE_PATH)/adaptor/include

# connv2
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/src
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/core/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/drv_init/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/debug_utility
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/debug_utility/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/debug_utility/connsyslog
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/debug_utility/connsyslog/platform/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/debug_utility/coredump
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/debug_utility/coredump/platform/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/debug_utility/metlog

# connv3
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/src
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/core/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/debug_utility
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/debug_utility/include

# By Plaftfrom
ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6885),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6885),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6885/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6885/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6893),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6893),)
$(warning $(MODULE_NAME) build mt6893)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6893/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6893/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6877),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6877),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6877/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6877/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6886),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6886),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6886/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6886/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6897),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6897),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6897/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6897/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6983),y)
# V2
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6983),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6983/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6983/include/CODA
endif
#V3
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6983),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/mt6983/include
endif
#V3 combo chip
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6639),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/mt6639/include
endif

endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6879),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6879),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6879/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6879/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6895),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6895),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6895/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6895/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6985),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6985),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6985/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6985/include/CODA
endif
#V3
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6985),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/mt6985/include
endif
#V3 combo chip
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6639),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/mt6639/include
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6989),y)
#V2
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6989),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6989/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6989/include/CODA
endif
#V3
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6989),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/mt6989/include
endif
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6376),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/mt6376/include
endif
#V3 combo chip
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6639),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/mt6639/include
endif
#Add mt6653 for de-risk
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6653),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/mt6653/include
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6878),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6878),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6878/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6878/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6991),y)
#V2
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6991),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6991/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6991/include/CODA
endif
#V3
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6991),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/mt6991/include
endif
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6376),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/mt6376/include
endif
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6653),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv3/platform/mt6653/include
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6899),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6899),)
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6899/include
ccflags-y += -I$(KO_CODE_PATH)/conn_drv/connv2/platform/mt6899/include/CODA
endif
endif

ifneq ($(TARGET_BUILD_VARIANT), user)
    ccflags-y += -D CONNINFRA_DBG_SUPPORT=1
else
    ccflags-y += -D CONNINFRA_DBG_SUPPORT=0
endif

# Build mode option
ifeq ($(TARGET_BUILD_VARIANT),eng)
    ccflags-y += -D CONNINFRA_PLAT_BUILD_MODE=1
else ifeq ($(TARGET_BUILD_VARIANT),userdebug)
    ccflags-y += -D CONNINFRA_PLAT_BUILD_MODE=2
else ifeq ($(TARGET_BUILD_VARIANT),user)
    ccflags-y += -D CONNINFRA_PLAT_BUILD_MODE=3
else
    $(info invalid $$TARGET_BUILD_VARIANT[${TARGET_BUILD_VARIANT}])
    ccflags-y += -D CONNINFRA_PLAT_BUILD_MODE=0
endif

$(MODULE_NAME)-objs += base/ring.o
$(MODULE_NAME)-objs += base/osal.o
$(MODULE_NAME)-objs += base/osal_dbg.o
$(MODULE_NAME)-objs += base/msg_thread.o
$(MODULE_NAME)-objs += conf/conninfra_conf.o

# adaptor
$(MODULE_NAME)-objs += adaptor/conninfra_dev.o
$(MODULE_NAME)-objs += adaptor/conn_kern_adaptor.o
$(MODULE_NAME)-objs += adaptor/connsyslog/fw_log_mcu.o
$(MODULE_NAME)-objs += adaptor/connsyslog/connsyslog_to_user.o
$(MODULE_NAME)-objs += adaptor/coredump/conndump_netlink.o
# Check internal and external project
ifneq ($(wildcard $(TOP)/vendor/mediatek/internal/connfem_enable),)
    $(info ConnInfra: MTK internal load)
    $(MODULE_NAME)-objs += adaptor/connadp_internal.o
else
    $(info ConnInfra: Customer load)
endif

############################
# connv2 core
############################
$(MODULE_NAME)-objs += conn_drv/connv2/core/conninfra_core.o
$(MODULE_NAME)-objs += conn_drv/connv2/src/conninfra.o
$(MODULE_NAME)-objs += conn_drv/connv2/src/connv2_drv.o

$(MODULE_NAME)-objs += conn_drv/connv2/platform/consys_hw.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/consys_hw_plat_data.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/clock_mng.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/pmic_mng.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/emi_mng.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/consys_reg_mng.o
$(MODULE_NAME)-objs += conn_drv/connv2/debug_utility/conninfra_dbg.o

############################
# connv3
############################
$(MODULE_NAME)-objs += conn_drv/connv3/src/connv3.o
$(MODULE_NAME)-objs += conn_drv/connv3/src/connv3_drv.o
$(MODULE_NAME)-objs += conn_drv/connv3/core/connv3_core.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/connv3_hw.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/connv3_hw_plat_data.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/connv3_pmic_mng.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/connv3_pinctrl_mng.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/connv3_hw_dbg.o
$(MODULE_NAME)-objs += conn_drv/connv3/debug_utility/connsyslog/connv3_mcu_log.o
$(MODULE_NAME)-objs += conn_drv/connv3/debug_utility/coredump/connv3_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv3/debug_utility/coredump/connv3_dump_mng.o

# By Plaftfrom
ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6885),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6885),)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6885/mt6885.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6885/mt6885_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6885/mt6885_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6885/mt6885_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6885/mt6885_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6885/mt6885_coredump.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6893),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6893),)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6893/mt6893.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6893/mt6893_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6893/mt6893_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6893/mt6893_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6893/mt6893_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6893/mt6893_coredump.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6877),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6877),)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6877/mt6877.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6877/mt6877_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6877/mt6877_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6877/mt6877_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6877/mt6877_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6877/mt6877_coredump.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6886),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6886),)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_ops.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_soc.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_emi_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_consys_reg_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_pos_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_coredump_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6886/mt6886_debug_gen.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6897),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6897),)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_ops.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_soc.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_emi_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_consys_reg_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_pos_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_coredump_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6897/mt6897_debug_gen.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6983),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6983),)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_ops.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_soc.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_emi_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_consys_reg_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_pos_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_coredump_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6983/mt6983_debug_gen.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6879),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6879),)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6879/mt6879.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6879/mt6879_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6879/mt6879_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6879/mt6879_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6879/mt6879_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6879/mt6879_pos_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6879/mt6879_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6879/mt6879_debug_gen.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6895),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6895),)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6895/mt6895.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6895/mt6895_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6895/mt6895_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6895/mt6895_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6895/mt6895_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6895/mt6895_pos_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6895/mt6895_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6895/mt6895_debug_gen.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6985),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6985),)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_pos_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_debug_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_ops.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_soc.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_consys_reg_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6985/mt6985_coredump_atf.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6989),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6989),)
$(info building conninfra mt6989)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_pos_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_debug_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_ops.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_soc.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_consys_reg_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6989/mt6989_coredump_atf.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6878),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6878),)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6878/mt6878.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6878/mt6878_ops.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6878/mt6878_soc.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6878/mt6878_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6878/mt6878_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6878/mt6878_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6878/mt6878_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6878/mt6878_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6878/mt6878_pos_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6878/mt6878_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6878/mt6878_debug_gen.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6991),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6991),)
$(info building conninfra mt6991)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6991/mt6991.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6991/mt6991_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6991/mt6991_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6991/mt6991_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6991/mt6991_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6991/mt6991_pos_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6991/mt6991_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6991/mt6991_debug_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6991/mt6991_ops.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6991/mt6991_soc.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6899),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv2/platform/mt6899),)
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6899/mt6899.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6899/mt6899_ops.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6899/mt6899_soc.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6899/mt6899_atf.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6899/mt6899_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6899/mt6899_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6899/mt6899_consys_reg.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6899/mt6899_pos.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6899/mt6899_pos_gen.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6899/mt6899_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv2/platform/mt6899/mt6899_debug_gen.o
endif
endif

# Debug utility
$(MODULE_NAME)-objs += conn_drv/connv2/debug_utility/connsyslog/ring_emi.o
$(MODULE_NAME)-objs += conn_drv/connv2/debug_utility/connsyslog/connsyslog.o
$(MODULE_NAME)-objs += conn_drv/connv2/debug_utility/coredump/connsys_coredump.o
$(MODULE_NAME)-objs += conn_drv/connv2/debug_utility/coredump/coredump_mng.o
$(MODULE_NAME)-objs += conn_drv/connv2/debug_utility/metlog/metlog.o

# Drv init
$(MODULE_NAME)-objs += conn_drv/connv2/drv_init/bluetooth_drv_init.o
$(MODULE_NAME)-objs += conn_drv/connv2/drv_init/conn_drv_init.o
$(MODULE_NAME)-objs += conn_drv/connv2/drv_init/fm_drv_init.o
$(MODULE_NAME)-objs += conn_drv/connv2/drv_init/gps_drv_init.o
$(MODULE_NAME)-objs += conn_drv/connv2/drv_init/wlan_drv_init.o



###############################################################################
# connv3 platform
###############################################################################
ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6983),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6983),)

$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6983/mt6983.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6983/mt6983_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6983/mt6983_pinctrl.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6639/mt6639_dbg.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6985),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6985),)

$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6985/mt6985.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6985/mt6985_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6985/mt6985_pinctrl.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6639/mt6639_dbg.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6989),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6989),)
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6989/mt6989.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6989/mt6989_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6989/mt6989_pinctrl.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6639/mt6639_dbg.o
# add mt6653 for de-risk plan
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6989/mt6989_mt6653.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6653/mt6653_dbg.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6991),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/conn_drv/connv3/platform/mt6991),)
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6991/mt6991.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6991/mt6991_pmic.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6991/mt6991_pinctrl.o
$(MODULE_NAME)-objs += conn_drv/connv3/platform/mt6653/mt6653_dbg.o
endif
endif


###############################################################################
# Test
###############################################################################
# Enable conninfra_test and connv3_test node
# BUILD_TEST_MODE := yes

ifeq ($(BUILD_TEST_MODE), yes)
ccflags-y += -D CFG_CONNINFRA_UT_SUPPORT

# connv2
ccflags-y += -I$(KO_CODE_PATH)/test/connv2/include
$(MODULE_NAME)-objs += test/connv2/conninfra_core_test.o
$(MODULE_NAME)-objs += test/connv2/conf_test.o
$(MODULE_NAME)-objs += test/connv2/cal_test.o
$(MODULE_NAME)-objs += test/connv2/msg_evt_test.o
$(MODULE_NAME)-objs += test/connv2/chip_rst_test.o
$(MODULE_NAME)-objs += test/connv2/conninfra_test.o
$(MODULE_NAME)-objs += test/connv2/connsyslog_test.o
$(MODULE_NAME)-objs += test/connv2/dump_test.o
$(MODULE_NAME)-objs += test/connv2/connv2_pos_test.o

# v3
ccflags-y += -I$(KO_CODE_PATH)/test/connv3/include
$(MODULE_NAME)-objs += test/connv3/connv3_test.o
$(MODULE_NAME)-objs += test/connv3/connv3_dump_test.o
endif
