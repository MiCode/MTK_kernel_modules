ifneq ($(wildcard $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include),)
include $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include
ROOT=$(OUT_DIR)
else
ROOT=$(O)
endif


EXTRA_SYMBOLS += $(abspath $(ROOT)/../vendor/mediatek/kernel_modules/met_drv_v3/Module.symvers)
ifneq (,$(wildcard ../../../met_drv_secure_v3))
	EXTRA_SYMBOLS += $(abspath $(ROOT)/../vendor/mediatek/kernel_modules/met_drv_secure_v3/Module.symvers)
endif

all: PRIVATE_SYMBOLS := $(EXTRA_SYMBOLS)
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
