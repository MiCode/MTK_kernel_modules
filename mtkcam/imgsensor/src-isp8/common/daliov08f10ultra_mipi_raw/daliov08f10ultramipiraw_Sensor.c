// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 daliov08f10ultramipiraw_Sensor.c
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
#include "daliov08f10ultramipiraw_Sensor.h"

static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static u16 get_gain2reg(u32 gain);
static int ultra_vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt);
static int daliov08f10ultra_close(struct subdrv_ctx *ctx);
static int daliov08f10ultra_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id);
static int daliov08f10ultra_return_sensor_id(struct subdrv_ctx *ctx);
static int daliov08f10ultra_control(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data);
static int daliov08f10ultra_streamon(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int daliov08f10ultra_streamoff(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int daliov08f10ultra_streaming_control(struct subdrv_ctx *ctx, bool enable);
static int daliov08f10ultra_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int daliov08f10ultra_set_auto_flicker_mode(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int daliov08f10ultra_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int daliov08f10ultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int daliov08f10ultra_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int daliov08f10ultra_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static void daliov08f10ultra_set_dummy(struct subdrv_ctx *ctx);
static void daliov08f10ultra_set_max_framerate(struct subdrv_ctx *ctx, u16 framerate, u8 min_framelength_en);
static void daliov08f10ultra_soft_power_up(struct subdrv_ctx *ctx);
static int daliov08f10ultra_get_csi_param(struct subdrv_ctx *ctx, enum SENSOR_SCENARIO_ID_ENUM scenario_id, struct mtk_csi_param *csi_param);

static bool streaming_status = false;
static bool soft_power_status = false; //false = soft power down, true = soft power up
static bool blc_trigger_flag = false; // true = need to write p7 0x00 0xf8

/* STRUCT */
static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_STREAMING_SUSPEND, daliov08f10ultra_streamoff},
	{SENSOR_FEATURE_SET_STREAMING_RESUME, daliov08f10ultra_streamon},
	{SENSOR_FEATURE_SET_GAIN, daliov08f10ultra_set_gain},
	{SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME, daliov08f10ultra_set_shutter_frame_length},
	{SENSOR_FEATURE_SET_ESHUTTER, daliov08f10ultra_set_shutter},
	{SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME, daliov08f10ultra_set_multi_shutter_frame_length},
	{SENSOR_FEATURE_SET_AUTO_FLICKER_MODE, daliov08f10ultra_set_auto_flicker_mode},
	{SENSOR_FEATURE_SET_TEST_PATTERN, daliov08f10ultra_set_test_pattern},
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
			.hsize = 3264,
			.vsize = 2040,
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
			.hsize = 1440,
			.vsize = 1080,
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
			.hsize = 2944,
			.vsize = 2208,
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
			.hsize = 1536,
			.vsize = 1152,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct subdrv_mode_struct mode_struct[] = {
	/* mode 0 : preview 3264x2448@30fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = daliov08f10ultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(daliov08f10ultra_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 472,
		.framelength = 2541,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3280,
			.full_h = 2464,
			.x0_offset = 8,
			.y0_offset = 8,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* mode 1 : capture same as preview 3264x2448@30fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = daliov08f10ultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(daliov08f10ultra_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36500000,
		.linelength = 478,
		.framelength = 2544,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3280,
			.full_h = 2464,
			.x0_offset = 8,
			.y0_offset = 8,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* mode 2 : video 3264x1840@30fps */
	//setting V20250325
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = daliov08f10ultra_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(daliov08f10ultra_normal_video_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 478,
		.framelength = 2501,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3280,
			.full_h = 2464,
			.x0_offset = 8,
			.y0_offset = 312,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* mode 3 : same as preview 3264x2448@30fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = daliov08f10ultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(daliov08f10ultra_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36500000,
		.linelength = 478,
		.framelength = 2544,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3280,
			.full_h = 2464,
			.x0_offset = 8,
			.y0_offset = 8,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* mode 4 : same as preview 3264x2448@30fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = daliov08f10ultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(daliov08f10ultra_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36500000,
		.linelength = 478,
		.framelength = 2544,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3280,
			.full_h = 2464,
			.x0_offset = 8,
			.y0_offset = 8,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* custom1 mode 5: video 3264x2040@30fps */
	//setting V20250325
	{
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = daliov08f10ultra_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(daliov08f10ultra_custom1_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 478,
		.framelength = 2501,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3280,
			.full_h = 2464,
			.x0_offset = 8,
			.y0_offset = 212,
			.w0_size = 3264,
			.h0_size = 2040,
			.scale_w = 3264,
			.scale_h = 2040,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 2040,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 2040,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* custom2 mode 6: binning crop 1440x1080@10fps */
	//setting V20250325
	{
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = daliov08f10ultra_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(daliov08f10ultra_custom2_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 478,
		.framelength = 7506,
		.max_framerate = 100,
		.mipi_pixel_rate = 144000000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3280,
			.full_h = 2464,
			.x0_offset = 200,
			.y0_offset = 152,
			.w0_size = 2880,
			.h0_size = 2160,
			.scale_w = 1440,
			.scale_h = 1080,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 1440,
			.h1_size = 1080,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1440,
			.h2_tg_size = 1080,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* custom3 mode 7: fullsize crop 2944x2208@30fps */
	//setting V20250325
	{
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = daliov08f10ultra_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(daliov08f10ultra_custom3_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 478,
		.framelength = 2501,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3280,
			.full_h = 2464,
			.x0_offset = 168,
			.y0_offset = 128,
			.w0_size = 2944,
			.h0_size = 2208,
			.scale_w = 2944,
			.scale_h = 2208,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2944,
			.h1_size = 2208,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2944,
			.h2_tg_size = 2208,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	/* custom4 mode 8: fullsize crop 1536x1152@30fps */
	//setting V20250325
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = daliov08f10ultra_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(daliov08f10ultra_custom4_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 478,
		.framelength = 2501,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
		.framelength_step = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 3280,
			.full_h = 2464,
			.x0_offset = 872,
			.y0_offset = 656,
			.w0_size = 1536,
			.h0_size = 1152,
			.scale_w = 1536,
			.scale_h = 1152,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 1536,
			.h1_size = 1152,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1536,
			.h2_tg_size = 1152,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = DALIOV08F10ULTRA_SENSOR_ID,
	.reg_addr_sensor_id = {0x00, 0x01},
	.i2c_addr_table = {0x6C, 0xFF}, // TBD
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
	.ana_gain_table = daliov08f10ultra_ana_gain_table,
	.ana_gain_table_size = sizeof(daliov08f10ultra_ana_gain_table),
	.tuning_iso_base = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 8,
	.exposure_max = 0x3FFFEB,
	.exposure_step = 1,
	.exposure_margin = 40,

	.frame_length_max = 0x3fffff,
	.ae_effective_frame = 3,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 2700000,

	.pdaf_type = PDAF_SUPPORT_NA,
	.hdr_type = HDR_SUPPORT_NA,
	.seamless_switch_support = FALSE,
	.temperature_support = FALSE,

	.g_temp = PARAM_UNDEFINED,
	.g_gain2reg = get_gain2reg,
	.s_gph = PARAM_UNDEFINED,

//	.reg_addr_stream = 0x3e,
	.reg_addr_mirror_flip = PARAM_UNDEFINED, // TBD
	// .reg_addr_exposure = {
	// 	{0x02, 0x03, 0x04},
	// },
	.long_exposure_support = FALSE,
	.reg_addr_exposure_lshift = PARAM_UNDEFINED,
	// .reg_addr_ana_gain = {
	// 	{0x24},
	// },
	// .reg_addr_dig_gain = {
	// 	{0x21, 0x22},
	// },
//	.reg_addr_frame_length = {0x34, 0x35, 0x36},
	.reg_addr_temp_en = PARAM_UNDEFINED,
	.reg_addr_temp_read = PARAM_UNDEFINED,
	.reg_addr_auto_extend = PARAM_UNDEFINED,
	.reg_addr_frame_count = PARAM_UNDEFINED,

	.init_setting_table = daliov08f10ultra_init_setting,
	.init_setting_len = ARRAY_SIZE(daliov08f10ultra_init_setting),
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
	.get_id = daliov08f10ultra_get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = common_open,
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = daliov08f10ultra_control,
	.feature_control = common_feature_control,
	.close = daliov08f10ultra_close,
	.get_frame_desc = common_get_frame_desc,
	.get_temp = common_get_temp,
	.get_csi_param = daliov08f10ultra_get_csi_param,
	.update_sof_cnt = common_update_sof_cnt,
	.vsync_notify = ultra_vsync_notify,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_DOVDD, {1800000, 1800000}, 1000},
	{HW_ID_RST, {0}, 1000},
	{HW_ID_AVDD, {2800000, 2800000}, 1000},
	{HW_ID_DVDD, {1200000, 1200000}, 6000},
	{HW_ID_RST, {1}, 1000},
	{HW_ID_MCLK, {24}, 0},
	{HW_ID_MCLK_DRIVING_CURRENT, {4}, 8000},
};

const struct subdrv_entry daliov08f10ultra_mipi_raw_entry = {
	.name = "daliov08f10ultra_mipi_raw",
	.id = DALIOV08F10ULTRA_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

static int daliov08f10ultra_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id)
{
	u8 i = 0;
	u8 retry = 2;

	while (ctx->s_ctx.i2c_addr_table[i] != 0xFF) {
		ctx->i2c_write_id = ctx->s_ctx.i2c_addr_table[i];
		do {
			*sensor_id = daliov08f10ultra_return_sensor_id(ctx);

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

static int daliov08f10ultra_return_sensor_id(struct subdrv_ctx *ctx)
{
	int sensor_id = 0;
	u8 addr_h = ctx->s_ctx.reg_addr_sensor_id.addr[0];
	u8 addr_l = ctx->s_ctx.reg_addr_sensor_id.addr[1];

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	sensor_id = (subdrv_i2c_rd_u8_u8(ctx, addr_h) << 8) | subdrv_i2c_rd_u8_u8(ctx, addr_l);

	DRV_LOG_MUST(ctx, "sensor_id is:0x%x\n", sensor_id);

	return sensor_id;
}

static int daliov08f10ultra_control(struct subdrv_ctx *ctx,
			enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	int i = 0, ret = 0;

	DRV_LOG(ctx, "scenario_id = %d\n", scenario_id);

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

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x32);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x0f);
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x30);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x0e);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0xFD, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0E);
	mdelay(6);

	subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0b);
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0x10, 0x08);
	subdrv_i2c_wr_u8_u8(ctx, 0x11, 0x5e);
	subdrv_i2c_wr_u8_u8(ctx, 0x12, 0x01);
	subdrv_i2c_wr_u8_u8(ctx, 0x13, 0x15);
	subdrv_i2c_wr_u8_u8(ctx, 0x14, 0x20);
	subdrv_i2c_wr_u8_u8(ctx, 0x1e, 0x13);
	subdrv_i2c_wr_u8_u8(ctx, 0x19, 0x40);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x00);
	mdelay(6);
	subdrv_i2c_wr_u8_u8(ctx, 0xae, 0x64);

	for (i = 0; i < len; i = i+2) {
		ret |= subdrv_i2c_wr_u8_u8(ctx, list[i], list[i+1]&0xff);
	}

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x32);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x0f);


	daliov08f10ultra_streaming_control(ctx, FALSE);

	DRV_LOG_MUST(ctx, "-\n");
	return ERROR_NONE;
}


static int daliov08f10ultra_streamon(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	return daliov08f10ultra_streaming_control(ctx, TRUE);
}

static int daliov08f10ultra_streamoff(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	return daliov08f10ultra_streaming_control(ctx, FALSE);
}

static int daliov08f10ultra_streaming_control(struct subdrv_ctx *ctx, bool enable)
{
	DRV_LOG_MUST(ctx, "streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable) {
		// subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		// subdrv_i2c_wr_u8_u8(ctx, 0xa0, 0x01);

		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
		// subdrv_i2c_wr_u8_u8(ctx, 0x15, 0x70);
		subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x03);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0xa0, 0x01);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x03);
		subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0f);
		// subdrv_i2c_wr_u8_u8(ctx, 0xa0, 0x01);
		streaming_status = true;
		soft_power_status = true;
		blc_trigger_flag = true;
	} else {
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0xa0, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0b);
		mdelay(33);
		subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x3a);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x07);
		streaming_status = false;
		soft_power_status = false;
		blc_trigger_flag = false;
	}

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
	DRV_LOG_MUST(ctx, "sensormode(%d) sof_cnt(%d)\n", ctx->current_scenario_id, sof_cnt);
	return 0;
}

static void daliov08f10ultra_set_dummy(struct subdrv_ctx *ctx)
{
	int dummy_line = 0;

	if (!soft_power_status) {
		daliov08f10ultra_soft_power_up(ctx);
	}

	dummy_line = (ctx->frame_length - ctx->vblank_convert) * 2;
	if (dummy_line < 0 || dummy_line > 0xffff) {
		DRV_LOG_MUST(ctx, "enable auto extended vts\n");
		ctx->dummy_line = 0xffff;
	} else {
		ctx->dummy_line = dummy_line;
	}
	DRV_LOG(ctx, "vts = %d, dummy_line = %d\n",
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

static void daliov08f10ultra_set_max_framerate(struct subdrv_ctx *ctx, u16 framerate, u8 min_framelength_en)
{
	u32 frame_length = ctx->frame_length;

	DRV_LOG_MUST(ctx, "framerate = %d, min framelength should enable %d\n", framerate, min_framelength_en);

	frame_length = ctx->pclk * 10 / ctx->line_length / framerate;
	if (frame_length >= ctx->min_frame_length)
		ctx->frame_length = frame_length;
	else
		ctx->frame_length = ctx->min_frame_length;

	if (ctx->frame_length > ctx->max_frame_length) {
		ctx->frame_length = ctx->max_frame_length;
	}
	ctx->dummy_line = ctx->frame_length - ctx->min_frame_length;

	if (min_framelength_en)
		ctx->min_frame_length = ctx->frame_length;
	daliov08f10ultra_set_dummy(ctx);
}

static int daliov08f10ultra_set_auto_flicker_mode(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	bool enable = *((bool *)para);

	if (enable) {
		ctx->autoflicker_en = KAL_TRUE;
		DRV_LOG_MUST(ctx, "flicker enable\n");
	} else {
		ctx->autoflicker_en = KAL_FALSE;
	}

	return ERROR_NONE;
}

static int daliov08f10ultra_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 gain = *((u32 *)para);
	u8 reg_gain;

	if (!soft_power_status) {
		daliov08f10ultra_soft_power_up(ctx);
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

	DRV_LOG(ctx, "ctx->sensor_mode: %d, gain = %d, ctx->ana_gain[0] = 0x%x ,reg_gain = 0x%x, max_gain:0x%x\n",
		ctx->current_scenario_id, gain, ctx->ana_gain[0], reg_gain, ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_ana_gain_range[0].max);

	if (ctx->frame_length >= 5002) {
		DRV_LOG(ctx, "(>= 5002)framelength =%d, open DPC\n", ctx->frame_length);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x02);
		subdrv_i2c_wr_u8_u8(ctx, 0x9a, 0x30);
	} else {
		DRV_LOG(ctx, "close DPC\n");
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x02);
		subdrv_i2c_wr_u8_u8(ctx, 0x9a, 0x20);
	}

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
	subdrv_i2c_wr_u8_u8(ctx, 0x24, reg_gain);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0x22, 0x40);//1x digital gain
	// subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
	// subdrv_i2c_wr_u8_u8(ctx, 0x12, 0x00);

	if (streaming_status)
		subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);

	return ERROR_NONE;
}

static int daliov08f10ultra_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 shutter = *((u64 *)para);
	u32 frame_length = 0;
	u32 fine_integ_line = 0;
	u16 realtime_fps = 0;

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

	if (ctx->autoflicker_en) {
		realtime_fps = ctx->pclk / ctx->line_length * 10 /
				ctx->frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			daliov08f10ultra_set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			daliov08f10ultra_set_max_framerate(ctx, 146, 0);
		else {
			daliov08f10ultra_set_dummy(ctx);
		}
	} else {
		daliov08f10ultra_set_dummy(ctx);
	}

	// Update Shutter
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
	subdrv_i2c_wr_u8_u8(ctx, 0x02, (shutter*2 >> 16) & 0xff);
	subdrv_i2c_wr_u8_u8(ctx, 0x03, (shutter*2 >> 8) & 0xff);
	subdrv_i2c_wr_u8_u8(ctx, 0x04, shutter*2 & 0xff);
	if (streaming_status)
		subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);

	DRV_LOG(ctx, "set shutter =%llu, framelength =%d\n", shutter, ctx->frame_length);

	return ERROR_NONE;
}

static int daliov08f10ultra_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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

		daliov08f10ultra_set_dummy(ctx);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
		subdrv_i2c_wr_u8_u8(ctx, 0x02, (shutters[0]*2 >> 16) & 0xff);
		subdrv_i2c_wr_u8_u8(ctx, 0x03, (shutters[0]*2 >> 8) & 0xff);
		subdrv_i2c_wr_u8_u8(ctx, 0x04, shutters[0]*2 & 0xff);
		if (streaming_status)
			subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);

		DRV_LOG(ctx, "shutter =%llu, framelength =%d\n", shutters[0], ctx->frame_length);
	}

	return ERROR_NONE;
}

static int daliov08f10ultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	DRV_LOG(ctx, "+\n");
	daliov08f10ultra_set_shutter_frame_length(ctx, para, len);
	return ERROR_NONE;
}

static int daliov08f10ultra_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

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

static void daliov08f10ultra_soft_power_up(struct subdrv_ctx *ctx)
{
	DRV_LOG(ctx, "soft_power_up_func\n");

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0x19, 0x40);
	// subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0b);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x06);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x04);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x38);
	mdelay(5);
	soft_power_status = true;
}

static int daliov08f10ultra_close(struct subdrv_ctx *ctx)
{
	DRV_LOG(ctx, "daliov08f10ultra_close\n");
	return ERROR_NONE;
}

static int daliov08f10ultra_get_csi_param(struct subdrv_ctx *ctx, enum SENSOR_SCENARIO_ID_ENUM scenario_id, struct mtk_csi_param *csi_param){
	switch (scenario_id) {
	default:
		csi_param->eq_enable = 1;
		csi_param->eq_bw     = 3;
		csi_param->eq_dg0_en = 0;
		csi_param->eq_sr0    = 0;
		csi_param->eq_dg1_en = 0;
		csi_param->eq_sr1    = 0;
		break;
	}
	DRV_LOG(ctx, "scenario_id:%u, eq param custom:%d/%d %d/%d %d/%d\n", scenario_id,
		csi_param->eq_enable, csi_param->eq_bw,
		csi_param->eq_dg0_en, csi_param->eq_sr0,
		csi_param->eq_dg1_en, csi_param->eq_sr1);
	return 0;
}
