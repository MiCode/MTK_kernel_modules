###############################################################################
# Necessary Check

ifneq ($(KERNEL_OUT),)
    ccflags-y += -imacros $(KERNEL_OUT)/include/generated/autoconf.h
endif

# Force build fail on modpost warning
KBUILD_MODPOST_FAIL_ON_WARNINGS := y

###############################################################################
# Option for some ALPS specific feature, ex: AEE.
ccflags-y += -D CONNINFRA_PLAT_ALPS=1
ccflags-y += -D MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE=1

ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/clkbuf_v1
ccflags-y += -I$(srctree)/drivers/misc/mediatek/clkbuf/src/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/connectivity/common
ccflags-y += -I$(srctree)/drivers/misc/mediatek/connectivity/power_throttling
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include/
ccflags-y += -I$(srctree)/include/linux/soc/mediatek/
ccflags-y += -I$(srctree)/drivers/gpu/drm/mediatek/mediatek_v2
ccflags-y += -I$(srctree)/drivers/memory/mediatek/

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


###############################################################################
# Common_main
###############################################################################
ccflags-y += -I$(KO_CODE_PATH)/include
ccflags-y += -I$(KO_CODE_PATH)/base/include
ccflags-y += -I$(KO_CODE_PATH)/core/include
ccflags-y += -I$(KO_CODE_PATH)/conf/include
ccflags-y += -I$(KO_CODE_PATH)/drv_init/include
ccflags-y += -I$(KO_CODE_PATH)/platform/include
ccflags-y += -I$(KO_CODE_PATH)/debug_utility
ccflags-y += -I$(KO_CODE_PATH)/debug_utility/include
ccflags-y += -I$(KO_CODE_PATH)/debug_utility/connsyslog
ccflags-y += -I$(KO_CODE_PATH)/debug_utility/connsyslog/platform/include
ccflags-y += -I$(KO_CODE_PATH)/debug_utility/coredump
ccflags-y += -I$(KO_CODE_PATH)/debug_utility/coredump/platform/include
ccflags-y += -I$(KO_CODE_PATH)/debug_utility/metlog

# By Plaftfrom
ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6885),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6885),)
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6885/include
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6885/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6893),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6893),)
$(warning $(MODULE_NAME) build mt6893)
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6893/include
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6893/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6877),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6877),)
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6877/include
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6877/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6983),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6983),)
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6983/include
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6983/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6879),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6879),)
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6879/include
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6879/include/CODA
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6895),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6895),)
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6895/include
ccflags-y += -I$(KO_CODE_PATH)/platform/mt6895/include/CODA
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
$(MODULE_NAME)-objs += base/msg_thread.o
$(MODULE_NAME)-objs += core/conninfra_core.o
$(MODULE_NAME)-objs += src/conninfra_dev.o
$(MODULE_NAME)-objs += src/conninfra.o
$(MODULE_NAME)-objs += conf/conninfra_conf.o
$(MODULE_NAME)-objs += platform/consys_hw.o
$(MODULE_NAME)-objs += platform/consys_hw_plat_data.o
$(MODULE_NAME)-objs += platform/clock_mng.o
$(MODULE_NAME)-objs += platform/pmic_mng.o
$(MODULE_NAME)-objs += platform/emi_mng.o
$(MODULE_NAME)-objs += platform/consys_reg_mng.o
$(MODULE_NAME)-objs += platform/coredump_mng.o
$(MODULE_NAME)-objs += debug_utility/conninfra_dbg.o

# By Plaftfrom
ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6885),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6885),)
$(MODULE_NAME)-objs += platform/mt6885/mt6885.o
$(MODULE_NAME)-objs += platform/mt6885/mt6885_pmic.o
$(MODULE_NAME)-objs += platform/mt6885/mt6885_emi.o
$(MODULE_NAME)-objs += platform/mt6885/mt6885_consys_reg.o
$(MODULE_NAME)-objs += platform/mt6885/mt6885_pos.o
$(MODULE_NAME)-objs += platform/mt6885/mt6885_coredump.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6893),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6893),)
$(MODULE_NAME)-objs += platform/mt6893/mt6893.o
$(MODULE_NAME)-objs += platform/mt6893/mt6893_pmic.o
$(MODULE_NAME)-objs += platform/mt6893/mt6893_emi.o
$(MODULE_NAME)-objs += platform/mt6893/mt6893_consys_reg.o
$(MODULE_NAME)-objs += platform/mt6893/mt6893_pos.o
$(MODULE_NAME)-objs += platform/mt6893/mt6893_coredump.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6877),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6877),)
$(MODULE_NAME)-objs += platform/mt6877/mt6877.o
$(MODULE_NAME)-objs += platform/mt6877/mt6877_pmic.o
$(MODULE_NAME)-objs += platform/mt6877/mt6877_emi.o
$(MODULE_NAME)-objs += platform/mt6877/mt6877_consys_reg.o
$(MODULE_NAME)-objs += platform/mt6877/mt6877_pos.o
$(MODULE_NAME)-objs += platform/mt6877/mt6877_coredump.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6983),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6983),)
$(MODULE_NAME)-objs += platform/mt6983/mt6983.o
$(MODULE_NAME)-objs += platform/mt6983/mt6983_pmic.o
$(MODULE_NAME)-objs += platform/mt6983/mt6983_emi.o
$(MODULE_NAME)-objs += platform/mt6983/mt6983_consys_reg.o
$(MODULE_NAME)-objs += platform/mt6983/mt6983_pos.o
$(MODULE_NAME)-objs += platform/mt6983/mt6983_pos_gen.o
$(MODULE_NAME)-objs += platform/mt6983/mt6983_coredump.o
$(MODULE_NAME)-objs += platform/mt6983/mt6983_debug_gen.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6879),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6879),)
$(MODULE_NAME)-objs += platform/mt6879/mt6879.o
$(MODULE_NAME)-objs += platform/mt6879/mt6879_pmic.o
$(MODULE_NAME)-objs += platform/mt6879/mt6879_emi.o
$(MODULE_NAME)-objs += platform/mt6879/mt6879_consys_reg.o
$(MODULE_NAME)-objs += platform/mt6879/mt6879_pos.o
$(MODULE_NAME)-objs += platform/mt6879/mt6879_pos_gen.o
$(MODULE_NAME)-objs += platform/mt6879/mt6879_coredump.o
$(MODULE_NAME)-objs += platform/mt6879/mt6879_debug_gen.o
endif
endif

ifeq ($(CONFIG_MTK_COMBO_CHIP_CONSYS_6895),y)
ifneq ($(wildcard $(PATH_TO_CONNINFRA_DRV)/platform/mt6895),)
$(MODULE_NAME)-objs += platform/mt6895/mt6895.o
$(MODULE_NAME)-objs += platform/mt6895/mt6895_pmic.o
$(MODULE_NAME)-objs += platform/mt6895/mt6895_emi.o
$(MODULE_NAME)-objs += platform/mt6895/mt6895_consys_reg.o
$(MODULE_NAME)-objs += platform/mt6895/mt6895_pos.o
$(MODULE_NAME)-objs += platform/mt6895/mt6895_pos_gen.o
$(MODULE_NAME)-objs += platform/mt6895/mt6895_coredump.o
$(MODULE_NAME)-objs += platform/mt6895/mt6895_debug_gen.o
endif
endif

# Debug utility
$(MODULE_NAME)-objs += debug_utility/connsyslog/ring_emi.o
$(MODULE_NAME)-objs += debug_utility/connsyslog/connsyslog.o
$(MODULE_NAME)-objs += debug_utility/connsyslog/fw_log_mcu.o
$(MODULE_NAME)-objs += debug_utility/connsyslog/fw_log_wifi_mcu.o
$(MODULE_NAME)-objs += debug_utility/connsyslog/fw_log_bt_mcu.o
$(MODULE_NAME)-objs += debug_utility/coredump/connsys_coredump.o
$(MODULE_NAME)-objs += debug_utility/coredump/conndump_netlink.o
$(MODULE_NAME)-objs += debug_utility/metlog/metlog.o

# Drv init
$(MODULE_NAME)-objs += drv_init/bluetooth_drv_init.o
$(MODULE_NAME)-objs += drv_init/conn_drv_init.o
$(MODULE_NAME)-objs += drv_init/fm_drv_init.o
$(MODULE_NAME)-objs += drv_init/gps_drv_init.o
$(MODULE_NAME)-objs += drv_init/wlan_drv_init.o

###############################################################################
# Test
###############################################################################
ifneq ($(TARGET_BUILD_VARIANT), user)
ccflags-y += -D CFG_CONNINFRA_UT_SUPPORT

ccflags-y += -I$(KO_CODE_PATH)/test/include
$(MODULE_NAME)-objs += test/conninfra_core_test.o
$(MODULE_NAME)-objs += test/conf_test.o
$(MODULE_NAME)-objs += test/cal_test.o
$(MODULE_NAME)-objs += test/msg_evt_test.o
$(MODULE_NAME)-objs += test/chip_rst_test.o
$(MODULE_NAME)-objs += test/conninfra_test.o
$(MODULE_NAME)-objs += test/connsyslog_test.o
$(MODULE_NAME)-objs += test/dump_test.o
endif
