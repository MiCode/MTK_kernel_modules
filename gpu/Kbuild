KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

ifneq ($(wildcard $(KO_CODE_PATH)/mt6878),)
	obj-m += mt6878/
endif

ifneq ($(wildcard $(KO_CODE_PATH)/mt6897),)
	obj-m += mt6897/
endif

ifneq ($(wildcard $(KO_CODE_PATH)/mt6985),)
	obj-m += mt6985/
endif

ifneq ($(wildcard $(KO_CODE_PATH)/mt6989),)
        obj-m += mt6989/
endif
