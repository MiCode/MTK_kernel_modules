ifneq ($(wildcard $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include),)
include $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include
KBUILD_OPTIONS += KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)"
endif

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS)

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
