extra_symbols := $(abspath $(O)/../vendor/mediatek/kernel_modules/met_drv_v3/Module.symvers)
ifneq (,$(wildcard ../../../met_drv_secure_v3))
	extra_symbols += $(abspath $(O)/../vendor/mediatek/kernel_modules/met_drv_secure_v3/Module.symvers)
endif

all: PRIVATE_SYMBOLS := $(extra_symbols)
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
