ifeq ($(LINUX_SRC), )
	export KERNEL_SRC := /lib/modules/$(shell uname -r)/build
else
	export KERNEL_SRC := $(LINUX_SRC)
endif
#################### Configurations ####################
# Compile Options for bt driver configuration.
CONFIG_SUPPORT_BT_DL_WIFI_PATCH=y
CONFIG_SUPPORT_BT_DL_ZB_PATCH=n
CONFIG_SUPPORT_BLUEZ=n
CONFIG_SUPPORT_DVT=n
CONFIG_SUPPORT_HW_DVT=n
CONFIG_SUPPORT_MULTI_DEV_NODE=n
CONFIG_GKI_SUPPORT=y
CONFIG_SUPPORT_WAKEUP_SER=y
CONFIG_DISABLE_SYMBOL_GET_SET=y

ifeq ($(CONFIG_MP_WAKEUP_SOURCE_SYSFS_STAT), y)
    ccflags-y += -DCONFIG_MP_WAKEUP_SOURCE_SYSFS_STAT
endif

ifeq ($(CONFIG_GKI_SUPPORT), y)
ccflags-y += -DCFG_CHIP_RESET_KO_SUPPORT
ccflags-y += -I$(src)/../../wlan_sdio_reset.ko_intermediates/LINKED/reset/include/
ccflags-y += -I$(src)/../../wlan/core/gen4-mt79xx/reset/include/

RESET_DIR = $(shell pwd)/../../ETC/wlan_sdio_reset.ko_intermediates/LINKED/Module.symvers
$(info 'RESET_DIR=' $(RESET_DIR))
KBUILD_EXTRA_SYMBOLS += $(RESET_DIR)
endif

ifeq ($(CONFIG_DISABLE_SYMBOL_GET_SET), y)
    ccflags-y += -DCONFIG_DISABLE_SYMBOL_GET_SET
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

ifeq ($(CONFIG_SUPPORT_MULTI_DEV_NODE), y)
    ccflags-y += -DCFG_SUPPORT_MULTI_DEV_NODE=1
else
    ccflags-y += -DCFG_SUPPORT_MULTI_DEV_NODE=0
endif

ifeq ($(CONFIG_SUPPORT_BMR_RX_CLK), y)
    ccflags-y += -DCFG_SUPPORT_BMR_RX_CLK=1
    ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
else
    ccflags-y += -DCFG_SUPPORT_BMR_RX_CLK=0
endif

#################### Configurations ####################
# For chip interface, driver supports "usb", "sdio", "uart_tty", "uart_serdev" and "btif"
MTK_CHIP_IF := sdio

ifeq ($(MTK_CHIP_IF), sdio)
    MOD_NAME = btmtk_sdio_unify
    CFILES := sdio/btmtksdio.c btmtk_woble.c btmtk_buffer_mode.c btmtk_chip_reset.c
    ccflags-y += -DCHIP_IF_SDIO
    ccflags-y += -DSDIO_DEBUG=0
    ccflags-y += -I$(src)/include/sdio
else ifeq ($(MTK_CHIP_IF), usb)
    MOD_NAME = btmtk_usb_unify
    CFILES := usb/btmtkusb.c btmtk_woble.c btmtk_chip_reset.c
    ccflags-y += -DCHIP_IF_USB
    ccflags-y += -I$(src)/include/usb
else ifeq ($(MTK_CHIP_IF), uart_tty)
    MOD_NAME = btmtk_uart_unify
    CFILES := uart/btmtktty.c btmtk_woble.c btmtk_chip_reset.c
    ccflags-y += -DCHIP_IF_UART_TTY
    ccflags-y += -I$(src)/include/uart/tty
else ifeq ($(MTK_CHIP_IF), uart_serdev)
    MOD_NAME = btmtk_uart_unify
    ccflags-y += -DCHIP_IF_UART_SERDEV
    CFILES := uart/btmtkserdev.c
    ccflags-y += -I$(src)/include/uart/serdev
else
    MOD_NAME = btmtkbtif_unify
    CFILES := btif/btmtk_btif.c
    ccflags-y += -DCHIP_IF_BTIF
    ccflags-y += -I$(src)/include/btif
endif

CFILES += btmtk_main.c btmtk_fw_log.c

ccflags-y += -I$(src)/include/ -I$(KERNEL_SRC)/include/ -I$(KERNEL_SRC)/drivers/bluetooth

ccflags-y += -DANDROID_OS
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
all:
	make -C $(KERNEL_SRC) M=$(PWD) modules
clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean
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
	./util/checkpatch.pl -f btmtk_fw_log.c
	./util/checkpatch.pl -f btmtk_main.c
	./util/checkpatch.pl -f btmtk_buffer_mode.c
	./util/checkpatch.pl -f btmtk_woble.c
	./util/checkpatch.pl -f btmtk_chip_reset.c

