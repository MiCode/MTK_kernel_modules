LOCAL_PATH := $(call my-dir)

ifeq (,$(wildcard $(LOCAL_PATH)/../game_int))

include $(CLEAR_VARS)
LOCAL_MODULE := game.ko
include $(MTK_KERNEL_MODULE)

endif
