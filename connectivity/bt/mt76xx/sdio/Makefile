###############################################################################
# Bluetooth character device driver

###############################################################################
# Necessary Check

ifeq ($(AUTOCONF_H),)
    $(error AUTOCONF_H is not defined)
endif

CONFIG_CHIP_RESET_KO_SUPPORT=n

#
# mtreset
#
ifeq ($(CONFIG_CHIP_RESET_KO_SUPPORT), y)
ccflags-y += -DCFG_SUPPORT_CHIP_RESET_KO=1
ccflags-y += -I$(src)/../../../wlan/core/gen4-mt7663/reset/include
else
ccflags-y += -DCFG_SUPPORT_CHIP_RESET_KO=0
endif

ccflags-y += -imacros $(AUTOCONF_H)
ccflags-y += -DCONFIG_MP_WAKEUP_SOURCE_SYSFS_STAT

MODULE_NAME := btmtksdio
obj-m += $(MODULE_NAME).o

#ccflags-y += -D CREATE_NODE_DYNAMIC=1
ccflags-y += -DANDROID_OS

$(MODULE_NAME)-objs += btmtk_sdio.o \
                       btmtk_main.o


ifneq ($(TARGET_BUILD_VARIANT), user)
	ccflags-y += -DCFG_ENABLE_DEBUG_WRITE=1
else
	ccflags-y += -DCFG_ENABLE_DEBUG_WRITE=0
endif