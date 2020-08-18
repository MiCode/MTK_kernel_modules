ifeq ($(MTK_BT_SUPPORT), yes)
ifeq ($(filter MTK_MT76%, $(MTK_BT_CHIP)),)
$(info MTK_BT_CHIP=$(MTK_BT_CHIP))
ifneq ($(filter MTK_CONSYS_MT6885, $(MTK_BT_CHIP)),)
	include $(call all-subdir-makefiles, connac2)
else
	include $(call all-subdir-makefiles, legacy)
endif
endif
endif
