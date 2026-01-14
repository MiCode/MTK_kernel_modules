// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 lapisov08f10ultramipiraw_Sensor.c
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
#include "lapisov08f10ultramipiraw_Sensor.h"

static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static u16 get_gain2reg(u32 gain);
static int ultra_vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt);
static int lapisov08f10ultra_close(struct subdrv_ctx *ctx);
static int lapisov08f10ultra_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id);
static int lapisov08f10ultra_return_sensor_id(struct subdrv_ctx *ctx);
static int lapisov08f10ultra_control(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data);
static int lapisov08f10ultra_streamon(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapisov08f10ultra_streamoff(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapisov08f10ultra_streaming_control(struct subdrv_ctx *ctx, bool enable);
static int lapisov08f10ultra_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapisov08f10ultra_set_auto_flicker_mode(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapisov08f10ultra_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapisov08f10ultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapisov08f10ultra_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapisov08f10ultra_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static void lapisov08f10ultra_set_dummy(struct subdrv_ctx *ctx);
static void lapisov08f10ultra_set_max_framerate(struct subdrv_ctx *ctx, u16 framerate, u8 min_framelength_en);
static int lapisov08f10ultra_get_vendor_id(struct subdrv_ctx *ctx);

u8 lapisov08f10_vendor_id;
u32 lapisov08f10_fusion_len;
u32 lapisov08f10_sn_len;
char lapisov08f10_fusionID[20];
char lapisov08f10_sensorSn[20];
static bool is_ov08f_otp_read = false;

static bool streaming_status = false;
static bool soft_power_status = false; //false = soft power down, true = soft power up
static bool blc_trigger_flag = false; // true = need to write p7 0x00 0xf8

/* STRUCT */
static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_STREAMING_SUSPEND, lapisov08f10ultra_streamoff},
	{SENSOR_FEATURE_SET_STREAMING_RESUME, lapisov08f10ultra_streamon},
	{SENSOR_FEATURE_SET_GAIN, lapisov08f10ultra_set_gain},
	{SENSOR_FEATURE_SET_ESHUTTER, lapisov08f10ultra_set_shutter},
	{SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME, lapisov08f10ultra_set_shutter_frame_length},
	{SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME, lapisov08f10ultra_set_multi_shutter_frame_length},
	{SENSOR_FEATURE_SET_AUTO_FLICKER_MODE, lapisov08f10ultra_set_auto_flicker_mode},
	{SENSOR_FEATURE_SET_TEST_PATTERN, lapisov08f10ultra_set_test_pattern},
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
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 1836,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};

static struct subdrv_mode_struct mode_struct[] = {
	/* mode 0 : preview 3264x2448@30fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = lapisov08f10ultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(lapisov08f10ultra_preview_setting),
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x4B,
		},
	},
	/* mode 1 : capture same as preview 3264x2448@30fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = lapisov08f10ultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(lapisov08f10ultra_preview_setting),
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x4B,
		},
	},
	/* mode 2 : video 3264x1836@30fps */
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = lapisov08f10ultra_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(lapisov08f10ultra_normal_video_setting),
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
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 0,
			.y0_offset = 306,
			.w0_size = 3264,
			.h0_size = 1836,
			.scale_w = 3264,
			.scale_h = 1836,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 1836,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 1836,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x4B,
		},
	},
	/* mode 3 : same as preview 3264x2448@30fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = lapisov08f10ultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(lapisov08f10ultra_preview_setting),
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x4B,
		},
	},
	/* mode 4 : same as preview 3264x2448@30fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = lapisov08f10ultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(lapisov08f10ultra_preview_setting),
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x4B,
		},
	},
	/* custom1 mode 5: same as preview 3264x2448@24fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = lapisov08f10ultra_bokeh_setting,
		.mode_setting_len = ARRAY_SIZE(lapisov08f10ultra_bokeh_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 36000000,
		.linelength = 472,
		.framelength = 3177,
		.max_framerate = 240,
		.mipi_pixel_rate = 288000000,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x4B,
		},
	},
	/* custom2 mode 6: same as preview 3264x2448@30fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = lapisov08f10ultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(lapisov08f10ultra_preview_setting),
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x4B,
		},
	},
	/* custom3 mode 7: same as preview 3264x2448@30fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = lapisov08f10ultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(lapisov08f10ultra_preview_setting),
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x4B,
		},
	},
	/* custom4 mode 8: same as preview 3264x2448@30fps */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = lapisov08f10ultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(lapisov08f10ultra_preview_setting),
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x4B,
		},
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = LAPISOV08F10ULTRA_SENSOR_ID,
	.reg_addr_sensor_id = {0x00, 0x01},
	.i2c_addr_table = {0x6C, 0xFF}, // TBD
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	// .eeprom_info = eeprom_info,
	// .eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {3264, 2448},
	.mirror = IMAGE_HV_MIRROR, // TBD

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_lane_num = SENSOR_MIPI_2_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 15.5,
	.ana_gain_type = 1,
	.ana_gain_step = 1,
	.ana_gain_table = lapisov08f10ultra_ana_gain_table,
	.ana_gain_table_size = sizeof(lapisov08f10ultra_ana_gain_table),
	.min_gain_iso = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 8,
	.exposure_max = 0x3FFFEF - 20,
	.exposure_step = 1,
	.exposure_margin = 20,

	.frame_length_max = 0x3fffff,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 2,
//	.start_exposure_offset = 153800,

	.pdaf_type = FALSE,
	.hdr_type = FALSE,
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

	.init_setting_table = lapisov08f10ultra_init_setting,
	.init_setting_len = ARRAY_SIZE(lapisov08f10ultra_init_setting),
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
	.get_vendr_id = common_get_vendor_id,
	.get_id = lapisov08f10ultra_get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = common_open,
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = lapisov08f10ultra_control,
	.feature_control = common_feature_control,
	.close = lapisov08f10ultra_close,
	.get_frame_desc = common_get_frame_desc,
	.get_temp = common_get_temp,
	.get_csi_param = common_get_csi_param,
	.update_sof_cnt = common_update_sof_cnt,
	.vsync_notify = ultra_vsync_notify,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_DOVDD, 1800000, 1},
	{HW_ID_RST, 0, 1},
	{HW_ID_AVDD, 2800000, 1},
	{HW_ID_DVDD, 1200000, 6},
	{HW_ID_RST, 1, 1},
	{HW_ID_MCLK, 24, 0},
	{HW_ID_MCLK_DRIVING_CURRENT, 6, 8},
};

const struct subdrv_entry lapisov08f10ultra_mipi_raw_entry = {
	.name = "lapisov08f10ultra_mipi_raw",
	.id = LAPISOV08F10ULTRA_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

static int lapisov08f10ultra_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id)
{
	u8 i = 0;
	u8 retry = 2;

	while (ctx->s_ctx.i2c_addr_table[i] != 0xFF) {
		ctx->i2c_write_id = ctx->s_ctx.i2c_addr_table[i];
		do {
			*sensor_id = lapisov08f10ultra_return_sensor_id(ctx);

			DRV_LOG(ctx, "i2c_write_id:0x%x sensor_id(cur/exp):0x%x/0x%x\n",
				ctx->i2c_write_id, *sensor_id, ctx->s_ctx.sensor_id);

			DRV_LOG_MUST(ctx, "i2c_write_id_ov08f10:0x%x sensor_id(cur/exp):0x%x/0x%x\n",
				ctx->i2c_write_id, *sensor_id, ctx->s_ctx.sensor_id);
			if (*sensor_id == ctx->s_ctx.sensor_id){
				if(!is_ov08f_otp_read){
					lapisov08f10ultra_get_vendor_id(ctx);
					is_ov08f_otp_read = true;
				}
				return ERROR_NONE;
			}
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

static int lapisov08f10ultra_return_sensor_id(struct subdrv_ctx *ctx)
{
	int sensor_id = 0;
	u8 addr_h = ctx->s_ctx.reg_addr_sensor_id.addr[0];
	u8 addr_l = ctx->s_ctx.reg_addr_sensor_id.addr[1];

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	sensor_id = (subdrv_i2c_rd_u8_u8(ctx, addr_h) << 8) | subdrv_i2c_rd_u8_u8(ctx, addr_l);

	DRV_LOG_MUST(ctx, "sensor_id is:0x%x\n", sensor_id);

	return sensor_id;
}

static int lapisov08f10ultra_control(struct subdrv_ctx *ctx,
			enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	int i = 0, ret = 0;
	u16 *list;
	u32 len;

	DRV_LOG_MUST(ctx, "scenario_id = %d\n", scenario_id);

	if (scenario_id >= ctx->s_ctx.sensor_mode_num) {
		DRV_LOGE(ctx, "invalid sid:%u, mode_num:%u\n",
			scenario_id, ctx->s_ctx.sensor_mode_num);
		scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW;
	}

	ctx->autoflicker_en = KAL_FALSE;
	ctx->vblank_convert = ctx->s_ctx.mode[scenario_id].framelength;

	ctx->current_scenario_id = scenario_id;
	list = ctx->s_ctx.mode[scenario_id].mode_setting_table;
	len = ctx->s_ctx.mode[scenario_id].mode_setting_len;

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x32);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x0f);
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x30);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x0e);
	subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0e);
	mdelay(3);
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
	mdelay(3);

	for (i = 0; i < len; i = i+2) {
		ret |= subdrv_i2c_wr_u8_u8(ctx, list[i], list[i+1]&0xff);
	}

	return ERROR_NONE;
}


static int lapisov08f10ultra_streamon(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	return lapisov08f10ultra_streaming_control(ctx, TRUE);
}

static int lapisov08f10ultra_streamoff(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	return lapisov08f10ultra_streaming_control(ctx, FALSE);
}

static int lapisov08f10ultra_streaming_control(struct subdrv_ctx *ctx, bool enable)
{
	DRV_LOG_MUST(ctx, "lp streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable) {
		// 20 fd 00
		// 20 21 06
		// 20 21 04
		// 20 21 00
		// 20 fd 00
		// 20 c2 38
		// sl 5 5 ;;;delay 5ms must have
		// ;;can also set new shutter/gain
		// 20 fd 01
		// 20 33 03
		// 20 01 03
		// 20 fd 00
		// 20 a0 01
		// 20 e7 03
		// 20 e7 00
		// 20 20 0f
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x06);
		subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x04);
		subdrv_i2c_wr_u8_u8(ctx, 0x21, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0xc2, 0x38);
		mdelay(5);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
		subdrv_i2c_wr_u8_u8(ctx, 0x33, 0x03);
		subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x03);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0xa0, 0x01);
		subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x03);
		subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x00);
		subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0f);
		streaming_status = true;
		soft_power_status = true;
		blc_trigger_flag = true;
	} else {
		// 20 fd 00
		// 20 a0 00
		// 20 fd 00
		// 20 20 0b
		// sl 33 33 ;;;delay 33ms must have
		// 20 c2 3a
		// 20 fd 00
		// 20 21 07
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

static int ultra_vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt)
{
	DRV_LOG_MUST(ctx, "sensormode(%d) sof_cnt(%d)\n", ctx->current_scenario_id, sof_cnt);
	return 0;
}

static void lapisov08f10ultra_set_dummy(struct subdrv_ctx *ctx)
{
	int dummy_line = 0;


	dummy_line = (ctx->frame_length - 2501) * 2;
	if (dummy_line < 0 || dummy_line > 0xffff) {
		DRV_LOG_MUST(ctx, "enable auto extended vts\n");
		ctx->dummy_line = 0xffff;
	} else {
		ctx->dummy_line = dummy_line;
	}
	DRV_LOG_MUST(ctx, "vts = %d, dummy_line = %d\n",
		ctx->frame_length, ctx->dummy_line);

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

static void lapisov08f10ultra_set_max_framerate(struct subdrv_ctx *ctx, u16 framerate, u8 min_framelength_en)
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
	lapisov08f10ultra_set_dummy(ctx);
}

static int lapisov08f10ultra_set_auto_flicker_mode(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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

static int lapisov08f10ultra_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 gain = *((u32 *)para);
	u8 reg_gain;
	DRV_LOG_MUST(ctx, "gain = %d\n", gain);



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
	subdrv_i2c_wr_u8_u8(ctx, 0x24, (u8)reg_gain);
	subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);

	return ERROR_NONE;
}

static int lapisov08f10ultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	return lapisov08f10ultra_set_shutter_frame_length(ctx, para, len);
}

static int lapisov08f10ultra_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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

	//update DPC
	if(shutter <= 5016){
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x02);
		subdrv_i2c_wr_u8_u8(ctx, 0x9a, 0x20);
	}else{
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x02);
		subdrv_i2c_wr_u8_u8(ctx, 0x9a, 0x30);
	}

	/* restore shutter */
	memset(ctx->exposure, 0, sizeof(ctx->exposure));
	ctx->exposure[0] = (u32)shutter;

	if (ctx->autoflicker_en) {
		realtime_fps = ctx->pclk / ctx->line_length * 10 /
				ctx->frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			lapisov08f10ultra_set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			lapisov08f10ultra_set_max_framerate(ctx, 146, 0);
		else {
			lapisov08f10ultra_set_dummy(ctx);
		}
	} else {
		lapisov08f10ultra_set_dummy(ctx);
	}

	// Update Shutter
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
	subdrv_i2c_wr_u8_u8(ctx, 0x02, (shutter*2 >> 16) & 0xff);
	subdrv_i2c_wr_u8_u8(ctx, 0x03, (shutter*2 >> 8) & 0xff);
	subdrv_i2c_wr_u8_u8(ctx, 0x04, shutter*2 & 0xff);
	subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);
	DRV_LOG_MUST(ctx, "set shutter =%llu, framelength =%d\n", shutter, ctx->frame_length);
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);

	return ERROR_NONE;
}

static int lapisov08f10ultra_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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

		lapisov08f10ultra_set_dummy(ctx);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);
		subdrv_i2c_wr_u8_u8(ctx, 0x02, (shutters[0]*2 >> 16) & 0xff);
		subdrv_i2c_wr_u8_u8(ctx, 0x03, (shutters[0]*2 >> 8) & 0xff);
		subdrv_i2c_wr_u8_u8(ctx, 0x04, shutters[0]*2 & 0xff);
		subdrv_i2c_wr_u8_u8(ctx, 0x01, 0x01);
		subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x01);

		DRV_LOG_MUST(ctx, "shutter =%llu, framelength =%d\n", shutters[0], ctx->frame_length);
	}

	return ERROR_NONE;
}

static int lapisov08f10ultra_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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

static int lapisov08f10ultra_close(struct subdrv_ctx *ctx)
{
	DRV_LOG_MUST(ctx, "lapisov08f10ultra_close\n");
	return ERROR_NONE;
}

static u8 lapisov08f10ultra_read_otp_data(struct subdrv_ctx *ctx, u16 addr){
	u8 val = 0;
	u16 block = 0;
	u16 offset = 0;
	u16 block_size = 128;

	block = addr / block_size;
	offset = addr % block_size;

	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x03);
	subdrv_i2c_wr_u8_u8(ctx, 0xa9, block); //block63
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x09);
	val = subdrv_i2c_rd_u8_u8(ctx, offset);
	return val;
}

static int lapisov08f10ultra_get_vendor_id(struct subdrv_ctx *ctx)
{
	int i = 0;
	u16 snReg = 3774;
	u16 fusionReg = 16;
	u16 vendorIdReg = 0x01;
	u8 otpflag[2];
	u32 offset = 0;

	//init otp
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0x1d, 0x00);
	subdrv_i2c_wr_u8_u8(ctx, 0x1c, 0x19);
	subdrv_i2c_wr_u8_u8(ctx, 0x20, 0x0f);
	subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x03);
	subdrv_i2c_wr_u8_u8(ctx, 0xe7, 0x00);
	mdelay(3);
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x03);
	subdrv_i2c_wr_u8_u8(ctx, 0xa1, 0x46);
	subdrv_i2c_wr_u8_u8(ctx, 0xa6, 0x44);
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x03);
	subdrv_i2c_wr_u8_u8(ctx, 0x9f, 0x20);
	subdrv_i2c_wr_u8_u8(ctx, 0x9d, 0x10);
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x03);
	subdrv_i2c_wr_u8_u8(ctx, 0xa9, 0x3F); //block63
	subdrv_i2c_wr_u8_u8(ctx, 0xfd, 0x09);

	otpflag[0] = subdrv_i2c_rd_u8_u8(ctx, 0x23);
	otpflag[1] = subdrv_i2c_rd_u8_u8(ctx, 0x24);

	if(otpflag[0] == 0x55 && otpflag[1] == 0x00){
		offset = 0x0200;    //group 1
	} else if(otpflag[0] == 0xff && otpflag[1] == 0x55){
		offset = 0x10D2; //group 2 
	} else{
		printk("ov08f read group flag error: Group1 flag 0x%x, Group2 flag 0x%x\n", otpflag[0], otpflag[1]);
	}

	snReg += offset;
	vendorIdReg += offset;
	fusionReg += offset;

	//vendor id
	lapisov08f10_vendor_id = lapisov08f10ultra_read_otp_data(ctx, vendorIdReg);
	//ADD FTM INFO
	lapisov08f10_fusion_len = 16;
	lapisov08f10_sn_len = 14;

	for(i = 0;i < lapisov08f10_sn_len; i++) {
		snReg++;
		lapisov08f10_sensorSn[i] = lapisov08f10ultra_read_otp_data(ctx, snReg);
	}
	//fusionID
	for(i = 0;i < lapisov08f10_fusion_len; i++) {
		lapisov08f10_fusionID[i] = lapisov08f10ultra_read_otp_data(ctx, fusionReg);
		fusionReg++;
	}
	return 0;
}