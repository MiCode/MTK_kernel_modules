LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_WLAN_SUPPORT), yes)

ifeq ($(WLAN_CHIP_ID), common)
	# for layer decoupling 2.0, we have to build all configurations
	WIFI_CHIP := CONNAC2X2_SOC3_0
	WIFI_NAME := wmt_chrdev_wifi_connac2
	include $(LOCAL_PATH)/build_cdev_wifi.mk
else
	WIFI_NAME := wmt_chrdev_wifi
	include $(LOCAL_PATH)/build_cdev_wifi.mk
endif

endif
