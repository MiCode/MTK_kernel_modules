LOCAL_PATH := $(call my-dir)
ifeq ($(MTK_WLAN_SUPPORT), yes)

ifneq (true,$(strip $(TARGET_NO_KERNEL)))
$(warning $(MTK_COMBO_CHIP))

ifeq (MT7902,$(strip $(MTK_COMBO_CHIP)))
$(warning $(MTK_COMBO_CHIP))

include $(CLEAR_VARS)
LOCAL_MODULE := wlan_drv_gen4_mt7961.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_INIT_RC := init.wlan_mt79xx_drv.rc
LOCAL_REQUIRED_MODULES := wlan_drv_gen4_mt7961_reset.ko
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH) -type f -name '*.[cho]')) Makefile
include $(MTK_KERNEL_MODULE)

WIFI_OPTS := CONFIG_MTK_COMBO_WIFI_HIF=pcie
WIFI_OPTS += MTK_COMBO_CHIP=$(MTK_COMBO_CHIP)
WIFI_OPTS += CONFIG_MTK_PREALLOC_MEMORY=y
WIFI_OPTS += CONFIG_GKI_SUPPORT=y
ifeq ($(HAVE_DEV_PATH),true)
WIFI_OPTS += CONFIG_PWR_DEBUG_SUPPORT=y
endif
$(linked_module): OPTS += $(WIFI_OPTS)


include $(CLEAR_VARS)
LOCAL_MODULE := wlan_drv_gen4_mt7961_prealloc.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
#LOCAL_INIT_RC := init.wlan_mt7902_drv.rc
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH) -type f -name '*.[cho]')) Makefile
include $(MTK_KERNEL_MODULE)

WIFI_OPTS := CONFIG_MTK_COMBO_WIFI_HIF=pcie
WIFI_OPTS += MTK_COMBO_CHIP=$(MTK_COMBO_CHIP)
WIFI_OPTS += CONFIG_MTK_PREALLOC_MEMORY=y
WIFI_OPTS += CONFIG_GKI_SUPPORT=y
ifeq ($(HAVE_DEV_PATH),true)
WIFI_OPTS += CONFIG_PWR_DEBUG_SUPPORT=y
endif
$(linked_module): OPTS += $(WIFI_OPTS)

include $(CLEAR_VARS)
LOCAL_MODULE := wlan_drv_gen4_mt7961_reset.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
#LOCAL_INIT_RC := init.wlan_mt7902_drv.rc
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH) -type f -name '*.[cho]')) Makefile
include $(MTK_KERNEL_MODULE)

WIFI_OPTS := CONFIG_MTK_COMBO_WIFI_HIF=pcie
WIFI_OPTS += MTK_COMBO_CHIP=$(MTK_COMBO_CHIP)
WIFI_OPTS += CONFIG_MTK_PREALLOC_MEMORY=y
WIFI_OPTS += CONFIG_GKI_SUPPORT=y
ifeq ($(HAVE_DEV_PATH),true)
WIFI_OPTS += CONFIG_PWR_DEBUG_SUPPORT=y
endif
$(linked_module): OPTS += $(WIFI_OPTS)

endif
endif
endif
