LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_WLAN_SUPPORT), yes)
ifneq (true,$(strip $(TARGET_NO_KERNEL)))

include $(CLEAR_VARS)
LOCAL_MODULE := wlan_drv_gen4m.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH) -type f -name '*.[cho]')) Makefile
LOCAL_REQUIRED_MODULES := wmt_chrdev_wifi.ko

include $(MTK_KERNEL_MODULE)


ifeq ($(MTK_WLAN_UT_TEST_MODE), yes)
LOCAL_SRC_UT_FILES := $(shell find $(LOCAL_PATH)_ut/ -type f -name '*')
LOCAL_DST_UT_FILES := $(patsubst $(LOCAL_PATH)_ut/%,$(intermediates)/LINKED/%,$(LOCAL_SRC_UT_FILES))
$(LOCAL_DST_UT_FILES): $(intermediates)/LINKED/% : $(LOCAL_PATH)_ut/%
	$(copy-file-to-target)

$(linked_module): $(LOCAL_DST_UT_FILES)

WIFI_HIF := ut
WIFI_CHIP := UT_TEST_MODE
endif


WIFI_NAME := wlan_drv_gen4m
WIFI_OPTS := CONFIG_MTK_COMBO_WIFI_HIF=$(WIFI_HIF) MODULE_NAME=$(WIFI_NAME) MTK_COMBO_CHIP=$(WIFI_CHIP) WLAN_CHIP_ID=$(WLAN_CHIP_ID) MTK_ANDROID_WMT=$(WIFI_WMT) WIFI_ENABLE_GCOV=$(WIFI_ENABLE_GCOV) WIFI_IP_SET=$(WIFI_IP_SET)

#### Copy Module.symvers from $(LOCAL_REQUIRED_MODULES) to this module #######
#### For symbol link (when CONFIG_MODVERSIONS is defined)
LOCAL_SRC_EXPORT_SYMBOL := $(subst $(LOCAL_MODULE),$(LOCAL_REQUIRED_MODULES),$(intermediates)/LINKED)/Module.symvers
$(LOCAL_SRC_EXPORT_SYMBOL): $(subst $(LOCAL_MODULE),$(LOCAL_REQUIRED_MODULES),$(linked_module))
LOCAL_EXPORT_SYMBOL := $(intermediates)/LINKED/Module.symvers
$(LOCAL_EXPORT_SYMBOL): $(intermediates)/LINKED/% : $(LOCAL_SRC_EXPORT_SYMBOL)
	$(copy-file-to-target)

$(linked_module): OPTS += $(WIFI_OPTS)
$(linked_module): $(LOCAL_EXPORT_SYMBOL)

endif
endif
