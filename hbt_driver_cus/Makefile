all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS)
ifneq (,$(wildcard ../hbt_driver))
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/../hbt_driver modules $(KBUILD_OPTIONS)
	cp -r $(O)/$(M)/../hbt_driver/*.ko $(O)/$(M)/*.ko
endif

modules_install:
ifeq (,$(wildcard ../hbt_driver))
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install
else
	$(MAKE) M=$(M)/../hbt_driver -C $(KERNEL_SRC) modules_install
endif

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
ifneq (,$(wildcard ../hbt_driver))
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/../hbt_driver clean
endif
