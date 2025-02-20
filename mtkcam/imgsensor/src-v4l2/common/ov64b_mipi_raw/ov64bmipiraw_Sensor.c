// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 ov64bmipiraw_Sensor.c
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
#include "ov64bmipiraw_Sensor.h"

static void set_sensor_cali(void *arg);
static int get_sensor_temperature(void *arg);
static void set_group_hold(void *arg, u8 en);
static void ov64b_set_dummy(struct subdrv_ctx *ctx);
static int ov64b_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static u16 get_gain2reg(u32 gain);
static int ov64b_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int  ov64b_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, ov64b_set_test_pattern},
	{SENSOR_FEATURE_SEAMLESS_SWITCH, ov64b_seamless_switch},
	{SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO, ov64b_set_max_framerate_by_scenario},
};

static struct eeprom_info_struct eeprom_info[] = {
	{
		.header_id = 0x010B00FF,
		.addr_header_id = 0x0000000B,
		.i2c_write_id = 0xAA,

		.pdc_support = TRUE,
		.pdc_size = 720,
		.addr_pdc = 0x1A46,
		.sensor_reg_addr_pdc = 0x5F80,

		.xtalk_support = TRUE,
		.xtalk_size = 288,
		.addr_xtalk = 0x1D31,
		.sensor_reg_addr_xtalk = 0x5A40,
	},
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 40,
	.i4OffsetY = 14,
	.i4PitchX = 16,
	.i4PitchY = 16,
	.i4PairNum = 8,
	.i4SubBlkW = 8,
	.i4SubBlkH = 4,
	.i4PosL = {{47, 16}, {55, 16}, {43, 20}, {51, 20},
	           {47, 24}, {55, 24}, {43, 28}, {51, 28}},
	.i4PosR = {{46, 16}, {54, 16}, {42, 20}, {50, 20},
	           {46, 24}, {54, 24}, {42, 28}, {50, 28}},
	.i4BlockNumX = 284,
	.i4BlockNumY = 214,
	.i4LeFirst = 0,
	.i4Crop = {
		// <prev> <cap> <vid> <hs_vid> <slim_vid>
		{0, 0}, {0, 0}, {0, 434}, {0, 434}, {0, 434},
		// <cust1> <cust2> <cust3> <cust4> <cust5>
		{0, 0}, {0, 434}, {0, 0}, {0, 0}, {0, 0},
		// <cust6> <cust7>
		{0, 0}, {0, 0},
	},
	.iMirrorFlip = 0,

	.i4FullRawW = 4624,
	.i4FullRawH = 3472,
	.sPDMapInfo[0] = {
		.i4PDPattern = 2,
		.i4PDRepetition = 4,
		.i4PDOrder = {1}, // R = 1, L = 0
	},
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_fullsize = {
	.i4OffsetX = 32,
	.i4OffsetY = 32,
	.i4PitchX = 16,
	.i4PitchY = 16,
	.i4PairNum = 2,
	.i4SubBlkW = 16,
	.i4SubBlkH = 8,
	.i4PosL = {{46, 37}, {39, 45}},
	.i4PosR = {{45, 37}, {38, 45}},
	.i4BlockNumX = 568,
	.i4BlockNumY = 430,
	.i4LeFirst = 0,
	.i4Crop = {
		// <prev> <cap> <vid> <hs_vid> <slim_vid>
		{0, 0}, {0, 0}, {0, 434}, {0, 434}, {0, 434},
		// <cust1> <cust2> <cust3> <cust4> <cust5>
		{0, 0}, {0, 434}, {0, 0}, {0, 0}, {0, 0},
		// <cust6> <cust7>
		{0, 0}, {0, 0},
	},
	.iMirrorFlip = 0,

	.i4FullRawW = 9248,
	.i4FullRawH = 6944,
	.sPDMapInfo[0] = {
		.i4PDPattern = 2,
		.i4PDRepetition = 1,
		.i4PDOrder = {1}, // R = 1, L = 0
	},
};

// mode 0: 4624*3472@30fps, normal preview + PD 1136 x 860
static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0d90,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0470,
			.vsize = 0x035c,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

// mode 1: same as preview mode + PD 1136 x 860
static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0d90,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0470,
			.vsize = 0x035c,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

// mode 2: 4624*2604@30fps, noraml video + PD 1136 x 648
static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0a2c,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0470,
			.vsize = 0x0288,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

// mode 3: 4624*2604@60fps, m-stream, non-pd
static struct mtk_mbus_frame_desc_entry frame_desc_hs_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0a2c,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};

// mode 4: 4624*2604@30fps, stagger 2exp, non-pd
static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0a2c,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0a2c,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
};

// mode 5: 4624*3472@30fps, stagger 2exp, non-pd
static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0d90,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0d90,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
};

// mode 6: 4624*2604@25fps, stagger 2exp + PD 1136 x 648
static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0a2c,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0a2c,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0470,
			.vsize = 0x0288,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

// mode 7: 4624*3472@20fps, stagger 2exp + PD 1136 x 860
static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0d90,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0d90,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0470,
			.vsize = 0x035c,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

// mode 8: 4624*3472@30fps, full size + center crop + PD 576 x 868
static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0d90,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0240,
			.vsize = 0x0364,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

// mode 9: 4624*3472@30fps, 4-cell pattern, full size + center crop + PD 576 x 868
static struct mtk_mbus_frame_desc_entry frame_desc_cus5[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1210,
			.vsize = 0x0d90,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0240,
			.vsize = 0x0364,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

// mode 10: 9248*6944@15fps, full size + PD 1136 x 1720
static struct mtk_mbus_frame_desc_entry frame_desc_cus6[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x2420,
			.vsize = 0x1b20,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0470,
			.vsize = 0x06b8,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

// mode 11: 9248*6944@15fps, 4-cell pattern, full size + PD 1136 x 1720
static struct mtk_mbus_frame_desc_entry frame_desc_cus7[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x2420,
			.vsize = 0x1b20,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0470,
			.vsize = 0x06b8,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

static struct subdrv_mode_struct mode_struct[] = {
	// mode 0: 4624*3472@30fps, normal preview + pd
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = ov64b_preview_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_preview_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = ov64b_seamless_normal_preview,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(ov64b_seamless_normal_preview),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 115200000,
		.linelength = 936,
		.framelength = 4108,
		.max_framerate = 300,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 0,
		.read_margin = 10,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 9248,
			.h0_size = 6944,
			.scale_w = 4624,
			.scale_h = 3472,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4624,
			.h1_size = 3472,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4624,
			.h2_tg_size = 3472,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1467,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.dpc_enabled = TRUE,
		.pdc_enabled = TRUE,
	},
	// mode 1: same as preview mode
	{
		.frame_desc = frame_desc_cap,
		.num_entries = ARRAY_SIZE(frame_desc_cap),
		.mode_setting_table = ov64b_capture_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_capture_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = ov64b_seamless_normal_preview,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(ov64b_seamless_normal_preview),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 115200000,
		.linelength = 936,
		.framelength = 4108,
		.max_framerate = 300,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 0,
		.read_margin = 10,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 9248,
			.h0_size = 6944,
			.scale_w = 4624,
			.scale_h = 3472,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4624,
			.h1_size = 3472,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4624,
			.h2_tg_size = 3472,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1467,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = TRUE,
		.pdc_enabled = TRUE,
	},
	// mode 2: 4624*2604@30fps, noraml video + pd
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = ov64b_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_normal_video_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = ov64b_seamless_normal_video,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(ov64b_seamless_normal_video),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 115200000,
		.linelength = 936,
		.framelength = 4108,
		.max_framerate = 300,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 0,
		.read_margin = 10,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 9248,
			.h0_size = 6944,
			.scale_w = 4624,
			.scale_h = 3472,
			.x1_offset = 0,
			.y1_offset = 434,
			.w1_size = 4624,
			.h1_size = 2604,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4624,
			.h2_tg_size = 2604,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1467,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = TRUE,
		.pdc_enabled = TRUE,
	},
	// mode 3: 4624*2604@60fps, m-stream
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = ov64b_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 115200000,
		.linelength = 528,
		.framelength = 3660,
		.max_framerate = 600,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 0,
		.read_margin = 10,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 9248,
			.h0_size = 6944,
			.scale_w = 4624,
			.scale_h = 3472,
			.x1_offset = 0,
			.y1_offset = 434,
			.w1_size = 4624,
			.h1_size = 2604,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4624,
			.h2_tg_size = 2604,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 2940,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = TRUE,
		.pdc_enabled = FALSE,
	},
	// mode 4: 4624*2604@30fps, stagger 2exp, non-pd
	{
		.frame_desc = frame_desc_slim_vid,
		.num_entries = ARRAY_SIZE(frame_desc_slim_vid),
		.mode_setting_table = ov64b_slim_video_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_slim_video_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = ov64b_seamless_slim_video,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(ov64b_seamless_slim_video),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 115200000,
		.linelength = 528,
		.framelength = 3632*2,
		.max_framerate = 300,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 2604*2,
		.read_margin = 48*2,
		.coarse_integ_step = 4,
		.framelength_step = 4*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].max = 2550 *2,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 9248,
			.h0_size = 6944,
			.scale_w = 4624,
			.scale_h = 3472,
			.x1_offset = 0,
			.y1_offset = 434,
			.w1_size = 4624,
			.h1_size = 2604,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4624,
			.h2_tg_size = 2604,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 2940,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = TRUE,
		.pdc_enabled = FALSE,
	},
	// mode 5: 4624*3472@30fps, stagger 2exp, non-pd
	{
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = ov64b_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_custom1_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = ov64b_seamless_custom1,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(ov64b_seamless_custom1),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 115200000,
		.linelength = 528,
		.framelength = 3660*2,
		.max_framerate = 300,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 3472*2,
		.read_margin = 48*2,
		.coarse_integ_step = 4,
		.framelength_step = 4*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].max = 3328 *2,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 9248,
			.h0_size = 6944,
			.scale_w = 4624,
			.scale_h = 3472,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4624,
			.h1_size = 3472,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4624,
			.h2_tg_size = 3472,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 2940,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = TRUE,
		.pdc_enabled = FALSE,
	},
	// mode 6: 4624*2604@25fps, stagger 2exp + pd
	{
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = ov64b_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_custom2_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = ov64b_seamless_custom2,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(ov64b_seamless_custom2),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 115200000,
		.linelength = 912,
		.framelength = 2744*2,
		.max_framerate = 230,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 2604*2,
		.read_margin = 48*2,
		.coarse_integ_step = 4,
		.framelength_step = 4*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].max = 2550 *2,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 9248,
			.h0_size = 6944,
			.scale_w = 4624,
			.scale_h = 3472,
			.x1_offset = 0,
			.y1_offset = 434,
			.w1_size = 4624,
			.h1_size = 2604,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4624,
			.h2_tg_size = 2604,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1467,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = TRUE,
		.pdc_enabled = TRUE,
	},
	// mode 7: 4624*3472@20fps, stagger 2exp + pd
	{
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = ov64b_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_custom3_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = ov64b_seamless_custom3,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(ov64b_seamless_custom3),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 115200000,
		.linelength = 888,
		.framelength = 3600*2,
		.max_framerate = 180,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 3472*2,
		.read_margin = 48*2,
		.coarse_integ_step = 4,
		.framelength_step = 4*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].max = 3328 *2,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 9248,
			.h0_size = 6944,
			.scale_w = 4624,
			.scale_h = 3472,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4624,
			.h1_size = 3472,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4624,
			.h2_tg_size = 3472,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1467,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = TRUE,
		.pdc_enabled = TRUE,
	},
	// mode 8: 4624*3472@30fps, full size + center crop + pd
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = ov64b_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_custom4_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = ov64b_seamless_custom4,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(ov64b_seamless_custom4),
		.hdr_mode = HDR_NONE,
		.pclk = 115200000,
		.linelength = 1008,
		.framelength = 3808,
		.max_framerate = 300,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 0,
		.read_margin = 10,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 2314,
			.y0_offset = 1736,
			.w0_size = 4624,
			.h0_size = 3472,
			.scale_w = 4624,
			.scale_h = 3472,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4624,
			.h1_size = 3472,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4624,
			.h2_tg_size = 3472,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = TRUE,
		.pdc_enabled = TRUE,
	},
	// mode 9: 4624*3472@30fps, 4-cell pattern, full size + center crop + pd
	{
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = ov64b_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_custom5_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = ov64b_seamless_custom5,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(ov64b_seamless_custom5),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 115200000,
		.linelength = 1008,
		.framelength = 3808,
		.max_framerate = 300,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 0,
		.read_margin = 10,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 2314,
			.y0_offset = 1736,
			.w0_size = 4624,
			.h0_size = 3472,
			.scale_w = 4624,
			.scale_h = 3472,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4624,
			.h1_size = 3472,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4624,
			.h2_tg_size = 3472,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_B,
		.dpc_enabled = TRUE,
		.pdc_enabled = TRUE,
	},
	// mode 10: 9248*6944@15fps, full size + pd
	{
		.frame_desc = frame_desc_cus6,
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.mode_setting_table = ov64b_custom6_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_custom6_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = ov64b_seamless_custom6,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(ov64b_seamless_custom6),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 115200000,
		.linelength = 1008,
		.framelength = 7616,
		.max_framerate = 150,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 0,
		.read_margin = 10,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 9248,
			.h0_size = 6944,
			.scale_w = 9248,
			.scale_h = 6944,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 9248,
			.h1_size = 6944,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 9248,
			.h2_tg_size = 6944,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info_fullsize,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = TRUE,
		.pdc_enabled = TRUE,
	},
	// mode 11: 9248*6944@15fps, 4-cell pattern, full size + pd
	{
		.frame_desc = frame_desc_cus7,
		.num_entries = ARRAY_SIZE(frame_desc_cus7),
		.mode_setting_table = ov64b_custom7_setting,
		.mode_setting_len = ARRAY_SIZE(ov64b_custom7_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = ov64b_seamless_custom7,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(ov64b_seamless_custom7),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 115200000,
		.linelength = 1008,
		.framelength = 7616,
		.max_framerate = 150,
		.mipi_pixel_rate = 1200000000,
		.readout_length = 0,
		.read_margin = 10,
		.imgsensor_winsize_info = {
			.full_w = 9248,
			.full_h = 6944,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 9248,
			.h0_size = 6944,
			.scale_w = 9248,
			.scale_h = 6944,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 9248,
			.h1_size = 6944,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 9248,
			.h2_tg_size = 6944,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info_fullsize,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_B,
		.dpc_enabled = TRUE,
		.pdc_enabled = TRUE,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = OV64B_SENSOR_ID,
	.reg_addr_sensor_id = {0x300A, 0x300B, 0x300C},
	.i2c_addr_table = {0x44, 0xFF}, // TBD
	// .i2c_addr_table = {0x48, 0xFF}, // disable 64b
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {9248, 6944},
	.mirror = IMAGE_NORMAL, // TBD

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_CSI2,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 15.5,
	.ana_gain_type = 1,
	.ana_gain_step = 4,
	.ana_gain_table = ov64b_ana_gain_table,
	.ana_gain_table_size = sizeof(ov64b_ana_gain_table),
	.min_gain_iso = 100,
	.exposure_def = 0x3D0,
	.exposure_min = 16,
	.exposure_max = 0xFFFFFF - 36,
	.exposure_step = 2,
	.exposure_margin = 36,

	.frame_length_max = 0xFFFFFF,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 0,

	.pdaf_type = PDAF_SUPPORT_CAMSV,
	.hdr_type = HDR_SUPPORT_STAGGER_FDOL,
	.seamless_switch_support = TRUE,
	.temperature_support = TRUE,

	.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,
	.s_cali = set_sensor_cali,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = PARAM_UNDEFINED, // TBD
	.reg_addr_exposure = {{0x3500, 0x3501, 0x3502},
				{0x3580, 0x3581, 0x3582},
				{0x3540, 0x3541, 0x3542}},
	.long_exposure_support = FALSE,
	.reg_addr_exposure_lshift = PARAM_UNDEFINED,
	.reg_addr_ana_gain = {{0x3508, 0x3509}, {0x3588, 0x3589}, {0x3548, 0x3549}},
	.reg_addr_frame_length = {0x3840, 0x380E, 0x380F},
	.reg_addr_temp_en = 0x4D12,
	.reg_addr_temp_read = 0x4D13,
	.reg_addr_auto_extend = PARAM_UNDEFINED,
	.reg_addr_frame_count = 0x387f, // To be verified

	.init_setting_table = ov64b_init_setting,
	.init_setting_len = ARRAY_SIZE(ov64b_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 0,
	.chk_s_off_end = 0,

	.checksum_value = 0x37E5E8C5,
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
	.update_sof_cnt = common_update_sof_cnt,
	.vsync_notify = vsync_notify,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_MCLK, 24, 0},
	{HW_ID_RST, 0, 0},
	{HW_ID_AVDD, 2800000, 0}, // pmic_ldo for avdd
	{HW_ID_AFVDD, 3100000, 0}, // pmic_ldo for afvdd
	{HW_ID_AFVDD1, 1800000, 0}, // pmic_gpo(3.1V ldo) for afvdd
	{HW_ID_DVDD, 1100000, 0}, // pmic_ldo/gpio(1.1V ldo) for dvdd
	{HW_ID_DOVDD, 1800000, 1}, // pmic_ldo/gpio(1.8V ldo) for dovdd
	{HW_ID_OISVDD, 3100000, 0}, // pmic_ldo for oisvdd
	{HW_ID_OISEN, 3100000, 0}, // gpio for oisen
	{HW_ID_MCLK_DRIVING_CURRENT, 8, 1},
	{HW_ID_RST, 1, 1}
};

const struct subdrv_entry ov64b_mipi_raw_entry = {
	.name = "ov64b_mipi_raw",
	.id = OV64B_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

/* FUNCTION */

static void set_sensor_cali(void *arg)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	u16 idx = 0;
	u8 support = FALSE;
	u8 *pbuf = NULL;
	u16 size = 0;
	u16 addr = 0;
	struct eeprom_info_struct *info = ctx->s_ctx.eeprom_info;

	if (!probe_eeprom(ctx))
		return;

	idx = ctx->eeprom_index;

	/* PDC data */
	support = info[idx].pdc_support;
	if (support) {
		pbuf = info[idx].preload_pdc_table;
		if (pbuf != NULL) {
			size = 720;
			addr = 0x5F80;
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			DRV_LOG(ctx, "set PDC calibration data done.");
		}
	}

	/* xtalk data */
	support = info[idx].xtalk_support;
	if (support) {
		pbuf = info[idx].preload_xtalk_table;
		if (pbuf != NULL) {
			size = 288;
			addr = 0x5A40;
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			DRV_LOG(ctx, "set xtalk calibration data done.");
		}
	}
}

static int get_sensor_temperature(void *arg)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	int temperature = 0;

	/*TEMP_SEN_CTL */
	subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_temp_en, 0x01);
	temperature = subdrv_i2c_rd_u8(ctx, ctx->s_ctx.reg_addr_temp_read);
	temperature = (temperature > 0xC0) ? (temperature - 0x100) : temperature;

	DRV_LOG(ctx, "temperature: %d degrees\n", temperature);
	return temperature;
}

static void set_group_hold(void *arg, u8 en)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	if (en) {
		set_i2c_buffer(ctx, 0x3208, 0x00);
	} else {
		set_i2c_buffer(ctx, 0x3208, 0x10);
		set_i2c_buffer(ctx, 0x3208, 0xA0);
	}
}

static void ov64b_set_dummy(struct subdrv_ctx *ctx)
{
	// bool gph = !ctx->is_seamless && (ctx->s_ctx.s_gph != NULL);

	// if (gph)
	// ctx->s_ctx.s_gph((void *)ctx, 1);
	write_frame_length(ctx, ctx->frame_length);
	// if (gph)
	// ctx->s_ctx.s_gph((void *)ctx, 0);

	commit_i2c_buffer(ctx);
}

static int ov64b_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *)para;
	enum SENSOR_SCENARIO_ID_ENUM scenario_id = (enum SENSOR_SCENARIO_ID_ENUM)*feature_data;
	u32 framerate = *(feature_data + 1);
	u32 frame_length, calc_fl, exp_cnt, i;

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
	exp_cnt = ctx->s_ctx.mode[scenario_id].exp_cnt;
	calc_fl = ctx->exposure[0];
	for (i = 1; i < exp_cnt; i++)
		calc_fl += ctx->exposure[i];
	calc_fl += ctx->s_ctx.exposure_margin*exp_cnt*exp_cnt;

	frame_length = ctx->s_ctx.mode[scenario_id].pclk / framerate * 10
		/ ctx->s_ctx.mode[scenario_id].linelength;
	ctx->frame_length =
		max(frame_length, ctx->s_ctx.mode[scenario_id].framelength);
	ctx->frame_length = min(ctx->frame_length, ctx->s_ctx.frame_length_max);
	ctx->current_fps = ctx->pclk / ctx->frame_length * 10 / ctx->line_length;
	ctx->min_frame_length = ctx->frame_length;
	DRV_LOG(ctx, "max_fps(input/output):%u/%u(sid:%u), frame_length:%u, calc_fl:%u, min_fl_en:1\n",
		framerate, ctx->current_fps, scenario_id, ctx->frame_length, calc_fl);
	if (ctx->frame_length > calc_fl)
		ov64b_set_dummy(ctx);
	else
		ctx->frame_length = calc_fl;

	return ERROR_NONE;
}

static u16 get_gain2reg(u32 gain)
{
	return gain * 256 / BASEGAIN;
}

static int ov64b_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	enum SENSOR_SCENARIO_ID_ENUM scenario_id;
	enum IMGSENSOR_HDR_MODE_ENUM scen1_hdr, scen2_hdr;
	struct mtk_hdr_ae *ae_ctrl = NULL;
	u64 *feature_data = (u64 *)para;
	u32 exp_cnt = 0;

	if (feature_data == NULL) {
		DRV_LOGE(ctx, "input scenario is null!");
		return ERROR_NONE;
	}
	scenario_id = *feature_data;
	if ((feature_data + 1) != NULL)
		ae_ctrl = (struct mtk_hdr_ae *)((uintptr_t)(*(feature_data + 1)));
	else
		DRV_LOGE(ctx, "no ae_ctrl input");

	check_current_scenario_id_bound(ctx);
	DRV_LOG(ctx, "E: set seamless switch %u %u\n", ctx->current_scenario_id, scenario_id);
	if (!ctx->extend_frame_length_en)
		DRV_LOGE(ctx, "please extend_frame_length before seamless_switch!\n");
	ctx->extend_frame_length_en = FALSE;

	if (scenario_id >= ctx->s_ctx.sensor_mode_num) {
		DRV_LOGE(ctx, "invalid sid:%u, mode_num:%u\n",
			scenario_id, ctx->s_ctx.sensor_mode_num);
		return ERROR_NONE;
	}
	if (ctx->s_ctx.mode[scenario_id].seamless_switch_group == 0 ||
		ctx->s_ctx.mode[scenario_id].seamless_switch_group !=
			ctx->s_ctx.mode[ctx->current_scenario_id].seamless_switch_group) {
		DRV_LOGE(ctx, "seamless_switch not supported\n");
		return ERROR_NONE;
	}
	if (ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table == NULL) {
		DRV_LOGE(ctx, "Please implement seamless_switch setting\n");
		return ERROR_NONE;
	}

	scen1_hdr = ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode;
	scen2_hdr = ctx->s_ctx.mode[scenario_id].hdr_mode;
	exp_cnt = ctx->s_ctx.mode[scenario_id].exp_cnt;
	ctx->is_seamless = TRUE;
	update_mode_info(ctx, scenario_id);

	i2c_table_write(ctx, addr_data_pair_seamless_switch_step1_ov64b,
		ARRAY_SIZE(addr_data_pair_seamless_switch_step1_ov64b));
	i2c_table_write(ctx,
		ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table,
		ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_len);

	if (ae_ctrl) {
		switch (ctx->s_ctx.mode[scenario_id].hdr_mode) {
		case HDR_RAW_STAGGER:
			set_multi_shutter_frame_length(ctx, (u64 *)&ae_ctrl->exposure, exp_cnt, 0);
			set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			break;
		default:
			set_shutter(ctx, ae_ctrl->exposure.le_exposure);
			set_gain(ctx, ae_ctrl->gain.le_gain);
			break;
		}
	}
	i2c_table_write(ctx, addr_data_pair_seamless_switch_step2_ov64b,
		ARRAY_SIZE(addr_data_pair_seamless_switch_step2_ov64b));
	// if (ae_ctrl) {
	// 	switch (ctx->s_ctx.mode[scenario_id].hdr_mode) {
	// 	case HDR_RAW_STAGGER:
	// 		set_multi_shutter_frame_length(ctx, (u64 *)&ae_ctrl->exposure, exp_cnt, 0);
	// 		set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
	// 		break;
	// 	default:
	// 		set_shutter(ctx, ae_ctrl->exposure.le_exposure);
	// 		set_gain(ctx, ae_ctrl->gain.le_gain);
	// 		break;
	// 	}
	// }
	if (scen1_hdr == HDR_RAW_STAGGER) {
		i2c_table_write(ctx, addr_data_pair_seamless_switch_step3_HDR_ov64b,
			ARRAY_SIZE(addr_data_pair_seamless_switch_step3_HDR_ov64b));
			DRV_LOG(ctx, "do hdr to linear mode\n");
	} else {
		i2c_table_write(ctx, addr_data_pair_seamless_switch_step3_ov64b,
			ARRAY_SIZE(addr_data_pair_seamless_switch_step3_ov64b));
			DRV_LOG(ctx, "do linear to hdr/linear mode\n");
	}
	ctx->is_seamless = FALSE;
	ctx->ref_sof_cnt = ctx->sof_cnt;
	ctx->fast_mode_on = TRUE;
	DRV_LOG(ctx, "X: set seamless switch done\n");
	return ERROR_NONE;
}

static int ov64b_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOGE(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	switch (mode) {
	case 2:
		subdrv_i2c_wr_u8(ctx, 0x50c1, 0x01);
		break;
	case 5:
		subdrv_i2c_wr_u8(ctx, 0x350a, 0x00);
		subdrv_i2c_wr_u8(ctx, 0x401a, 0x00);
		break;
	default:
		break;
	}

	if (mode != ctx->test_pattern)
		switch (ctx->test_pattern) {
		case 2:
			subdrv_i2c_wr_u8(ctx, 0x50c1, 0x00);
			break;
		case 5:
			subdrv_i2c_wr_u8(ctx, 0x350a, 0x01);
			subdrv_i2c_wr_u8(ctx, 0x401a, 0x40);
			break;
		default:
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

static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt)
{
	DRV_LOG(ctx, "sof_cnt(%u) ctx->ref_sof_cnt(%u) ctx->fast_mode_on(%d)",
		sof_cnt, ctx->ref_sof_cnt, ctx->fast_mode_on);
	if (ctx->fast_mode_on && (sof_cnt > ctx->ref_sof_cnt)) {
		ctx->fast_mode_on = FALSE;
		ctx->ref_sof_cnt = 0;
		DRV_LOG(ctx, "seamless_switch disabled.");
	}
	return 0;
}
