
LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_GPS_SUPPORT), yes)
$(warning this is mylog)
include $(CLEAR_VARS)
LOCAL_MODULE := mi_gps_pwr.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_INIT_RC := init.mi_gps_pwr.rc

include $(MTK_KERNEL_MODULE)

endif

