LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := mali_prot_alloc.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_INIT_RC := init.mali_prot_alloc.rc
include $(MTK_KERNEL_MODULE)

$(info [GPU][DDK] GPU_OPTS = $(GPU_OPTS))
$(linked_module): OPTS += $(GPU_OPTS)

