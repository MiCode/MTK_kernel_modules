$(info [bt_drv] linux_v2/makefile start)
# Support GKI mixed build
ifeq ($(DEVICE_MODULES_PATH),)
DEVICE_MODULES_PATH = $(DEVICE_MODULES_PATH)
CONFIG_SUPPORT_DEVICE_NODE=n
CONFIG_SUPPORT_UARTDBG=n
else
CONFIG_SUPPORT_DEVICE_NODE=y
CONFIG_SUPPORT_UARTDBG=y
LINUXINCLUDE := $(DEVCIE_MODULES_INCLUDE) $(LINUXINCLUDE)
endif

CONN_ALPS_OUT_PATH := $(wildcard $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/))
CONNECTIVITY_OUT_PATH := $(strip $(if $(CONN_ALPS_OUT_PATH), $(CONN_ALPS_OUT_PATH), $(CONN_EAP_OUT_PATH)))

$(info Connectivity out path = $(CONNECTIVITY_OUT_PATH))

LOG_TAG := [BT_Drv][linux_v2]
ifneq ($(CONNECTIVITY_OUT_PATH),)
    MTK_PROJ_TYPE := sp
    ifneq ($(wildcard $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include),)
        include $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include
        extra_symbols := $(abspath $(OUT_DIR)/../vendor/mediatek/kernel_modules/connectivity/common/Module.symvers)
        extra_symbols += $(abspath $(OUT_DIR)/../vendor/mediatek/kernel_modules/connectivity/connfem/Module.symvers)
        extra_symbols += $(abspath $(OUT_DIR)/../vendor/mediatek/kernel_modules/connectivity/conninfra/Module.symvers)
        M_OPTION := $(M)
    else
        extra_symbols := $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/common/Module.symvers)
        extra_symbols += $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/connfem/Module.symvers)
        extra_symbols += $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/conninfra/Module.symvers)
        M_OPTION := $(PWD)
    endif
else
    MTK_PROJ_TYPE := ce
    M_OPTION := $(PWD)
endif

ifeq ($(LINUX_SRC), )
	export KERNEL_SRC := /lib/modules/$(shell uname -r)/build
else
	export KERNEL_SRC := $(LINUX_SRC)
endif
#################### Configurations ####################
# Compile Options for bt driver configuration.
CONFIG_SUPPORT_BT_DL_WIFI_PATCH=y
CONFIG_SUPPORT_BT_DL_ZB_PATCH=y
CONFIG_SUPPORT_BLUEZ=n
CONFIG_SUPPORT_DVT=n
CONFIG_SUPPORT_HW_DVT=n
CONFIG_SUPPORT_MULTI_DEV_NODE=n
BT_CONFIG_TRACING=n
CFG_SUPPORT_HOSTWAKE=n
CONFIG_SUPPORT_WAKEUP_SER=n
CONFIG_MTK_BT_DFD_DUMP_SUPPORT=y
CFG_SUPPORT_RHW_DBG_SOP=n
CFG_SUPPORT_RHW_DUMP=n
ifeq ($(CONFIG_MP_WAKEUP_SOURCE_SYSFS_STAT), y)
    ccflags-y += -DCONFIG_MP_WAKEUP_SOURCE_SYSFS_STAT
endif
ifeq ($(CONFIG_DISABLE_SYMBOL_GET_SET), y)
    ccflags-y += -DCONFIG_DISABLE_SYMBOL_GET_SET
endif

KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

ifndef TOP
    TOP := $(srctree)/..
endif

ifneq ($(TARGET_BUILD_VARIANT), user)
    ccflags-y += -DBUILD_QA_DBG=1
else
    ccflags-y += -DBUILD_QA_DBG=0
endif

ifeq ($(CONFIG_SUPPORT_BT_DL_WIFI_PATCH), y)
    ccflags-y += -DCFG_SUPPORT_BT_DL_WIFI_PATCH=1
else
    ccflags-y += -DCFG_SUPPORT_BT_DL_WIFI_PATCH=0
endif

ifeq ($(CONFIG_SUPPORT_BT_DL_ZB_PATCH), y)
    ccflags-y += -DCFG_SUPPORT_BT_DL_ZB_PATCH=1
else
    ccflags-y += -DCFG_SUPPORT_BT_DL_ZB_PATCH=0
endif

ifeq ($(CONFIG_SUPPORT_BLUEZ), y)
    ccflags-y += -DCFG_SUPPORT_BLUEZ=1
else
    ccflags-y += -DCFG_SUPPORT_BLUEZ=0
endif

ifeq ($(CONFIG_SUPPORT_HW_DVT), y)
    ccflags-y += -DCFG_SUPPORT_HW_DVT=1
else
    ccflags-y += -DCFG_SUPPORT_HW_DVT=0
endif

ifeq ($(SUPPORT_WAKEUP_IRQ), yes)
    ccflags-y += -DCFG_SUPPORT_WAKEUP_IRQ=1
else
    ccflags-y += -DCFG_SUPPORT_WAKEUP_IRQ=0
endif

ifeq ($(CONFIG_SUPPORT_WAKEUP_SER), y)
    ccflags-y += -DCFG_SUPPORT_WAKEUP_IRQ=1
else
    ccflags-y += -DCFG_SUPPORT_WAKEUP_IRQ=0
endif


ifeq ($(CONFIG_SUPPORT_DVT), y)
    ccflags-y += -DCFG_SUPPORT_DVT=1
else
    ccflags-y += -DCFG_SUPPORT_DVT=0
endif

ifneq ($(TARGET_BUILD_VARIANT), user)
    ccflags-y += -DSTPBTFWLOG_ENABLE=1
    ccflags-y += -DBUILD_QA_DBG=1
else
    ifeq ($(MTK_PROJ_TYPE), sp)
        ccflags-y += -DSTPBTFWLOG_ENABLE=1
    else
        ccflags-y += -DSTPBTFWLOG_ENABLE=0
    endif
    ccflags-y += -DBUILD_QA_DBG=0
endif

ifeq ($(CONFIG_GKI_SUPPORT), y)
    ccflags-y += -DCFG_GKI_SUPPORT=1
else
    ccflags-y += -DCFG_GKI_SUPPORT=0
endif

ifeq ($(CONFIG_SUPPORT_MULTI_DEV_NODE), y)
    ccflags-y += -DCFG_SUPPORT_MULTI_DEV_NODE=1
else
    ccflags-y += -DCFG_SUPPORT_MULTI_DEV_NODE=0
endif

ifeq ($(CONFIG_SUPPORT_LEAUDIO_CLK), y)
    ccflags-y += -DCFG_SUPPORT_LEAUDIO_CLK=1
    ifeq ($(CONFIG_SUPPORT_DEINT_IRQ), y)
        ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
        ccflags-y += -DCFG_SUPPORT_DEINT_IRQ=1
    else
        ccflags-y += -DCFG_SUPPORT_DEINT_IRQ=0
    endif
else
    ccflags-y += -DCFG_SUPPORT_LEAUDIO_CLK=0
endif

ifeq ($(CONFIG_ARCH_MEDIATEK),y)
    ccflags-y += -D BTMTK_PLAT_ALPS=1
else
    ccflags-y += -D BTMTK_PLAT_ALPS=0
endif

ifeq ($(CFG_SUPPORT_HOSTWAKE), y)
    ccflags-y += -DCFG_SUPPORT_HOSTWAKE=1
else
    ccflags-y += -DCFG_SUPPORT_HOSTWAKE=0
endif

ifeq ($(CONFIG_MTK_BT_DFD_DUMP_SUPPORT), y)
    ccflags-y += -DCFG_MTK_BT_DFD_DUMP_SUPPORT=1
else
    ccflags-y += -DCFG_MTK_BT_DFD_DUMP_SUPPORT=0
endif

ifeq ($(CFG_SUPPORT_RHW_DBG_SOP), y)
    ccflags-y += -DCFG_SUPPORT_RHW_DBG_SOP=1
else
    ccflags-y += -DCFG_SUPPORT_RHW_DBG_SOP=0
endif

ifeq ($(CFG_SUPPORT_RHW_DUMP), y)
    ccflags-y += -DCFG_SUPPORT_RHW_DUMP=1
else
    ccflags-y += -DCFG_SUPPORT_RHW_DUMP=0
endif

ccflags-y += -DDEBUG_DUMP_TIME
ccflags-y += -DBTMTK_ISOC_TEST=0

#################### Configurations ####################
# For chip interface, driver supports "usb", "sdio", "uart_tty", "uart_serdev" and "btif"
MTK_CHIP_IF := usb
ifeq ($(MTK_PROJ_TYPE), sp)
    MTK_CHIP_IF := uart_tty
    ccflags-y += -DANDROID_OS
else
    ccflags-y += -DLINUX_OS
endif

ifeq ($(MTK_CHIP_IF), sdio)
    MOD_NAME = btmtk_sdio_unify
    CFILES := sdio/btmtksdio.c btmtk_woble.c btmtk_chip_reset.c
    ccflags-y += -DCHIP_IF_SDIO
    ccflags-y += -DSDIO_DEBUG=0
    ccflags-y += -I$(KO_CODE_PATH)/include/sdio
else ifeq ($(MTK_CHIP_IF), usb)
    MOD_NAME = btmtk_usb_unify
    CFILES := usb/btmtkusb.c btmtk_woble.c btmtk_chip_reset.c
    ccflags-y += -DCHIP_IF_USB
    ccflags-y += -I$(KO_CODE_PATH)/include/usb
else ifeq ($(MTK_CHIP_IF), uart_tty)
    MOD_NAME = btmtk_uart_unify
    CFILES := uart/btmtktty.c btmtk_woble.c btmtk_chip_reset.c
    ifeq ($(MTK_PROJ_TYPE), ce)
        CFILES += proj/btmtk_proj_ce.c
    else
        CFILES += proj/btmtk_proj_sp.c proj/btmtk_proj_sp_debug.c btmtk_queue.c btmtk_char_dev.c
        ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/connectivity/power_throttling
        ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/gpu/drm/mediatek/mediatek_v2/
	ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/connectivity/common
	ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat
    endif
    ccflags-y += -DCHIP_IF_UART_TTY
    ccflags-y += -I$(KO_CODE_PATH)/include/uart/tty
else ifeq ($(MTK_CHIP_IF), uart_serdev)
    MOD_NAME = btmtk_uart_unify
    ccflags-y += -DCHIP_IF_UART_SERDEV
    CFILES := uart/btmtkserdev.c btmtk_woble.c btmtk_chip_reset.c
    ccflags-y += -I$(KO_CODE_PATH)/include/uart/serdev
else
    MOD_NAME = btmtk_btif_unify
    CFILES := btif/btmtk_btif.c
    ccflags-y += -DCHIP_IF_BTIF
    ccflags-y += -I$(KO_CODE_PATH)/include/btif
endif

ifeq ($(CONFIG_SUPPORT_DEVICE_NODE), y)
ccflags-y += -DUSE_DEVICE_NODE=1
ccflags-y += -DSLEEP_ENABLE=1
else
ccflags-y += -DUSE_DEVICE_NODE=0
ccflags-y += -DSLEEP_ENABLE=0
endif

ifeq ($(BT_CONFIG_TRACING),y)
    ccflags-y += -DBT_CONFIG_TRACING
endif

ifeq ($(CONFIG_SUPPORT_UARTDBG), y)
ccflags-y += -DCONFIG_SUPPORT_UARTDBG=1
else
ccflags-y += -DCONFIG_SUPPORT_UARTDBG=0
endif

CFILES += btmtk_main.c btmtk_fw_log.c btmtk_buffer_mode.c
CFILES += chip/btmtk_chip_7902.c chip/btmtk_chip_common.c chip/btmtk_chip_7961.c chip/btmtk_chip_7922.c chip/btmtk_chip_66xx.c chip/btmtk_chip_connac3.c chip/btmtk_chip_6639.c

ccflags-y += -I$(src)/include/ -I$(src)/
ccflags-y += -I$(src)/include/ -I$(KERNEL_SRC)/include/ -I$(KERNEL_SRC)/drivers/bluetooth -I$(src)/proj/include/ -I$(KO_CODE_PATH)/proj/include/ -I$(KO_CODE_PATH)/include/chip -I$(KO_CODE_PATH)/include/

ifeq ($(MTK_PROJ_TYPE), sp)
  $(info [BT_Drv] TOP = $(TOP))
  $(info [BT_Drv] KBUILD_EXTRA_SYMBOLS = $(KBUILD_EXTRA_SYMBOLS))
  #KBUILD_EXTRA_SYMBOLS := /mfs/mtkslt1121/mtk24223/CAS_REAL/alps-dev-s0_bsp-connac3-1210--2022_03_08_12_00/merged/out_krn/target/product/mgk_64_k510/obj/ETC/conninfra.ko_intermediates/LINKED/Module.symvers
  CONN_INFRA_SRC := $(TOP)/vendor/mediatek/kernel_modules/connectivity/conninfra
  CONN_FEM_SRC := $(TOP)/vendor/mediatek/kernel_modules/connectivity/connfem
  ccflags-y += -I$(CONN_INFRA_SRC)/include
  ccflags-y += -I$(CONN_INFRA_SRC)/conn_drv/connv3/debug_utility
  ccflags-y += -I$(CONN_INFRA_SRC)/conn_drv/connv3/debug_utility/include
  ccflags-y += -I$(CONN_INFRA_SRC)/conn_drv/connv3/debug_utility/connsyslog
  ccflags-y += -I$(CONN_INFRA_SRC)/conn_drv/connv3/debug_utility/coredump
  ccflags-y += -I$(CONN_FEM_SRC)/include
endif

ccflags-y += -DBTMTK_LOG_LVL=0
ccflags-y += -D_FORTIFY_SOURCE=2
ccflags-y += -Werror

$(MOD_NAME)-objs := $(CFILES:.c=.o)

obj-m += $(MOD_NAME).o

ifneq ($(TARGET_BUILD_VARIANT), user)
ccflags-y += -DBTMTK_DEBUG_SOP
endif

#VPATH = /opt/toolchains/gcc-linaro-aarch64-linux-gnu-4.9-2014.09_linux
#UART_MOD_NAME = btmtk_uart
#UART_CFILES := \
#	btmtk_uart_main.c
#$(UART_MOD_NAME)-objs := $(UART_CFILES:.c=.o)
###############################################################################
# Common
###############################################################################
#obj-m := $(UART_MOD_NAME).o
all: PRIVATE_LOG_TAG := $(LOG_TAG)
all: EXTRA_SYMBOLS += $(extra_symbols)
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M_OPTION) modules $(KBUILD_OPTIONS) LOG_TAG=$(PRIVATE_LOG_TAG) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)"

modules_install:
	$(MAKE) M=$(M_OPTION) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M_OPTION) clean

# Check coding style
# export IGNORE_CODING_STYLE_RULES := NEW_TYPEDEFS,LEADING_SPACE,CODE_INDENT,SUSPECT_CODE_INDENT
ccs:
	./util/checkpatch.pl -f ./sdio/btmtksdio.c
	./util/checkpatch.pl -f ./include/sdio/btmtk_sdio.h
	./util/checkpatch.pl -f ./include/btmtk_define.h
	./util/checkpatch.pl -f ./include/btmtk_drv.h
	./util/checkpatch.pl -f ./include/btmtk_chip_if.h
	./util/checkpatch.pl -f ./include/btmtk_main.h
	./util/checkpatch.pl -f ./include/btmtk_buffer_mode.h
	./util/checkpatch.pl -f ./include/uart/tty/btmtk_uart_tty.h
	./util/checkpatch.pl -f ./uart/btmtktty.c
	./util/checkpatch.pl -f ./include/btmtk_fw_log.h
	./util/checkpatch.pl -f ./include/btmtk_woble.h
	./util/checkpatch.pl -f ./include/uart/btmtk_uart.h
	./util/checkpatch.pl -f ./uart/btmtk_uart_main.c
	./util/checkpatch.pl -f ./include/usb/btmtk_usb.h
	./util/checkpatch.pl -f ./usb/btmtkusb.c
	./util/checkpatch.pl -f ./proj/btmtk_proj_ce.c
	./util/checkpatch.pl -f ./proj/btmtk_proj_sp.c
	./util/checkpatch.pl -f btmtk_fw_log.c
	./util/checkpatch.pl -f btmtk_main.c
	./util/checkpatch.pl -f btmtk_buffer_mode.c
	./util/checkpatch.pl -f btmtk_woble.c
	./util/checkpatch.pl -f btmtk_chip_reset.c
	./util/checkpatch.pl -f btmtk_queue.c
	./util/checkpatch.pl -f btmtk_char_dev.c
$(info [bt_drv] linux_v2/makefile end)
