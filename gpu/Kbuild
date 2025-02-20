KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)
ifneq (,$(filter $(CONFIG_MTK_GPU_MT6768_SUPPORT),y m))
ifneq ($(wildcard $(KO_CODE_PATH)/mt6768),)
        obj-m += mt6768/
endif
else
ifneq (,$(filter $(CONFIG_MTK_GPU_MT6761_SUPPORT),y m))
ifneq ($(wildcard $(KO_CODE_PATH)/mt6761),)
        obj-m += mt6761/
endif
else
ifneq (,$(filter $(CONFIG_MTK_GPU_MT6765_SUPPORT),y m))
ifneq ($(wildcard $(KO_CODE_PATH)/mt6765),)
        obj-m += mt6765/
endif
else ifeq ($(CONFIG_MTK_GPU_MT6877_SUPPORT),m)
ifneq ($(wildcard $(KO_CODE_PATH)/mt6877),)
        obj-m += mt6877/
endif
else
ifeq ($(CONFIG_MTK_GPU_MT6893_SUPPORT),m)
ifneq ($(wildcard $(KO_CODE_PATH)/mt6893),)
	obj-m += mt6893/
endif
else
ifeq ($(CONFIG_MTK_GPU_MT6781_SUPPORT),m)
ifneq ($(wildcard $(KO_CODE_PATH)/mt6781),)
	obj-m += mt6781/
endif
else
ifeq ($(CONFIG_MTK_GPU_MT6853_SUPPORT),m)
ifneq ($(wildcard $(KO_CODE_PATH)/mt6853),)
	obj-m += mt6853/
endif
else
ifeq ($(CONFIG_MTK_GPU_MT6833_SUPPORT),m)
ifneq ($(wildcard $(KO_CODE_PATH)/mt6833),)
	obj-m += mt6833/
endif
else
ifneq ($(wildcard $(KO_CODE_PATH)/mt6878),)
	obj-m += mt6878/
endif
ifneq ($(wildcard $(KO_CODE_PATH)/mt6897),)
	obj-m += mt6897/
endif
ifneq ($(wildcard $(KO_CODE_PATH)/mt6899),)
	obj-m += mt6899/
endif
ifneq ($(wildcard $(KO_CODE_PATH)/mt6985),)
	obj-m += mt6985/
endif
ifneq ($(wildcard $(KO_CODE_PATH)/mt6989),)
        obj-m += mt6989/
endif
ifneq ($(wildcard $(KO_CODE_PATH)/mt6991),)
        obj-m += mt6991/
endif
endif
endif
endif
endif
endif
endif
endif
