modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" $(@)
ifneq (,$(wildcard hbt_driver/Makefile))
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/hbt_driver $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" $(@)
	cp -r $(O)/$(M)/hbt_driver/*.ko $(O)/$(M)/*.ko
endif

modules_install:
ifneq (,$(wildcard hbt_driver/Makefile))
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/hbt_driver $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" $(@)
else
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" $(@)
endif

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" $(@)
ifneq (,$(wildcard hbt_driver/Makefile))
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/hbt_driver $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" $(@)
endif
