KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

_CONNAC_VER := 2_0
#_CONNAC_DENPENDENCY_SYMBOLS := wlan/adaptor/build/connac2x/Module.symvers
_MODULE_NAME := wlan_drv_gen4m_mt7903sdio
_MODULE_NAME_PREALLOC := wlan_drv_gen4m_mt7903sdio_prealloc
_MODULE_NAME_RESET := wlan_drv_gen4m_mt7903sdio_reset

GKI := y
WM_RAM := alps
USE_PREALLOC := y
USE_RESETKO := y

WLAN_CHIP_ID := 7926
MTK_COMBO_CHIP := MT7926

_HIF := sdio
CONFIG_BUILD_RESETKO := y

$(info M=$(M))
$(info Kernel src=$(KERNEL_SRC))

include $(KERNEL_SRC)/$(M)/../../Makefile.bazel
