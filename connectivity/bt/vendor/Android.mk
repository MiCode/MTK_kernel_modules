###############################################################################
# Generally Android.mk can not get KConfig setting
# we can use this way to get
# include the final KConfig
# but there is no $(AUTO_CONF) at the first time (no out folder) when make
#
#ifneq (,$(wildcard $(AUTO_CONF)))
#include $(AUTO_CONF)
#include $(CLEAR_VARS)
#endif

###############################################################################
###############################################################################
# Generally Android.mk can not get KConfig setting                            #
#                                                                             #
# do not have any KConfig checking in Android.mk                              #
# do not have any KConfig checking in Android.mk                              #
# do not have any KConfig checking in Android.mk                              #
#                                                                             #
# e.g. ifeq ($(CONFIG_MTK_COMBO_WIFI), m)                                     #
#          xxxx                                                               #
#      endif                                                                  #
#                                                                             #
# e.g. ifneq ($(filter "MT6632",$(CONFIG_MTK_COMBO_CHIP)),)                   #
#          xxxx                                                               #
#      endif                                                                  #
#                                                                             #
# All the KConfig checking should move to Makefile for each module            #
# All the KConfig checking should move to Makefile for each module            #
# All the KConfig checking should move to Makefile for each module            #
#                                                                             #
###############################################################################
###############################################################################

LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_BT_SUPPORT),yes)
$(warning cccc "$(MTK_BT_CHIP)")
include $(CLEAR_VARS)
LOCAL_MODULE := btmtk_vnd.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_REQUIRED_MODULES := bt_drv_6878.ko
LOCAL_INIT_RC := init.btmtk_vendor.rc
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH) -type f -name '*.[cho]')) Makefile
include $(MTK_KERNEL_MODULE)
BT_OPTS := BT_ANDROID_MK_BUILD=yes
endif
