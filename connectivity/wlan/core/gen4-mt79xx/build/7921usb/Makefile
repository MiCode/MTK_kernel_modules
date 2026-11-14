KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

_CONNAC_VER := 2_0
_MODULE_NAME := wlan_mt7961
_MODULE_NAME_PREALLOC := mtprealloc_mt7961
_MODULE_NAME_RESET := mtreset

GKI := y
WM_RAM := dtv
CONFIG_PROJECT_DEFCONFIG_DIR := dtv_main
USE_PREALLOC := y
USE_RESETKO := y

WLAN_CHIP_ID := 7961
MTK_COMBO_CHIP := MT7961

_HIF := usb
CONFIG_BUILD_RESETKO := y

$(info M=$(M))
$(info Kernel src=$(KERNEL_SRC))

include $(KERNEL_SRC)/$(M)/../../Makefile_dtv.bazel
