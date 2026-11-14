LOCAL_PATH := $(call my-dir)

ifeq (,$(wildcard $(LOCAL_PATH)/../fpsgo_int))

include $(CLEAR_VARS)
LOCAL_MODULE := fpsgo.ko
include $(MTK_KERNEL_MODULE)

endif
