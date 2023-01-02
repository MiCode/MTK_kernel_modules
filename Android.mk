LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_WLAN_SUPPORT), yes)

include $(CLEAR_VARS)
LOCAL_MODULE := wmt_chrdev_wifi.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_INIT_RC := init.wlan_drv.rc
LOCAL_REQUIRED_MODULES := wmt_drv.ko
include $(MTK_KERNEL_MODULE)

endif
