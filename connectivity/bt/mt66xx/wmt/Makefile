$(info [bt_drv] wmt/makefile start)


LOG_TAG := [BT_Drv][wmt]
ifneq ($(wildcard $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include),)
include $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include
extra_symbols := $(abspath $(OUT_DIR)/../vendor/mediatek/kernel_modules/connectivity/common/Module.symvers)
else
extra_symbols := $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/common/Module.symvers)
endif


all: PRIVATE_LOG_TAG := $(LOG_TAG)
all: EXTRA_SYMBOLS += $(extra_symbols)
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS) BT_PLATFORM=connac1x LOG_TAG=$(PRIVATE_LOG_TAG) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)"

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean

$(info [bt_drv] wmt/makefile end)
