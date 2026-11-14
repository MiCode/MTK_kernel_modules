KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

_CONNAC_VER := 2_0
#_CONNAC_DENPENDENCY_SYMBOLS := wlan/adaptor/build/connac2x/Module.symvers
_MODULE_NAME := wlan_drv_gen4m_mt7921pcie
_MODULE_NAME_PREALLOC := wlan_drv_gen4m_mt7921pcie_prealloc
_MODULE_NAME_RESET :=

GKI := y
WM_RAM := alps
USE_PREALLOC := y
USE_RESETKO := n

WLAN_CHIP_ID := 7921
MTK_COMBO_CHIP := MT7961

_HIF := pcie
CONFIG_BUILD_RESETKO := n

$(info M=$(M))
$(info Kernel src=$(KERNEL_SRC))

include $(KERNEL_SRC)/$(M)/../../Makefile.bazel
