LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_WLAN_SUPPORT), yes)

  BUILD_CDEV_WIFI := $(LOCAL_PATH)/build_cdev_wifi.mk

ifeq ($(WLAN_BUILD_COMMON), true)
  # for layer decoupling 2.0, we have to build all configurations
  WIFI_NAME := wmt_chrdev_wifi
  include $(BUILD_CDEV_WIFI)

  WIFI_NAME := wmt_chrdev_wifi_connac2
  include $(BUILD_CDEV_WIFI)

  WIFI_NAME := wmt_chrdev_wifi_connac3
  include $(BUILD_CDEV_WIFI)
else
  WIFI_NAME := wmt_chrdev_wifi
  include $(BUILD_CDEV_WIFI)
endif

endif
