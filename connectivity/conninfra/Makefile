###############################################################################
# Necessary Check

ifeq ($(AUTOCONF_H),)
    $(error AUTOCONF_H is not defined)
endif



ccflags-y += -imacros $(AUTOCONF_H)

#ifeq ($(CONFIG_MTK_COMBO_CHIP),)
#    $(error CONFIG_MTK_COMBO_CHIP not defined)
#endif

#ifeq ($(TARGET_BUILD_VARIANT),$(filter $(TARGET_BUILD_VARIANT),userdebug user))
#    ldflags-y += -s
#endif

# Force build fail on modpost warning
KBUILD_MODPOST_FAIL_ON_WARNINGS := y
###############################################################################

#ccflags-y += -D MTK_WCN_REMOVE_KERNEL_MODULE
ifeq ($(CONFIG_ARM64), y)
    ccflags-y += -D CONFIG_MTK_WCN_ARM64
endif

# Option for some ALPS specific feature, ex: AEE.
ccflags-y += -D CONNINFRA_PLAT_ALPS=1

#ifeq ($(CONFIG_MTK_CONN_LTE_IDC_SUPPORT),y)
#    ccflags-y += -D WMT_IDC_SUPPORT=1
#else
#    ccflags-y += -D WMT_IDC_SUPPORT=0
#endif

ccflags-y += -D MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT
ccflags-y += -D MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE=1

ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/clkbuf_v1
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/clkbuf_v1/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/eccci
ccflags-y += -I$(srctree)/drivers/misc/mediatek/eccci/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/eemcs
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include/mach
ccflags-y += -I$(srctree)/drivers/misc/mediatek/emi/submodule
ccflags-y += -I$(srctree)/drivers/misc/mediatek/emi/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/mmc/core
ccflags-y += -I$(srctree)/drivers/misc/mediatek/connectivity/common
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include/
###############################################################################


ccflags-y += -Werror
ccflags-y += -Wno-error=format
ccflags-y += -Wno-error=format-extra-args

###############################################################################
MODULE_NAME := conninfra
ifeq ($(CONFIG_WLAN_DRV_BUILD_IN),y)
$(warning $(MODULE_NAME) build-in boot.img)
obj-y += $(MODULE_NAME).o
else
$(warning $(MODULE_NAME) is kernel module)
obj-m += $(MODULE_NAME).o
endif

###############################################################################
# common_main
###############################################################################
ccflags-y += -I$(src)/include
ccflags-y += -I$(src)/base/include
ccflags-y += -I$(src)/core/include
ccflags-y += -I$(src)/conf/include
ccflags-y += -I$(src)/platform/include
ccflags-y += -I$(src)/debug_utility
ccflags-y += -I$(src)/debug_utility/include
ccflags-y += -I$(src)/debug_utility/connsyslog
ccflags-y += -I$(src)/debug_utility/connsyslog/platform/include
ccflags-y += -I$(src)/debug_utility/coredump
ccflags-y += -I$(src)/debug_utility/coredump/platform/include
ccflags-y += -I$(src)/debug_utility/step/include

# By Plaftfrom
ccflags-y += -I$(src)/platform/$(MTK_PLATFORM)/include


ifneq ($(TARGET_BUILD_VARIANT), user)
    ccflags-y += -D CONNINFRA_DBG_SUPPORT=1
else
    ccflags-y += -D CONNINFRA_DBG_SUPPORT=0
endif

# STEP: (Support Connac)
# MTK eng/userdebug/user load: Support
# Customer eng/userdebug load: Support
# Customer user load: Not support

ifeq ($(wildcard vendor/mediatek/proprietary/external/aee_config_internal/init.aee.mtk.system.rc),)
	ccflags-y += -D CFG_CONNINFRA_STEP
else
	ifneq ($(TARGET_BUILD_VARIANT),user)
		ccflags-y += -D CFG_CONNINFRA_STEP
	endif
endif

#ifeq ($(findstring evb, $(MTK_PROJECT)), evb)
#ccflags-y += -D CFG_WMT_EVB
#endif

#ifneq ($(filter "CONSYS_%",$(CONFIG_MTK_COMBO_CHIP)),)
#$(MODULE_NAME)-objs += common_main/platform/$(MTK_PLATFORM).o
#endif

$(MODULE_NAME)-objs += base/ring.o
$(MODULE_NAME)-objs += base/osal.o
#$(MODULE_NAME)-objs += base/log.o
$(MODULE_NAME)-objs += base/msg_thread.o
$(MODULE_NAME)-objs += core/conninfra_core.o
$(MODULE_NAME)-objs += src/conninfra_dev.o
$(MODULE_NAME)-objs += src/conninfra.o
$(MODULE_NAME)-objs += conf/conninfra_conf.o
$(MODULE_NAME)-objs += platform/consys_hw.o
$(MODULE_NAME)-objs += platform/clock_mng.o
$(MODULE_NAME)-objs += platform/pmic_mng.o
$(MODULE_NAME)-objs += platform/emi_mng.o
$(MODULE_NAME)-objs += platform/consys_reg_mng.o

$(MODULE_NAME)-objs += debug_utility/conninfra_dbg.o

# By Plaftfrom
$(MODULE_NAME)-objs += platform/$(MTK_PLATFORM)/$(MTK_PLATFORM).o
$(MODULE_NAME)-objs += platform/$(MTK_PLATFORM)/$(MTK_PLATFORM)_pmic.o
$(MODULE_NAME)-objs += platform/$(MTK_PLATFORM)/$(MTK_PLATFORM)_emi.o
$(MODULE_NAME)-objs += platform/$(MTK_PLATFORM)/$(MTK_PLATFORM)_consys_reg.o
$(MODULE_NAME)-objs += platform/$(MTK_PLATFORM)/$(MTK_PLATFORM)_pos.o
#$(MODULE_NAME)-objs += platform/$(MTK_PLATFORM)/$(MTK_PLATFORM).o
#$(MODULE_NAME)-objs += platform/$(MTK_PLATFORM)/$(MTK_PLATFORM)_pmic.o
#$(MODULE_NAME)-objs += platform/$(MTK_PLATFORM)/$(MTK_PLATFORM)_clock.o

# STEP
$(MODULE_NAME)-objs += debug_utility/step/conninfra_step.o
$(MODULE_NAME)-objs += debug_utility/step/conninfra_step_parse.o
$(MODULE_NAME)-objs += debug_utility/step/conninfra_step_parse_act.o

# Debug utility
$(MODULE_NAME)-objs += debug_utility/connsyslog/ring_emi.o
$(MODULE_NAME)-objs += debug_utility/connsyslog/connsyslog.o
$(MODULE_NAME)-objs += debug_utility/connsyslog/platform/$(MTK_PLATFORM)/$(MTK_PLATFORM).o
$(MODULE_NAME)-objs += debug_utility/coredump/connsys_coredump.o
$(MODULE_NAME)-objs += debug_utility/coredump/conndump_netlink.o
$(MODULE_NAME)-objs += debug_utility/coredump/platform/$(MTK_PLATFORM)/$(MTK_PLATFORM).o

###############################################################################
# test
###############################################################################
ifneq ($(TARGET_BUILD_VARIANT), user)
ccflags-y += -D CFG_CONNINFRA_UT_SUPPORT

ccflags-y += -I$(src)/test/include
$(MODULE_NAME)-objs += test/conninfra_core_test.o
$(MODULE_NAME)-objs += test/conf_test.o
$(MODULE_NAME)-objs += test/cal_test.o
$(MODULE_NAME)-objs += test/msg_evt_test.o
$(MODULE_NAME)-objs += test/chip_rst_test.o
$(MODULE_NAME)-objs += test/conninfra_test.o
$(MODULE_NAME)-objs += test/mailbox_test.o
$(MODULE_NAME)-objs += test/connsyslog_test.o
$(MODULE_NAME)-objs += test/conninfra_step_test.o
$(MODULE_NAME)-objs += test/conninfra_step_test_util.o
$(MODULE_NAME)-objs += test/dump_test.o
endif

