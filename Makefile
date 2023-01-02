###############################################################################
# Necessary Check

ifneq ($(KERNEL_OUT),)
    ccflags-y += -imacros $(KERNEL_OUT)/include/generated/autoconf.h
endif


# Force build fail on modpost warning
KBUILD_MODPOST_FAIL_ON_WARNINGS := y

ccflags-y += \
    -I$(srctree)/drivers/misc/mediatek/include/mt-plat \
    -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/common/common_main/include \
    -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/common/common_main/linux/include

ifneq ($(CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH),)
ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/common/debug_utility
endif

ifeq ($(CONFIG_MTK_CONN_LTE_IDC_SUPPORT),y)
    ccflags-y += -D WMT_IDC_SUPPORT=1
else
    ccflags-y += -D WMT_IDC_SUPPORT=0
endif

ccflags-y += -D MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT

ccflags-y += -D CREATE_NODE_DYNAMIC=1

MODULE_NAME := wmt_chrdev_wifi
ifeq ($(CONFIG_WLAN_DRV_BUILD_IN),y)
$(warning $(MODULE_NAME) build-in boot.img)
obj-y += $(MODULE_NAME).o
else
$(warning $(MODULE_NAME) is kernel module)
obj-m += $(MODULE_NAME).o
endif

# Wi-Fi character device driver
$(MODULE_NAME)-objs += wmt_cdev_wifi.o
ifneq ($(CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH),)
$(MODULE_NAME)-objs += fw_log_wifi.o
endif

