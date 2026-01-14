LOCAL_PATH := $(call my-dir)

ifeq (,$(wildcard $(LOCAL_PATH)/../sched_int))

include $(CLEAR_VARS)
LOCAL_MODULE := scheduler_ext.ko
include $(MTK_KERNEL_MODULE)

include $(CLEAR_VARS)
LOCAL_MODULE := cpuqos_ext.ko
include $(MTK_KERNEL_MODULE)

include $(CLEAR_VARS)
LOCAL_MODULE := eas_ext.ko
include $(MTK_KERNEL_MODULE)

endif
