KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

_CONNAC_VER := 2_0
_MODULE_NAME := wlan_drv_gen4_mt7961

$(info M=$(M))
$(info Kernel src=$(KERNEL_SRC))

KVERSION := $(shell uname -r)

CONN_ALPS_OUT_PATH := $(wildcard $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/))
CONNECTIVITY_OUT_PATH := $(if $(CONN_ALPS_OUT_PATH), $(CONN_ALPS_OUT_PATH), $(CONN_EAP_OUT_PATH))

$(info Default out path=$(CONN_ALPS_OUT_PATH))
$(info Connectivity out path=$(CONNECTIVITY_OUT_PATH))

ifneq ($(CONNECTIVITY_OUT_PATH),)
	SEGMENT := SP
	KERNEL_DIR=$(KERNEL_SRC)
	MODULE_PWD=$(M)/../..
	include $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include
else
	KERNEL_DIR=/lib/modules/$(shell uname -r)/build
	MODULE_PWD=$(PWD)
endif
MODULE_PWD=../vendor/mediatek/kernel_modules/connectivity/wlan/core/gen4-mt79xx
$(info gen4m depends on following symbols:[${EXTRA_SYMBOLS}])
modules modules_install clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(MODULE_PWD) $(KBUILD_OPTIONS) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" MODULE_NAME=$(_MODULE_NAME) SEGMENT=$(SEGMENT) $(@)
