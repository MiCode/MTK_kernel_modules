###############################################################################
# Support GKI mixed build
ifeq ($(DEVICE_MODULES_PATH),)
DEVICE_MODULES_PATH = $(srctree)
else
LINUXINCLUDE := $(DEVCIE_MODULES_INCLUDE) $(LINUXINCLUDE)
TOP := $(srctree)/..
endif

# Necessary Check

$(info [wlan] CONNAC_VER=$(CONNAC_VER))
$(info [wlan] MODULE_NAME=$(MODULE_NAME))

ifneq ($(KERNEL_OUT),)
    ccflags-y += -imacros $(KERNEL_OUT)/include/generated/autoconf.h
endif

ifndef TOP
    TOP := $(srctree)/..
endif

# Force build fail on modpost warning
KBUILD_MODPOST_FAIL_ON_WARNINGS := y

ifeq ($(CONNAC_VER), 3_0)
    ccflags-y += -DCFG_ANDORID_CONNINFRA_SUPPORT=0
    ccflags-y += -DCFG_ANDORID_CONNINFRA_COREDUMP_SUPPORT=0
    ccflags-y += -DCFG_SUPPORT_CONNAC3X=1
    CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH := n
else ifeq ($(CONNAC_VER), 2_0)
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/include
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/conn_drv/connv2/debug_utility
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/conn_drv/connv2/debug_utility/include
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/conn_drv/connv2/debug_utility/connsyslog
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra/conn_drv/connv2/debug_utility/coredump
    ccflags-y += -DCFG_ANDORID_CONNINFRA_SUPPORT=1
    ccflags-y += -DCFG_ANDORID_CONNINFRA_COREDUMP_SUPPORT=1
    ccflags-y += -DCFG_SUPPORT_CONNAC2X=1
else
    ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/common/common_main/include
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/common/common_main/linux/include
    ccflags-y += -DCFG_ANDORID_CONNINFRA_SUPPORT=0
    ccflags-y += -DCFG_ANDORID_CONNINFRA_COREDUMP_SUPPORT=0
    ccflags-y += -DCFG_SUPPORT_CONNAC1X=1
endif

ifeq ($(CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH), y)
    ccflags-y += -DCFG_MTK_CONNSYS_DEDICATED_LOG_PATH
    ccflags-y += -I$(TOP)/vendor/mediatek/kernel_modules/connectivity/common/debug_utility
endif

ifeq ($(CONFIG_MTK_CONN_LTE_IDC_SUPPORT),y)
    ccflags-y += -DWMT_IDC_SUPPORT=1
else
    ccflags-y += -DWMT_IDC_SUPPORT=0
endif

ccflags-y += -D MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT

ccflags-y += -D CREATE_NODE_DYNAMIC=1

ifeq ($(CONFIG_WLAN_DRV_BUILD_IN),y)
    $(warning $(MODULE_NAME) build-in boot.img)
    obj-y += $(MODULE_NAME).o
else
    $(warning $(MODULE_NAME) is kernel module)
    obj-m += $(MODULE_NAME).o
endif

# Wi-Fi character device driver
$(MODULE_NAME)-objs += wmt_cdev_wifi.o

ifneq ($(CONNAC_VER), 1_0)
    $(MODULE_NAME)-objs += wifi_pwr_on.o
endif
