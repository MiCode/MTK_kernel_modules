#export KERNEL_DIR := /lib/modules/$(shell uname -r)/build

###############################################################################
# apollo system build
###############################################################################
include $(LINUX_ROOT)/linux_core/driver/ko/target/target.mak


ifeq "$(UTOPIA)" "true"
KERNEL_DIR := $(UTOPIA_KERNEL_ROOT)/$(subst linux-,,$(KERNEL_VER))
KERNEL_SRC := $(UTOPIA_KERNEL_ROOT)/$(subst linux-,,$(KERNEL_VER))
else
KERNEL_DIR=$(KERNEL_OBJ_ROOT)/$(KERNEL_VER)/$(KERNEL_CONFIG)_modules
KERNEL_SRC=$(KERNEL_ROOT)/$(KERNEL_VER)
endif
CURRENT_DIR=$(PWD)/../../../linux_mts/ko_modules/bt_driver_linux_v2
#KERNEL_DIR=/proj/mtk94020/Capy_buzzard_0429/out/mediatek_linux/output/mediatek/mt5895_cn/rel/obj/kernel/linux_core/kernel/4.9/mt5895_android_smp_mod_defconfig_modules
#KERNEL_SRC=/proj/mtk94020/Capy_buzzard_0429/kernel/zodiac/4.9
#CURRENT_DIR=$(PWD)


#################### Configurations ####################
# Compile Options for bt driver configuration.
CONFIG_SUPPORT_BT_DL_WIFI_PATCH=y
CONFIG_SUPPORT_BLUEZ=n
CONFIG_SUPPORT_DVT=n
CONFIG_SUPPORT_HW_DVT=n
CONFIG_SUPPORT_MULTI_DEV_NODE=n

ifeq ($(CONFIG_SUPPORT_BT_DL_WIFI_PATCH), y)
    ccflags-y += -DCFG_SUPPORT_BT_DL_WIFI_PATCH=1
else
    ccflags-y += -DCFG_SUPPORT_BT_DL_WIFI_PATCH=0
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
    ccflags-y += -DCFG_SUPPORT_WAKEUP_IRQ
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

#################### Configurations ####################
# For chip interface, driver supports "usb", "sdio", "uart_tty", "uart_serdev" and "btif"
MTK_CHIP_IF := usb

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

ccflags-y += -Werror

$(MOD_NAME)-objs := $(CFILES:.c=.o)

obj-m += $(MOD_NAME).o


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
	$(warning ======================================================)
	$(warning Make MTK BT Driver Unify, btmtk_usb_unify.ko)
	$(warning KERNEL_DIR=$(KERNEL_DIR))
	$(warning CURRENT_DIR=$(CURRENT_DIR))
	$(warning CC=$(CC))
	$(warning CROSS_COMPILE=$(CROSS_COMPILE))
	$(warning TARGET_IC=$(TARGET_IC))
	$(warning KERNEL_VER=$(KERNEL_VER))
	$(warning KERNEL_CONFIG=$(KERNEL_CONFIG))
	$(warning OBJ_ROOT=$(OBJ_ROOT))
	$(warning OUTPUT_ROOT=$(OUTPUT_ROOT))
	$(warning ccflags-y = $(ccflags-y))
	$(warning ======================================================)
ifeq "$(UTOPIA)" "true"
	make -C $(KERNEL_DIR) M=$(CURRENT_DIR)  modules  CROSS_COMPILE=$(KERNEL_CROSS_COMPILE)
	if [ -d $(OUTPUT_ROOT)/kernel_modules ]; then \
		cp -f $(CURRENT_DIR)/btmtk_usb_unify.ko $(OUTPUT_ROOT)/kernel_modules/btmtk_usb_unify.ko; \
	fi
else
	make -C $(KERNEL_DIR) M=$(CURRENT_DIR) modules
	if [ -d $(OUTPUT_ROOT)/kernel_modules ]; then \
		cp -f $(OBJ_ROOT)/linux_mts/ko_modules/bt_driver_linux_v2/btmtk_usb_unify.ko $(OUTPUT_ROOT)/kernel_modules/btmtk_usb_unify.ko; \
	fi
endif

clean:
	$(warning ======================================================)
	$(warning Clean MTK BT Driver Unify, btmtk_usb_unify.ko)
	$(warning KERNEL_DIR=$(KERNEL_DIR))
	$(warning CURRENT_DIR=$(CURRENT_DIR))
	$(warning OBJ_ROOT=$(OBJ_ROOT))
	$(warning OUTPUT_ROOT=$(OUTPUT_ROOT))
	$(warning ======================================================)
	if [ -d $(OUTPUT_ROOT)/kernel_modules ]; then \
		rm -f $(OUTPUT_ROOT)/kernel_modules/btmtk_usb_unify.ko; \
	fi
	if [ -d $(KERNEL_DIR) ]; then \
		make -C $(KERNEL_DIR) M=$(CURRENT_DIR) clean; \
	fi
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

