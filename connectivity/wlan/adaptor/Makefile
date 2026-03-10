extra_symbols := $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/conninfra/Module.symvers)
extra_symbols += $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/common/Module.symvers)

$(info [wlan] WLAN_BUILD_COMMON=$(WLAN_BUILD_COMMON))

all:
ifdef WLAN_BUILD_COMMON
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(extra_symbols)"
else

ifneq ($(wildcard $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/common/Module.symvers)) ,)
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/build/connac1x modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(extra_symbols)" CONNAC_VER=1_0 MODULE_NAME=wmt_chrdev_wifi
endif

ifneq ($(wildcard $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/conninfra/Module.symvers)) ,)
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/build/connac2x modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(extra_symbols)" CONNAC_VER=2_0 MODULE_NAME=wmt_chrdev_wifi_connac2
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/build/connac3x modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(extra_symbols)" CONNAC_VER=3_0 MODULE_NAME=wmt_chrdev_wifi_connac3
endif

endif

modules_install:
ifdef WLAN_BUILD_COMMON
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install
else

ifneq ($(wildcard $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/common/Module.symvers)) ,)
	$(MAKE) M=$(M)/build/connac1x -C $(KERNEL_SRC) modules_install
endif

ifneq ($(wildcard $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/conninfra/Module.symvers)) ,)
	$(MAKE) M=$(M)/build/connac2x -C $(KERNEL_SRC) modules_install
	$(MAKE) M=$(M)/build/connac3x -C $(KERNEL_SRC) modules_install
endif

endif

clean:
ifdef WLAN_BUILD_COMMON
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
else

ifneq ($(wildcard $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/common/Module.symvers)) ,)
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/build/connac1x clean
endif

ifneq ($(wildcard $(abspath $(O)/../vendor/mediatek/kernel_modules/connectivity/conninfra/Module.symvers)) ,)
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/build/connac2x clean
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/build/connac3x clean
endif

endif
