LOCAL_PATH := $(call my-dir)
$(info [BT_Drv] uart_launcher)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -Wall -Werror -Wno-error=date-time

LOCAL_SHARED_LIBRARIES := libcutils liblog libdl libnvram
LOCAL_HEADER_LIBRARIES := libcutils_headers libnvram_headers
LOCAL_C_INCLUDES := \
  vendor/mediatek/proprietary/external/nvram/libnvram \
  vendor/mediatek/proprietary/custom/common/cgen/cfgfileinc \
  vendor/mediatek/proprietary/custom/common/cgen/cfgdefault \
  vendor/mediatek/proprietary/custom/common/cgen/inc \
  vendor/mediatek/proprietary/custom/k6991v1_64/cgen/inc \
  vendor/mediatek/proprietary/custom/evb6991_64/cgen/inc \
  vendor/mediatek/proprietary/custom/k6989v1_64/cgen/inc \
  vendor/mediatek/proprietary/custom/k6989v1_64_for_dx4/cgen/inc \
  $(wildcard vendor/mediatek/proprietary/custom/*/cgen/inc) \
  vendor/mediatek/proprietary/custom/k6989v1_64_for_dx4_gpu/cgen/inc \
  vendor/mediatek/proprietary/custom/common/cgen/cfgfileinc/connxo/MT6991 \
  vendor/mediatek/proprietary/custom/common/cgen/cfgfileinc/connxo/MT6989
LOCAL_SRC_FILES  := uart_launcher.c
LOCAL_MODULE := uart_launcher
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_TAGS := optional
include $(MTK_EXECUTABLE)

