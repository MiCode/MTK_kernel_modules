LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_WLAN_SUPPORT), yes)
ifneq (true,$(strip $(TARGET_NO_KERNEL)))

include $(CLEAR_VARS)
LOCAL_MODULE := wmt_chrdev_wifi.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk

LOCAL_INIT_RC := init.wlan_drv.rc
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH) -type f -name '*.[cho]')) Makefile
LOCAL_REQUIRED_MODULES := wmt_drv.ko

include $(MTK_KERNEL_MODULE)

#### Copy Module.symvers from $(LOCAL_REQUIRED_MODULES) to this module #######
#### For symbol link (when CONFIG_MODVERSIONS is defined)
WMT_EXPORT_SYMBOL := $(subst $(LOCAL_MODULE),$(LOCAL_REQUIRED_MODULES),$(intermediates)/LINKED)/Module.symvers
$(WMT_EXPORT_SYMBOL): $(subst $(LOCAL_MODULE),$(LOCAL_REQUIRED_MODULES),$(linked_module))
WMT_CHRDEV_WIFI_EXPORT_SYMBOL := $(intermediates)/LINKED/Module.symvers
$(WMT_CHRDEV_WIFI_EXPORT_SYMBOL).in: $(intermediates)/LINKED/% : $(WMT_EXPORT_SYMBOL)
	$(copy-file-to-target)
	cp $(WMT_EXPORT_SYMBOL) $(WMT_CHRDEV_WIFI_EXPORT_SYMBOL)

$(linked_module): $(WMT_CHRDEV_WIFI_EXPORT_SYMBOL).in

endif
endif
