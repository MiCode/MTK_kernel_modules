LOCAL_PATH := $(call my-dir)

ifneq ($(filter yes,$(sort $(MTK_WLAN_SUPPORT) $(MTK_BT_SUPPORT) $(MTK_GPS_SUPPORT) $(MTK_FM_SUPPORT))),)

ifneq (true,$(strip $(TARGET_NO_KERNEL)))
ifneq ($(filter yes,$(MTK_COMBO_SUPPORT)),)

include $(CLEAR_VARS)
LOCAL_MODULE := conninfra.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk

LOCAL_INIT_RC := init.conninfra.rc
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH) -type f -name '*.[cho]')) Makefile
LOCAL_REQUIRED_MODULES :=

include $(MTK_KERNEL_MODULE)

else
        $(warning wmt_drv-MTK_COMBO_SUPPORT: [$(MTK_COMBO_SUPPORT)])
endif
endif

endif
