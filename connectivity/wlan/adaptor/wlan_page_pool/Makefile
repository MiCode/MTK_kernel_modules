all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS)

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install
compile_commands.json:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) compile_commands.json $(KBUILD_OPTIONS)

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
