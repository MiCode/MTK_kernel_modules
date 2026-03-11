all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS)
ifneq (,$(wildcard ../fpsgo_int))
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/../fpsgo_int modules $(KBUILD_OPTIONS)
	cp -r $(O)/$(M)/../fpsgo_int/*.ko $(O)/$(M)/*.ko
endif

modules_install:
ifeq (,$(wildcard ../fpsgo_int))
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install
else
	$(MAKE) M=$(M)/../fpsgo_int -C $(KERNEL_SRC) modules_install
endif

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
ifneq (,$(wildcard ../fpsgo_int))
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/../fpsgo_int clean
endif
