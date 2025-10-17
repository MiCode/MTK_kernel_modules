// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 turnerov08f10frontmipiraw_Sensor.c
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
#include "turnerov08f10frontmipiraw_Sensor.h"

static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static u16 get_gain2reg(u32 gain);
static int front_vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt);
static int turnerov08f10front_close(struct subdrv_ctx *ctx);
static int turnerov08f10front_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id);
static int turnerov08f10front_return_sensor_id(struct subdrv_ctx *ctx);
static int turnerov08f10front_control(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data);
static int turnerov08f10front_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int turnerov08f10front_streamon(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int turnerov08f10front_streamoff(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int turnerov08f10front_streaming_control(struct subdrv_ctx *ctx, bool enable);
static int turnerov08f10front_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int turnerov08f10front_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int turnerov08f10ultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int turnerov08f10front_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int turnerov08f10front_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static void turnerov08f10front_set_dummy(struct subdrv_ctx *ctx);
static void turnerov08f10front_soft_power_up(struct subdrv_ctx *ctx);
static int set_streaming_control(void *arg, bool enable);

static bool streaming_status = false;
static bool soft_power_status = false; //false = soft power down, true = soft power up
static bool blc_trigger_flag = false; // true = need to write p7 0x00 0xf8

/* STRUCT */
static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_STREAMING_SUSPEND, turnerov08f10front_streamoff},
	{SENSOR_FEATURE_SET_STREAMING_RESUME, turnerov08f10front_streamon},
	{SENSOR_FEATURE_SET_GAIN, turnerov08f10front_set_gain},
	{SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME, turnerov08f10front_set_shutter_frame_length},
	{SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO, turnerov08f10front_set_max_framerate_by_scenario},
	{SENSOR_FEATURE_SET_ESHUTTER, turnerov08f10ultra_set_shutter},
	{SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME, turnerov08f10front_set_multi_shutter_frame_length},
	{SENSOR_FEATURE_SET_TEST_PATTERN, turnerov08f10front_set_test_pattern},
};

// static struct eeprom_info_struct eeprom_info[] = {
// 	{
// 		// .header_id = 0x010B00FF,
// 		// .header_id = 0x0,
// 		// .addr_header_id = 0x0000000B,
// 		.i2c_write_id = 0xA4,

// 		.pdc_support = TRUE,
// 		.pdc_size = 720,
// 		.addr_pdc = 0x12D2,
// 		.sensor_reg_addr_pdc = 0x5F80,
// 	},
// };

static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 2448,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 1840,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 1280,
			.vsize = 960,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 1280,
			.vsize = 960,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 640,
			.vsize = 480,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 640,
			.vsize = 480,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct subdrv_mode_struct mode_struct[] = {
	/* mode 0 : preview 3264x2448@30fps */
	//setting 20250325 V1.8
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = turnerov08f10front_preview_setting,
		.mode_setting_len = ARRAY_SIZE(turnerov08f10front_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36500000,
		.linelength = 478,
		.framelength = 2544,
		.max_framerate = 300,
		.mipi_pixel_rate = 280800000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 3264,
			.h0_size = 2448,
			.scale_w = 3264,
			.scale_h = 2448,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 2448,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 2448,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* mode 1 : capture same as preview 3264x2448@30fps */
	//setting 20250325 V1.8
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = turnerov08f10front_preview_setting,
		.mode_setting_len = ARRAY_SIZE(turnerov08f10front_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36500000,
		.linelength = 478,
		.framelength = 2544,
		.max_framerate = 300,
		.mipi_pixel_rate = 280800000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 3264,
			.h0_size = 2448,
			.scale_w = 3264,
			.scale_h = 2448,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 2448,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 2448,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* mode 2 : normal video 3264x1840@30fps */
	//setting 20250325 V1.8
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = turnerov08f10front_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(turnerov08f10front_normal_video_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 478,
		.framelength = 2501,
		.max_framerate = 300,
		.mipi_pixel_rate = 280800000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 0,
			.y0_offset = 304,
			.w0_size = 3264,
			.h0_size = 1840,
			.scale_w = 3264,
			.scale_h = 1840,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 1840,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 1840,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* mode 3 : hs_video same as preview 3264x2448@30fps */
	//setting 20250325 V1.8
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = turnerov08f10front_preview_setting,
		.mode_setting_len = ARRAY_SIZE(turnerov08f10front_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36500000,
		.linelength = 478,
		.framelength = 2544,
		.max_framerate = 300,
		.mipi_pixel_rate = 280800000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 3264,
			.h0_size = 2448,
			.scale_w = 3264,
			.scale_h = 2448,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 2448,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 2448,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* mode 4 : slim_video same as preview 3264x2448@30fps */
	//setting 20250325 V1.8
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = turnerov08f10front_preview_setting,
		.mode_setting_len = ARRAY_SIZE(turnerov08f10front_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36500000,
		.linelength = 478,
		.framelength = 2544,
		.max_framerate = 300,
		.mipi_pixel_rate = 280800000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 3264,
			.h0_size = 2448,
			.scale_w = 3264,
			.scale_h = 2448,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 2448,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 2448,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* custom1 : binning crop 1280x960@10fps */
	//setting 20250325 V1.8
	{
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = turnerov08f10front_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(turnerov08f10front_custom1_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 239,
		.framelength = 15060,
		.max_framerate = 100,
		.mipi_pixel_rate = 280800000,
		.framelength_step = 1,
		//.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		//.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		//.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,

		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 352,
			.y0_offset = 264,
			.w0_size = 2560,
			.h0_size = 1920,
			.scale_w = 1280,
			.scale_h = 960,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 1280,
			.h1_size = 960,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1280,
			.h2_tg_size = 960,
		},

		//aov add parameter
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x82,
			.dphy_clk_settle=0x13,
			.dphy_data_settle =0x13,
		},
		.aov_mode = 1,
		.rosc_mode = 0, //sensor内部 MCLK
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,

		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* custom2 : binning crop 1280x960@20fps */
	//setting 20250325 V1.8
	{
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = turnerov08f10front_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(turnerov08f10front_custom2_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 239,
		.framelength = 7530,
		.max_framerate = 200,
		.mipi_pixel_rate = 280800000,
		.framelength_step = 1,
		//.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		//.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		//.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,

		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 352,
			.y0_offset = 264,
			.w0_size = 2560,
			.h0_size = 1920,
			.scale_w = 1280,
			.scale_h = 960,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 1280,
			.h1_size = 960,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1280,
			.h2_tg_size = 960,
		},

		//aov add parameter
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x82,
			.dphy_clk_settle=0x13,
			.dphy_data_settle =0x13,
		},
		.aov_mode = 1,
		.rosc_mode = 0,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,

		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* custom3 : binning crop 640x480@10fps */
	//setting 20250325 V1.8
	{
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = turnerov08f10front_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(turnerov08f10front_custom3_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 239,
		.framelength = 15060,
		.max_framerate = 100,
		.mipi_pixel_rate = 280800000,
		.framelength_step = 1,
		//.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		//.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		//.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,

		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 992,
			.y0_offset = 744,
			.w0_size = 1280,
			.h0_size = 960,
			.scale_w = 640,
			.scale_h = 480,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},

		//aov add parameter
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x82,
			.dphy_clk_settle=0x13,
			.dphy_data_settle =0x13,
		},
		.aov_mode = 1,
		.rosc_mode = 0,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,

		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* custom4 : binning crop 640x480@20fps */
	//setting 20250325 V1.8
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = turnerov08f10front_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(turnerov08f10front_custom4_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 239,
		.framelength = 7530,
		.max_framerate = 200,
		.mipi_pixel_rate = 288000000,
		.framelength_step = 1,
		//.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		//.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		//.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,

		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 992,
			.y0_offset = 744,
			.w0_size = 1280,
			.h0_size = 960,
			.scale_w = 640,
			.scale_h = 480,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},

		//aov add parameter
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x82,
			.dphy_clk_settle=0x13,
			.dphy_data_settle =0x13,
		},
		.aov_mode = 1,
		.rosc_mode = 0,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,

		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = TURNEROV08F10FRONT_SENSOR_ID,
	.reg_addr_sensor_id = {0x00, 0x01},
	.i2c_addr_table = {0x20, 0xFF}, // TBD
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	// .eeprom_info = eeprom_info,
	// .eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {3280, 2464},
	.mirror = IMAGE_NORMAL, // TBD

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_lane_num = SENSOR_MIPI_2_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 15.5,
	.ana_gain_type = 1,
	.ana_gain_step = 1,
	.ana_gain_table = turnerov08f10front_ana_gain_table,
	.ana_gain_table_size = sizeof(turnerov08f10front_ana_gain_table),
	.tuning_iso_base = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 8,
	.exposure_max = 0x3FFFEB,
	.exposure_step = 1,
	.exposure_margin = 40,

	.frame_length_max = 0x3fffff,
	.ae_effective_frame = 3,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 153800,

	.pdaf_type = PDAF_SUPPORT_NA,
	.hdr_type = HDR_SUPPORT_NA,
	.seamless_switch_support = FALSE,
	.temperature_support = FALSE,

	.g_temp = PARAM_UNDEFINED,
	.g_gain2reg = get_gain2reg,
	.s_gph = PARAM_UNDEFINED,

	//aov add parameter
	.s_streaming_control = set_streaming_control,

	//.reg_addr_stream = 0x3e,
	.reg_addr_mirror_flip = PARAM_UNDEFINED, // TBD
	//.reg_addr_exposure = {
	// 	{0x02, 0x03, 0x04},
	//},
	.long_exposure_support = FALSE,
	.reg_addr_exposure_lshift = PARAM_UNDEFINED,
	//.reg_addr_ana_gain = {
	//	{0x24},
	//},
	//.reg_addr_dig_gain = {
	//	{0x21, 0x22},
	//},
	//.reg_addr_frame_length = {0x34, 0x35, 0x36},
	.reg_addr_temp_en = PARAM_UNDEFINED,
	.reg_addr_temp_read = PARAM_UNDEFINED,
	.reg_addr_auto_extend = PARAM_UNDEFINED,
	.reg_addr_frame_count = PARAM_UNDEFINED,

	.init_setting_table = turnerov08f10front_init_setting,
	.init_setting_len = ARRAY_SIZE(turnerov08f10front_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 0,
	.chk_s_off_end = 0,

	//TBD
	.checksum_value = 0xAF3E324F,

	//aov add parameter
	.aov_sensor_support = TRUE,
	.sensor_mode_ops = 0,
	.sensor_debug_sensing_ut_on_scp = TRUE,
	.sensor_debug_dphy_global_timing_continuous_clk = FALSE,
	.reg_addr_aov_mode_mirror_flip = 0x3874,//TODO
	.init_in_open = TRUE,
	.streaming_ctrl_imp = FALSE,
};

static struct subdrv_ops ops = {
	.get_id = turnerov08f10front_get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = common_open,
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = turnerov08f10front_control,
	.feature_control = common_feature_control,
	.close = turnerov08f10front_close,
	.get_frame_desc = common_get_frame_desc,
	.get_temp = common_get_temp,
	//.get_csi_param = turnerov08f10front_get_csi_param,
	.update_sof_cnt = common_update_sof_cnt,
	.vsync_notify = front_vsync_notify,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_DOVDD, {1800000, 1800000}, 1000},
	{HW_ID_RST, {0}, 1000},
	{HW_ID_AVDD1, {3200000, 3200000}, 6000},//vbb Update DOVDD after AVDD
	{HW_ID_DVDD, {1450000, 1450000}, 1000},//dvdd vcam ldo
	{HW_ID_AVDD, {2800000, 2800000}, 1000},
	{HW_ID_DVDD1, {1200000, 1200000}, 6000},
	{HW_ID_RST, {1}, 1000},
	{HW_ID_MCLK1, {24}, 0},
	{HW_ID_MCLK1_DRIVING_CURRENT, {6}, 6000},
};

const struct subdrv_entry turnerov08f10front_mipi_raw_entry = {
	.name = "turnerov08f10front_mipi_raw",
	.id = TURNEROV08F10FRONT_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

static int turnerov08f10front_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id)
{
	u8 i = 0;
	u8 retry = 2;

	while (ctx->s_ctx.i2c_addr_table[i] != 0xFF) {
		ctx->i2c_write_id = ctx->s_ctx.i2c_addr_table[i];
		do {
			*sensor_id = turnerov08f10front_return_sensor_id(ctx);

			DRV_LOG(ctx, "i2c_write_id:0x%x sensor_id(cur/exp):0x%x/0x%x\n",
				ctx->i2c_write_id, *sensor_id, ctx->s_ctx.sensor_id);

			DRV_LOG_MUST(ctx, "i2c_write_id_ov08f10:0x%x sensor_id(cur/exp):0x%x/0x%x\n",
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

static int turnerov08f10front_return_sensor_id(struct subdrv_ctx *ctx)
{
	int sensor_id = 0;
	u8 addr_h = ctx->s_ctx.reg_addr_sensor_id.addr[0];
	u8 addr_l = ctx->s_ctx.reg_addr_sensor_id.addr[1];

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	sensor_id = (subdrv_i2c_rd_u8_u8(ctx, addr_h) << 8) | subdrv_i2c_rd_u8_u8(ctx, addr_l) + 1;

	DRV_LOG_MUST(ctx, "sensor_id is:0x%x\n", sensor_id);

	return sensor_id;
}

static int turnerov08f10front_control(struct subdrv_ctx *ctx,
			enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	int i = 0, ret = 0;

	DRV_LOG_MUST(ctx, "scenario_id = %d\n", scenario_id);

	if (scenario_id >= ctx->s_ctx.sensor_mode_num) {
		DRV_LOGE(ctx, "invalid sid:%u, mode_num:%u\n",
			scenario_id, ctx->s_ctx.sensor_mode_num);
		scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW;
	}

	ctx->autoflicker_en = KAL_FALSE;
	ctx->vblank_convert = ctx->s_ctx.mode[scenario_id].framelength;

	ctx->current_scenario_id = scenario_id;
	u16 *list = ctx->s_ctx.mode[scenario_id].mode_setting_table;
	u32 len = ctx->s_ctx.mode[scenario_id].mode_setting_len;

	subdrv_i2c_wr_u8_u8(ctx, 0xFD, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0E);

	mdelay(6);
	for (i = 0; i < len; i = i+2) {
		ret |= subdrv_i2c_wr_u8_u8(ctx, list[i], list[i+1]&0xff);
	}

	turnerov08f10front_streaming_control(ctx, FALSE);// stream off

	DRV_LOG_MUST(ctx, "-\n");
	return ERROR_NONE;
}

static int turnerov08f10front_streamon(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	if (ctx->s_ctx.aov_sensor_support && ctx->s_ctx.streaming_ctrl_imp) {
		if (ctx->s_ctx.s_streaming_control != NULL)
			ctx->s_ctx.s_streaming_control((void *) ctx, true);
		else
			DRV_LOG_MUST(ctx, "please implement drive aov streaming control!(sid:%u)\n",
				ctx->current_scenario_id);
		ctx->is_streaming = true;
		return ERROR_NONE;
	}
	if (ctx->s_ctx.aov_sensor_support && ctx->s_ctx.mode[ctx->current_scenario_id].aov_mode) {
		DRV_LOG_MUST(ctx,"stream ctrl implement on scp side!(sid:%u)\n",
			ctx->current_scenario_id);
		ctx->is_streaming = true;

		turnerov08f10front_soft_power_up(ctx);
		return ERROR_NONE;
	}
	return turnerov08f10front_streaming_control(ctx, TRUE);
}

static int turnerov08f10front_streamoff(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	if (ctx->s_ctx.aov_sensor_support && ctx->s_ctx.streaming_ctrl_imp) {
		if (ctx->s_ctx.s_streaming_control != NULL)
			ctx->s_ctx.s_streaming_control((void *) ctx, false);
		else
			DRV_LOG_MUST(ctx, "please implement drive aov streaming control!(sid:%u)\n",
				ctx->current_scenario_id);
		ctx->is_streaming = false;
		return ERROR_NONE;
	}
	if (ctx->s_ctx.aov_sensor_support && ctx->s_ctx.mode[ctx->current_scenario_id].aov_mode) {
		DRV_LOG_MUST(ctx,"stream ctrl implement on scp side!(sid:%u)\n",
			ctx->current_scenario_id);
		ctx->is_streaming = false;
		return ERROR_NONE;
	}
	return turnerov08f10front_streaming_control(ctx, FALSE);
}

static int turnerov08f10front_streaming_control(struct subdrv_ctx *ctx, bool enable)
{
	DRV_LOG_MUST(ctx, "streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable) {
		// subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		// subdrv_i2c_wr_u8_u8(ctx, 0xa0, 0x01);

		// subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		// subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x0e);
		// subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x00);
		// subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
		// subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x03);
		// subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		// subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0f);
		// subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x03);
		// subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x00);
		// subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		// subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x38);
		// subdrv_i2c_wr_u8_u8(ctx, 0xa0, 0x01);

		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
		subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x03);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0xa0, 0x01);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x03);
		subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0f);

		streaming_status = true;
		soft_power_status = true;
		blc_trigger_flag = true;
	} else {
		// subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		// subdrv_i2c_wr_u8_u8(ctx, 0xa0, 0x00);
		// mdelay(10);
		// subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0b);
		// subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		// subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x3a);
		// subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x0f);

        subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0xa0, 0x00);
		mdelay(10);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x03);
		subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0b);
		subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x3a);
		subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x07);

		streaming_status = false;
		soft_power_status = false;
		blc_trigger_flag = false;
	}
	mdelay(10);

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

static int front_vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt)
{
	DRV_LOG_MUST(ctx, "sensormode(%d) sof_cnt(%d)\n", ctx->current_scenario_id, sof_cnt);
	return 0;
}

static void turnerov08f10front_set_dummy(struct subdrv_ctx *ctx)
{
	int dummy_line = 0;

	if (!soft_power_status) {
		turnerov08f10front_soft_power_up(ctx);
	}

	dummy_line = (ctx->frame_length - ctx->vblank_convert) * 2;
	if (dummy_line < 0 || dummy_line > 0xffff) {
		DRV_LOG_MUST(ctx, "enable auto extended vts\n");
		ctx->dummy_line = 0xffff;
	} else {
		ctx->dummy_line = dummy_line;
	}
	DRV_LOG_MUST(ctx, "vts = %d, dummy_line = %d\n",
		ctx->frame_length, dummy_line);

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
	subdrv_i2c_wr_u8_u8(ctx, 0x05, ctx->dummy_line >> 8);
	subdrv_i2c_wr_u8_u8(ctx, 0x06, ctx->dummy_line & 0xff);
	if (streaming_status)
		subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);

	/* update FL RG value after setting buffer for writting RG */
	ctx->frame_length_rg = ctx->frame_length;
}

static u16 get_gain2reg(u32 gain)
{
	return (gain * 16 / BASEGAIN);
}

static int turnerov08f10front_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
		turnerov08f10front_set_dummy(ctx);

	return ERROR_NONE;

}

static int turnerov08f10front_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 gain = *((u32 *)para);
	u8 reg_gain;
	DRV_LOG_MUST(ctx, "gain = %d\n", gain);

	if (!soft_power_status) {
		turnerov08f10front_soft_power_up(ctx);
	}

	/* check boundary of gain */
	gain = max(gain,
		ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_ana_gain_range[0].min);
	gain = min(gain,
		ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_ana_gain_range[0].max);

	reg_gain = get_gain2reg(gain);

	/* restore gain */
	memset(ctx->ana_gain, 0, sizeof(ctx->ana_gain));
	ctx->ana_gain[0] = gain;

	DRV_LOG_MUST(ctx, "ctx->sensor_mode: %d, gain = %d, ctx->ana_gain[0] = 0x%x ,reg_gain = 0x%x, max_gain:0x%x\n",
		ctx->current_scenario_id, gain, ctx->ana_gain[0], reg_gain, ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_ana_gain_range[0].max);

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
	subdrv_i2c_wr_u8_u8(ctx, 0x24, reg_gain);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0x22, 0x40);//1x digital gain
	// subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);

	if (streaming_status)
		subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);

	return ERROR_NONE;
}

static int turnerov08f10front_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 shutter = *((u64 *)para);
	u32 frame_length = 0;
	u32 fine_integ_line = 0;

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
	ctx->exposure[0] = (u32)shutter;

	turnerov08f10front_set_dummy(ctx);

	// Update Shutter
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
	subdrv_i2c_wr_u8_u8(ctx, 0x02, (shutter*2 >> 16) & 0xff);
	subdrv_i2c_wr_u8_u8(ctx, 0x03, (shutter*2 >> 8) & 0xff);
	subdrv_i2c_wr_u8_u8(ctx, 0x04, shutter*2 & 0xff);
	// subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);

	if (streaming_status)
		subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);
	DRV_LOG_MUST(ctx, "set shutter =%llu, framelength =%d\n", shutter, ctx->frame_length);

	return ERROR_NONE;
}

static int turnerov08f10front_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *) para;
	u64 *shutters = (u64 *)(* feature_data);
	u16 exp_cnt = (u16) (*(feature_data + 1));
	u16 frame_length = (u16) (*(feature_data + 2));

	if (exp_cnt == 1) {
		ctx->shutter = shutters[0];

		if (shutters[0] > ctx->s_ctx.mode[ctx->current_scenario_id].framelength - ctx->s_ctx.exposure_margin)
			ctx->frame_length = shutters[0] + ctx->s_ctx.exposure_margin;
		else
			ctx->frame_length = ctx->s_ctx.mode[ctx->current_scenario_id].framelength;
		if (frame_length > ctx->frame_length)
			ctx->frame_length = frame_length;

		if (ctx->frame_length > ctx->exposure_max)
			ctx->frame_length = ctx->exposure_max;

		if (shutters[0] < ctx->exposure_min)
			shutters[0] = ctx->exposure_min;

		turnerov08f10front_set_dummy(ctx);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
		subdrv_i2c_wr_u8_u8(ctx, 0x02, (shutters[0]*2 >> 16) & 0xff);
		subdrv_i2c_wr_u8_u8(ctx, 0x03, (shutters[0]*2 >> 8) & 0xff);
		subdrv_i2c_wr_u8_u8(ctx, 0x04, shutters[0]*2 & 0xff);
	        if (streaming_status)
			subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);

		DRV_LOG_MUST(ctx, "shutter =%llu, framelength =%d\n", shutters[0], ctx->frame_length);
	}

	return ERROR_NONE;
}

static int turnerov08f10ultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	DRV_LOG(ctx, "+\n");
	turnerov08f10front_set_shutter_frame_length(ctx, para, len);
	return ERROR_NONE;
}

static int turnerov08f10front_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);
	DRV_LOG(ctx, "mode is %d, test_pattern mode on\n", mode);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	switch (mode) {
	case 5:
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
	subdrv_i2c_wr_u8_u8(ctx, 0x12, 0x01);
		break;
	default:
		break;
	}

	ctx->test_pattern = mode;
	return ERROR_NONE;
}

static void turnerov08f10front_soft_power_up(struct subdrv_ctx *ctx)
{
	DRV_LOG_MUST(ctx, "soft_power_up_func\n");

	// subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	// subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x30);
	// subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x0e);
	// subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x00);

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0b);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x06);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x04);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x38);
	soft_power_status = true;
}

static int turnerov08f10front_close(struct subdrv_ctx *ctx)
{
	DRV_LOG_MUST(ctx, "turnerov08f10front_close\n");
	return ERROR_NONE;
}

static int set_streaming_control(void *arg, bool enable)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	int ret = 0;

	DRV_LOG_MUST(ctx, "streaming_enable(0=Sw Standby,1=streaming):(%d)\n", enable);

	if (ctx->s_ctx.mode[ctx->current_scenario_id].aov_mode) {
		DRV_LOG_MUST(ctx, "AOV mode(%d) streaming control on apmcu side\n", ctx->sensor_mode);
	}

	if (enable) {
		ret = turnerov08f10front_streaming_control(ctx, TRUE); //stream on
		DRV_LOG_MUST(ctx, "AOV mode stream on ret(%d)\n", ret);
		ctx->test_pattern = 0;
	} else {
		turnerov08f10front_streaming_control(ctx, FALSE);// stream off
		DRV_LOG_MUST(ctx, "AOV mode stream off ret(%d)\n", ret);
	}
	return ret;
}