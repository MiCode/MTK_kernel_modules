// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 imx598mipiraw_Sensor.c
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
#include "imx598mipiraw_Sensor.h"

static int get_sensor_temperature(void *arg);
static void set_group_hold(void *arg, u8 en);
static u16 get_gain2reg(u32 gain);
static int imx598_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int imx598_set_test_pattern_data(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, imx598_set_test_pattern},
	{SENSOR_FEATURE_SET_TEST_PATTERN_DATA, imx598_set_test_pattern_data},
};

static struct eeprom_info_struct eeprom_info[] = {
	{
		.header_id = 0x010B00FF,
		.addr_header_id = 0x0000000B,
		.i2c_write_id = 0xA9,
	},
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 17,
	.i4OffsetY = 12,
	.i4PitchX = 8,
	.i4PitchY = 16,
	.i4PairNum = 8,
	.i4SubBlkW = 8,
	.i4SubBlkH = 2,
	.i4PosL = {{20, 13}, {18, 15}, {22, 17}, {24, 19},
	           {20, 21}, {18, 23}, {22, 25}, {24, 27}},
	.i4PosR = {{19, 13}, {17, 15}, {21, 17}, {23, 19},
	           {19, 21}, {17, 23}, {21, 25}, {23, 27}},
	.i4BlockNumX = 496,
	.i4BlockNumY = 186,
	.i4LeFirst = 0,
	.i4Crop = {
		// <prev> <cap> <vid> <hs_vid> <slim_vid>
		{0, 0}, {0, 0}, {0, 372}, {0, 372}, {0, 372},
		// <cust1> <<cust2>> <<cust3>>
		{0, 0}, {0, 0}, {0, 0},
	},
	.iMirrorFlip = 0,

	.sPDMapInfo[0] = {
		.i4PDPattern = 2,
		.i4PDRepetition = 4,
		.i4PDOrder = {1}, // R = 1, L = 0
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0FA0,
			.vsize = 0x0BB8,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x34,
			.hsize = 0x03E0,
			.vsize = 0x05D0,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0FA0,
			.vsize = 0x0BB8,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x34,
			.hsize = 0x03E0,
			.vsize = 0x05D0,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0FA0,
			.vsize = 0x08D0,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_hs_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0FA0,
			.vsize = 0x08D0,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0FA0,
			.vsize = 0x08D0,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0FA0,
			.vsize = 0x0BB8,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x07D0,
			.vsize = 0x046C,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x07D0,
			.vsize = 0x05DC,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x07D0,
			.vsize = 0x05DC,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct subdrv_mode_struct mode_struct[] = {
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = imx598_preview_setting,
		.mode_setting_len = ARRAY_SIZE(imx598_preview_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 864000000,
		.linelength = 7872,
		.framelength = 3658,
		.max_framerate = 300,
		.mipi_pixel_rate = 880000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.coarse_integ_step = 1,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.imgsensor_winsize_info = {
			.full_w = 8000,
			.full_h = 6000,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 8000,
			.h0_size = 6000,
			.scale_w = 4000,
			.scale_h = 3000,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4000,
			.h1_size = 3000,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4000,
			.h2_tg_size = 3000,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 634,
		.delay_frame = 3,
		.csi_param = {
			.dphy_data_settle = 59,
			.dphy_clk_settle = 59,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	},
	{
		.frame_desc = frame_desc_cap,
		.num_entries = ARRAY_SIZE(frame_desc_cap),
		.mode_setting_table = imx598_capture_setting,
		.mode_setting_len = ARRAY_SIZE(imx598_capture_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 864000000,
		.linelength = 7872,
		.framelength = 3658,
		.max_framerate = 300,
		.mipi_pixel_rate = 880000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.coarse_integ_step = 1,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.imgsensor_winsize_info = {
			.full_w = 8000,
			.full_h = 6000,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 8000,
			.h0_size = 6000,
			.scale_w = 4000,
			.scale_h = 3000,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4000,
			.h1_size = 3000,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4000,
			.h2_tg_size = 3000,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 634,
		.delay_frame = 3,
		.csi_param = {
			.dphy_data_settle = 59,
			.dphy_clk_settle = 59,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	},
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = imx598_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx598_normal_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 691200000,
		.linelength = 4592,
		.framelength = 5016,
		.max_framerate = 300,
		.mipi_pixel_rate = 880000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.coarse_integ_step = 1,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 5,
		.imgsensor_winsize_info = {
			.full_w = 8000,
			.full_h = 6000,
			.x0_offset = 0,
			.y0_offset = 744,
			.w0_size = 8000,
			.h0_size = 4512,
			.scale_w = 4000,
			.scale_h = 2256,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4000,
			.h1_size = 2256,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4000,
			.h2_tg_size = 2256,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = NULL,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 357,
		.delay_frame = 3,
		.csi_param = {
			.dphy_data_settle = 59,
			.dphy_clk_settle = 59,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	},
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = imx598_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx598_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 691200000,
		.linelength = 4592,
		.framelength = 2508,
		.max_framerate = 600,
		.mipi_pixel_rate = 880000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.coarse_integ_step = 1,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 5,
		.imgsensor_winsize_info = {
			.full_w = 8000,
			.full_h = 6000,
			.x0_offset = 0,
			.y0_offset = 744,
			.w0_size = 8000,
			.h0_size = 4512,
			.scale_w = 4000,
			.scale_h = 2256,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4000,
			.h1_size = 2256,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4000,
			.h2_tg_size = 2256,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = NULL,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 357,
		.delay_frame = 3,
		.csi_param = {
			.dphy_data_settle = 59,
			.dphy_clk_settle = 59,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	},
	{
		.frame_desc = frame_desc_slim_vid,
		.num_entries = ARRAY_SIZE(frame_desc_slim_vid),
		.mode_setting_table = imx598_slim_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx598_slim_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 864000000,
		.linelength = 4592,
		.framelength = 6270,
		.max_framerate = 300,
		.mipi_pixel_rate = 880000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.coarse_integ_step = 1,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 5,
		.imgsensor_winsize_info = {
			.full_w = 8000,
			.full_h = 6000,
			.x0_offset = 0,
			.y0_offset = 744,
			.w0_size = 8000,
			.h0_size = 4512,
			.scale_w = 4000,
			.scale_h = 2256,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4000,
			.h1_size = 2256,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4000,
			.h2_tg_size = 2256,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = NULL,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 357,
		.delay_frame = 3,
		.csi_param = {
			.dphy_data_settle = 59,
			.dphy_clk_settle = 59,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	},
	{
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = imx598_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(imx598_custom1_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 864000000,
		.linelength = 4592,
		.framelength = 6270,
		.max_framerate = 300,
		.mipi_pixel_rate = 880000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.coarse_integ_step = 1,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 5,
		.imgsensor_winsize_info = {
			.full_w = 8000,
			.full_h = 6000,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 8000,
			.h0_size = 6000,
			.scale_w = 4000,
			.scale_h = 3000,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4000,
			.h1_size = 3000,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4000,
			.h2_tg_size = 3000,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = NULL,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 357,
		.delay_frame = 3,
		.csi_param = {
			.dphy_data_settle = 59,
			.dphy_clk_settle = 59,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	},
	{
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = imx598_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(imx598_custom2_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 656000000,
		.linelength = 2912,
		.framelength = 7508,
		.max_framerate = 300,
		.mipi_pixel_rate = 880000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.imgsensor_winsize_info = {
			.full_w = 8000,
			.full_h = 6000,
			.x0_offset = 0,
			.y0_offset = 736,
			.w0_size = 8000,
			.h0_size = 4528,
			.scale_w = 2000,
			.scale_h = 1132,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2000,
			.h1_size = 1132,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2000,
			.h2_tg_size = 1132,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = NULL,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 875,
		.delay_frame = 3,
		.csi_param = {
			.dphy_data_settle = 59,
			.dphy_clk_settle = 59,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	},
	{
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = imx598_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(imx598_custom3_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 652800000,
		.linelength = 2912,
		.framelength = 7472,
		.max_framerate = 300,
		.mipi_pixel_rate = 880000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.imgsensor_winsize_info = {
			.full_w = 8000,
			.full_h = 6000,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 8000,
			.h0_size = 6000,
			.scale_w = 2000,
			.scale_h = 1500,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2000,
			.h1_size = 1500,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2000,
			.h2_tg_size = 1500,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = NULL,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 875,
		.delay_frame = 3,
		.csi_param = {
			.dphy_data_settle = 59,
			.dphy_clk_settle = 59,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	},
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = imx598_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(imx598_custom4_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 864000000,
		.linelength = 2912,
		.framelength = 4944,
		.max_framerate = 600,
		.mipi_pixel_rate = 880000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.imgsensor_winsize_info = {
			.full_w = 8000,
			.full_h = 6000,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 8000,
			.h0_size = 6000,
			.scale_w = 2000,
			.scale_h = 1500,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2000,
			.h1_size = 1500,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2000,
			.h2_tg_size = 1500,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = NULL,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 875,
		.delay_frame = 3,
		.csi_param = {
			.dphy_data_settle = 59,
			.dphy_clk_settle = 59,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = IMX598_SENSOR_ID,
	.reg_addr_sensor_id = {0x0016, 0x0017},
	.i2c_addr_table = {0x34, 0xFF},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {8000, 6000},
	.mirror = IMAGE_NORMAL,

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = 1150,
	.ana_gain_max = BASEGAIN * 64,
	.ana_gain_type = 0,//0:sony, 1:ov or samusng....etc no used
	.ana_gain_step = 1,// no used
	.ana_gain_table = imx598_ana_gain_table,
	.ana_gain_table_size = sizeof(imx598_ana_gain_table),
	.min_gain_iso = 100, // no change
	.exposure_def = 0x3D0, //no change
	.exposure_min = 16,
	.exposure_max = 128 * (0xFFFF - 48),
	.exposure_step = 2, // Get Maximum Step
	.exposure_margin = 48,
	.dig_gain_min = BASE_DGAIN * 1,
	.dig_gain_max = BASE_DGAIN * 16,
	.dig_gain_step = 4,

	.frame_length_max = 0xFFFF,
	.ae_effective_frame = 2,///SHUTTER AND GAIN N+1 (long expose used)
	.frame_time_delay_frame = 3,
	.start_exposure_offset = 5500000,

	.pdaf_type = PDAF_SUPPORT_CAMSV,
	.hdr_type = HDR_SUPPORT_NA,  //check this sensor is stagger or not.
	.seamless_switch_support = FALSE,
	.temperature_support = TRUE,

	.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,
	.s_cali = NULL,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = 0x0101,
	.reg_addr_exposure = {
			{0x0202, 0x0203},
	},
	.long_exposure_support = TRUE,
	.reg_addr_exposure_lshift = 0x3100,
	.reg_addr_ana_gain = {
			{0x0204, 0x0205},
	},
	.reg_addr_dig_gain = {
			{0x020E, 0x020F},
	},
	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_temp_en = 0x0138,
	.reg_addr_temp_read = 0x013A,
	.reg_addr_auto_extend = 0x0350,
	.reg_addr_frame_count = 0x0005,

	.init_setting_table = imx598_init_setting,
	.init_setting_len = ARRAY_SIZE(imx598_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 0,
	.chk_s_off_end = 0,

	.checksum_value = 0xecaae2a0,
};

static struct subdrv_ops ops = {
	.get_id = common_get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = common_open,
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = common_control,
	.feature_control = common_feature_control,
	.close = common_close,
	.get_frame_desc = common_get_frame_desc,
	.get_temp = common_get_temp,
	.get_csi_param = common_get_csi_param,
	.vsync_notify = vsync_notify,
	.update_sof_cnt = common_update_sof_cnt,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_MCLK, 24, 0},
	{HW_ID_RST, 0, 0},
	{HW_ID_MCLK_DRIVING_CURRENT, 4, 1},
	{HW_ID_AVDD2, 1800000, 0}, // pmic_gpo(2.8V ldo) for avdd
	{HW_ID_AVDD, 2800000, 0}, // pmic_ldo for avdd
	{HW_ID_AVDD4, 1800000, 0}, // pmic_ldo/pmic_gpo(1.8V ldo) for avdd1
	{HW_ID_AFVDD1, 1800000, 0}, // pmic_gpo(2.8V ldo) for afvdd
	{HW_ID_AFVDD, 2800000, 0}, // pmic_ldo for afvdd
	{HW_ID_DOVDD, 1800000, 0}, // pmic_ldo/gpio(1.8V ldo) for dovdd
	{HW_ID_DVDD, 1100000, 1}, // pmic_ldo for dvdd
	{HW_ID_RST, 1, 1}
};

const struct subdrv_entry imx598_mipi_raw_entry = {
	.name = "imx598_mipi_raw",
	.id = IMX598_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

/* FUNCTION */
static int get_sensor_temperature(void *arg)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	u8 temperature = 0;
	int temperature_convert = 0;

	temperature = subdrv_i2c_rd_u8(ctx, ctx->s_ctx.reg_addr_temp_read);

	if (temperature < 0x50)
		temperature_convert = temperature;
	else if (temperature < 0x80)
		temperature_convert = 80;
	else if (temperature < 0xED)
		temperature_convert = -20;
	else
		temperature_convert = (char)temperature;

	DRV_LOG(ctx, "temperature: %d degrees\n", temperature_convert);
	return temperature_convert;
}

static void set_group_hold(void *arg, u8 en)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	if (en)
		set_i2c_buffer(ctx, 0x0104, 0x01);
	else
		set_i2c_buffer(ctx, 0x0104, 0x00);
}

static u16 get_gain2reg(u32 gain)
{
	/*the below formula is unconditional carry */
	return (1024 - (1024 * BASEGAIN + (gain >> 1)) / gain);
}

static int imx598_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG_MUST(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	if (mode)
		subdrv_i2c_wr_u8(ctx, 0x0601, mode); /*100% Color bar*/
	else if (ctx->test_pattern)
		subdrv_i2c_wr_u8(ctx, 0x0601, 0x00); /*No pattern*/

	ctx->test_pattern = mode;
	return ERROR_NONE;
}

static int imx598_set_test_pattern_data(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	struct mtk_test_pattern_data *data = (struct mtk_test_pattern_data *)para;
	u16 R = (data->Channel_R >> 22) & 0x3ff;
	u16 Gr = (data->Channel_Gr >> 22) & 0x3ff;
	u16 Gb = (data->Channel_Gb >> 22) & 0x3ff;
	u16 B = (data->Channel_B >> 22) & 0x3ff;

	subdrv_i2c_wr_u8(ctx, 0x0602, (R >> 8));
	subdrv_i2c_wr_u8(ctx, 0x0603, (R & 0xff));
	subdrv_i2c_wr_u8(ctx, 0x0604, (Gr >> 8));
	subdrv_i2c_wr_u8(ctx, 0x0605, (Gr & 0xff));
	subdrv_i2c_wr_u8(ctx, 0x0606, (B >> 8));
	subdrv_i2c_wr_u8(ctx, 0x0607, (B & 0xff));
	subdrv_i2c_wr_u8(ctx, 0x0608, (Gb >> 8));
	subdrv_i2c_wr_u8(ctx, 0x0609, (Gb & 0xff));

	DRV_LOG_MUST(ctx, "mode(%u) R/Gr/Gb/B = 0x%04x/0x%04x/0x%04x/0x%04x\n",
		ctx->test_pattern, R, Gr, Gb, B);
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

static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt)
{
	DRV_LOG(ctx, "sof_cnt(%u) ctx->ref_sof_cnt(%u) ctx->fast_mode_on(%d)",
		sof_cnt, ctx->ref_sof_cnt, ctx->fast_mode_on);
	if (ctx->fast_mode_on && (sof_cnt > ctx->ref_sof_cnt)) {
		ctx->fast_mode_on = FALSE;
		ctx->ref_sof_cnt = 0;
		DRV_LOG(ctx, "seamless_switch disabled.");
		set_i2c_buffer(ctx, 0x3010, 0x00);
		commit_i2c_buffer(ctx);
	}
	return 0;
}
