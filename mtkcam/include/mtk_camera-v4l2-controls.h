/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAMERA_V4l2_CONTROLS_H
#define __MTK_CAMERA_V4l2_CONTROLS_H

/**
 * To be noticed:
 * This header is for ISP7_1 and ISP7S platform only.
 */

/* Common part of all platform */
#include "mtk_camera-v4l2-controls-common.h"

/* C A M S Y S */
#define V4L2_CID_MTK_CAM_USED_ENGINE_LIMIT \
	(V4L2_CID_USER_MTK_CAM_BASE + 1)
#define V4L2_CID_MTK_CAM_BIN_LIMIT \
	(V4L2_CID_USER_MTK_CAM_BASE + 2)
#define V4L2_CID_MTK_CAM_FRZ_LIMIT \
	(V4L2_CID_USER_MTK_CAM_BASE + 3)
#define V4L2_CID_MTK_CAM_RESOURCE_PLAN_POLICY \
	(V4L2_CID_USER_MTK_CAM_BASE + 4)
#define V4L2_CID_MTK_CAM_USED_ENGINE \
	(V4L2_CID_USER_MTK_CAM_BASE + 5)
#define V4L2_CID_MTK_CAM_BIN \
	(V4L2_CID_USER_MTK_CAM_BASE + 6)
#define V4L2_CID_MTK_CAM_FRZ \
	(V4L2_CID_USER_MTK_CAM_BASE + 7)
#define V4L2_CID_MTK_CAM_USED_ENGINE_TRY \
	(V4L2_CID_USER_MTK_CAM_BASE + 8)
#define V4L2_CID_MTK_CAM_BIN_TRY \
	(V4L2_CID_USER_MTK_CAM_BASE + 9)
#define V4L2_CID_MTK_CAM_FRZ_TRY \
	(V4L2_CID_USER_MTK_CAM_BASE + 10)
#define V4L2_CID_MTK_CAM_PIXEL_RATE \
	(V4L2_CID_USER_MTK_CAM_BASE + 11)
#define V4L2_CID_MTK_CAM_FEATURE \
	(V4L2_CID_USER_MTK_CAM_BASE + 12)
#define V4L2_CID_MTK_CAM_SYNC_ID \
	(V4L2_CID_USER_MTK_CAM_BASE + 13)
#define V4L2_CID_MTK_CAM_RAW_PATH_SELECT \
	(V4L2_CID_USER_MTK_CAM_BASE + 14)
#define V4L2_CID_MTK_CAM_HSF_EN \
	(V4L2_CID_USER_MTK_CAM_BASE + 15)
#define V4L2_CID_MTK_CAM_PDE_INFO \
	(V4L2_CID_USER_MTK_CAM_BASE + 16)
#define V4L2_CID_MTK_CAM_MSTREAM_EXPOSURE \
	(V4L2_CID_USER_MTK_CAM_BASE + 17)
#define V4L2_CID_MTK_CAM_RAW_RESOURCE_CALC \
	(V4L2_CID_USER_MTK_CAM_BASE + 18)
#define V4L2_CID_MTK_CAM_TG_FLASH_CFG \
	(V4L2_CID_USER_MTK_CAM_BASE + 19)
#define V4L2_CID_MTK_CAM_RAW_RESOURCE_UPDATE \
	(V4L2_CID_USER_MTK_CAM_BASE + 20)
#define V4L2_CID_MTK_CAM_CAMSYS_HW_MODE \
	(V4L2_CID_USER_MTK_CAM_BASE + 21)
#define V4L2_CID_MTK_CAM_FRAME_SYNC \
	(V4L2_CID_USER_MTK_CAM_BASE + 22)
#define V4L2_CID_MTK_CAM_STREAMOFF_FLAGS \
	(V4L2_CID_USER_MTK_CAM_BASE + 23)
#define V4L2_CID_MTK_CAM_INTERNAL_MEM_CTRL \
	(V4L2_CID_USER_MTK_CAM_BASE + 24)
#define V4L2_CID_MTK_CAM_CAMSYS_HDR_TIMESTAMP \
	(V4L2_CID_USER_MTK_CAM_BASE + 25)
#define V4L2_CID_MTK_CAM_APU_INFO \
	(V4L2_CID_USER_MTK_CAM_BASE + 26)
#define V4L2_CID_MTK_CAM_CAMSYS_VF_RESET \
	(V4L2_CID_USER_MTK_CAM_BASE + 27)

/* used for v2 resoruce struct testing */
#define V4L2_CID_MTK_CAM_RAW_RESOURCE_CALC_TEST \
	(V4L2_CID_USER_MTK_CAM_BASE + 47)

/* Allowed value of V4L2_CID_MTK_CAM_RAW_PATH_SELECT */
#define V4L2_MTK_CAM_RAW_PATH_SELECT_BPC	1
#define V4L2_MTK_CAM_RAW_PATH_SELECT_FUS	3
#define V4L2_MTK_CAM_RAW_PATH_SELECT_DGN	4
#define V4L2_MTK_CAM_RAW_PATH_SELECT_LSC	5
#define V4L2_MTK_CAM_RAW_PATH_SELECT_HLR	6
#define V4L2_MTK_CAM_RAW_PATH_SELECT_LTM	7

#define V4L2_MTK_CAM_TG_FALSH_ID_MAX		4
#define V4L2_MTK_CAM_TG_FLASH_MODE_SINGLE	0
#define V4L2_MTK_CAM_TG_FLASH_MODE_CONTINUOUS	1
#define V4L2_MTK_CAM_TG_FLASH_MODE_MULTIPLE	2

/* Support flags for V4L2_CID_MTK_CAM_STREAMOFF_FLAGS */
#define V4L2_MTK_CAM_STREAMOFF_FLAG_KEEP_BUF		0x00000001
#define V4L2_MTK_CAM_STREAMOFF_FLAG_FORCE_RELEASE_BUF	0x00000002

struct mtk_cam_shutter_gain {
	__u32 shutter;
	__u32 gain;
};

struct mtk_cam_mstream_exposure {
	struct mtk_cam_shutter_gain exposure[2];
	unsigned int valid;
	int req_id;
};

/* store the tg flush setting from user */
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


#define MTK_CAM_RESOURCE_DEFAULT	0xFFFF

/*
 * struct mtk_cam_resource_sensor - sensor resoruces for format negotiation
 *
 */
struct mtk_cam_resource_sensor {
	struct v4l2_fract interval;
	__u32 hblank;
	__u32 vblank;
	__u64 pixel_rate;
	__u64 cust_pixel_rate;
	__u64 driver_buffered_pixel_rate;
};

/*
 * struct mtk_cam_resource_raw - MTK camsys raw resoruces for format negotiation
 *
 * @feature: value of V4L2_CID_MTK_CAM_FEATURE the user want to check the
 *		  resource with. If it is used in set CTRL, we will apply the value
 *		  to V4L2_CID_MTK_CAM_FEATURE ctrl directly.
 * @strategy: indicate the order of multiple raws, binning or DVFS to be selected
 *	      when doing format negotiation of raw's source pads (output pads).
 *	      Please pass MTK_CAM_RESOURCE_DEFAULT if you want camsys driver to
 *	      determine it.
 * @raw_max: indicate the max number of raw to be used for the raw pipeline.
 *	     Please pass MTK_CAM_RESOURCE_DEFAULT if you want camsys driver to
 *	     determine it.
 * @raw_min: indicate the max number of raw to be used for the raw pipeline.
 *	     Please pass MTK_CAM_RESOURCE_DEFAULT if you want camsys driver to
 *	     determine it.
 * @raw_used: The number of raw used. The used don't need to writ this failed,
 *	      the driver always updates the field.
 * @bin: indicate if the driver should enable the bining or not. The driver
 *	 update the field depanding the hardware supporting status. Please pass
 *	 MTK_CAM_RESOURCE_DEFAULT if you want camsys driver to determine it.
 * @path_sel: indicate the user selected raw path. The driver
 *	      update the field depanding the hardware supporting status. Please
 *	      pass MTK_CAM_RESOURCE_DEFAULT if you want camsys driver to
 *	      determine it.
 * @pixel_mode: the pixel mode driver used in the raw pipeline. It is written by
 *		driver only.
 * @throughput: the throughput be used in the raw pipeline. It is written by
 *		driver only.
 * @img_wbuf_size: the img working buffer size considering chaning sensor after
 *		   streaming on, it is required in VHDR SAT scenarios.
 *
 */
struct mtk_cam_resource_raw {
	__s64	feature;
	__u16	strategy;
	__u8	raw_max;
	__u8	raw_min;
	__u8	raw_used;
	__u8	bin;
	__u8	path_sel;
	__u8	pixel_mode;
	__u64	throughput;
	__s64	hw_mode;
	__u32	img_wbuf_size;
	__u32	max_img_wbuf_size;
};

/*
 * struct mtk_cam_resource - MTK camsys resoruces for format negotiation
 *
 * @sink_fmt: sink_fmt pad's format, it must be return by g_fmt or s_fmt
 *		from driver.
 * @sensor_res: senor information to calculate the required resource, it is
 *		read-only and camsys driver will not change it.
 * @raw_res: user hint and resource negotiation result.
 * @status:	TBC
 *
 */
struct mtk_cam_resource {
	struct v4l2_mbus_framefmt *sink_fmt;
	struct mtk_cam_resource_sensor sensor_res;
	struct mtk_cam_resource_raw raw_res;
	__u8 status;
};

enum mtk_cam_ctrl_type {
	CAM_SET_CTRL = 0,
	CAM_TRY_CTRL,
	CAM_CTRL_NUM,
};

/**
 * struct mtk_cam_pde_info - PDE module information for raw
 *
 * @pdo_max_size: the max pdo size of pde sensor.
 * @pdi_max_size: the max pdi size of pde sensor or max pd table size.
 * @pd_table_offset: the offest of meta config for pd table content.
 * @meta_cfg_size: the enlarged meta config size.
 * @meta_0_size: the enlarged meta 0 size.
 */
struct mtk_cam_pde_info {
	__u32 pdo_max_size;
	__u32 pdi_max_size;
	__u32 pd_table_offset;
	__u32 meta_cfg_size;
	__u32 meta_0_size;
};

/**
 *  mtk cam V4l2 ctrl structures V2
 */

/**
 * struct mtk_cam_resource_sensor_v2
 *
 * @no_bufferd_prate_calc: notify driver don't use buffered pixel rate
 *			   as the thrpughput requriement; use
 *			   pixel_rate passed directly. If the pixel_rate is coming
 *			   from sensor driver's custom pixel rate, please set it
 *			   true since we can't trust the hblank value in such
 *			   case;
 * @driver_buffered_pixel_rate: only used in legacy driver and could be phased-out.
 */
struct mtk_cam_resource_sensor_v2 {
	__u32	width;
	__u32	height;
	__u32	code;
	struct	v4l2_fract interval;
	__u32	hblank;
	__u32	vblank;
	__u64	pixel_rate;
	__u8	no_bufferd_prate_calc;
	__u64	driver_buffered_pixel_rate;
};

/**
 *  enum mtk_cam_scen_id - camsys hardware scenario ids
 *
 * @MTK_CAM_SCEN_NORMAL: The default scenario
 * MTK_CAM_SCEN_M2M_NORMAL: the m2m scenario
 */
enum mtk_cam_scen_id {
	MTK_CAM_SCEN_NORMAL,
	MTK_CAM_SCEN_MSTREAM,
	MTK_CAM_SCEN_SMVR,
	MTK_CAM_SCEN_ODT_NORMAL,
	MTK_CAM_SCEN_ODT_MSTREAM,
	MTK_CAM_SCEN_M2M_NORMAL,
	MTK_CAM_SCEN_TIMESHARE,
	MTK_CAM_SCEN_CAMSV_RGBW, // for ISP7.1, output W chn via CAMSV
	MTK_CAM_SCEN_EXT_ISP,
};

enum mtk_cam_exp_order {
	MTK_CAM_EXP_SE_LE,
	MTK_CAM_EXP_LE_SE,
};

enum mtk_cam_frame_order {
	MTK_CAM_FRAME_BAYER_W,
	MTK_CAM_FRAME_W_BAYER,
};

/**
 * struct mtk_cam_scen_normal - common properties
 *         in different scenario
 * @max_exp_num: max number of exposure
 * @exp_num: current number of exposure
 * @exp_order: order of exposure readout,
 *         see mtk_cam_exp_order
 * @w_chn_supported: support W channel
 * @w_chn_enabled: w/ or w/o W channel
 * @frame_order: order of bayer-w, see mtk_cam_frame_order
 * @mem_saving: memory saving
 */
struct mtk_cam_scen_normal {
	__u8 max_exp_num:4;
	__u8 exp_num:4;
	__u8 exp_order:4;
	__u8 w_chn_supported:4;
	__u8 w_chn_enabled:4;
	__u8 frame_order:4;
	__u8 mem_saving:4;
};

enum mtk_cam_mstream_type {
	MTK_CAM_MSTREAM_1_EXPOSURE		= 0,
	MTK_CAM_MSTREAM_NE_SE			= 5,
	MTK_CAM_MSTREAM_SE_NE			= 6,
};

/**
 * struct mtk_cam_scen_mstream - mstream scenario user hints
 *
 * @type: the hardware scenario of the frame, please check
 *	       mtk_cam_mstream_type for the allowed value
 * @mem_saving: 1 means enable mem_saving
 */
struct mtk_cam_scen_mstream {
	__u32	type;
	__u8	mem_saving;
};

enum mtk_cam_subsample_num_allowed {
	MTK_CAM_SMVR_2_SUBSAMPLE		= 2,
	MTK_CAM_SMVR_4_SUBSAMPLE		= 4,
	MTK_CAM_SMVR_8_SUBSAMPLE		= 8,
	MTK_CAM_SMVR_16_SUBSAMPLE		= 16,
	MTK_CAM_SMVR_32_SUBSAMPLE		= 32,
};

/**
 * struct mtk_cam_scen_smvr - smvr scenario user hints
 *
 * @subsample_num: the subsample number of the frame, please check
 *	       mtk_cam_subsample_num_allowed for the allowed value
 * @output_first_frame_only: set it true when the user donesn't need
 *		     the images except the first one in SMVR scenario.
 */
struct mtk_cam_scen_smvr {
	__u8 subsample_num;
	__u8 output_first_frame_only;
};

enum mtk_cam_extisp_type {
	MTK_CAM_EXTISP_CUS_1			= 1,
	MTK_CAM_EXTISP_CUS_2			= 2,
	MTK_CAM_EXTISP_CUS_3			= 3,
};

/**
 * struct mtk_cam_scen_extisp - smvr scenario user hints
 *
 * @subsampletype_num: the ext isp type of the frame, please check
 *	       mtk_cam_extisp_type for the allowed value
 */
struct mtk_cam_scen_extisp {
	enum mtk_cam_extisp_type type;
};

enum mtk_cam_timeshare_group {
	MTK_CAM_TIMESHARE_GROUP_1 = 1,
};

/**
 * struct mtk_cam_scen_timeshare - smvr scenario user hints
 *
 * @group: the time group of the frame, please check
 *	       mtk_cam_timeshare_group for the allowed value
 */
struct mtk_cam_scen_timeshare {
	__u8 group;
};

/**
 * struct mtk_cam_scen - hardware scenario user hints
 *
 * @id: the id of the hardware scenario.
 * @scen: union of struct of diff scenario:
 * MTK_CAM_SCEN_NORMAL, MTK_CAM_SCEN_ODT_NORMAL,
 * MTK_CAM_SCEN_M2M_NORMAL=> normal
 * MTK_CAM_SCEN_MSTREAM => mstream
 * MTK_CAM_SCEN_SMVR => smvr
 * MTK_CAM_SCEN_EXT_ISP => extisp
 * MTK_CAM_SCEN_TIMESHARE => timeshare
 */

struct mtk_cam_scen {
	enum mtk_cam_scen_id id;
	union {
		struct mtk_cam_scen_normal normal;
		struct mtk_cam_scen_mstream	mstream;
		struct mtk_cam_scen_smvr	smvr;
		struct mtk_cam_scen_extisp	extisp;
		struct mtk_cam_scen_timeshare	timeshare;
	} scen;
	char dbg_str[16];
};

#define MTK_CAM_RAW_A	0x0001
#define MTK_CAM_RAW_B	0x0002
#define MTK_CAM_RAW_C	0x0004

/* to be refined, not to use bit mask */
enum mtk_cam_bin {
	MTK_CAM_BIN_OFF	=	0,
	MTK_CAM_BIN_ON =	(1 << 0),
	MTK_CAM_CBN_2X2_ON =	(1 << 4),
	MTK_CAM_CBN_3X3_ON =	(1 << 5),
	MTK_CAM_CBN_4X4_ON =	(1 << 6),
	MTK_CAM_QBND_ON	=	(1 << 8)
};

/**
 * struct mtk_cam_resource_raw_v2
 *
 * @raws_max_num: Max number of raws can be used. This is only
 *		  used when user let driver select raws.
 *		  (raws = 0, and raws_must = 0)
 */
struct mtk_cam_resource_raw_v2 {
	struct mtk_cam_scen scen;
	__u8	raws;
	__u8	raws_must;
	__u8	raws_max_num;
	__u8	bin;
	__u8	raw_pixel_mode;
	__u8	hw_mode;
	__u32	img_wbuf_size;
	__u32	img_wbuf_num;
};

struct mtk_cam_resource_v2 {
	struct mtk_cam_resource_sensor_v2 sensor_res;
	struct mtk_cam_resource_raw_v2 raw_res;
};

#define MTK_CAM_INTERNAL_MEM_MAX 8

struct mtk_cam_internal_buf {
	__s32 fd;
	__u32 length;
};

struct mtk_cam_internal_mem {
	__u32 num;
	struct mtk_cam_internal_buf bufs[MTK_CAM_INTERNAL_MEM_MAX];
};

/**
 * struct mtk_cam_apu_info - apu related information
 *  @is_update: kernel control only
 *
 */

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
	__u8 is_update;
	__u8 apu_path;
	__u8 vpu_i_point;
	__u8 vpu_o_point;
	__u8 sysram_en;
	__u8 opp_index;
	__u32 block_y_size;
};

#endif /* __MTK_CAMERA_V4l2_CONTROLS_H */
