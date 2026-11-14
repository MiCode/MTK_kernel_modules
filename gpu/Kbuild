KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

ifneq (,$(filter $(CONFIG_MTK_GPU_MT6768_SUPPORT),y m))
ifneq ($(wildcard $(KO_CODE_PATH)/mt6768),)
        obj-m += mt6768/
endif
else
ifneq (,$(filter $(CONFIG_MTK_GPU_MT6789_SUPPORT),y m))
ifneq ($(wildcard $(KO_CODE_PATH)/mt6789),)
        obj-m += mt6789/
endif
else
ifneq (,$(filter $(CONFIG_MTK_GPU_MT6855_SUPPORT),y m))
ifneq ($(wildcard $(KO_CODE_PATH)/mt6855),)
        obj-m += mt6855/
endif
else
ifneq ($(wildcard $(KO_CODE_PATH)/mt6991),)
        obj-m += mt6991/
endif

ifneq ($(wildcard $(KO_CODE_PATH)/mt6993),)
        obj-m += mt6993/
endif
ifneq ($(wildcard $(KO_CODE_PATH)/mt6895),)
        obj-m += mt6895/
endif
endif
endif
endif
