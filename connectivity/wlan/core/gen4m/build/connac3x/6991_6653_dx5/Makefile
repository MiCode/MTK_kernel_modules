# SPDX-License-Identifier: BSD-2-Clause
KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

_CONNAC_VER := 3_0
_CONNAC_DENPENDENCY_SYMBOLS := wlan/adaptor/build/connac3x/Module.symvers
_MODULE_NAME := wlan_drv_gen4m_6991_6653_dx5

$(info M=$(M))
$(info Kernel src=$(KERNEL_SRC))

include $(KERNEL_SRC)/$(M)/../../../Makefile
