# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.
#
# MediaTek Inc. (C) 2017. All rights reserved.
#
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
# RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
# AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
#
# The following software/firmware and/or related documentation ("MediaTek Software")
# have been modified by MediaTek Inc. All revisions are subject to any receiver's
# applicable license agreements with MediaTek Inc.

LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_WLAN_SUPPORT), yes)

ifeq ($(LINUX_KERNEL_VERSION),)
    $(error LINUX_KERNEL_VERSION is not defined)
endif

KERNEL_ENV_PATH := $(LINUX_KERNEL_VERSION)
ifndef KERNEL_ROOT_DIR
KERNEL_ROOT_DIR := $(PWD)
endif#KERNEL_ZIMAGE_OUT

ifneq (true,$(strip $(TARGET_NO_KERNEL)))
ifndef KERNEL_DIR
  KERNEL_DIR := $(KERNEL_ENV_PATH)
endif#KERNEL_DIR

  ifeq ($(KERNEL_CROSS_COMPILE),)
    ifeq ($(KERNEL_TARGET_ARCH), arm64)
      KERNEL_CROSS_COMPILE := $(KERNEL_ROOT_DIR)/$(TARGET_TOOLS_PREFIX)
    else
      KERNEL_CROSS_COMPILE := $(KERNEL_ROOT_DIR)/prebuilts/gcc/$(HOST_PREBUILT_TAG)/arm/arm-eabi-$(TARGET_GCC_VERSION)/bin/arm-eabi-
    endif
  endif

    KERNEL_OUT ?= $(if $(filter /% ~%,$(TARGET_OUT_INTERMEDIATES)),,$(KERNEL_ROOT_DIR)/)$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ
  ifndef KERNEL_ZIMAGE_OUT
    ifeq ($(KERNEL_TARGET_ARCH), arm64)
      ifeq ($(MTK_APPENDED_DTB_SUPPORT), yes)
        KERNEL_ZIMAGE_OUT := $(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/Image.gz-dtb
      else
        KERNEL_ZIMAGE_OUT := $(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/Image.gz
      endif
    else
      ifeq ($(MTK_APPENDED_DTB_SUPPORT), yes)
        KERNEL_ZIMAGE_OUT := $(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/zImage-dtb
      else
        KERNEL_ZIMAGE_OUT := $(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/zImage
      endif
    endif
  endif#KERNEL_ZIMAGE_OUT

  ifndef KERNEL_MAKE_OPTION
    KERNEL_MAKE_OPTION := O=$(KERNEL_OUT) ARCH=$(KERNEL_TARGET_ARCH) CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) ROOTDIR=$(KERNEL_ROOT_DIR) $(if $(strip $(SHOW_COMMANDS)),V=1)
  endif

# check kernel folder exist
ifeq (,$(wildcard $(KERNEL_DIR)))
    $(error kernel $(KERNEL_DIR) is not existed)
endif

# check cross compiler exist
ifeq (,$(wildcard $(KERNEL_CROSS_COMPILE)gcc))
    $(error $(KERNEL_CROSS_COMPILE) is not existed)
endif

# set for module Makefile
export AUTOCONF_H=$(KERNEL_OUT)/include/generated/autoconf.h

ifeq (MT7668,$(strip $(MTK_COMBO_CHIP)))

include $(CLEAR_VARS)
LOCAL_MODULE := wlan_drv_gen4_mt7668.ko
LOCAL_STRIP_MODULE := true
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := first
LOCAL_INIT_RC := init.wlan_mt7668_drv.rc
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/modules

LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH) -type f -name '*.[cho]')) Makefile
#LOCAL_REQUIRED_MODULES := wmt_chrdev_wifi.ko
include $(BUILD_SYSTEM)/base_rules.mk


#### Copy Module.symvers from $(LOCAL_REQUIRED_MODULES) to this module #######
#### For symbol link (when CONFIG_MODVERSIONS is defined)
#LOCAL_SRC_EXPORT_SYMBOL := $(subst $(LOCAL_MODULE),$(LOCAL_REQUIRED_MODULES),$(intermediates))/Module.symvers
#$(LOCAL_SRC_EXPORT_SYMBOL): $(subst $(LOCAL_MODULE),$(LOCAL_REQUIRED_MODULES),$(LOCAL_BUILT_MODULE))
#LOCAL_EXPORT_SYMBOL := $(intermediates)/Module.symvers
#$(LOCAL_EXPORT_SYMBOL): $(intermediates)/% : $(LOCAL_SRC_EXPORT_SYMBOL)
#	$(copy-file-to-target)
LOCAL_GENERATED_SOURCES := $(addprefix $(intermediates)/,$(LOCAL_SRC_FILES))

$(LOCAL_GENERATED_SOURCES): $(intermediates)/% : $(LOCAL_PATH)/% | $(ACP)
	$(copy-file-to-target)

$(LOCAL_BUILT_MODULE): KOUT := $(KERNEL_OUT)
$(LOCAL_BUILT_MODULE): KOPTS := $(KERNEL_MAKE_OPTION) M=$(abspath $(intermediates))
$(LOCAL_BUILT_MODULE): $(LOCAL_GENERATED_SOURCES) $(KERNEL_ZIMAGE_OUT)
#$(LOCAL_BUILT_MODULE): $(LOCAL_GENERATED_SOURCES) $(KERNEL_ZIMAGE_OUT) $(LOCAL_EXPORT_SYMBOL) $(LOCAL_SRC_EXPORT_SYMBOL) $(AUTO_CONF)
$(LOCAL_BUILT_MODULE): $(LOCAL_GENERATED_SOURCES) $(KERNEL_ZIMAGE_OUT) $(AUTO_CONF)
	@echo $@: $^
	$(MAKE) -C $(KOUT) $(KOPTS)

endif
endif
endif
