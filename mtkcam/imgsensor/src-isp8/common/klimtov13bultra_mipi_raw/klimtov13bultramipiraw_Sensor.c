// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 klimtov13bultramipiraw_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include "klimtov13bultramipiraw_Sensor.h"

static void set_group_hold(void *arg, u8 en);
static void klimtov13bultra_set_dummy(struct subdrv_ctx *ctx);
static int klimtov13bultra_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static u16 get_gain2reg(u32 gain);
static int klimtov13bultra_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id);
static int klimtov13bultra_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static int ultra_vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt);
static int klimtov13bultra_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int klimtov13bultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int klimtov13bultra_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int klimtov13bultra_set_multi_dig_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int klimtov13bultra_get_csi_param(struct subdrv_ctx *ctx, enum SENSOR_SCENARIO_ID_ENUM scenario_id, struct mtk_csi_param *csi_param);

/* STRUCT */
static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, klimtov13bultra_set_test_pattern},
	{SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO, klimtov13bultra_set_max_framerate_by_scenario},
	{SENSOR_FEATURE_SET_ESHUTTER, klimtov13bultra_set_shutter},
	{SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME, klimtov13bultra_set_shutter_frame_length},
	{SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME, klimtov13bultra_set_multi_shutter_frame_length},
	{SENSOR_FEATURE_SET_MULTI_DIG_GAIN, klimtov13bultra_set_multi_dig_gain}
};

static struct eeprom_info_struct eeprom_info[] = {
	{
		// .header_id = 0x010B00FF,
		// .addr_header_id = 0x0000000B,
		.i2c_write_id = 0xA0,

		.pdc_support = TRUE,
		.pdc_size = 720,
		.addr_pdc = 0x12D2,
		.sensor_reg_addr_pdc = 0x5F80,

	},
};

// mode 0: 4000x3000@30fps, normal preview
static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4000,
			.vsize = 3000,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};
// mode 2: 4000*2256@30fps, noraml video
static struct mtk_mbus_frame_desc_entry frame_desc_video[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4000,
			.vsize = 2256,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};
// mode 5: 2000*1128@60fps 60fps video
static struct mtk_mbus_frame_desc_entry frame_desc_custom1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 2000,
			.vsize = 1128,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};
// mode 6: 3424*2568@24fps bokeh
static struct mtk_mbus_frame_desc_entry frame_desc_bokeh[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3840,
			.vsize = 2880,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

// mode 8: 1920*1440@30fps 2x bokeh
static struct mtk_mbus_frame_desc_entry frame_desc_bokeh2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 1920,
			.vsize = 1440,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

//mode 10: 4000*2500@30fps 16:10 video
static struct mtk_mbus_frame_desc_entry frame_desc_video2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4000,
			.vsize = 2500,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

// mode 0: 4000x3000
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
    .i4OffsetX   =   0,
    .i4OffsetY   =   0,
    .i4PitchX    =  32,
    .i4PitchY    =  32,
    .i4PairNum   =   8,
    .i4SubBlkW   =  16,
    .i4SubBlkH   =   8,
    .i4BlockNumX = 125,
    .i4BlockNumY = 93,
	.i4PosL = {
		{14, 6}, {30, 6}, {6, 10}, {22, 10},
		{14, 22}, {30, 22}, {6, 26}, {22, 26}
	},
	.i4PosR = {
		{14, 2}, {30, 2}, {6, 14}, {22, 14},
		{14, 18}, {30, 18}, {6, 30}, {22, 30}
	},
	.i4Crop = {
		{0, 0}, {0, 0}, {0, 372}, {0, 0}, {0, 0}, {0, 186}, {80, 60}, {80, 60}, {1040, 780}, {1040, 780}, {0, 250}
	},
    .iMirrorFlip = 0,	/*0 IMAGE_NORMAL, 1 IMAGE_H_MIRROR, 2 IMAGE_V_MIRROR, 3 IMAGE_HV_MIRROR*/
};

// mode 2: 4000x2256
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_normal_video =
{
    .i4OffsetX   =   0,
    .i4OffsetY   =   0,
    .i4PitchX    =  32,
    .i4PitchY    =  32,
    .i4PairNum   =   8,
    .i4SubBlkW   =  16,
    .i4SubBlkH   =   8,
    .i4BlockNumX = 125,
    .i4BlockNumY = 70,
	.i4PosL = {
		{14, 6}, {30, 6}, {6, 10}, {22, 10},
		{14, 22}, {30, 22}, {6, 26}, {22, 26}
	},
	.i4PosR = {
		{14, 2}, {30, 2}, {6, 14}, {22, 14},
		{14, 18}, {30, 18}, {6, 30}, {22, 30}
	},
	.i4Crop = {
		{0, 0}, {0, 0}, {0, 372}, {0, 0}, {0, 0}, {0, 186}, {80, 60}, {80, 60}, {1040, 780}, {1040, 780}, {0, 250}
	},
    .iMirrorFlip = 0,	/*0 IMAGE_NORMAL, 1 IMAGE_H_MIRROR, 2 IMAGE_V_MIRROR, 3 IMAGE_HV_MIRROR*/
};

// mode 6/7: 3840x2880
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_bokeh1 =
{
    .i4OffsetX   =   0,
    .i4OffsetY   =   0,
    .i4PitchX    =  32,
    .i4PitchY    =  32,
    .i4PairNum   =   8,
    .i4SubBlkW   =  16,
    .i4SubBlkH   =   8,
    .i4BlockNumX = 120,
    .i4BlockNumY = 90,
	.i4PosL = {
		{14, 6}, {30, 6}, {6, 10}, {22, 10},
		{14, 22}, {30, 22}, {6, 26}, {22, 26}
	},
	.i4PosR = {
		{14, 2}, {30, 2}, {6, 14}, {22, 14},
		{14, 18}, {30, 18}, {6, 30}, {22, 30}
	},
	.i4Crop = {
		{0, 0}, {0, 0}, {0, 372}, {0, 0}, {0, 0}, {0, 186}, {80, 60}, {80, 60}, {1040, 780}, {1040, 780}, {0, 250}
	},
    .iMirrorFlip = 0,	/*0 IMAGE_NORMAL, 1 IMAGE_H_MIRROR, 2 IMAGE_V_MIRROR, 3 IMAGE_HV_MIRROR*/
};

// mode 8/9: 1920x1440
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_bokeh2 =
{
    .i4OffsetX   =   0,
    .i4OffsetY   =   0,
    .i4PitchX    =  32,
    .i4PitchY    =  32,
    .i4PairNum   =   8,
    .i4SubBlkW   =  16,
    .i4SubBlkH   =   8,
    .i4BlockNumX = 60,
    .i4BlockNumY = 45,
	.i4PosL = {
		{14, 6}, {30, 6}, {6, 10}, {22, 10},
		{14, 22}, {30, 22}, {6, 26}, {22, 26}
	},
	.i4PosR = {
		{14, 2}, {30, 2}, {6, 14}, {22, 14},
		{14, 18}, {30, 18}, {6, 30}, {22, 30}
	},
	.i4Crop = {
		{0, 0}, {0, 0}, {0, 372}, {0, 0}, {0, 0}, {0, 186}, {80, 60}, {80, 60}, {1040, 780}, {1040, 780}, {0, 250}
	},
    .iMirrorFlip = 0,	/*0 IMAGE_NORMAL, 1 IMAGE_H_MIRROR, 2 IMAGE_V_MIRROR, 3 IMAGE_HV_MIRROR*/
};

// mode 10: 4000x2500
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_video =
{
    .i4OffsetX   =   0,
    .i4OffsetY   =   0,
    .i4PitchX    =  32,
    .i4PitchY    =  32,
    .i4PairNum   =   8,
    .i4SubBlkW   =  16,
    .i4SubBlkH   =   8,
    .i4BlockNumX = 125,
    .i4BlockNumY = 77,
	.i4PosL = {
		{14, 0}, {30, 0}, {6, 4}, {22, 4},
		{14, 16}, {30, 16}, {6, 20}, {22, 20}
	},
	.i4PosR = {
		{14, -4}, {30, -4}, {6, 8}, {22, 8},
		{14, 12}, {30, 12}, {6, 24}, {22, 24}
	},
	.i4Crop = {
		{0, 0}, {0, 0}, {0, 372}, {0, 0}, {0, 0}, {0, 186}, {80, 60}, {80, 60}, {1040, 780}, {1040, 780}, {0, 250}
	},
    .iMirrorFlip = 0,	/*0 IMAGE_NORMAL, 1 IMAGE_H_MIRROR, 2 IMAGE_V_MIRROR, 3 IMAGE_HV_MIRROR*/
};

// mode 0: 4000x3000@30fps, normal preview
#define preview_mode_struct \
{\
	.frame_desc = frame_desc_prev,\
	.num_entries = ARRAY_SIZE(frame_desc_prev),\
	.mode_setting_table = klimtov13bultra_preview_setting,\
	.mode_setting_len = ARRAY_SIZE(klimtov13bultra_preview_setting),\
	.seamless_switch_group = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_table = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_len = PARAM_UNDEFINED,\
	.hdr_mode = HDR_NONE,\
	.raw_cnt = 1,\
	.exp_cnt = 1,\
	.pclk = 112000000,\
	.linelength = 1176,\
	.framelength = 3174,\
	.max_framerate = 300,\
	.mipi_pixel_rate = 448000000,\
	.readout_length = 0,\
	.read_margin = 10,\
	.framelength_step = 2,\
	.imgsensor_winsize_info = {\
		.full_w = 4000,\
		.full_h = 3000,\
		.x0_offset = 0,\
		.y0_offset = 0,\
		.w0_size = 4000,\
		.h0_size = 3000,\
		.scale_w = 4000,\
		.scale_h = 3000,\
		.x1_offset = 0,\
		.y1_offset = 0,\
		.w1_size = 4000,\
		.h1_size = 3000,\
		.x2_tg_offset = 0,\
		.y2_tg_offset = 0,\
		.w2_tg_size = 4000,\
		.h2_tg_size = 3000,\
	},\
	.pdaf_cap = TRUE,\
	.imgsensor_pd_info = &imgsensor_pd_info,\
	.ae_binning_ratio = 1000,\
	.fine_integ_line = 0,\
	.delay_frame = 2,\
}

// mode 2: 4000*2256@30fps, noraml video
#define normal_video_mode_struct  \
{\
	.frame_desc = frame_desc_video,\
	.num_entries = ARRAY_SIZE(frame_desc_video),\
	.mode_setting_table = klimtov13bultra_normal_video_setting,\
	.mode_setting_len = ARRAY_SIZE(klimtov13bultra_normal_video_setting),\
	.seamless_switch_group = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_table = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_len = PARAM_UNDEFINED,\
	.hdr_mode = HDR_NONE,\
	.raw_cnt = 1,\
	.exp_cnt = 1,\
	.pclk = 112000000,\
	.linelength = 1176,\
	.framelength = 3174,\
	.max_framerate = 300,\
	.mipi_pixel_rate = 448000000,\
	.readout_length = 0,\
	.read_margin = 10,\
	.framelength_step = 2,\
	.imgsensor_winsize_info = {\
		.full_w = 4000,\
		.full_h = 3000,\
		.x0_offset = 0,\
		.y0_offset = 372,\
		.w0_size = 4000,\
		.h0_size = 2256,\
		.scale_w = 4000,\
		.scale_h = 2256,\
		.x1_offset = 0,\
		.y1_offset = 0,\
		.w1_size = 4000,\
		.h1_size = 2256,\
		.x2_tg_offset = 0,\
		.y2_tg_offset = 0,\
		.w2_tg_size = 4000,\
		.h2_tg_size = 2256,\
	},\
	.pdaf_cap = TRUE,\
	.imgsensor_pd_info = &imgsensor_pd_info_normal_video,\
	.ae_binning_ratio = 1000,\
	.fine_integ_line = 0,\
	.delay_frame = 2,\
}

// mode 5: 2000*1128@60fps 60fps video
#define custom1_struct  \
{\
	.frame_desc = frame_desc_custom1,\
	.num_entries = ARRAY_SIZE(frame_desc_custom1),\
	.mode_setting_table = klimtov13bultra_custom1_setting,\
	.mode_setting_len = ARRAY_SIZE(klimtov13bultra_custom1_setting),\
	.seamless_switch_group = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_table = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_len = PARAM_UNDEFINED,\
	.hdr_mode = HDR_NONE,\
	.raw_cnt = 1,\
	.exp_cnt = 1,\
	.pclk = 111767040,\
	.linelength = 1176,\
	.framelength = 1584,\
	.max_framerate = 600,\
	.mipi_pixel_rate = 448000000,\
	.readout_length = 0,\
	.read_margin = 10,\
	.framelength_step = 2,\
	.imgsensor_winsize_info = {\
		.full_w = 4000,\
		.full_h = 3000,\
		.x0_offset = 0,\
		.y0_offset = 372,\
		.w0_size = 4000,\
		.h0_size = 2256,\
		.scale_w = 2000,\
		.scale_h = 1128,\
		.x1_offset = 0,\
		.y1_offset = 0,\
		.w1_size = 2000,\
		.h1_size = 1128,\
		.x2_tg_offset = 0,\
		.y2_tg_offset = 0,\
		.w2_tg_size = 2000,\
		.h2_tg_size = 1128,\
	},\
	.pdaf_cap = FALSE,\
	.imgsensor_pd_info = PARAM_UNDEFINED,\
	.ae_binning_ratio = 1000,\
	.fine_integ_line = 0,\
	.delay_frame = 2,\
}

// mode 6: 3840*2880@30fps 1x bokeh
#define bokeh_mode_struct  \
{\
	.frame_desc = frame_desc_bokeh,\
	.num_entries = ARRAY_SIZE(frame_desc_bokeh),\
	.mode_setting_table = klimtov13bultra_custom2_setting,\
	.mode_setting_len = ARRAY_SIZE(klimtov13bultra_custom2_setting),\
	.seamless_switch_group = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_table = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_len = PARAM_UNDEFINED,\
	.hdr_mode = HDR_NONE,\
	.raw_cnt = 1,\
	.exp_cnt = 1,\
	.pclk = 112000000,\
	.linelength = 1176,\
	.framelength = 3968,\
	.max_framerate = 240,\
	.mipi_pixel_rate = 448000000,\
	.readout_length = 0,\
	.read_margin = 10,\
	.framelength_step = 2,\
	.imgsensor_winsize_info = {\
		.full_w = 4000,\
		.full_h = 3000,\
		.x0_offset = 80,\
		.y0_offset = 60,\
		.w0_size = 3840,\
		.h0_size = 2880,\
		.scale_w = 3840,\
		.scale_h = 2880,\
		.x1_offset = 0,\
		.y1_offset = 0,\
		.w1_size = 3840,\
		.h1_size = 2880,\
		.x2_tg_offset = 0,\
		.y2_tg_offset = 0,\
		.w2_tg_size = 3840,\
		.h2_tg_size = 2880,\
	},\
	.pdaf_cap = TRUE,\
	.imgsensor_pd_info = &imgsensor_pd_info_bokeh1,\
	.ae_binning_ratio = 1000,\
	.fine_integ_line = 0,\
	.delay_frame = 2,\
}

// mode 8: 1920*1440@30fps 2x bokeh
#define bokeh2_mode_struct  \
{\
	.frame_desc = frame_desc_bokeh2,\
	.num_entries = ARRAY_SIZE(frame_desc_bokeh2),\
	.mode_setting_table = klimtov13bultra_custom3_setting,\
	.mode_setting_len = ARRAY_SIZE(klimtov13bultra_custom3_setting),\
	.seamless_switch_group = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_table = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_len = PARAM_UNDEFINED,\
	.hdr_mode = HDR_NONE,\
	.raw_cnt = 1,\
	.exp_cnt = 1,\
	.pclk = 112000000,\
	.linelength = 1176,\
	.framelength = 3966,\
	.max_framerate = 240,\
	.mipi_pixel_rate = 448000000,\
	.readout_length = 0,\
	.read_margin = 10,\
	.framelength_step = 2,\
	.imgsensor_winsize_info = {\
		.full_w = 4000,\
		.full_h = 3000,\
		.x0_offset = 1040,\
		.y0_offset = 780,\
		.w0_size = 1920,\
		.h0_size = 1440,\
		.scale_w = 1920,\
		.scale_h = 1440,\
		.x1_offset = 0,\
		.y1_offset = 0,\
		.w1_size = 1920,\
		.h1_size = 1440,\
		.x2_tg_offset = 0,\
		.y2_tg_offset = 0,\
		.w2_tg_size = 1920,\
		.h2_tg_size = 1440,\
	},\
	.pdaf_cap = TRUE,\
	.imgsensor_pd_info = &imgsensor_pd_info_bokeh2,\
	.ae_binning_ratio = 1000,\
	.fine_integ_line = 0,\
	.delay_frame = 2,\
}

// mode 10: 4000*2500@30fps 16:10 video
#define video_mode_struct  \
{\
	.frame_desc = frame_desc_video2,\
	.num_entries = ARRAY_SIZE(frame_desc_video2),\
	.mode_setting_table = klimtov13bultra_custom4_setting,\
	.mode_setting_len = ARRAY_SIZE(klimtov13bultra_custom4_setting),\
	.seamless_switch_group = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_table = PARAM_UNDEFINED,\
	.seamless_switch_mode_setting_len = PARAM_UNDEFINED,\
	.hdr_mode = HDR_NONE,\
	.raw_cnt = 1,\
	.exp_cnt = 1,\
	.pclk = 111625920,\
	.linelength = 1176,\
	.framelength = 3164,\
	.max_framerate = 300,\
	.mipi_pixel_rate = 448000000,\
	.readout_length = 0,\
	.read_margin = 10,\
	.framelength_step = 2,\
	.imgsensor_winsize_info = {\
		.full_w = 4000,\
		.full_h = 3000,\
		.x0_offset = 0,\
		.y0_offset = 250,\
		.w0_size = 4000,\
		.h0_size = 2500,\
		.scale_w = 4000,\
		.scale_h = 2500,\
		.x1_offset = 0,\
		.y1_offset = 0,\
		.w1_size = 4000,\
		.h1_size = 2500,\
		.x2_tg_offset = 0,\
		.y2_tg_offset = 0,\
		.w2_tg_size = 4000,\
		.h2_tg_size = 2500,\
	},\
	.pdaf_cap = TRUE,\
	.imgsensor_pd_info = &imgsensor_pd_info_video,\
	.ae_binning_ratio = 1000,\
	.fine_integ_line = 0,\
	.delay_frame = 2,\
}

static struct subdrv_mode_struct mode_struct[] = {
	preview_mode_struct,		//mode 0
	preview_mode_struct,		//mode 1
	normal_video_mode_struct,	//mode 2
	preview_mode_struct,		//mode 3
	preview_mode_struct,		//mode 4
	custom1_struct,				//mode 5
	bokeh_mode_struct,			//mode 6
	bokeh_mode_struct,			//mode 7 leica authentic Mode
	bokeh2_mode_struct,         //mode 8
	bokeh2_mode_struct,         //mode 9
	video_mode_struct           //mode 10
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = KLIMTOV13BULTRA_SENSOR_ID,
	.reg_addr_sensor_id = {0x300B, 0x300C},
	.i2c_addr_table = {0x6c, 0xFF}, // TBD
	.i2c_burst_write_support = FALSE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {4000, 3000},
	.mirror = IMAGE_NORMAL, // TBD

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
	.dig_gain_min = BASEGAIN * 1,
	.dig_gain_max = BASEGAIN * 4,
	.dig_gain_step = 1, //1024/1024
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 15.5,
	.ana_gain_type = 1,
	.ana_gain_step = 1,
	.ana_gain_table = klimtov13bultra_ana_gain_table,
	.ana_gain_table_size = sizeof(klimtov13bultra_ana_gain_table),
	.tuning_iso_base = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 4,
	.exposure_max = 0x36626A - 8,
	.exposure_step = 2,
	.exposure_margin = 8,

	.frame_length_max = 0x36626A,
	.ae_effective_frame = 3,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 1755600,

	.pdaf_type = PDAF_SUPPORT_RAW,
	.hdr_type = HDR_SUPPORT_NA,
	.seamless_switch_support = FALSE,
	.temperature_support = TRUE,

	.g_temp = PARAM_UNDEFINED,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = PARAM_UNDEFINED, // TBD
	.reg_addr_exposure = {
		{0x3500, 0x3501, 0x3502}
	},
	.long_exposure_support = FALSE,
	.reg_addr_exposure_lshift = PARAM_UNDEFINED,
	.reg_addr_ana_gain = {
		{0x3508, 0x3509}
	},
	.reg_addr_dig_gain = {
		{0x350A, 0x350B, 0x350C}
	},
	.reg_addr_frame_length = {0x380e, 0x380f},
	.reg_addr_temp_en = PARAM_UNDEFINED,
	.reg_addr_temp_read = PARAM_UNDEFINED,
	.reg_addr_auto_extend = 0x3822,
	.reg_addr_frame_count = PARAM_UNDEFINED,

	.init_setting_table = klimtov13bultra_init_setting,
	.init_setting_len = ARRAY_SIZE(klimtov13bultra_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 0,
	.chk_s_off_end = 0,

	//TBD
	.checksum_value = 0xAF3E324F,
};

static struct subdrv_ops ops = {
	.get_id = klimtov13bultra_get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = common_open,
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = common_control,
	.feature_control = common_feature_control,
	.close = common_close,
	.get_frame_desc = common_get_frame_desc,
	.get_temp = common_get_temp,
	.get_csi_param = klimtov13bultra_get_csi_param,
	.update_sof_cnt = common_update_sof_cnt,
	.vsync_notify = ultra_vsync_notify,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_DVDD1,  {1300000, 1300000},  5000}, //ldo supply
	{HW_ID_RST,    {0},		1000},
	{HW_ID_AVDD,  {2800000, 2800000}, 2000},
	{HW_ID_DOVDD,  {1800000, 1800000}, 1000},
	{HW_ID_DVDD,   {1200000, 1200000}, 1000},
	{HW_ID_RST,    {1},		6000},
	{HW_ID_MCLK,   {24},		0},
	{HW_ID_MCLK_DRIVING_CURRENT, {4}, 1000},
};

const struct subdrv_entry klimtov13bultra_mipi_raw_entry = {
	.name = SENSOR_DRVNAME_KLIMTOV13BULTRA_MIPI_RAW,
	.id = KLIMTOV13BULTRA_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

static void set_group_hold(void *arg, u8 en)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	if (en) {
		set_i2c_buffer(ctx, 0x3208, 0x00);
	} else {
		set_i2c_buffer(ctx, 0x3208, 0x10);
		set_i2c_buffer(ctx, 0x3208, 0xa0);
	}
}

static int klimtov13bultra_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id)
{
	u8 i = 0;
	u8 retry = 2;
	u32 addr_h = ctx->s_ctx.reg_addr_sensor_id.addr[0];
	u32 addr_l = ctx->s_ctx.reg_addr_sensor_id.addr[1];
	u32 addr_ll = ctx->s_ctx.reg_addr_sensor_id.addr[2];

	while (ctx->s_ctx.i2c_addr_table[i] != 0xFF) {
		ctx->i2c_write_id = ctx->s_ctx.i2c_addr_table[i];
		do {
			*sensor_id = (subdrv_ixc_rd_u8(ctx, addr_h) << 8) |
				subdrv_ixc_rd_u8(ctx, addr_l);
			if (addr_ll) {
				*sensor_id = ((*sensor_id) << 8) | subdrv_ixc_rd_u8(ctx, addr_ll);
			}

			*sensor_id = (*sensor_id) + 1;

			DRV_LOG_MUST(ctx, "i2c_write_id:0x%x sensor_id(cur/exp):0x%x/0x%x\n",
				ctx->i2c_write_id, *sensor_id, ctx->s_ctx.sensor_id);
			if (*sensor_id == ctx->s_ctx.sensor_id)
				return ERROR_NONE;
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != ctx->s_ctx.sensor_id) {
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}

static void klimtov13bultra_set_dummy(struct subdrv_ctx *ctx)
{
}

static int klimtov13bultra_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *)para;
	enum SENSOR_SCENARIO_ID_ENUM scenario_id = (enum SENSOR_SCENARIO_ID_ENUM)*feature_data;
	u32 framerate = *(feature_data + 1);
	u32 frame_length;

	if (scenario_id >= ctx->s_ctx.sensor_mode_num) {
		DRV_LOG(ctx, "invalid sid:%u, mode_num:%u\n",
			scenario_id, ctx->s_ctx.sensor_mode_num);
		scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW;
	}

	if (framerate == 0) {
		DRV_LOG(ctx, "framerate should not be 0\n");
		return ERROR_NONE;
	}

	if (ctx->s_ctx.mode[scenario_id].linelength == 0) {
		DRV_LOG(ctx, "linelength should not be 0\n");
		return ERROR_NONE;
	}

	if (ctx->line_length == 0) {
		DRV_LOG(ctx, "ctx->line_length should not be 0\n");
		return ERROR_NONE;
	}

	if (ctx->frame_length == 0) {
		DRV_LOG(ctx, "ctx->frame_length should not be 0\n");
		return ERROR_NONE;
	}

	frame_length = ctx->s_ctx.mode[scenario_id].pclk / framerate * 10
		/ ctx->s_ctx.mode[scenario_id].linelength;
	ctx->frame_length =
		max(frame_length, ctx->s_ctx.mode[scenario_id].framelength);
	ctx->frame_length = min(ctx->frame_length, ctx->s_ctx.frame_length_max);
	ctx->current_fps = ctx->pclk / ctx->frame_length * 10 / ctx->line_length;
	ctx->min_frame_length = ctx->frame_length;
	DRV_LOG(ctx, "max_fps(input/output):%u/%u(sid:%u), min_fl_en:1\n",
		framerate, ctx->current_fps, scenario_id);
	if (ctx->frame_length > (ctx->exposure[0] + ctx->s_ctx.exposure_margin))
		klimtov13bultra_set_dummy(ctx);

	return ERROR_NONE;
}

static u16 get_gain2reg(u32 gain)
{
	return gain * 256 / BASEGAIN;;
}

static int klimtov13bultra_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	switch (mode) {
	case 5:
		subdrv_i2c_wr_u8(ctx, 0x5080, 0x91);
		break;
	default:
		subdrv_i2c_wr_u8(ctx, 0x5080, 0x00);
		break;
	}
	ctx->test_pattern = mode;
	return ERROR_NONE;
}

static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id)
{
	memcpy(&(ctx->s_ctx), &static_ctx, sizeof(struct subdrv_static_ctx));
	subdrv_ctx_init(ctx);
	ctx->i2c_client = i2c_client;
	ctx->i2c_write_id = i2c_write_id;
	return 0;
}

static int ultra_vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt)
{
	u16 sensor_output_cnt;

	sensor_output_cnt = (subdrv_i2c_rd_u8(ctx, 0x4848) << 8);
	sensor_output_cnt |= subdrv_i2c_rd_u8(ctx, 0x4849);
	DRV_LOG_MUST(ctx, "sensormode(%d) sof_cnt(%d) sensor_output_cnt(%d)\n",
		ctx->current_scenario_id, sof_cnt, sensor_output_cnt);
	return 0;
};

static int klimtov13bultra_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 shutter = *((u64 *)para);
	u32 frame_length = 0;
	u32 fine_integ_line = 0;
	bool gph = !ctx->is_seamless && (ctx->s_ctx.s_gph != NULL);

	ctx->frame_length = frame_length ? frame_length : ctx->min_frame_length;
	check_current_scenario_id_bound(ctx);
	/* check boundary of shutter */
	fine_integ_line = ctx->s_ctx.mode[ctx->current_scenario_id].fine_integ_line;
	shutter = FINE_INTEG_CONVERT(shutter, fine_integ_line);
	shutter = max_t(u64, shutter,
		(u64)ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_shutter_range[0].min);
	shutter = min_t(u64, shutter,
		(u64)ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_shutter_range[0].max);
	/* check boundary of framelength */
	ctx->frame_length = max((u32)shutter + ctx->s_ctx.exposure_margin, ctx->min_frame_length);
	ctx->frame_length = min(ctx->frame_length, ctx->s_ctx.frame_length_max);
	/* restore shutter */
	memset(ctx->exposure, 0, sizeof(ctx->exposure));
	ctx->exposure[0] = (u32) shutter;
	/* group hold start */
	if (gph)
		ctx->s_ctx.s_gph((void *)ctx, 1);

	/* write framelength */
	if (ctx->frame_length > 0x7fff){
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_auto_extend, 0x04); //extended frame length
	} else {
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_auto_extend, 0x14); //disable extended frame length
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_frame_length.addr[0], (ctx->frame_length >> 8) & 0xFF);
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_frame_length.addr[1], ctx->frame_length & 0xFF);
	}
	/* update FL RG value after setting buffer for writting RG */
	ctx->frame_length_rg = ctx->frame_length;

	/* write shutter */
	if (ctx->s_ctx.reg_addr_exposure[0].addr[2]) {
		set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[0].addr[0],
			(ctx->exposure[0] >> 16) & 0xFF);
		set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[0].addr[1],
			(ctx->exposure[0] >> 8) & 0xFF);
		set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[0].addr[2],
			ctx->exposure[0] & 0xFF);
	} else {
		set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[0].addr[0],
			(ctx->exposure[0] >> 8) & 0xFF);
		set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[0].addr[1],
			ctx->exposure[0] & 0xFF);
	}

	DRV_LOG_MUST(ctx, "exp[0x%x], fll(input/output):%u/%u, flick_en:%d\n",
		ctx->exposure[0], frame_length, ctx->frame_length, ctx->autoflicker_en);
	if (!ctx->ae_ctrl_gph_en) {
		if (gph)
			ctx->s_ctx.s_gph((void *)ctx, 0);
		commit_i2c_buffer(ctx);
	}
	/* group hold end */
	return 0;
}

static int klimtov13bultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	klimtov13bultra_set_shutter_frame_length(ctx, para, len);
	return 0;
}
static int klimtov13bultra_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *) para;
	u64 *shutters = (u64 *)(*feature_data);
	u16 exp_cnt   = (u16)(*(feature_data + 1));
	u16 frame_length = (u16)(*(feature_data + 2));

	int i = 0;
	int fine_integ_line = 0;
	u16 last_exp_cnt = 1;
	u32 calc_fl[3] = {0};
	int readout_diff = 0;
	bool gph = !ctx->is_seamless && (ctx->s_ctx.s_gph != NULL);
	u32 rg_shutters[3] = {0};
	u32 cit_step = 0;

	ctx->frame_length = frame_length ? frame_length : ctx->min_frame_length;
	if (exp_cnt > ARRAY_SIZE(ctx->exposure)) {
		DRV_LOGE(ctx, "invalid exp_cnt:%u>%lu\n", exp_cnt, ARRAY_SIZE(ctx->exposure));
		exp_cnt = ARRAY_SIZE(ctx->exposure);
	}
	check_current_scenario_id_bound(ctx);

	/* check boundary of shutter */
	for (i = 1; i < ARRAY_SIZE(ctx->exposure); i++)
		last_exp_cnt += ctx->exposure[i] ? 1 : 0;
	fine_integ_line = ctx->s_ctx.mode[ctx->current_scenario_id].fine_integ_line;
	cit_step = ctx->s_ctx.mode[ctx->current_scenario_id].coarse_integ_step;
	for (i = 0; i < exp_cnt; i++) {
		shutters[i] = FINE_INTEG_CONVERT(shutters[i], fine_integ_line);
		shutters[i] = max_t(u64, shutters[i],
			(u64)ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_shutter_range[i].min);
		shutters[i] = min_t(u64, shutters[i],
			(u64)ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_shutter_range[i].max);
		if (cit_step)
			shutters[i] = round_up(shutters[i], cit_step);
	}

	/* check boundary of framelength */
	/* - (1) previous se + previous me + current le */
	calc_fl[0] = (u32) shutters[0];
	for (i = 1; i < last_exp_cnt; i++)
		calc_fl[0] += ctx->exposure[i];
	calc_fl[0] += ctx->s_ctx.exposure_margin*exp_cnt*exp_cnt;

	/* - (2) current se + current me + current le */
	calc_fl[1] = (u32) shutters[0];
	for (i = 1; i < exp_cnt; i++)
		calc_fl[1] += (u32) shutters[i];
	calc_fl[1] += ctx->s_ctx.exposure_margin*exp_cnt*exp_cnt;

	/* - (3) readout time cannot be overlapped */
	calc_fl[2] =
		(ctx->s_ctx.mode[ctx->current_scenario_id].readout_length +
		ctx->s_ctx.mode[ctx->current_scenario_id].read_margin);
	if (last_exp_cnt == exp_cnt)
		for (i = 1; i < exp_cnt; i++) {
			readout_diff = ctx->exposure[i] - (u32) shutters[i];
			calc_fl[2] += readout_diff > 0 ? readout_diff : 0;
		}
	for (i = 0; i < ARRAY_SIZE(calc_fl); i++)
		ctx->frame_length = max(ctx->frame_length, calc_fl[i]);
	ctx->frame_length =	max(ctx->frame_length, ctx->min_frame_length);
	ctx->frame_length =	min(ctx->frame_length, ctx->s_ctx.frame_length_max);
	/* restore shutter */
	memset(ctx->exposure, 0, sizeof(ctx->exposure));
	for (i = 0; i < exp_cnt; i++)
		ctx->exposure[i] = (u32) shutters[i];
	/* group hold start */
	if (gph)
		ctx->s_ctx.s_gph((void *)ctx, 1);

	/* write framelength */
	if (ctx->frame_length > 0x7fff){
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_auto_extend, 0x04); //extended frame length
	} else {
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_auto_extend, 0x14); //disable extended frame length
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_frame_length.addr[0], (ctx->frame_length >> 8) & 0xFF);
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_frame_length.addr[1], ctx->frame_length & 0xFF);
	}

	/* update FL RG value after setting buffer for writting RG */
	ctx->frame_length_rg = ctx->frame_length;

	/* write shutter */
	switch (exp_cnt) {
	case 1:
		rg_shutters[0] = (u32) shutters[0] / exp_cnt;
		break;
	case 2:
		rg_shutters[0] = (u32) shutters[0] / exp_cnt;
		rg_shutters[2] = (u32) shutters[1] / exp_cnt;
		break;
	case 3:
		rg_shutters[0] = (u32) shutters[0] / exp_cnt;
		rg_shutters[1] = (u32) shutters[1] / exp_cnt;
		rg_shutters[2] = (u32) shutters[2] / exp_cnt;
		break;
	default:
		break;
	}
	if (ctx->s_ctx.reg_addr_exposure_lshift != PARAM_UNDEFINED) {
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_exposure_lshift, 0);
		ctx->l_shift = 0;
	}
	for (i = 0; i < 3; i++) {
		if (rg_shutters[i]) {
			if (ctx->s_ctx.reg_addr_exposure[i].addr[2]) {
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[0],
					(rg_shutters[i] >> 16) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[1],
					(rg_shutters[i] >> 8) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[2],
					rg_shutters[i] & 0xFF);
			} else {
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[0],
					(rg_shutters[i] >> 8) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[1],
					rg_shutters[i] & 0xFF);
			}
		}
	}
	DRV_LOG_MUST(ctx, "exp[0x%x/0x%x/0x%x], fll(input/output):%u/%u, flick_en:%d\n",
		rg_shutters[0], rg_shutters[1], rg_shutters[2],
		frame_length, ctx->frame_length, ctx->autoflicker_en);
	if (!ctx->ae_ctrl_gph_en) {
		if (gph)
			ctx->s_ctx.s_gph((void *)ctx, 0);
		commit_i2c_buffer(ctx);
	}
	/* group hold end */
	return 0;
}

static u32 dgain2reg(struct subdrv_ctx *ctx, u32 dgain)
{
	u32 step = max((ctx->s_ctx.dig_gain_step), (u32)1);
	u16 integ = (u16) (dgain / BASE_DGAIN); // integer parts
	u16 dec = (u16) ((dgain % BASE_DGAIN) / step); // decimal parts
	u32 ret = ((u32)integ << 16) | dec;
	DRV_LOG(ctx, "dgain reg = 0x%x\n", ret);
	return ret;
}

static int klimtov13bultra_set_multi_dig_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *) para;
	u32 *gains  = (u32 *)(*feature_data);
	u16 exp_cnt = (u16) (*(feature_data + 1));

	int i = 0;
	u32 rg_gains[IMGSENSOR_STAGGER_EXPOSURE_CNT] = {0};
	bool gph = !ctx->is_seamless && (ctx->s_ctx.s_gph != NULL);

	if (exp_cnt > ARRAY_SIZE(ctx->dig_gain)) {
		DRV_LOGE(ctx, "invalid exp_cnt:%u>%lu\n", exp_cnt, ARRAY_SIZE(ctx->dig_gain));
		exp_cnt = ARRAY_SIZE(ctx->dig_gain);
	}
	for (i = 0; i < exp_cnt; i++) {
		/* check boundary of gain */
		gains[i] = max(gains[i], ctx->s_ctx.dig_gain_min);
		gains[i] = min(gains[i], ctx->s_ctx.dig_gain_max);
		gains[i] = dgain2reg(ctx, gains[i]);
	}

	/* restore gain */
	memset(ctx->dig_gain, 0, sizeof(ctx->dig_gain));
	for (i = 0; i < exp_cnt; i++)
		ctx->dig_gain[i] = gains[i];

	/* group hold start */
	if (gph && !ctx->ae_ctrl_gph_en)
		ctx->s_ctx.s_gph((void *)ctx, 1);

	/* write gain */
	switch (exp_cnt) {
	case 1:
		rg_gains[0] = gains[0];
		break;
	case 2:
		rg_gains[0] = gains[0];
		rg_gains[2] = gains[1];
		break;
	case 3:
		rg_gains[0] = gains[0];
		rg_gains[1] = gains[1];
		rg_gains[2] = gains[2];
		break;
	default:
		break;
	}
	for (i = 0;
	     (i < ARRAY_SIZE(rg_gains)) && (i < ARRAY_SIZE(ctx->s_ctx.reg_addr_dig_gain));
	     i++) {
		if (!rg_gains[i])
			continue; // skip zero gain setting

		if (ctx->s_ctx.reg_addr_dig_gain[i].addr[0]) {
			set_i2c_buffer(ctx,
				ctx->s_ctx.reg_addr_dig_gain[i].addr[0],
				(rg_gains[i] >> 16) & 0x03);
		}
		if (ctx->s_ctx.reg_addr_dig_gain[i].addr[1]) {
			set_i2c_buffer(ctx,
				ctx->s_ctx.reg_addr_dig_gain[i].addr[1],
				(rg_gains[i] >> 2) & 0xFF);
		}
		if (ctx->s_ctx.reg_addr_dig_gain[i].addr[2]) {
			set_i2c_buffer(ctx,
				ctx->s_ctx.reg_addr_dig_gain[i].addr[2],
				(rg_gains[i] & 0x03) << 6);
		}
	}

	if (!ctx->ae_ctrl_gph_en) {
		if (gph)
			ctx->s_ctx.s_gph((void *)ctx, 0);
		commit_i2c_buffer(ctx);
	}

	DRV_LOG(ctx, "dgain reg[lg/mg/sg]: 0x%x 0x%x 0x%x\n",
		rg_gains[0], rg_gains[1], rg_gains[2]);
	return 0;
}

static int klimtov13bultra_get_csi_param(struct subdrv_ctx *ctx,
	enum SENSOR_SCENARIO_ID_ENUM scenario_id,
	struct mtk_csi_param *csi_param)
{
	DRV_LOG(ctx, "+ scenario_id:%u,aov_csi_clk:%u\n",scenario_id, ctx->aov_csi_clk);
	switch (scenario_id) {
	default:
		csi_param->legacy_phy = 0;
		csi_param->not_fixed_trail_settle = 1;
		csi_param->not_fixed_dphy_settle = 1;
		csi_param->dphy_data_settle = 0x28;
		csi_param->dphy_clk_settle = 0x28;
		csi_param->dphy_trail = 0x40;
		break;
	}
	return 0;
}

