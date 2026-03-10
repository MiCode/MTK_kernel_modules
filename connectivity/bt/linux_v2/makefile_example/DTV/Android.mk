LOCAL_PATH := $(call my-dir)

ifneq ($(filter 3.10 3.18 4.9 4.19 5.4 5.4_12, $(subst linux-,,$(KERNEL_VER))),)

include $(CLEAR_VARS)
LOCAL_MODULE := btmtk_usb_unify.ko
LOCAL_REQUIRED_MODULES := wlan_mt7961_reset.ko
include $(MTK_KERNEL_MODULE)

endif