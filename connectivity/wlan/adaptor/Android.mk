LOCAL_PATH := $(call my-dir)
MAIN_PATH := $(LOCAL_PATH)

ifeq ($(MTK_WLAN_SUPPORT), yes)

ifeq ($(WLAN_BUILD_COMMON), true)
	# for layer decoupling 2.0, we have to build all configurations
	CONNAC_VER := 1_0
	WIFI_NAME := wmt_chrdev_wifi
	include $(MAIN_PATH)/build_cdev_wifi.mk

	CONNAC_VER := 2_0
	WIFI_NAME := wmt_chrdev_wifi_connac2
	include $(MAIN_PATH)/build_cdev_wifi.mk

	CONNAC_VER := 3_0
	WIFI_NAME := wmt_chrdev_wifi_connac3
	include $(MAIN_PATH)/build_cdev_wifi.mk

	include $(MAIN_PATH)/wlan_page_pool/Android.mk
else
	WIFI_NAME := wmt_chrdev_wifi
	include $(MAIN_PATH)/build_cdev_wifi.mk
endif

endif
