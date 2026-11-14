LOCAL_PATH := $(call my-dir)

ifeq (,$(wildcard $(LOCAL_PATH)/../hbt_driver))

include $(CLEAR_VARS)
LOCAL_MODULE := hbt.ko
include $(MTK_KERNEL_MODULE)

endif
