LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_WLAN_SUPPORT), yes)
ifeq (MT7668,$(strip $(MTK_COMBO_CHIP)))

include $(CLEAR_VARS)
LOCAL_MODULE := wlan_drv_gen4_mt7668.ko
LOCAL_STRIP_MODULE := true
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_INIT_RC := init.wlan_mt7668_drv.rc
include $(MTK_KERNEL_MODULE)

endif
endif
