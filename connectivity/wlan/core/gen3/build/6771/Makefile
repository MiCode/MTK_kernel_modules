KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

_CONNAC_VER := gen3
_CONNAC_DENPENDENCY_SYMBOLS := wlan/adaptor/build/connac1x/Module.symvers
_CONNAC_DENPENDENCY_SYMBOLS += common/Module.symvers
_MODULE_NAME := wlan_drv_gen3

$(info M=$(M))
$(info Kernel src=$(KERNEL_SRC))

include $(KERNEL_SRC)/$(M)/../../Makefile
