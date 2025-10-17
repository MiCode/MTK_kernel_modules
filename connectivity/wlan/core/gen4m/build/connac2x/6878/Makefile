KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

_CONNAC_VER := 2_0
_CONNAC_DENPENDENCY_SYMBOLS := wlan/adaptor/build/connac2x/Module.symvers
_MODULE_NAME := wlan_drv_gen4m_6878

$(info M=$(M))
$(info Kernel src=$(KERNEL_SRC))

include $(KERNEL_SRC)/$(M)/../../../Makefile
