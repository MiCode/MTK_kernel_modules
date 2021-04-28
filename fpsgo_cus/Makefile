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
# THAT IT IS RECEIVER\'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER\'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER\'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK\'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK\'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
#
# The following software/firmware and/or related documentation ("MediaTek Software")
# have been modified by MediaTek Inc. All revisions are subject to any receiver\'s
# applicable license agreements with MediaTek Inc.
#

FBT_TOP = $(srctree)/drivers/misc/mediatek/performance/fpsgo_v3/fbt
MTK_TOP = $(srctree)/drivers/misc/mediatek/

ifeq ($(CONFIG_MTK_FPSGO_V3),$(filter $(CONFIG_MTK_FPSGO_V3),m))
ccflags-y += -DCONFIG_MTK_FPSGO_V3
subdir-ccflags-y += -DCONFIG_MTK_FPSGO_V3
endif

ccflags-y += \
	     -I$(srctree)/include/ \
	     -I$(MTK_TOP)/include/ \
	     -I$(FBT_TOP)/include/ \

obj-m += fpsgo.o
fpsgo-y += src/fpsgo_main.o

ifeq (y,$(CONFIG_MTK_FPSGO_V3))
	ifeq (y,$(CONFIG_THUMB2_KERNEL))
		fpsgo-y += src/xgf_v3_ko_thumb2.o
	else
		fpsgo-$(CONFIG_ARM) += src/xgf_v3_ko_arm32.o
		ifeq (y,$(CONFIG_CFI_CLANG))
			ifeq (y,$(CONFIG_CFI_PERMISSIVE))
				fpsgo-$(CONFIG_ARM64) += src/xgf_v3_ko_arm64_cfi_d.o
			else
				fpsgo-$(CONFIG_ARM64) += src/xgf_v3_ko_arm64_cfi.o
			endif
		else
			fpsgo-$(CONFIG_ARM64) += src/xgf_v3_ko_arm64.o
		endif
	endif
else
	ifeq (m,$(CONFIG_MTK_FPSGO_V3))
		fpsgo-y += src/xgf_v3_ko_quark.o
	endif
endif