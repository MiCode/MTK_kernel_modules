/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAMERA_V4l2_CONTROLS_7SP_H
#define __MTK_CAMERA_V4l2_CONTROLS_7SP_H

/* For ISP7SP platform only */
#include "mtk_camera-v4l2-controls-common.h"

#define V4L2_CID_MTK_CAM_USED_ENGINE_LIMIT (V4L2_CID_USER_MTK_CAM_BASE + 1)
#define V4L2_CID_MTK_CAM_BIN_LIMIT (V4L2_CID_USER_MTK_CAM_BASE + 2)
#define V4L2_CID_MTK_CAM_FRZ_LIMIT (V4L2_CID_USER_MTK_CAM_BASE + 3)
#define V4L2_CID_MTK_CAM_RESOURCE_PLAN_POLICY (V4L2_CID_USER_MTK_CAM_BASE + 4)
#define V4L2_CID_MTK_CAM_USED_ENGINE (V4L2_CID_USER_MTK_CAM_BASE + 5)
#define V4L2_CID_MTK_CAM_BIN (V4L2_CID_USER_MTK_CAM_BASE + 6)
#define V4L2_CID_MTK_CAM_FRZ (V4L2_CID_USER_MTK_CAM_BASE + 7)
#define V4L2_CID_MTK_CAM_USED_ENGINE_TRY (V4L2_CID_USER_MTK_CAM_BASE + 8)
#define V4L2_CID_MTK_CAM_BIN_TRY (V4L2_CID_USER_MTK_CAM_BASE + 9)
#define V4L2_CID_MTK_CAM_FRZ_TRY (V4L2_CID_USER_MTK_CAM_BASE + 10)
#define V4L2_CID_MTK_CAM_PIXEL_RATE (V4L2_CID_USER_MTK_CAM_BASE + 11)
#define V4L2_CID_MTK_CAM_FEATURE (V4L2_CID_USER_MTK_CAM_BASE + 12)
#define V4L2_CID_MTK_CAM_SYNC_ID (V4L2_CID_USER_MTK_CAM_BASE + 13)
#define V4L2_CID_MTK_CAM_RAW_PATH_SELECT (V4L2_CID_USER_MTK_CAM_BASE + 14)
#define V4L2_CID_MTK_CAM_HSF_EN (V4L2_CID_USER_MTK_CAM_BASE + 15)

#define V4L2_CID_MTK_CAM_MSTREAM_EXPOSURE (V4L2_CID_USER_MTK_CAM_BASE + 17)
#define V4L2_CID_MTK_CAM_RAW_RESOURCE_CALC (V4L2_CID_USER_MTK_CAM_BASE + 18)
#define V4L2_CID_MTK_CAM_TG_FLASH_CFG (V4L2_CID_USER_MTK_CAM_BASE + 19)
#define V4L2_CID_MTK_CAM_RAW_RESOURCE_UPDATE (V4L2_CID_USER_MTK_CAM_BASE + 20)
#define V4L2_CID_MTK_CAM_CAMSYS_HW_MODE (V4L2_CID_USER_MTK_CAM_BASE + 21)
#define V4L2_CID_MTK_CAM_FRAME_SYNC (V4L2_CID_USER_MTK_CAM_BASE + 22)
#define V4L2_CID_MTK_CAM_INTERNAL_MEM_CTRL (V4L2_CID_USER_MTK_CAM_BASE + 24)
#define V4L2_CID_MTK_CAM_CAMSYS_HDR_TIMESTAMP (V4L2_CID_USER_MTK_CAM_BASE + 25)
#define V4L2_CID_MTK_CAM_APU_INFO (V4L2_CID_USER_MTK_CAM_BASE + 26)
#define V4L2_CID_MTK_CAM_CAMSYS_VF_RESET (V4L2_CID_USER_MTK_CAM_BASE + 27)
#define V4L2_CID_MTK_CAM_REQ_INFO (V4L2_CID_USER_MTK_CAM_BASE + 28)
#define V4L2_CID_MTK_CAM_CQ_TRIGGER_DEADLINE (V4L2_CID_USER_MTK_CAM_BASE + 29)

#define V4L2_CID_MTK_CAM_RAW_RESOURCE_CALC_TEST (V4L2_CID_USER_MTK_CAM_BASE + 47)


#define V4L2_MTK_CAM_RAW_PATH_SELECT_BPC 1
#define V4L2_MTK_CAM_RAW_PATH_SELECT_FUS 3
#define V4L2_MTK_CAM_RAW_PATH_SELECT_DGN 4
#define V4L2_MTK_CAM_RAW_PATH_SELECT_LSC 5
#define V4L2_MTK_CAM_RAW_PATH_SELECT_HLR 6
#define V4L2_MTK_CAM_RAW_PATH_SELECT_LTM 7

#define V4L2_MTK_CAM_TG_FALSH_ID_MAX 4
#define V4L2_MTK_CAM_TG_FLASH_MODE_SINGLE 0
#define V4L2_MTK_CAM_TG_FLASH_MODE_CONTINUOUS 1
#define V4L2_MTK_CAM_TG_FLASH_MODE_MULTIPLE 2

struct mtk_cam_shutter_gain {
	__u32 shutter;
	__u32 gain;
};

struct mtk_cam_mstream_exposure {
	struct mtk_cam_shutter_gain exposure[2];
	int req_id;
};

enum mtk_cam_scen_id {
	MTK_CAM_SCEN_NORMAL,
	MTK_CAM_SCEN_MSTREAM,
	MTK_CAM_SCEN_SMVR,
	MTK_CAM_SCEN_ODT_NORMAL,
	MTK_CAM_SCEN_ODT_MSTREAM,
	MTK_CAM_SCEN_M2M_NORMAL,
	MTK_CAM_SCEN_TIMESHARE,
	MTK_CAM_SCEN_CAMSV_RGBW,
	MTK_CAM_SCEN_EXT_ISP,
};

enum mtk_cam_exp_order {
	MTK_CAM_EXP_SE_LE,
	MTK_CAM_EXP_LE_SE,
	MTK_CAM_EXP_SE_ME_LE,
	MTK_CAM_EXP_LE_ME_SE,
};

enum mtk_cam_frame_order {
	MTK_CAM_FRAME_BAYER_W,
	MTK_CAM_FRAME_W_BAYER,
};

enum mtk_cam_stagger_type {
	MTK_CAM_STAGGER_NONE,
	MTK_CAM_STAGGER_NORMAL,
	MTK_CAM_STAGGER_DCG_SENSOR_MERGE,
	MTK_CAM_STAGGER_DCG_AP_MERGE,
	MTK_CAM_STAGGER_LBMF,
};

struct mtk_cam_scen_normal {
	__u8 max_exp_num : 4;
	__u8 exp_num : 4;
	__u8 exp_order : 4;
	__u8 w_chn_supported : 4;
	__u8 w_chn_enabled : 4;
	__u8 frame_order : 4;
	__u8 mem_saving : 4;
	__u8 stagger_type : 4;
};

enum mtk_cam_mstream_type {
	MTK_CAM_MSTREAM_1_EXPOSURE = 0,
	MTK_CAM_MSTREAM_NE_SE = 5,
	MTK_CAM_MSTREAM_SE_NE = 6,
};

struct mtk_cam_scen_mstream {
	__u32 type;
	__u8 mem_saving;
};

enum mtk_cam_subsample_num_allowed {
	MTK_CAM_SMVR_2_SUBSAMPLE = 2,
	MTK_CAM_SMVR_4_SUBSAMPLE = 4,
	MTK_CAM_SMVR_8_SUBSAMPLE = 8,
	MTK_CAM_SMVR_16_SUBSAMPLE = 16,
	MTK_CAM_SMVR_32_SUBSAMPLE = 32,
};

struct mtk_cam_scen_smvr {
	__u8 subsample_num;
	__u8 output_first_frame_only;
};

enum mtk_cam_extisp_type {
	MTK_CAM_EXTISP_CUS_1 = 1,
	MTK_CAM_EXTISP_CUS_2 = 2,
	MTK_CAM_EXTISP_CUS_3 = 3,
};

struct mtk_cam_scen_extisp {
	enum mtk_cam_extisp_type type;
};

enum mtk_cam_timeshare_group {
	MTK_CAM_TIMESHARE_GROUP_1 = 1,
};

struct mtk_cam_scen_timeshare {
	__u8 group;
};

struct mtk_cam_scen {
	enum mtk_cam_scen_id id;
	union {
		struct mtk_cam_scen_normal normal;
		struct mtk_cam_scen_mstream mstream;
		struct mtk_cam_scen_smvr smvr;
		struct mtk_cam_scen_extisp extisp;
		struct mtk_cam_scen_timeshare timeshare;
	} scen;
};

enum mtk_cam_bin {
	MTK_CAM_BIN_OFF = 0,
	MTK_CAM_BIN_ON = (1 << 0),
	MTK_CAM_CBN_2X2_ON = (1 << 4),
	MTK_CAM_CBN_3X3_ON = (1 << 5),
	MTK_CAM_CBN_4X4_ON = (1 << 6),
	MTK_CAM_QBND_ON = (1 << 8)
};

enum mtk_cam_hw_mode {
	MTK_CAM_HW_MODE_DEFAULT = 0,
	MTK_CAM_HW_MODE_ON_THE_FLY = 1,
	MTK_CAM_HW_MODE_DIRECT_COUPLED = 2,
};

enum mtk_cam_data_pattern {
	MTK_CAM_PATTERN_BAYER,
	MTK_CAM_PATTERN_4CELL,
};

enum mtk_cam_sen_apply_ctrl {
	MTK_CAM_SEN_APPLY_NORMAL,
	MTK_CAM_SEN_APPLY_DIRECT_APPLY,
};

struct mtk_cam_resource_sensor_v2 {
	__u32 width;
	__u32 height;
	__u32 code;
	struct v4l2_fract interval;
	__u32 hblank;
	__u32 vblank;
	__u64 pixel_rate;
	__u8 no_bufferd_prate_calc;
	__u64 driver_buffered_pixel_rate;
	__u8 pattern;
};

#define MTK_CAM_RAW_A 0x0001
#define MTK_CAM_RAW_B 0x0002
#define MTK_CAM_RAW_C 0x0004
#define MTK_CAM_RESOURCE_DEFAULT 0xFFFF

struct mtk_cam_resource_raw_v2 {
	struct mtk_cam_scen scen;
	__u8 raws;
	__u8 raws_must;
	__u8 raws_max_num;
	__u8 bin;
	__u8 raw_pixel_mode;
	__u8 camsv_pixel_mode;
	__u8 hw_mode;
	__u32 freq;
	__u32 img_wbuf_size;
	__u32 img_wbuf_num;
	__u32 slb_size;
	__u8 sen_apply_ctrl;
};

struct mtk_cam_resource_v2 {
	struct mtk_cam_resource_sensor_v2 sensor_res;
	struct mtk_cam_resource_raw_v2 raw_res;
};

struct mtk_cam_tg_flash_config {
	__u32 flash_enable;
	__u32 flash_mode;
	__u32 flash_pluse_num;
	__u32 flash_offset;
	__u32 flash_high_width;
	__u32 flash_low_width;
	__u32 flash_light_id;
};

struct mtk_cam_hdr_timestamp_info {
	__u64 le;
	__u64 le_mono;
	__u64 ne;
	__u64 ne_mono;
	__u64 se;
	__u64 se_mono;
};

struct mtk_cam_internal_buf {
	__s32 fd;
	__u32 length;
};

#define MTK_CAM_INTERNAL_MEM_MAX 8

struct mtk_cam_internal_mem {
	__u32 num;
	struct mtk_cam_internal_buf bufs[MTK_CAM_INTERNAL_MEM_MAX];
};

enum mtk_cam_apu_tap_point {
	AFTER_SEP_R1,
	AFTER_BPC,
	AFTER_LTM,
};

enum mtk_cam_apu_path {
	APU_NONE,
	APU_FRAME_MODE,
	APU_DC_RAW,
	RAW_DC_APU,
	RAW_DC_APU_DC_RAW,
};

struct mtk_cam_apu_info {
	__u8 apu_path;
	__u8 vpu_i_point;
	__u8 vpu_o_point;
	__u8 sysram_en;
	__u8 opp_index;
	__u32 block_y_size;
};
enum mtk_cam_req_type {
	NORMAL_REQUEST,
	SENSOR_REQUEST,
	ISP_REQUEST,
};

struct mtk_cam_req_info {
	__u8 req_type;
	__u32 req_sync_id;
};

#endif /* __MTK_CAMERA_V4l2_CONTROLS_7SP_H */
