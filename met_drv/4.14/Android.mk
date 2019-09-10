LOCAL_PATH := $(call my-dir)

ifneq ($(LINUX_KERNEL_VERSION),)
TEMP_LINUX_KERNEL_VERSION := $(LINUX_KERNEL_VERSION)
export LINUX_KERNEL_VERSION=$(TEMP_LINUX_KERNEL_VERSION)
endif

ifneq (,$(filter $(word 2,$(subst -, ,$(LINUX_KERNEL_VERSION))),$(subst /, ,$(LOCAL_PATH))))

include $(CLEAR_VARS)
LOCAL_MODULE := met.ko
LOCAL_STRIP_MODULE := true

include $(MTK_KERNEL_MODULE)

endif # Kernel version matches current path
