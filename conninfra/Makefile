
include $(wildcard $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include)

ifneq ($(_FSM_DENPENDENCY_SYMBOLS),)
	EXTRA_SYMBOLS += $(_FSM_DENPENDENCY_SYMBOLS)
	EXTRA_CFLAGS := -D CFG_CONNINFRA_EAP_COCLOCK=1
	CONNINFRA_COCLOCK_SUPPORT := y
else
	EXTRA_CFLAGS := -D CFG_CONNINFRA_EAP_COCLOCK=0
	CONNINFRA_COCLOCK_SUPPORT := n
endif

MODULE_PWD=../vendor/mediatek/kernel_modules/connectivity/conninfra

modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_PWD) $(KBUILD_OPTIONS) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" CFG_CONNINFRA_COCLOCK_SUPPORT="$(CONNINFRA_COCLOCK_SUPPORT)" $(@)

ifneq ($(MODULE_PWD), $(M))
	mkdir -p $(O)/$(M)
	cp -f $(O)/$(MODULE_PWD)/Module.symvers $(O)/$(M)/Module.symvers
	cp -f $(O)/$(MODULE_PWD)/*.ko $(O)/$(M)
endif
