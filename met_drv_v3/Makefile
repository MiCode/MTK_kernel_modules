extra_symbols := $(abspath $(O)/../vendor/mediatek/kernel_modules/met_drv_v3/Module.symvers)
ifneq (,$(wildcard ../met_drv_secure_v3))
	extra_symbols += $(abspath $(O)/../vendor/mediatek/kernel_modules/met_drv_secure_v3/Module.symvers)
endif

all: PRIVATE_SYMBOLS := $(extra_symbols)
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS)
ifneq (,$(wildcard ../met_drv_secure_v3))
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/../met_drv_secure_v3 modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"
endif
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_gpu_api modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_gpu_adv_api modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_vcore_api modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_backlight_api modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_emi_api modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_sspm_api modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_mcupm_api modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_ipi_api modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_scmi_api modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(PRIVATE_SYMBOLS)"

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install
ifneq (,$(wildcard ../met_drv_secure_v3))
	$(MAKE) M=$(M)/../met_drv_secure_v3 -C $(KERNEL_SRC) modules_install
endif
	$(MAKE) M=$(M)/met_api/met_gpu_api -C $(KERNEL_SRC) modules_install
	$(MAKE) M=$(M)/met_api/met_gpu_adv_api -C $(KERNEL_SRC) modules_install
	$(MAKE) M=$(M)/met_api/met_vcore_api -C $(KERNEL_SRC) modules_install
	$(MAKE) M=$(M)/met_api/met_backlight_api -C $(KERNEL_SRC) modules_install
	$(MAKE) M=$(M)/met_api/met_emi_api -C $(KERNEL_SRC) modules_install
	$(MAKE) M=$(M)/met_api/met_sspm_api -C $(KERNEL_SRC) modules_install
	$(MAKE) M=$(M)/met_api/met_mcupm_api -C $(KERNEL_SRC) modules_install
	$(MAKE) M=$(M)/met_api/met_ipi_api -C $(KERNEL_SRC) modules_install
	$(MAKE) M=$(M)/met_api/met_scmi_api -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
ifneq (,$(wildcard ../met_drv_secure_v3))
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/../met_drv_secure_v3 clean
endif
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_gpu_api clean
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_gpu_adv_api clean
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_vcore_api clean
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_backlight_api clean
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_emi_api clean
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_sspm_api clean
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_mcupm_api clean
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_ipi_api clean
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/met_api/met_scmi_api clean
