include $(wildcard $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include)

EXTRA_SYMBOLS += $(OUT_DIR)/../vendor/mediatek/kernel_modules/met_drv_v3/Module.symvers
ifneq ($(wildcard $(CURDIR)/../../met_drv_secure_v3),)
EXTRA_SYMBOLS += $(OUT_DIR)/../vendor/mediatek/kernel_modules/met_drv_secure_v3/Module.symvers
endif

modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" $(@)
