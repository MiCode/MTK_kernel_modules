# Support GKI mixed build
ifeq ($(DEVICE_MODULES_PATH),)
DEVICE_MODULES_PATH = $(srctree)
else
LINUXINCLUDE := $(DEVCIE_MODULES_INCLUDE) $(LINUXINCLUDE)
endif

KO_MTKCAM_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)
KO_MTKCAM_INCLUDE_PATH := $(KO_MTKCAM_CODE_PATH)/include

$(info DEVICE_MODULES_PATH=$(DEVICE_MODULES_PATH) LINUXINCLUDE=$(LINUXINCLUDE))
$(info KO_MTKCAM_CODE_PATH=$(KO_MTKCAM_CODE_PATH))
$(info KO_MTKCAM_INCLUDE_PATH=$(KO_MTKCAM_INCLUDE_PATH))

subdir-ccflags-y += -I$(KO_MTKCAM_CODE_PATH)/ -I$(KO_MTKCAM_INCLUDE_PATH)/

obj-y += camsys/
obj-m += scpsys/
obj-m += cam_cal/
obj-$(CONFIG_VIDEO_MTK_ISP_IMGSYS) += imgsys/
obj-$(CONFIG_VIDEO_MTK_ISP_HCP) += mtk-hcp/
obj-$(CONFIG_VIDEO_MTK_ISP_IMGSYS) += mtk-ipesys-me/
obj-$(CONFIG_MTK_CAMERA_FD_ISP7S_ISP7SP)	+= mtk-aie/
obj-$(CONFIG_MTK_CAMERA_MAE_SUPPORT) += mtk-mae/

obj-$(CONFIG_MTK_CAMERA_DPE_ISP7SP)	 += mtk-dpe/
obj-$(CONFIG_MTK_CAMERA_ISP_PDA_SUPPORT) += mtk-pda/
obj-$(CONFIG_MTK_C2PS) += sched/
obj-y += imgsensor/
obj-$(CONFIG_MTK_CCU_RPROC) += ccusys/
obj-$(CONFIG_MTK_IMGSYS_FRM_SYNC_ISP8) += img_frm_sync/
obj-$(CONFIG_MTK_ISP_PSPM) += isp_pspm/

