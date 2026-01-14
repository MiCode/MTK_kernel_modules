# SPDX-License-Identifier: GPL-2.0-only
ifneq ($(KERNELRELEASE),)
# kbuild part
include Kbuild

else
obj-y += camsys/
obj-m += scpsys/
obj-m += cam_cal/
obj-m += imgsys/
obj-m += mtk-hcp/
obj-m += mtk-ipesys-me/
obj-$(CONFIG_MTK_CAMERA_FD_ISP7S_ISP7SP)	+= mtk-aie/
obj-$(CONFIG_MTK_CAMERA_DPE_ISP7SP)	 += mtk-dpe/
#obj-$(CONFIG_MTK_C2PS) += sched/
obj-y += imgsensor/
obj-$(CONFIG_MTK_CCU_RPROC) += ccusys/

subdir-ccflags-y += -D__XIAOMI_CAMERA__

## normal Makefile
#SUBDIRS := $(wildcard */.)
#
#.PHONY: all $(SUBDIRS)
#
## run make in each subdirectory
#all: $(SUBDIRS)
#	echo $(abspath $(lastword $(MAKEFILE_LIST)))
#
#$(SUBDIRS):
#	echo $(abspath $(lastword $(MAKEFILE_LIST)))
#	echo "KERNEL_SRC is $(KERNEL_SRC)"
##	$(MAKE) -C $@
#	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)"


ifneq ($(wildcard $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include),)
include $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include
endif

$(info MAKE=$(MAKE) KERNEL_SRC=$(KERNEL_SRC) M=$(M) KBUILD_OPTIONS=$(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS=$(EXTRA_SYMBOLS))

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)"

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean

endif
