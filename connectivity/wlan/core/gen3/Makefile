#export AUTOCONF_H=$(O)/include/generated/autoconf.h
KVERSION := $(shell uname -r)

CONN_ALPS_OUT_PATH := $(wildcard $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/))
CONNECTIVITY_OUT_PATH := $(if $(CONN_ALPS_OUT_PATH), $(CONN_ALPS_OUT_PATH), $(CONN_EAP_OUT_PATH))

$(info Default out path=$(CONN_ALPS_OUT_PATH))
$(info Connectivity out path=$(CONNECTIVITY_OUT_PATH))

ifneq ($(CONNECTIVITY_OUT_PATH),)
	KERNEL_DIR=$(KERNEL_SRC)
	MODULE_PWD=$(M)/../..
	include $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include
ifneq ($(filter-out gen3 1_0,$(_CONNAC_VER)),)
	EXTRA_SYMBOLS += $(abspath $(CONNECTIVITY_OUT_PATH)/conninfra/Module.symvers)
	EXTRA_SYMBOLS += $(abspath $(CONNECTIVITY_OUT_PATH)/connfem/Module.symvers)
	EXTRA_SYMBOLS += $(abspath $(CONNECTIVITY_OUT_PATH)/wlan/adaptor/wlan_page_pool/Module.symvers)
endif
	EXTRA_SYMBOLS += $(foreach dep, $(_CONNAC_DENPENDENCY_SYMBOLS), $(abspath $(CONNECTIVITY_OUT_PATH)/$(dep)))
else
	KERNEL_DIR=/lib/modules/$(shell uname -r)/build
	MODULE_PWD=$(PWD)
	EXTRA_SYMBOLS := $(MODULE_PWD)/../conninfra/Module.symvers
	EXTRA_SYMBOLS += $(MODULE_PWD)/../connfem/Module.symvers
	EXTRA_SYMBOLS += $(MODULE_PWD)/../adaptor/wlan_page_pool/Module.symvers
endif
MODULE_PWD=../vendor/mediatek/kernel_modules/connectivity/wlan/core/gen3
$(info gen3 depends on following symbols:[${EXTRA_SYMBOLS}])
modules modules_install compile_commands.json clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(MODULE_PWD) $(KBUILD_OPTIONS) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" MODULE_NAME=$(_MODULE_NAME) $(@)
	mkdir -p $(O)/$(M)
	cp -f $(O)/$(MODULE_PWD)/Module.symvers $(O)/$(M)/Module.symvers
	if [ "$(@)" = "compile_commands.json" ]; then cp -f $(O)/$(MODULE_PWD)/compile_commands.json $(O)/$(M)/compile_commands.json; fi
	cp -f $(O)/$(MODULE_PWD)/*.ko $(O)/$(M)
