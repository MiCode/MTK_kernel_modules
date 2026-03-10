LOCAL_PATH := $(call my-dir)
$(info [BT_Drv] uart_launcher)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -Wall -Werror -Wno-error=date-time

LOCAL_SHARED_LIBRARIES := libcutils liblog libdl
LOCAL_HEADER_LIBRARIES := libcutils_headers
LOCAL_SRC_FILES  := uart_launcher.c
LOCAL_MODULE := uart_launcher
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_TAGS := optional
include $(MTK_EXECUTABLE)
