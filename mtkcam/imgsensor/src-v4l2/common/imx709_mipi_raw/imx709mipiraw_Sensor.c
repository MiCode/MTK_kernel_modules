// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *     IMX709mipiraw_Sensor.c
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include "imx709mipiraw_Sensor.h"


#define DEBUG_LOG_EN 0
#define PFX "IMX709_camera_sensor"
#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_DEBUG(...) do { if ((DEBUG_LOG_EN)) LOG_INF(__VA_ARGS__); } while (0)

static const char * const clk_names[] = {
	ADAPTOR_CLK_NAMES
};

static const char * const reg_names[] = {
	ADAPTOR_REGULATOR_NAMES
};

static const char * const state_names[] = {
	ADAPTOR_STATE_NAMES
};

static int init_ctx(
	struct subdrv_ctx *ctx, struct i2c_client *i2c_client, u8 i2c_write_id);
static int imx709_open(struct subdrv_ctx *ctx);
static int imx709_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);
static int get_csi_param(
	struct subdrv_ctx *ctx,
	enum SENSOR_SCENARIO_ID_ENUM scenario_id,
	struct mtk_csi_param *csi_param);
static int get_sensor_temperature(void *arg);
static u16 get_gain2reg(u32 gain);
static void set_group_hold(void *arg, u8 en);
#if EEPROM_READY
static void set_sensor_cali(void *arg);
#endif
static void set_data_rate_global_timing_phy_ctrl(void *arg);
static int set_pwr_seq_reset_view_to_sensing(void *arg);
static int set_streaming_control(void *arg, bool enable);
static int imx709_check_sensor_id(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int imx709_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int imx709_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{
		.feature_id = SENSOR_FEATURE_CHECK_SENSOR_ID,
		.func = imx709_check_sensor_id
	},
	{
		.feature_id = SENSOR_FEATURE_SEAMLESS_SWITCH,
		.func = imx709_seamless_switch
	},
	{
		.feature_id = SENSOR_FEATURE_SET_TEST_PATTERN,
		.func = imx709_set_test_pattern
	},
};

static struct eeprom_info_struct eeprom_info[] = {
#if EEPROM_READY
	{
		.header_id = 0x0059000B,
		.addr_header_id = 0x00000006,
		.i2c_write_id = 0xA8,

		.qsc_support = TRUE,
		.qsc_size = 1560,
		.addr_qsc = 0x1900,
		.sensor_reg_addr_qsc = 0xC800, // useless
	},
	{
		.header_id = 0x0059000B,
		.addr_header_id = 0x00000006,
		.i2c_write_id = 0xA8,

		.lrc_support = TRUE,
		.lrc_size = 260,
		.addr_lrc = 0x1F20,
		.sensor_reg_addr_qsc = 0xC800, // useless
	},
#else
	{0},
#endif
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 16,
	.i4OffsetY = 16,
	.i4PitchX = 16,
	.i4PitchY = 16,
	.i4PairNum = 16,
	.i4SubBlkW = 8,
	.i4SubBlkH = 2,
	.i4PosL = {
		{18, 17}, {26, 17}, {16, 19}, {24, 19},
		{20, 21}, {28, 21}, {22, 23}, {30, 23},
		{18, 25}, {26, 25}, {16, 27}, {24, 27},
		{20, 29}, {28, 29}, {22, 31}, {30, 31}
	},
	.i4PosR = {
		{19, 17}, {27, 17}, {17, 19}, {25, 19},
		{21, 21}, {29, 21}, {23, 23}, {31, 23},
		{19, 25}, {27, 25}, {17, 27}, {25, 27},
		{21, 29}, {29, 29}, {23, 31}, {31, 31}
	},
	.i4BlockNumX = 202,
	.i4BlockNumY = 152,
	.iMirrorFlip = 3,
	.i4Crop = {
		{8, 8}, {8, 8}, {8, 304}, {0, 0}, {0, 0},
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {824, 620}
	},
	.i4FullRawW = 3280,
	.i4FullRawH = 2464,
	.i4VCPackNum = 1,
	.i4ModeIndex = 0,
	.sPDMapInfo[0] = {
		.i4VCFeature = VC_PDAF_STATS,
		.i4PDPattern = 2,
		.i4PDRepetition = 4,
		.i4PDOrder = {1},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 2448,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 2448,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 1856,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_hs[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 1856,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_slim[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 1856,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 640,
			.vsize = 480,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 480,
			.vsize = 320,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 320,
			.vsize = 240,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 640,
			.vsize = 480,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus5[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 640,
			.vsize = 480,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus6[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 480,
			.vsize = 320,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus7[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 480,
			.vsize = 320,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus8[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 320,
			.vsize = 240,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus9[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 320,
			.vsize = 240,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus10[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 640,
			.vsize = 480,
			.user_data_desc = VC_STAGGER_NE,
		},
	}
};

static int stream_refcnt_for_aov;

static struct subdrv_mode_struct mode_struct[] = {
	{	/* reg_B3_1 viewing 3264x2448@30fps */
		.mode_setting_table = imx709_preview_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_preview_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 863200000,
		.linelength = 7400,
		.framelength = 3888,
		.max_framerate = 300,
		.mipi_pixel_rate = 420400000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 16,
			.w0_size = 6560,
			.h0_size = 4896,
			.scale_w = 6560,
			.scale_h = 2448,
			.x1_offset = 8,
			.y1_offset = 0,
			.w1_size = 6544,
			.h1_size = 2448,
			.x2_tg_offset = 1640,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 2448,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x13,
			.dphy_clk_settle = 0x13,
			.dphy_trail = 0x34,
			.dphy_csi2_resync_dmy_cycle = 0xF,
		},
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
	},
	{	/* reg_B3_1 viewing 3264x2448@30fps */
		.mode_setting_table = imx709_capture_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_capture_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 863200000,
		.linelength = 7400,
		.framelength = 3888,
		.max_framerate = 300,
		.mipi_pixel_rate = 420400000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 16,
			.w0_size = 6560,
			.h0_size = 4896,
			.scale_w = 6560,
			.scale_h = 2448,
			.x1_offset = 8,
			.y1_offset = 0,
			.w1_size = 6544,
			.h1_size = 2448,
			.x2_tg_offset = 1640,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 2448,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x13,
			.dphy_clk_settle = 0x13,
			.dphy_trail = 0x34,
			.dphy_csi2_resync_dmy_cycle = 0xF,
		},
		.frame_desc = frame_desc_cap,
		.num_entries = ARRAY_SIZE(frame_desc_cap),
	},
	{	/* reg_C5 viewing 3264x1856@30fps */
		.mode_setting_table = imx709_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_normal_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 863200000,
		.linelength = 7400,
		.framelength = 3888,
		.max_framerate = 300,
		.mipi_pixel_rate = 440800000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 608,
			.w0_size = 6560,
			.h0_size = 3712,
			.scale_w = 6560,
			.scale_h = 1856,
			.x1_offset = 8,
			.y1_offset = 0,
			.w1_size = 6544,
			.h1_size = 1856,
			.x2_tg_offset = 1640,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 1856,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x12,
			.dphy_clk_settle = 0x12,
			.dphy_trail = 0x31,
			.dphy_csi2_resync_dmy_cycle = 0xF,
		},
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
	},
	{	/* reg_G-2 viewing 3264x1856@60fps */
		.mode_setting_table = imx709_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 849600000,
		.linelength = 7400,
		.framelength = 1920,
		.max_framerate = 600,
		.mipi_pixel_rate = 415200000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 608,
			.w0_size = 6560,
			.h0_size = 3712,
			.scale_w = 6560,
			.scale_h = 1856,
			.x1_offset = 8,
			.y1_offset = 0,
			.w1_size = 6544,
			.h1_size = 1856,
			.x2_tg_offset = 1640,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 1856,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x13,
			.dphy_clk_settle = 0x13,
			.dphy_trail = 0x35,
			.dphy_csi2_resync_dmy_cycle = 0xF,
		},
		.frame_desc = frame_desc_hs,
		.num_entries = ARRAY_SIZE(frame_desc_hs),
	},
	{	/* reg_G-2 viewing 3264x1856@60fps */
		.mode_setting_table = imx709_slim_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_slim_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 849600000,
		.linelength = 7400,
		.framelength = 1920,
		.max_framerate = 600,
		.mipi_pixel_rate = 415200000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 608,
			.w0_size = 6560,
			.h0_size = 3712,
			.scale_w = 6560,
			.scale_h = 1856,
			.x1_offset = 8,
			.y1_offset = 0,
			.w1_size = 6544,
			.h1_size = 1856,
			.x2_tg_offset = 1640,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 1856,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x13,
			.dphy_clk_settle = 0x13,
			.dphy_trail = 0x35,
			.dphy_csi2_resync_dmy_cycle = 0xF,
		},
		.frame_desc = frame_desc_slim,
		.num_entries = ARRAY_SIZE(frame_desc_slim),
	},
	{	/* reg_H2 sensing 640x480@10FPS */
		.mode_setting_table = imx709_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_custom1_setting),
		.mode_setting_table_for_md = imx709_md1_setting,
		.mode_setting_len_for_md = ARRAY_SIZE(imx709_md1_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 167700000,
		.linelength = 936,
		.framelength = 17912,
		.max_framerate = 100,
		.mipi_pixel_rate = 205500000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 544,
			.w0_size = 6560,
			.h0_size = 3840,
			.scale_w = 820,
			.scale_h = 480,
			.x1_offset = 90,
			.y1_offset = 0,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},
		.aov_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x13,
			.dphy_clk_settle = 0x13,
			.dphy_trail = 0x82,
			.dphy_csi2_resync_dmy_cycle = 0x26,
		},
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,
	},
	{	/* reg_I3 sensing 480x320@10FPS */
		.mode_setting_table = imx709_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_custom2_setting),
		.mode_setting_table_for_md = imx709_md2_setting,
		.mode_setting_len_for_md = ARRAY_SIZE(imx709_md2_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 215800000,
		.linelength = 936,
		.framelength = 23048,
		.max_framerate = 100,
		.mipi_pixel_rate = 224000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 1184,
			.w0_size = 6560,
			.h0_size = 2560,
			.scale_w = 820,
			.scale_h = 320,
			.x1_offset = 170,
			.y1_offset = 0,
			.w1_size = 480,
			.h1_size = 320,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 480,
			.h2_tg_size = 320,
		},
		.aov_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x12,
			.dphy_clk_settle = 0x12,
			.dphy_trail = 0x75,
			.dphy_csi2_resync_dmy_cycle = 0x23,
		},
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,
	},
	{	/* reg_K2 sensing 320x240@10FPS */
		.mode_setting_table = imx709_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_custom3_setting),
		.mode_setting_table_for_md = imx709_md3_setting,
		.mode_setting_len_for_md = ARRAY_SIZE(imx709_md3_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 431600000,
		.linelength = 1674,
		.framelength = 25776,
		.max_framerate = 100,
		.mipi_pixel_rate = 213000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 544,
			.w0_size = 6560,
			.h0_size = 3840,
			.scale_w = 410,
			.scale_h = 240,
			.x1_offset = 45,
			.y1_offset = 0,
			.w1_size = 320,
			.h1_size = 240,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 320,
			.h2_tg_size = 240,
		},
		.aov_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x13,
			.dphy_clk_settle = 0x13,
			.dphy_trail = 0x7F,
			.dphy_csi2_resync_dmy_cycle = 0x25,
		},
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,
	},
	{	/* reg_H3 sensing 640x480@5FPS */
		.mode_setting_table = imx709_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_custom4_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 105300000,
		.linelength = 936,
		.framelength = 22496,
		.max_framerate = 50,
		.mipi_pixel_rate = 150000000,
		.readout_length = 2374*2,
		.read_margin = 10*2,
		.framelength_step = 4*2,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 544,
			.w0_size = 6560,
			.h0_size = 3840,
			.scale_w = 820,
			.scale_h = 480,
			.x1_offset = 90,
			.y1_offset = 0,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},
		.aov_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x14,
			.dphy_clk_settle = 0x14,
			.dphy_trail = 0xB3,
			.dphy_csi2_resync_dmy_cycle = 0x34,
		},
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,
	},
	{	/* reg_H4 sensing 640x480@2FPS */
		.mode_setting_table = imx709_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_custom5_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 105300000,
		.linelength = 936,
		.framelength = 56248,
		.max_framerate = 20,
		.mipi_pixel_rate = 150000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 544,
			.w0_size = 6560,
			.h0_size = 3840,
			.scale_w = 820,
			.scale_h = 480,
			.x1_offset = 90,
			.y1_offset = 0,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},
		.aov_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x14,
			.dphy_clk_settle = 0x14,
			.dphy_trail = 0xB3,
			.dphy_csi2_resync_dmy_cycle = 0x34,
		},
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,
	},
	{	/* reg_I4 sensing 480x320@5FPS */
		.mode_setting_table = imx709_custom6_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_custom6_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 105300000,
		.linelength = 936,
		.framelength = 22496,
		.max_framerate = 50,
		.mipi_pixel_rate = 150000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 1184,
			.w0_size = 6560,
			.h0_size = 2560,
			.scale_w = 820,
			.scale_h = 320,
			.x1_offset = 170,
			.y1_offset = 0,
			.w1_size = 480,
			.h1_size = 320,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 480,
			.h2_tg_size = 320,
		},
		.aov_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x14,
			.dphy_clk_settle = 0x14,
			.dphy_trail = 0xB3,
			.dphy_csi2_resync_dmy_cycle = 0x34,
		},
		.frame_desc = frame_desc_cus6,
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,
	},
	{	/* reg_I5 sensing 480x320@2FPS */
		.mode_setting_table = imx709_custom7_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_custom7_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 105300000,
		.linelength = 936,
		.framelength = 56248,
		.max_framerate = 20,
		.mipi_pixel_rate = 150000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 1184,
			.w0_size = 6560,
			.h0_size = 2560,
			.scale_w = 820,
			.scale_h = 320,
			.x1_offset = 170,
			.y1_offset = 0,
			.w1_size = 480,
			.h1_size = 320,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 480,
			.h2_tg_size = 320,
		},
		.aov_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x14,
			.dphy_clk_settle = 0x14,
			.dphy_trail = 0xB3,
			.dphy_csi2_resync_dmy_cycle = 0x34,
		},
		.frame_desc = frame_desc_cus7,
		.num_entries = ARRAY_SIZE(frame_desc_cus7),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,
	},
	{	/* reg_K3 sensing 320x240@5FPS */
		.mode_setting_table = imx709_custom8_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_custom8_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 210600000,
		.linelength = 1674,
		.framelength = 25160,
		.max_framerate = 50,
		.mipi_pixel_rate = 150000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 544,
			.w0_size = 6560,
			.h0_size = 3840,
			.scale_w = 410,
			.scale_h = 240,
			.x1_offset = 45,
			.y1_offset = 0,
			.w1_size = 320,
			.h1_size = 240,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 320,
			.h2_tg_size = 240,
		},
		.aov_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x14,
			.dphy_clk_settle = 0x14,
			.dphy_trail = 0xB3,
			.dphy_csi2_resync_dmy_cycle = 0x34,
		},
		.frame_desc = frame_desc_cus8,
		.num_entries = ARRAY_SIZE(frame_desc_cus8),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,
	},
	{	/* reg_K4 sensing 320x240@2FPS */
		.mode_setting_table = imx709_custom9_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_custom9_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 210600000,
		.linelength = 1674,
		.framelength = 62896,
		.max_framerate = 20,
		.mipi_pixel_rate = 150000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 544,
			.w0_size = 6560,
			.h0_size = 3840,
			.scale_w = 410,
			.scale_h = 240,
			.x1_offset = 45,
			.y1_offset = 0,
			.w1_size = 320,
			.h1_size = 240,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 320,
			.h2_tg_size = 240,
		},
		.aov_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x14,
			.dphy_clk_settle = 0x14,
			.dphy_trail = 0xB3,
			.dphy_csi2_resync_dmy_cycle = 0x34,
		},
		.frame_desc = frame_desc_cus9,
		.num_entries = ARRAY_SIZE(frame_desc_cus9),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO,
	},
	{	/* reg_M viewing 640x480@10FPS */
		.mode_setting_table = imx709_custom10_setting,
		.mode_setting_len = ARRAY_SIZE(imx709_custom10_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 465600000,
		.linelength = 3700,
		.framelength = 12576,
		.max_framerate = 100,
		.mipi_pixel_rate = 120000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 1504,
			.w0_size = 6560,
			.h0_size = 1920,
			.scale_w = 3280,
			.scale_h = 480,
			.x1_offset = 1320,
			.y1_offset = 0,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_VIEWING_MODE,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			// csi clk: 242M
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_data_settle = 0x14,
			.dphy_clk_settle = 0x14,
			.dphy_trail = 0xB3,
			.dphy_csi2_resync_dmy_cycle = 0x34,
		},
		.frame_desc = frame_desc_cus10,
		.num_entries = ARRAY_SIZE(frame_desc_cus10),
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = IMX709_SENSOR_ID,
	.reg_addr_sensor_id = {0x0016, 0x0017},
	.i2c_addr_table = {0x20, 0xFF},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {6560, 4928},
	.mirror = IMAGE_HV_MIRROR,
#ifdef AOV_EINT_UT
#ifdef AOV_IMX809_EINT_UT
	.mclk = 26,
#else
	.mclk = 24,
#endif
#else
	.mclk = 26,
#endif
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 32,
	.ana_gain_type = 0,
	.ana_gain_step = 1,
	.ana_gain_table = imx709_ana_gain_table,
	.ana_gain_table_size = sizeof(imx709_ana_gain_table),
	.min_gain_iso = 100,
	.exposure_def = 0x3D0,
	.exposure_min = 8,
	.exposure_max =  0xFFFF - 24,
	.exposure_step = 1,
	.exposure_margin = 24,

	.frame_length_max = 0xFFFF,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 3,
	.start_exposure_offset = 3000000,

	.pdaf_type = PDAF_SUPPORT_CAMSV,
	.hdr_type = HDR_SUPPORT_STAGGER_FDOL,
	.seamless_switch_support = TRUE,
	.temperature_support = TRUE,

	.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,
#if EEPROM_READY
	.s_cali = set_sensor_cali,
#else
	.s_cali = NULL,
#endif
	.s_data_rate_global_timing_phy_ctrl = set_data_rate_global_timing_phy_ctrl,
	.s_pwr_seq_reset_view_to_sensing = set_pwr_seq_reset_view_to_sensing,
	.s_streaming_control = set_streaming_control,

	.reg_addr_stream = 0x0100,
#ifdef AOV_EINT_UT
	.reg_addr_mirror_flip = PARAM_UNDEFINED,
#else
	.reg_addr_mirror_flip = 0x0101,
#endif

	.reg_addr_exposure = {
			{0x0202, 0x0203},
			{0x0224, 0x0225},
	},
	.long_exposure_support = FALSE,
	.reg_addr_exposure_lshift = PARAM_UNDEFINED,
	.reg_addr_ana_gain = {
			{0x0204, 0x0205},
			{0x0216, 0x0217},
	},
	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_temp_en = 0x0138,
	.reg_addr_temp_read = 0x013A,
	.reg_addr_auto_extend = 0x0350,
	.reg_addr_frame_count = PARAM_UNDEFINED,
	.reg_addr_fast_mode = 0x3010,

	.init_setting_table = imx709_init_setting,
	.init_setting_len = ARRAY_SIZE(imx709_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),

	.checksum_value = 0xAF3E324F,
	.aov_sensor_support = TRUE,
	.aov_csi_clk = 242,
	.sensor_mode_ops = 0,
	.sensor_debug_sensing_ut_on_scp = TRUE,
	.sensor_debug_dphy_global_timing_continuous_clk = TRUE,
	.reg_addr_aov_mode_mirror_flip = 0x3874,
	.init_in_open = FALSE,
	.streaming_ctrl_imp = TRUE,
};

static struct subdrv_ops ops = {
	.init_ctx = init_ctx,
	.open = imx709_open,
	.get_id = imx709_get_imgsensor_id,
	.vsync_notify = vsync_notify,
	.get_csi_param = get_csi_param,
	.get_temp = common_get_temp,
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = common_control,
	.feature_control = common_feature_control,
	.close = common_close,
	.get_frame_desc = common_get_frame_desc,
	.update_sof_cnt = common_update_sof_cnt,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_SCL, 0, 0},	/* default i2c bus scl 4 on apmcu side */
	{HW_ID_SDA, 0, 0},	/* default i2c bus sda 4 on apmcu side */
#ifdef AOV_EINT_UT
#ifdef AOV_IMX809_EINT_UT
	{HW_ID_MCLK1, 26, 0},
#else
	{HW_ID_MCLK1, 24, 0},
#endif
#else
	{HW_ID_MCLK1, 26, 0},
#endif
	{HW_ID_PONV, 0, 1},
	{HW_ID_RST1, 0, 1},
	{HW_ID_AVDD, 2900000, 1}, // pmic_ldo for avdd
	{HW_ID_AVDD2, 1800000, 1}, // pmic_gpo(2.8V ldo) for avdd
	{HW_ID_DOVDD, 1800000, 1}, // pmic_ldo/gpio(1.8V ldo) for dovdd
	{HW_ID_DVDD2, 855000, 1}, // pmic_ldo for dvdd
	{HW_ID_MCLK1_DRIVING_CURRENT, 6, 1},
#ifdef AOV_EINT_UT
	{HW_ID_PONV, 0, 1},
#else
	{HW_ID_PONV, 1, 1},
#endif
	{HW_ID_RST1, 1, 4}
};

const struct subdrv_entry imx709_mipi_raw_entry = {
	.name = "imx709_mipi_raw",
	.id = IMX709_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id)
{
	memcpy(&(ctx->s_ctx), &static_ctx, sizeof(struct subdrv_static_ctx));
	subdrv_ctx_init(ctx);
	ctx->i2c_client = i2c_client;
	ctx->i2c_write_id = i2c_write_id;
	return 0;
}

static int imx709_open(struct subdrv_ctx *ctx)
{
	u32 sensor_id = 0;
	u32 scenario_id = 0;

	DRV_LOG_MUST(ctx, "for imx709 start\n");

	/* get sensor id */
	if (imx709_get_imgsensor_id(ctx, &sensor_id) != ERROR_NONE)
		return ERROR_SENSOR_CONNECT_FAIL;

	memset(ctx->exposure, 0, sizeof(ctx->exposure));
	memset(ctx->ana_gain, 0, sizeof(ctx->gain));
	ctx->exposure[0] = ctx->s_ctx.exposure_def;
	ctx->ana_gain[0] = ctx->s_ctx.ana_gain_def;
	ctx->current_scenario_id = scenario_id;
	ctx->pclk = ctx->s_ctx.mode[scenario_id].pclk;
	ctx->line_length = ctx->s_ctx.mode[scenario_id].linelength;
	ctx->frame_length = ctx->s_ctx.mode[scenario_id].framelength;
	ctx->current_fps = 10 * ctx->pclk / ctx->line_length / ctx->frame_length;
	ctx->readout_length = ctx->s_ctx.mode[scenario_id].readout_length;
	ctx->read_margin = ctx->s_ctx.mode[scenario_id].read_margin;
	ctx->min_frame_length = ctx->frame_length;
	ctx->autoflicker_en = FALSE;
	ctx->test_pattern = 0;
	ctx->ihdr_mode = 0;
	ctx->pdaf_mode = 0;
	ctx->hdr_mode = 0;
	ctx->extend_frame_length_en = 0;
	ctx->is_seamless = 0;
	ctx->fast_mode_on = 0;
	ctx->sof_cnt = 0;
	ctx->ref_sof_cnt = 0;
	ctx->is_streaming = 0;

	return ERROR_NONE;
}

static int imx709_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id)
{
	u8 i = 0;
	u8 retry = 2;
	u32 addr_h = ctx->s_ctx.reg_addr_sensor_id.addr[0];
	u32 addr_l = ctx->s_ctx.reg_addr_sensor_id.addr[1];
	u32 addr_ll = ctx->s_ctx.reg_addr_sensor_id.addr[2];

	DRV_LOG(ctx, "for imx709 id\n");

	while (ctx->s_ctx.i2c_addr_table[i] != 0xFF) {
		ctx->i2c_write_id = ctx->s_ctx.i2c_addr_table[i];
		do {
			*sensor_id = (subdrv_i2c_rd_u8(ctx, addr_h) << 8) |
				subdrv_i2c_rd_u8(ctx, addr_l);
			if (addr_ll)
				*sensor_id = ((*sensor_id) << 8) | subdrv_i2c_rd_u8(ctx, addr_ll);
			DRV_LOG_MUST(ctx,
				"i2c_write_id:0x%x sensor_id(cur/exp):0x%x/0x%x\n",
				ctx->i2c_write_id, *sensor_id, ctx->s_ctx.sensor_id);
			if (*sensor_id == IMX709_SENSOR_ID) {
				*sensor_id = ctx->s_ctx.sensor_id;
				return ERROR_NONE;
			}
			DRV_LOG_MUST(ctx,
				"Read sensor id fail, id(0x%x)\n",
				ctx->i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != ctx->s_ctx.sensor_id) {
		/* if Sensor ID is not correct,
		 * Must set *sensor_id to 0xFFFFFFFF
		 */
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
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

static int get_csi_param(struct subdrv_ctx *ctx,
	enum SENSOR_SCENARIO_ID_ENUM scenario_id,
	struct mtk_csi_param *csi_param)
{
	// LOG_INF("+ scenario_id:%u,aov_csi_clk:%u\n",
		// scenario_id, ctx->aov_csi_clk);

	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		csi_param->legacy_phy = 0;
		csi_param->not_fixed_trail_settle = 1;
		csi_param->not_fixed_dphy_settle = 1;
		if (ctx->aov_phy_ctrl_ver == NULL) {
			csi_param->dphy_data_settle = 0x13;
			csi_param->dphy_clk_settle = 0x13;
			csi_param->dphy_trail = 0x34;
			csi_param->dphy_csi2_resync_dmy_cycle = 0xF;
		} else if (!strcasecmp(ctx->aov_phy_ctrl_ver,
			MT6989_PHY_CTRL_VERSIONS)) {
			csi_param->dphy_data_settle = 0x18;
			csi_param->dphy_clk_settle = 0x18;
			csi_param->dphy_trail = 0x43;
			csi_param->dphy_csi2_resync_dmy_cycle = 0x13;
		} else {
			DRV_LOGE(ctx, "phy_ctrl_ver is invalid\n");
			return -EINVAL;
		}
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		csi_param->legacy_phy = 0;
		csi_param->not_fixed_trail_settle = 1;
		csi_param->not_fixed_dphy_settle = 1;
		if (ctx->aov_phy_ctrl_ver == NULL) {
			csi_param->dphy_data_settle = 0x12;
			csi_param->dphy_clk_settle = 0x12;
			csi_param->dphy_trail = 0x31;
			csi_param->dphy_csi2_resync_dmy_cycle = 0xF;
		} else if (!strcasecmp(ctx->aov_phy_ctrl_ver,
			MT6989_PHY_CTRL_VERSIONS)) {
			csi_param->dphy_data_settle = 0x17;
			csi_param->dphy_clk_settle = 0x17;
			csi_param->dphy_trail = 0x3F;
			csi_param->dphy_csi2_resync_dmy_cycle = 0x13;
		} else {
			DRV_LOGE(ctx, "phy_ctrl_ver is invalid\n");
			return -EINVAL;
		}
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		csi_param->legacy_phy = 0;
		csi_param->not_fixed_trail_settle = 1;
		csi_param->not_fixed_dphy_settle = 1;
		if (ctx->aov_phy_ctrl_ver == NULL) {
			csi_param->dphy_data_settle = 0x13;
			csi_param->dphy_clk_settle = 0x13;
			csi_param->dphy_trail = 0x35;
			csi_param->dphy_csi2_resync_dmy_cycle = 0xF;
		} else if (!strcasecmp(ctx->aov_phy_ctrl_ver,
			MT6989_PHY_CTRL_VERSIONS)) {
			csi_param->dphy_data_settle = 0x19;
			csi_param->dphy_clk_settle = 0x19;
			csi_param->dphy_trail = 0x44;
			csi_param->dphy_csi2_resync_dmy_cycle = 0x14;
		} else {
			DRV_LOGE(ctx, "phy_ctrl_ver is invalid\n");
			return -EINVAL;
		}
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		csi_param->legacy_phy = 0;
		csi_param->not_fixed_trail_settle = 1;
		csi_param->not_fixed_dphy_settle = 1;
		switch (ctx->aov_csi_clk) {
		case 312:
			csi_param->dphy_data_settle = 0x19;
			csi_param->dphy_clk_settle = 0x19;
			csi_param->dphy_trail = 0xA8;
			csi_param->dphy_csi2_resync_dmy_cycle = 0x31;
			break;
		case 242:
			csi_param->dphy_data_settle = 0x13;
			csi_param->dphy_clk_settle = 0x13;
			csi_param->dphy_trail = 0x82;
			csi_param->dphy_csi2_resync_dmy_cycle = 0x26;
			break;
		case 130:
			if (ctx->aov_phy_ctrl_ver == NULL) {
				csi_param->dphy_data_settle = 0xB;
				csi_param->dphy_clk_settle = 0xB;
				csi_param->dphy_trail = 0x46;
				csi_param->dphy_csi2_resync_dmy_cycle = 0x15;
			} else if (!strcasecmp(ctx->aov_phy_ctrl_ver,
				MT6989_PHY_CTRL_VERSIONS)) {
				csi_param->dphy_data_settle = 0x10;
				csi_param->dphy_clk_settle = 0x10;
				csi_param->dphy_trail = 0x10;
				csi_param->dphy_csi2_resync_dmy_cycle = 0x15;
			} else {
				DRV_LOGE(ctx, "phy_ctrl_ver is invalid\n");
				return -EINVAL;
			}
			break;
		}
		break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
		csi_param->legacy_phy = 0;
		csi_param->not_fixed_trail_settle = 1;
		csi_param->not_fixed_dphy_settle = 1;
		switch (ctx->aov_csi_clk) {
		case 312:
			csi_param->dphy_data_settle = 0x17;
			csi_param->dphy_clk_settle = 0x17;
			csi_param->dphy_trail = 0x96;
			csi_param->dphy_csi2_resync_dmy_cycle = 0x2D;
			break;
		case 242:
			csi_param->dphy_data_settle = 0x12;
			csi_param->dphy_clk_settle = 0x12;
			csi_param->dphy_trail = 0x75;
			csi_param->dphy_csi2_resync_dmy_cycle = 0x23;
			break;
		case 130:
			if (ctx->aov_phy_ctrl_ver== NULL) {
				csi_param->dphy_data_settle = 0xA;
				csi_param->dphy_clk_settle = 0xA;
				csi_param->dphy_trail = 0x3F;
				csi_param->dphy_csi2_resync_dmy_cycle = 0x13;
			} else if (!strcasecmp(ctx->aov_phy_ctrl_ver,
				MT6989_PHY_CTRL_VERSIONS)) {
				csi_param->dphy_data_settle = 0x10;
				csi_param->dphy_clk_settle = 0x10;
				csi_param->dphy_trail = 0x10;
				csi_param->dphy_csi2_resync_dmy_cycle = 0x13;
			} else {
				DRV_LOGE(ctx, "phy_ctrl_ver is invalid\n");
				return -EINVAL;
			}
			break;
		}
		break;
	case SENSOR_SCENARIO_ID_CUSTOM3:
		csi_param->legacy_phy = 0;
		csi_param->not_fixed_trail_settle = 1;
		csi_param->not_fixed_dphy_settle = 1;
		switch (ctx->aov_csi_clk) {
		case 312:
			csi_param->dphy_data_settle = 0x18;
			csi_param->dphy_clk_settle = 0x18;
			csi_param->dphy_trail = 0xA3;
			csi_param->dphy_csi2_resync_dmy_cycle = 0x2F;
			break;
		case 242:
			csi_param->dphy_data_settle = 0x13;
			csi_param->dphy_clk_settle = 0x13;
			csi_param->dphy_trail = 0x7F;
			csi_param->dphy_csi2_resync_dmy_cycle = 0x25;
			break;
		case 130:
			if (ctx->aov_phy_ctrl_ver == NULL) {
				csi_param->dphy_data_settle = 0xA;
				csi_param->dphy_clk_settle = 0xA;
				csi_param->dphy_trail = 0x44;
				csi_param->dphy_csi2_resync_dmy_cycle = 0x14;
			} else if (!strcasecmp(ctx->aov_phy_ctrl_ver,
				MT6989_PHY_CTRL_VERSIONS)) {
				csi_param->dphy_data_settle = 0x10;
				csi_param->dphy_clk_settle = 0x10;
				csi_param->dphy_trail = 0x10;
				csi_param->dphy_csi2_resync_dmy_cycle = 0x14;
			} else {
				DRV_LOGE(ctx, "phy_ctrl_ver is invalid\n");
				return -EINVAL;
			}
			break;
		}
		break;
	case SENSOR_SCENARIO_ID_CUSTOM4:
	case SENSOR_SCENARIO_ID_CUSTOM5:
	case SENSOR_SCENARIO_ID_CUSTOM6:
	case SENSOR_SCENARIO_ID_CUSTOM7:
	case SENSOR_SCENARIO_ID_CUSTOM8:
	case SENSOR_SCENARIO_ID_CUSTOM9:
	case SENSOR_SCENARIO_ID_CUSTOM10:
		csi_param->legacy_phy = 0;
		csi_param->not_fixed_trail_settle = 1;
		csi_param->not_fixed_dphy_settle = 1;
		switch (ctx->aov_csi_clk) {
		case 312:
			csi_param->dphy_data_settle = 0x19;
			csi_param->dphy_clk_settle = 0x19;
			csi_param->dphy_trail = 0xE6;
			csi_param->dphy_csi2_resync_dmy_cycle = 0x43;
			break;
		case 242:
			csi_param->dphy_data_settle = 0x14;
			csi_param->dphy_clk_settle = 0x14;
			csi_param->dphy_trail = 0xB3;
			csi_param->dphy_csi2_resync_dmy_cycle = 0x34;
			break;
		case 130:
			if (ctx->aov_phy_ctrl_ver == NULL) {
				csi_param->dphy_data_settle = 0xB;
				csi_param->dphy_clk_settle = 0xB;
				csi_param->dphy_trail = 0x60;
				csi_param->dphy_csi2_resync_dmy_cycle = 0x1C;
			} else if (!strcasecmp(ctx->aov_phy_ctrl_ver,
				MT6989_PHY_CTRL_VERSIONS)) {
				csi_param->dphy_data_settle = 0x10;
				csi_param->dphy_clk_settle = 0x10;
				csi_param->dphy_trail = 0x10;
				csi_param->dphy_csi2_resync_dmy_cycle = 0x1C;
			} else {
				DRV_LOGE(ctx, "phy_ctrl_ver is invalid\n");
				return -EINVAL;
			}
			break;
		}
		break;
	default:
		csi_param->legacy_phy = 0;
		csi_param->not_fixed_trail_settle = 0;
		csi_param->not_fixed_dphy_settle = 0;
		break;
	}
	DRV_LOG_MUST(ctx,
		"[%s] scenario:%u,aov_csi_clk[%u] %d|%d|%d|%d|%d|%d|%d|%d\n",
		__func__,
		scenario_id,
		ctx->aov_csi_clk,
		csi_param->cphy_settle,
		csi_param->dphy_clk_settle,
		csi_param->dphy_data_settle,
		csi_param->dphy_trail,
		csi_param->not_fixed_trail_settle,
		csi_param->legacy_phy,
		csi_param->dphy_csi2_resync_dmy_cycle,
		csi_param->not_fixed_dphy_settle);
	return 0;
}

static int get_sensor_temperature(void *arg)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	u8 temperature = 0;
	int temperature_convert = 0;

	temperature = subdrv_i2c_rd_u8(ctx, ctx->s_ctx.reg_addr_temp_read);

	if (temperature <= 0x60)
		temperature_convert = temperature;
	else if (temperature >= 0x61 && temperature <= 0x7F)
		temperature_convert = 97;
	else if (temperature >= 0x80 && temperature <= 0xE2)
		temperature_convert = -30;
	else
		temperature_convert = (char)temperature | 0xFFFFFF0;

	DRV_LOG_MUST(ctx, "temperature: %d degrees\n", temperature_convert);
	return temperature_convert;
}

static u16 get_gain2reg(u32 gain)
{
	return (16384 - (16384 * BASEGAIN) / gain);
}

static void set_group_hold(void *arg, u8 en)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	if (en)
		set_i2c_buffer(ctx, 0x0104, 0x01);
	else
		set_i2c_buffer(ctx, 0x0104, 0x00);
}

#if EEPROM_READY
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

	/* QSC data */
	support = info[idx].qsc_support;
	pbuf = info[idx].preload_qsc_table;
	size = info[idx].qsc_size;
	addr = info[idx].sensor_reg_addr_qsc;
	if (support) {
		if (pbuf != NULL && addr > 0 && size > 0) {
			subdrv_i2c_wr_u8(ctx, 0x86A9, 0x4E);
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			subdrv_i2c_wr_u8(ctx, 0x32D2, 0x01);
			DRV_LOG(ctx, "set QSC calibration data done.");
		} else {
			subdrv_i2c_wr_u8(ctx, 0x32D2, 0x00);
		}
	}
}
#endif

static void set_data_rate_global_timing_phy_ctrl(void *arg)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	DRV_LOG_MUST(ctx,
		"E! scenario(%d)\n", ctx->current_scenario_id);

	// global timing phy ctrl
	switch (ctx->current_scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		subdrv_i2c_wr_u8(ctx, 0x0808, 0x02);	// PHY_CTRL
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x080B, 0x9C);
		} else {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);	// TCLK_POST_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x080B, 0x8F);	// TCLK_POST_EX[7:0]
		}
		subdrv_i2c_wr_u8(ctx, 0x080C, 0x00);	// THS_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080D, 0x4F);	// THS_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x080E, 0x00);	// THS_ZERO_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080F, 0x7F);	// THS_ZERO_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0810, 0x00);	// THS_TRAIL_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0811, 0x47);	// THS_TRAIL_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0812, 0x00);	// TCLK_TRAIL_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0813, 0x47);	// TCLK_TRAIL_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0814, 0x00);	// TCLK_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0815, 0x4F);	// TCLK_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0816, 0x01);	// TCLK_ZERO_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0817, 0x1F);	// TCLK_ZERO_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0818, 0x00);	// TLPX_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0819, 0x3F);	// TLPX_EX[7:0]
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);	// THS_EXIT_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x0825, 0x7F);	// THS_EXIT_EX[7:0]
		} else {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x0825, 0xE6);
		}
		subdrv_i2c_wr_u8(ctx, 0x0826, 0x00);	// TCLK_PRE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0827, 0x0F);	// TCLK_PRE_EX[7:0]
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		subdrv_i2c_wr_u8(ctx, 0x0808, 0x02);	// PHY_CTRL
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x080B, 0x99);
		} else {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);	// TCLK_POST_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x080B, 0x8F);	// TCLK_POST_EX[7:0]
		}
		subdrv_i2c_wr_u8(ctx, 0x080C, 0x00);	// THS_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080D, 0x4F);	// THS_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x080E, 0x00);	// THS_ZERO_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080F, 0x8F);	// THS_ZERO_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0810, 0x00);	// THS_TRAIL_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0811, 0x4F);	// THS_TRAIL_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0812, 0x00);	// TCLK_TRAIL_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0813, 0x47);	// TCLK_TRAIL_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0814, 0x00);	// TCLK_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0815, 0x4F);	// TCLK_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0816, 0x01);	// TCLK_ZERO_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0817, 0x2F);	// TCLK_ZERO_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0818, 0x00);	// TLPX_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0819, 0x3F);	// TLPX_EX[7:0]
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);	// THS_EXIT_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x0825, 0x7F);	// THS_EXIT_EX[7:0]
		} else {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x0825, 0xE7);
		}
		subdrv_i2c_wr_u8(ctx, 0x0826, 0x00);	// TCLK_PRE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0827, 0x0F);	// TCLK_PRE_EX[7:0]
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		subdrv_i2c_wr_u8(ctx, 0x0808, 0x02);	// PHY_CTRL
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x080B, 0x9D);
		} else {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);	// TCLK_POST_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x080B, 0x8F);	// TCLK_POST_EX[7:0]
		}
		subdrv_i2c_wr_u8(ctx, 0x080C, 0x00);	// THS_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080D, 0x4F);	// THS_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x080E, 0x00);	// THS_ZERO_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080F, 0x7F);	// THS_ZERO_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0810, 0x00);	// THS_TRAIL_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0811, 0x47);	// THS_TRAIL_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0812, 0x00);	// TCLK_TRAIL_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0813, 0x47);	// TCLK_TRAIL_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0814, 0x00);	// TCLK_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0815, 0x4F);	// TCLK_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0816, 0x01);	// TCLK_ZERO_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0817, 0x1F);	// TCLK_ZERO_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0818, 0x00);	// TLPX_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0819, 0x3F);	// TLPX_EX[7:0]
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);	// THS_EXIT_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x0825, 0x7F);	// THS_EXIT_EX[7:0]
		} else {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x0825, 0xE6);
		}
		subdrv_i2c_wr_u8(ctx, 0x0826, 0x00);	// TCLK_PRE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0827, 0x0F);	// TCLK_PRE_EX[7:0]
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
#ifndef AOV_EINT_UT
		subdrv_i2c_wr_u8(ctx, 0x0808, 0x02);	// PHY_CTRL
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x080B, 0xC3);
		} else {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);	// TCLK_POST_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x080B, 0x64);	// TCLK_POST_EX[7:0]
		}
		subdrv_i2c_wr_u8(ctx, 0x080C, 0x00);	// THS_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080D, 0x1F);	// THS_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x080E, 0x00);	// THS_ZERO_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080F, 0x3F);	// THS_ZERO_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0810, 0x00);	// THS_TRAIL_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0811, 0x3F);	// THS_TRAIL_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0812, 0x00);	// TCLK_TRAIL_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0813, 0x1F);	// TCLK_TRAIL_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0814, 0x00);	// TCLK_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0815, 0x1F);	// TCLK_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0816, 0x00);	// TCLK_ZERO_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0817, 0x7F);	// TCLK_ZERO_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0818, 0x00);	// TLPX_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0819, 0x1F);	// TLPX_EX[7:0]
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);	// THS_EXIT_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x0825, 0x2F);	// THS_EXIT_EX[7:0]
		} else {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x0825, 0xFD);
		}
		subdrv_i2c_wr_u8(ctx, 0x0826, 0x00);	// TCLK_PRE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0827, 0x0F);	// TCLK_PRE_EX[7:0]
#endif
		break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
		subdrv_i2c_wr_u8(ctx, 0x0808, 0x02);	// PHY_CTRL
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x080B, 0xC1);
		} else {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);	// TCLK_POST_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x080B, 0x64);	// TCLK_POST_EX[7:0]
		}
		subdrv_i2c_wr_u8(ctx, 0x080C, 0x00);	// THS_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080D, 0x27);	// THS_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x080E, 0x00);	// THS_ZERO_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080F, 0x3F);	// THS_ZERO_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0810, 0x00);	// THS_TRAIL_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0811, 0x3F);	// THS_TRAIL_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0812, 0x00);	// TCLK_TRAIL_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0813, 0x1F);	// TCLK_TRAIL_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0814, 0x00);	// TCLK_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0815, 0x1F);	// TCLK_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0816, 0x00);	// TCLK_ZERO_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0817, 0x8F);	// TCLK_ZERO_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0818, 0x00);	// TLPX_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0819, 0x1F);	// TLPX_EX[7:0]
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);	// THS_EXIT_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x0825, 0x3F);	// THS_EXIT_EX[7:0]
		} else {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x0825, 0xFD);
		}
		subdrv_i2c_wr_u8(ctx, 0x0826, 0x00);	// TCLK_PRE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0827, 0x0F);	// TCLK_PRE_EX[7:0]
		break;
	case SENSOR_SCENARIO_ID_CUSTOM3:
		subdrv_i2c_wr_u8(ctx, 0x0808, 0x02);	// PHY_CTRL
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x080B, 0xC2);
		} else {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);	// TCLK_POST_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x080B, 0x64);	// TCLK_POST_EX[7:0]
		}
		subdrv_i2c_wr_u8(ctx, 0x080C, 0x00);	// THS_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080D, 0x1F);	// THS_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x080E, 0x00);	// THS_ZERO_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080F, 0x3F);	// THS_ZERO_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0810, 0x00);	// THS_TRAIL_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0811, 0x3F);	// THS_TRAIL_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0812, 0x00);	// TCLK_TRAIL_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0813, 0x1F);	// TCLK_TRAIL_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0814, 0x00);	// TCLK_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0815, 0x1F);	// TCLK_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0816, 0x00);	// TCLK_ZERO_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0817, 0x7F);	// TCLK_ZERO_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0818, 0x00);	// TLPX_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0819, 0x1F);	// TLPX_EX[7:0]
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);	// THS_EXIT_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x0825, 0x2F);	// THS_EXIT_EX[7:0]
		} else {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x0825, 0xFD);
		}
		subdrv_i2c_wr_u8(ctx, 0x0826, 0x00);	// TCLK_PRE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0827, 0x0F);	// TCLK_PRE_EX[7:0]
		break;
	case SENSOR_SCENARIO_ID_CUSTOM4:
	case SENSOR_SCENARIO_ID_CUSTOM5:
	case SENSOR_SCENARIO_ID_CUSTOM6:
	case SENSOR_SCENARIO_ID_CUSTOM7:
	case SENSOR_SCENARIO_ID_CUSTOM8:
	case SENSOR_SCENARIO_ID_CUSTOM9:
	case SENSOR_SCENARIO_ID_CUSTOM10:
		subdrv_i2c_wr_u8(ctx, 0x0808, 0x02);	// PHY_CTRL
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x080B, 0xC9);
		} else {
			subdrv_i2c_wr_u8(ctx, 0x080A, 0x00);	// TCLK_POST_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x080B, 0x64);	// TCLK_POST_EX[7:0]
		}
		subdrv_i2c_wr_u8(ctx, 0x080C, 0x00);	// THS_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080D, 0x17);	// THS_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x080E, 0x00);	// THS_ZERO_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x080F, 0x2F);	// THS_ZERO_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0810, 0x00);	// THS_TRAIL_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0811, 0x3F);	// THS_TRAIL_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0812, 0x00);	// TCLK_TRAIL_MIN_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0813, 0x17);	// TCLK_TRAIL_MIN_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0814, 0x00);	// TCLK_PREPARE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0815, 0x17);	// TCLK_PREPARE_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0816, 0x00);	// TCLK_ZERO_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0817, 0x5F);	// TCLK_ZERO_EX[7:0]
		subdrv_i2c_wr_u8(ctx, 0x0818, 0x00);	// TLPX_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0819, 0x1F);	// TLPX_EX[7:0]
		if (!ctx->sensor_debug_dphy_global_timing_continuous_clk) {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);	// THS_EXIT_EX[9:8]
			subdrv_i2c_wr_u8(ctx, 0x0825, 0x2F);	// THS_EXIT_EX[7:0]
		} else {
			subdrv_i2c_wr_u8(ctx, 0x0824, 0x00);
			subdrv_i2c_wr_u8(ctx, 0x0825, 0xFD);
		}
		subdrv_i2c_wr_u8(ctx, 0x0826, 0x00);	// TCLK_PRE_EX[9:8]
		subdrv_i2c_wr_u8(ctx, 0x0827, 0x0F);	// TCLK_PRE_EX[7:0]
		break;
	default:
		DRV_LOGE(ctx,
			"Wrong scenario(%d) write global timing (fail)!\n",
			ctx->current_scenario_id);
		break;
	}
}

#ifdef PWR_SEQ_ALL_USE_FOR_AOV_MODE_TRANSITION
static int pwr_seq_common_disable_for_mode_transition(struct adaptor_ctx *ctx)
{
	int ret = 0;

	DRV_LOG_MUST(ctx, "E!\n");

	// 1. set gpio
	// mclk_driving_current_off
	ret = pinctrl_select_state(ctx->pinctrl,
		ctx->state[STATE_MCLK1_OFF]);
	if (ret < 0) {
		DRV_LOG_MUST(ctx,
			"select(%s)(fail),ret(%d)\n",
			state_names[STATE_MCLK1_OFF], ret);
		return ret;
	}
	DRV_LOG_MUST(ctx, "select(%s)(correct)\n", state_names[STATE_MCLK1_OFF]);
	mdelay(6);
	// 2. set reg
	// disable DOVDD
	ret = regulator_disable(ctx->regulator[REGULATOR_DOVDD]);
	if (ret) {
		DRV_LOG_MUST(ctx,
			"disable(%s)(fail),ret(%d)\n",
			reg_names[REGULATOR_DOVDD], ret);
		return ret;
	}
	DRV_LOG_MUST(ctx, "select(%s)(correct)\n", reg_names[REGULATOR_DOVDD]);
	mdelay(1);
	// disable DVDD1
	ret = regulator_disable(ctx->regulator[REGULATOR_DVDD1]);
	if (ret) {
		DRV_LOG_MUST(ctx, "disable(%s)(fail),ret(%d)\n",
		reg_names[REGULATOR_DVDD1], ret);
		return ret;
	}
	DRV_LOG_MUST(ctx, "select(%s)(correct)\n", reg_names[REGULATOR_DVDD1]);
	mdelay(4);
	// disable AVDD2
	ret = regulator_disable(ctx->regulator[REGULATOR_AVDD2]);
	if (ret) {
		DRV_LOG_MUST(ctx,
			"disable(%s)(fail),ret(%d)\n",
			reg_names[REGULATOR_AVDD2], ret);
		return ret;
	}
	DRV_LOG_MUST(ctx, "select(%s)(correct)\n", reg_names[REGULATOR_AVDD2]);
	mdelay(3);
	// 3. set mclk
	// disable mclk
	clk_disable_unprepare(ctx->clk[CLK1_MCLK1]);

	return ret;
}

static int pwr_seq_common_enable_for_mode_transition(struct adaptor_ctx *ctx)
{
	int ret = 0;

	DRV_LOG_MUST(ctx, "E!\n");

	// 1. set mclk
	// 24MHz
	ret = clk_prepare_enable(ctx->clk[CLK1_MCLK1]);
	if (ret) {
		DRV_LOG_MUST(ctx, "enable mclk(fail),ret(%d)\n", ret);
		return ret;
	}
	DRV_LOG_MUST(ctx, "enable mclk(correct)\n");
	ret = clk_set_parent(ctx->clk[CLK1_MCLK1], ctx->clk[CLK1_26M]);
	if (ret) {
		DRV_LOG_MUST(ctx, "enable mclk's parent(fail),ret(%d)\n", ret);
		WRAP_AEE_EXCEPTION("clk_set_parent", "Err");
		return ret;
	}
	DRV_LOG_MUST(ctx, "enable mclk's parent(correct)\n");
	// 2. set reg
	// enable AVDD2
	ret = regulator_set_voltage(ctx->regulator[REGULATOR_AVDD2], 1800000, 1800000);
	if (ret) {
		DRV_LOG_MUST(ctx,
			"set voltage(%s)(%d)(fail),ret(%d)\n",
			reg_names[REGULATOR_AVDD2], 1800000, ret);
		return ret;
	}
	DRV_LOG_MUST(ctx, "set voltage(%s)(%d)(correct)\n",
		reg_names[REGULATOR_AVDD2], 1800000);
	ret = regulator_enable(ctx->regulator[REGULATOR_AVDD2]);
	if (ret) {
		DRV_LOG_MUST(ctx,
			"enable(%s)(fail),ret(%d)\n",
			reg_names[REGULATOR_AVDD2], ret);
		return ret;
	}
	DRV_LOG_MUST(ctx, "enable(%s)(correct)\n", reg_names[REGULATOR_AVDD2]);
	mdelay(3);
	// enable DVDD1
	ret = regulator_set_voltage(ctx->regulator[REGULATOR_DVDD1], 855000, 855000);
	if (ret) {
		DRV_LOG_MUST(ctx,
			"set voltage(%s)(%d)(fail),ret(%d)\n",
			reg_names[REGULATOR_DVDD1], 855000, ret);
		return ret;
	}
	DRV_LOG_MUST(ctx,
		"set voltage(%s)(%d)(correct)\n",
		reg_names[REGULATOR_DVDD1], 855000);
	ret = regulator_enable(ctx->regulator[REGULATOR_DVDD1]);
	if (ret) {
		DRV_LOG_MUST(ctx,
			"enable(%s)(fail),ret(%d)\n",
			reg_names[REGULATOR_DVDD1], ret);
		return ret;
	}
	DRV_LOG_MUST(ctx, "enable(%s)(correct)\n", reg_names[REGULATOR_DVDD1]);
	mdelay(4);
	// enable DOVDD
	ret = regulator_set_voltage(ctx->regulator[REGULATOR_DOVDD], 1800000, 1800000);
	if (ret) {
		DRV_LOG_MUST(ctx,
			"set voltage(%s)(%d)(fail),ret(%d)\n",
			reg_names[REGULATOR_DOVDD], 1800000, ret);
		return ret;
	}
	DRV_LOG_MUST(ctx, "set voltage(%s)(%d)(correct)\n",
		reg_names[REGULATOR_DOVDD], 1800000);
	ret = regulator_enable(ctx->regulator[REGULATOR_DOVDD]);
	if (ret) {
		DRV_LOG_MUST(ctx,
			"enable(%s)(fail),ret(%d)\n",
			reg_names[REGULATOR_DOVDD], ret);
		return ret;
	}
	DRV_LOG_MUST(ctx, "enable(%s)(correct)\n", reg_names[REGULATOR_DOVDD]);
	mdelay(1);
	// 3. set gpio
	// mclk_driving_current_on 6MA
	ret = pinctrl_select_state(ctx->pinctrl, ctx->state[STATE_MCLK1_6MA]);
	if (ret < 0) {
		DRV_LOG_MUST(ctx,
			"select(%s)(fail),ret(%d)\n",
			state_names[STATE_MCLK1_6MA], ret);
		return ret;
	}
	DRV_LOG_MUST(ctx, "select(%s)(correct)\n", state_names[STATE_MCLK1_6MA]);
	mdelay(6);

	return ret;
}
#endif

static int set_pwr_seq_reset_view_to_sensing(void *arg)
{
#ifdef AOV_EINT_UT
	return 0;
#else
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	int ret = 0;
	struct adaptor_ctx *_adaptor_ctx = NULL;
	struct v4l2_subdev *sd = NULL;

	if (ctx->i2c_client)
		sd = i2c_get_clientdata(ctx->i2c_client);
	if (sd)
		_adaptor_ctx = to_ctx(sd);
	if (!_adaptor_ctx)
		return -ENODEV;

	/* switch viewing mode sw stand-by to hw stand-by */
	// 1. set gpio
	// xclr(reset) = 0
	ret = pinctrl_select_state(
		_adaptor_ctx->pinctrl,
		_adaptor_ctx->state[STATE_RST1_LOW]);
	if (ret < 0) {
		DRV_LOG_MUST(ctx,
			"select(%s)(fail),ret(%d)\n",
			state_names[STATE_RST1_LOW], ret);
		return ret;
	}
	DRV_LOG(ctx, "select(%s)(correct)\n", state_names[STATE_RST1_LOW]);
	mdelay(1);	// response time T4-T6 in datasheet
	// ponv = 0
	ret = pinctrl_select_state(
		_adaptor_ctx->pinctrl,
		_adaptor_ctx->state[STATE_PONV_LOW]);
	if (ret < 0) {
		DRV_LOG_MUST(ctx,
			"select(%s)(fail),ret(%d)\n",
			state_names[STATE_PONV_LOW], ret);
		return ret;
	}
	DRV_LOG(ctx, "select(%s)(correct)\n", state_names[STATE_PONV_LOW]);
	mdelay(1);	// response time T4-T6 in datasheet
#ifdef PWR_SEQ_ALL_USE_FOR_AOV_MODE_TRANSITION
	ret = pwr_seq_common_disable_for_mode_transition(_adaptor_ctx);
	if (ret < 0) {
		DRV_LOG_MUST(ctx,
			"pwr_seq_common_disable_for_mode_transition(fail),ret(%d)\n",
			ret);
		return ret;
	}
	DRV_LOG(ctx, "pwr_seq_common_disable_for_mode_transition(correct)\n");
	// switch hw stand-by to sensing mode sw stand-by
	ret = pwr_seq_common_enable_for_mode_transition(_adaptor_ctx);
	if (ret < 0) {
		DRV_LOG_MUST(ctx,
			"pwr_seq_common_enable_for_mode_transition(fail),ret(%d)\n",
			ret);
		return ret;
	}
	DRV_LOG(ctx, "pwr_seq_common_enable_for_mode_transition)(correct)\n");
#endif
	// xclr(reset) = 1
	ret = pinctrl_select_state(
		_adaptor_ctx->pinctrl,
		_adaptor_ctx->state[STATE_RST1_HIGH]);
	if (ret < 0) {
		DRV_LOG_MUST(ctx,
			"select(%s)(fail),ret(%d)\n",
			state_names[STATE_RST1_HIGH], ret);
		return ret;
	}
	DRV_LOG(ctx, "select(%s)(correct)\n", state_names[STATE_RST1_HIGH]);
	mdelay(4);	// response time T7 in datasheet

	return ret;
#endif
}

static int pwr_seq_reset_sens_to_viewing(struct subdrv_ctx *ctx)
{
	int ret = 0;
	struct adaptor_ctx *_adaptor_ctx = NULL;
	struct v4l2_subdev *sd = NULL;

	if (ctx->i2c_client)
		sd = i2c_get_clientdata(ctx->i2c_client);
	if (sd)
		_adaptor_ctx = to_ctx(sd);
	if (!_adaptor_ctx)
		return -ENODEV;

	// i2c bus scl4 on apmcu side
	ret = pinctrl_select_state(
		_adaptor_ctx->pinctrl,
		_adaptor_ctx->state[STATE_SCL_AP]);
	if (ret < 0) {
		DRV_LOG_MUST(ctx,
			"select(%s)(fail),ret(%d)\n",
			state_names[STATE_SCL_AP], ret);
		return ret;
	}
	DRV_LOG(ctx, "select(%s)(correct)\n", state_names[STATE_SCL_AP]);

	// i2c bus sda4 on apmcu side
	ret = pinctrl_select_state(
		_adaptor_ctx->pinctrl,
		_adaptor_ctx->state[STATE_SDA_AP]);
	if (ret < 0) {
		DRV_LOG_MUST(ctx,
			"select(%s)(fail),ret(%d)\n",
			state_names[STATE_SDA_AP], ret);
		return ret;
	}
	DRV_LOG(ctx, "select(%s)(correct)\n", state_names[STATE_SDA_AP]);
	_adaptor_ctx->is_i2c_bus_scp = false;
	mdelay(1);

	subdrv_i2c_wr_u8(ctx, 0x0100, 0x00);
	DRV_LOG_MUST(ctx, "MODE_SEL(%08x)\n", subdrv_i2c_rd_u8(ctx, 0x0100));
#ifdef PWR_SEQ_ALL_USE_FOR_AOV_MODE_TRANSITION
	ret = pwr_seq_common_disable_for_mode_transition(_adaptor_ctx);
	if (ret < 0) {
		DRV_LOG_MUST(ctx,
			"pwr_seq_common_disable_for_mode_transition(fail),ret(%d)\n",
			ret);
		return ret;
	}
	DRV_LOG(ctx, "pwr_seq_common_disable_for_mode_transition(correct)\n");

	// switch hw stand-by to viewing mode sw stand-by
	ret = pwr_seq_common_enable_for_mode_transition(_adaptor_ctx);
	if (ret < 0) {
		DRV_LOG_MUST(ctx,
			"pwr_seq_common_enable_for_mode_transition(fail),ret(%d)\n",
			ret);
		return ret;
	}
	DRV_LOG(ctx, "pwr_seq_common_enable_for_mode_transition(correct)\n");
#endif

	return ret;
}

static int set_streaming_control(void *arg, bool enable)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	int ret = 0;

	DRV_LOG(ctx, "E!\n");

	DRV_LOG_MUST(ctx,
		"streaming_enable(0=Sw Standby,1=streaming):(%d)\n", enable);

	if (ctx->s_ctx.mode[ctx->current_scenario_id].aov_mode) {
		if (ctx->sensor_debug_sensing_ut_on_scp) {
			if (enable)
				stream_refcnt_for_aov = 1;
			else {
				if (stream_refcnt_for_aov) {
					ret = pwr_seq_reset_sens_to_viewing(ctx);
					if (ret)
						DRV_LOG_MUST(ctx,
							"pwr_seq_reset_sens_to_viewing(fail),ret(%d)\n",
							ret);
					else
						DRV_LOG_MUST(ctx,
							"pwr_seq_reset_sens_to_viewing(correct),ret(%d)\n",
							ret);
				}
				stream_refcnt_for_aov = 0;
				DRV_LOG_MUST(ctx,
					"off[correct],stream_refcnt_for_aov(%d)\n",
					stream_refcnt_for_aov);
			}
			DRV_LOG_MUST(ctx,
				"AOV mode(%d) streaming control on scp side\n",
				ctx->sensor_mode);
			return ERROR_NONE;
		}
		DRV_LOG_MUST(ctx,
			"AOV mode(%d) streaming control on apmcu side\n",
			ctx->sensor_mode);
	}

	if (enable) {
		if (subdrv_i2c_rd_u8(ctx, 0x0350) != 0x01) {
			DRV_LOG_MUST(ctx, "single cam scenario enable auto-extend\n");
			subdrv_i2c_wr_u8(ctx, 0x0350, 0x01);
		}

		if (!ctx->sensor_debug_sensing_ut_on_scp) {
			stream_refcnt_for_aov = 1;
			DRV_LOG_MUST(ctx,
				"on(correct),stream_refcnt_for_aov(%d)\n",
				stream_refcnt_for_aov);
#ifndef AOV_EINT_UT
			if (ctx->s_ctx.mode[ctx->current_scenario_id].aov_mode) {
				// subdrv_i2c_wr_u8(ctx, 0x32A0, 0x01);
				subdrv_i2c_wr_u8(ctx, 0x42B0, 0x00);
			}
#endif
		}
		subdrv_i2c_wr_u8(ctx, 0x0100, 0X01);
		DRV_LOG_MUST(ctx,
			"MODE_SEL(%08x)\n", subdrv_i2c_rd_u8(ctx, 0x0100));
		ctx->test_pattern = 0;
	} else {
		subdrv_i2c_wr_u8(ctx, 0x0100, 0x00);
		if (!ctx->sensor_debug_sensing_ut_on_scp) {
			if (stream_refcnt_for_aov) {
				if (ctx->s_ctx.mode[ctx->current_scenario_id].aov_mode) {
					ret = pwr_seq_reset_sens_to_viewing(ctx);
					if (ret)
						DRV_LOG_MUST(ctx,
							"pwr_seq_reset_sens_to_viewing(fail),ret(%d)\n",
							ret);
					else
						DRV_LOG_MUST(ctx,
							"pwr_seq_reset_sens_to_viewing(correct),ret(%d)\n",
							ret);
				}
			}
			stream_refcnt_for_aov = 0;
			DRV_LOG_MUST(ctx,
				"off(correct),stream_refcnt_for_aov(%d)\n",
				stream_refcnt_for_aov);
		}
	}

	return ret;
}

static int imx709_check_sensor_id(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	return imx709_get_imgsensor_id(ctx, (u32 *)para);
}

static int imx709_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	enum SENSOR_SCENARIO_ID_ENUM scenario_id;
	struct mtk_hdr_ae *ae_ctrl = NULL;
	u32 *feature_data = (u32 *)para;
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
	DRV_LOG_MUST(ctx,
		"E: set seamless switch %u %u\n",
		ctx->current_scenario_id, scenario_id);
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
	if (ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table
		== NULL) {
		DRV_LOGE(ctx, "Please implement seamless_switch setting\n");
		return ERROR_NONE;
	}

	exp_cnt = ctx->s_ctx.mode[scenario_id].exp_cnt;
	ctx->is_seamless = TRUE;
	update_mode_info(ctx, scenario_id);

	subdrv_i2c_wr_u8(ctx, 0x0104, 0x01);
	subdrv_i2c_wr_u8(ctx, 0x3010, 0x02);
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
	subdrv_i2c_wr_u8(ctx, 0x0104, 0x00);

	ctx->fast_mode_on = TRUE;
	ctx->ref_sof_cnt = ctx->sof_cnt;
	ctx->is_seamless = FALSE;
	DRV_LOG_MUST(ctx, "X: set seamless switch done\n");
	return ERROR_NONE;
}

static int imx709_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG_MUST(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	switch (mode) {
	case 5:
		subdrv_i2c_wr_u8(ctx, 0x020E, 0x00); /* dig_gain = 0 */
		break;
	default:
		subdrv_i2c_wr_u8(ctx, 0x0601, mode);
		break;
	}

	if ((ctx->test_pattern) && (mode != ctx->test_pattern)) {
		if (ctx->test_pattern == 5)
			subdrv_i2c_wr_u8(ctx, 0x020E, 0x01);
		else if (mode == 0)
			subdrv_i2c_wr_u8(ctx, 0x0601, 0x00); /* No pattern */
	}

	ctx->test_pattern = mode;
	return ERROR_NONE;
}
