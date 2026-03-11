export AUTOCONF_H=$(O)/include/generated/autoconf.h

extra_symbols := $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/conninfra/Module.symvers)

all: PRIVATE_SYSMBOLS := $(extra_symbols)
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYSMBOLS)"

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
