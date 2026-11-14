LOCAL_PATH := $(call my-dir)

ifeq (,$(wildcard $(LOCAL_PATH)/../msync2_frd_int))
include $(CLEAR_VARS)
LOCAL_MODULE := msync2_frd.ko
include $(MTK_KERNEL_MODULE)
endif
