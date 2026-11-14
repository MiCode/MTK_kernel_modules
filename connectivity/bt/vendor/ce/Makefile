$(info [bt_drv] vendor/makefile start)

CONFIG_SUPPORT_DEINT_IRQ=n

ifeq ($(CONFIG_SUPPORT_DEINT_IRQ), y)
    ccflags-y += -DCFG_SUPPORT_DEINT_IRQ=1
else
    ccflags-y += -DCFG_SUPPORT_DEINT_IRQ=0
endif
ccflags-y += -I$(src)/
ccflags-y += -Werror

EXTRA_SYMBOLS += $(OUT_DIR)/../vendor/mediatek/kernel_modules/connectivity/bt/linux_v2/Module.symvers

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $(KBUILD_OPTIONS) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)"

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
$(info [bt_drv] vendor/makefile end)