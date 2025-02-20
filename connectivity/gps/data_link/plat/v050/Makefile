ifneq ($(wildcard $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include),)
include $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include
EXTRA_SYMBOLS += $(abspath $(OUT_DIR)/../vendor/mediatek/kernel_modules/connectivity/conninfra/Module.symvers)
else
EXTRA_SYMBOLS += $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/conninfra/Module.symvers)
endif

export AUTOCONF_H=$(O)/include/generated/autoconf.h

KBUILD_OPTIONS += KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)"

all: PRIVATE_SYSMBOLS := $(extra_symbols)
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS)

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
