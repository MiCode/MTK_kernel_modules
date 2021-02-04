LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_BT_SUPPORT),yes)

ifeq ($(filter %MT6885, $(MTK_BT_CHIP)),)
include $(CLEAR_VARS)
LOCAL_MODULE := bt_drv.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_INIT_RC := init.bt_drv.rc
LOCAL_REQUIRED_MODULES := wmt_drv.ko

include $(MTK_KERNEL_MODULE)
endif

endif
