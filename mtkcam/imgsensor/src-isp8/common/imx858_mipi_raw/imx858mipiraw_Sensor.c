// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 imx858mipiraw_Sensor.c
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
#include "imx858mipiraw_Sensor.h"


static void set_sensor_cali(void *arg);
static int get_sensor_temperature(void *arg);
static void set_group_hold(void *arg, u8 en);
static u16 get_gain2reg(u32 gain);
static int imx858_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int imx858_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);

static int imx858_mcss_update_subdrv_para(void *arg, int scenario_id);
static int imx858_mcss_init(void *arg);
static int imx858_get_sensor_sync_mode(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int imx858_mcss_set_mask_frame(struct subdrv_ctx *ctx, u32 num, u32 is_critical);

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, imx858_set_test_pattern},
	{SENSOR_FEATURE_SEAMLESS_SWITCH, imx858_seamless_switch},
	{SENSOR_FEATURE_GET_SENSOR_SYNC_MODE, imx858_get_sensor_sync_mode},
};


static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = { //mode 0
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0e80, // 3712
			.vsize = 0x0ae0, //2784
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0e80, // 3712
			.vsize = 0x02b8, // 696
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0e80, // 3712
			.vsize = 0x0ae0, //2784
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0e80, // 3712
			.vsize = 0x02b8, // 696
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = { //mode 2
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000, //4096
			.vsize = 0x0900, //2304
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000, //4096
			.vsize = 0x0240, //576
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_hs_vid[] = { //mode 3
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0e80, // 3712
			.vsize = 0x0ae0, //2784
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0e80, // 3712
			.vsize = 0x02b8, // 696
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = { //mode 4
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0e80, // 3712
			.vsize = 0x0ae0, //2784
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0e80, // 3712
			.vsize = 0x02b8, // 696
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = { //mode 5
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0e80, // 3712
			.vsize = 0x0ae0, // 2784
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x0e80, // 3712
			.vsize = 0x0ae0, // 2784
			.user_data_desc = VC_STAGGER_ME,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0e80, // 3712
			.vsize = 0x0ae0, // 2784
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0740, // 1856
			.vsize = 0x0570, // 1392
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

static struct subdrv_mode_struct mode_struct[] = {
	{//mode 0
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = imx858_preview_setting,
		.mode_setting_len = ARRAY_SIZE(imx858_preview_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx858_seamless_preview,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx858_seamless_preview),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 878400000,  // VTPXCK system pixel rate
		.linelength = 7500,
		.framelength = 3902,
		.max_framerate = 300,
		.mipi_pixel_rate = 1019660000, // OPSYCK system pixel rate
		.readout_length = 2856,
		.read_margin = 10,  // normal mode no need
		.framelength_step = 4,  // no need
		.coarse_integ_step = 4,  // no need
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 288,
			.w0_size = 8192,
			.h0_size = 5568,
			.scale_w = 4096,
			.scale_h = 2784,
			.x1_offset = 192,
			.y1_offset = 0,
			.w1_size = 3712,
			.h1_size = 2784,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3712,
			.h2_tg_size = 2784,
		},
		.pdaf_cap = TRUE,
		.ae_binning_ratio = 1000,  // Outout Pixel Level Ratio
		.fine_integ_line = 388,
		.delay_frame = 3,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
		.csi_param = {
			.cphy_settle = 74,
		},
		.support_mcss = 1,
	},
	{//mode 1
		.frame_desc = frame_desc_cap,
		.num_entries = ARRAY_SIZE(frame_desc_cap),
		.mode_setting_table = imx858_preview_setting,
		.mode_setting_len = ARRAY_SIZE(imx858_preview_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx858_seamless_preview,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx858_seamless_preview),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 878400000,  // VTPXCK system pixel rate
		.linelength = 7500,
		.framelength = 3902,
		.max_framerate = 300,
		.mipi_pixel_rate = 1019660000, // OPSYCK system pixel rate
		.readout_length = 2856,
		.read_margin = 10,  // normal mode no need
		.framelength_step = 4,  // no need
		.coarse_integ_step = 4,  // no need
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 288,
			.w0_size = 8192,
			.h0_size = 5568,
			.scale_w = 4096,
			.scale_h = 2784,
			.x1_offset = 192,
			.y1_offset = 0,
			.w1_size = 3712,
			.h1_size = 2784,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3712,
			.h2_tg_size = 2784,
		},
		.pdaf_cap = TRUE,
		.ae_binning_ratio = 1000,  // Outout Pixel Level Ratio
		.fine_integ_line = 388,
		.delay_frame = 3,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
		.csi_param = {
			.cphy_settle = 74,
		},
		.support_mcss = 1,
	},
	{//mode 2
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = imx858_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx858_normal_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 878400000,  // VTPXCK system pixel rate
		.linelength = 7500,
		.framelength = 3902,
		.max_framerate = 300,
		.mipi_pixel_rate = 1851430000, // OPSYCK system pixel rate
		.readout_length = 2856,
		.read_margin = 10,  // normal mode no need
		.framelength_step = 1,  // no need
		.coarse_integ_step = 1,  // no need
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 768,
			.w0_size = 8192,
			.h0_size = 4608,
			.scale_w = 4096,
			.scale_h = 2304,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 2304,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 2304,
		},
		.pdaf_cap = TRUE,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
		.csi_param = {
			.cphy_settle = 74,
		},
		.dpc_enabled = true, /* reg 0x0b06 */
		.support_mcss = 1,
	},
	{//mode 3
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = imx858_preview_setting,
		.mode_setting_len = ARRAY_SIZE(imx858_preview_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx858_seamless_preview,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx858_seamless_preview),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 878400000,  // VTPXCK system pixel rate
		.linelength = 7500,
		.framelength = 3902,
		.max_framerate = 300,
		.mipi_pixel_rate = 1019660000, // OPSYCK system pixel rate
		.readout_length = 2856,
		.read_margin = 10,  // normal mode no need
		.framelength_step = 4,  // no need
		.coarse_integ_step = 4,  // no need
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 288,
			.w0_size = 8192,
			.h0_size = 5568,
			.scale_w = 4096,
			.scale_h = 2784,
			.x1_offset = 192,
			.y1_offset = 0,
			.w1_size = 3712,
			.h1_size = 2784,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3712,
			.h2_tg_size = 2784,
		},
		.pdaf_cap = TRUE,
		.ae_binning_ratio = 1000,  // Outout Pixel Level Ratio
		.fine_integ_line = 388,
		.delay_frame = 3,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
		.csi_param = {
			.cphy_settle = 74,
		},
		.support_mcss = 1,
	},
	{//mode 4
		.frame_desc = frame_desc_slim_vid,
		.num_entries = ARRAY_SIZE(frame_desc_slim_vid),
		.mode_setting_table = imx858_preview_setting,
		.mode_setting_len = ARRAY_SIZE(imx858_preview_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx858_seamless_preview,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx858_seamless_preview),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 878400000,  // VTPXCK system pixel rate
		.linelength = 7500,
		.framelength = 3902,
		.max_framerate = 300,
		.mipi_pixel_rate = 1019660000, // OPSYCK system pixel rate
		.readout_length = 2856,
		.read_margin = 10,  // normal mode no need
		.framelength_step = 4,  // no need
		.coarse_integ_step = 4,  // no need
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 288,
			.w0_size = 8192,
			.h0_size = 5568,
			.scale_w = 4096,
			.scale_h = 2784,
			.x1_offset = 192,
			.y1_offset = 0,
			.w1_size = 3712,
			.h1_size = 2784,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3712,
			.h2_tg_size = 2784,
		},
		.pdaf_cap = TRUE,
		.ae_binning_ratio = 1000,  // Outout Pixel Level Ratio
		.fine_integ_line = 388,
		.delay_frame = 3,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
		.csi_param = {
			.cphy_settle = 74,
		},
		.support_mcss = 1,
	},
	{//mode 5 2DOL
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = imx858_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(imx858_custom1_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx858_seamless_custom1,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx858_seamless_custom1),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 878400000,  // VTPXCK system pixel rate
		.linelength = 5076,
		.framelength = 2876*2,
		.max_framerate = 300,
		.mipi_pixel_rate = 1019660000, // OPSYCK system pixel rate
		.readout_length = 2856*2, //(80+(Y_ADD_END-Y_ADD_STA+65))/2 =(80+(5855-288+65))/2
		.read_margin = 48, // fix value, see SRM
		.framelength_step = 4*2,
		.coarse_integ_step = 4*2,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 288,
			.w0_size = 8192,
			.h0_size = 5568,
			.scale_w = 4096,
			.scale_h = 2784,
			.x1_offset = 192,
			.y1_offset = 0,
			.w1_size = 3712,
			.h1_size = 2784,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3712,
			.h2_tg_size = 2784,
		},
		.pdaf_cap = TRUE,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 388,
		.delay_frame = 3,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
		.csi_param = {
			.cphy_settle = 78,
		},
		.support_mcss = 1,
	},
	{//mode 6 ISZ
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = imx858_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(imx858_custom2_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx858_seamless_custom2,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx858_seamless_custom2),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 878400000,  // VTPXCK system pixel rate
		.linelength = 8960, // line_length_pck
		.framelength = 3252, // frame_length_lines
		.max_framerate = 300,
		.mipi_pixel_rate = 1019660000, // OPSYCK system pixel rate
		.readout_length = 2856,
		.read_margin = 48,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 2048,
			.y0_offset = 1680,
			.w0_size = 4096,
			.h0_size = 2784,
			.scale_w = 4096,
			.scale_h = 2784,
			.x1_offset = 192,
			.y1_offset = 0,
			.w1_size = 3712,
			.h1_size = 2784,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3712,
			.h2_tg_size = 2784,
		},
		.pdaf_cap = TRUE,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 388,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 78,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_B,
		.support_mcss = 1,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = IMX858_SENSOR_ID,
	.reg_addr_sensor_id = {0x0016, 0x0017},
	.i2c_addr_table = {0x20, 0xFF}, //{0x20, 0xFF},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.eeprom_info = 0,
	.eeprom_num = 0,
	.resolution = {8192, 6144},
	.mirror = IMAGE_HV_MIRROR,

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_CPHY,
	.mipi_lane_num = SENSOR_MIPI_3_LANE,
	.ob_pedestal = 0x40,

/* This is SW workaround for physical layout mistake, bayer order should be R first*/
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,

	.ana_gain_def = BASEGAIN * 4, // hardcode
	.ana_gain_min = BASEGAIN * 2, // 3.1dB --> 10^(3.1/10) = 2.042
	.ana_gain_max = BASEGAIN * 1000, // 30dB --> 10^(30/10) = 1000
	.ana_gain_type = 0,
	.ana_gain_step = 1,
	.ana_gain_table = imx858_ana_gain_table,
	.ana_gain_table_size = sizeof(imx858_ana_gain_table),
	.tuning_iso_base = 100,
	.exposure_def = 0x3D0,
	.exposure_min = 4, // Constraints of COARSE_INTEG_TIME
	.exposure_max = 128 * (0xFFFC - 48), // Constraints of COARSE_INTEG_TIME
	.exposure_step = 1, // Constraints of COARSE_INTEG_TIME
	.exposure_margin = 48, // Constraints of COARSE_INTEG_TIME
	.dig_gain_min = BASE_DGAIN * 1, // no need (only use ana gain)
	.dig_gain_max = BASE_DGAIN * 16, // no need (only use ana gain)
	.dig_gain_step = 4, // no need (only use ana gain)

	.frame_length_max = 0xFFFC,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 3,
	.start_exposure_offset = 500000, // tuning for sensor fusion

	.pdaf_type = PDAF_SUPPORT_CAMSV_QPD, // ask vendor
	.hdr_type = HDR_SUPPORT_STAGGER_FDOL,
	.seamless_switch_support = TRUE,
	.seamless_switch_type = SEAMLESS_SWITCH_CUT_VB_INIT_SHUT,
	.seamless_switch_hw_re_init_time_ns = 2750000,
	.seamless_switch_prsh_hw_fixed_value = 48,
	.seamless_switch_prsh_length_lc = 0,
	.reg_addr_prsh_length_lines = {0x3058, 0x3059, 0x305a, 0x305b},
	.reg_addr_prsh_mode = 0x3056,

	.temperature_support = FALSE,

	.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,
	.s_cali = set_sensor_cali,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = 0x0101,
	.reg_addr_exposure = {
			{0x0202, 0x0203},
			{0x0224, 0x0225},
	},
	.long_exposure_support = TRUE,
	.reg_addr_exposure_lshift = 0x3160,
	.reg_addr_ana_gain = {
			{0x0204, 0x0205},
			{0x0216, 0x0217},
	},
	.reg_addr_dig_gain = {
			{0x020E, 0x020F},
			{0x0218, 0x0219},
	},
	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_temp_en = 0x0138,
	.reg_addr_temp_read = 0x013A,
	.reg_addr_auto_extend = 0x0350,
	.reg_addr_frame_count = 0x0005,
	.reg_addr_fast_mode = 0x3010,

	.init_setting_table = imx858_init_setting,
	.init_setting_len = ARRAY_SIZE(imx858_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 1,
	.chk_s_off_end = 0,

	.checksum_value = 0xAF3E324F,

	/* MCSS */
	.use_mcss_gph_sync = 0,
	.reg_addr_mcss_slave_add_en_2nd = 0x3020,
	.reg_addr_mcss_slave_add_acken_2nd = 0x3021,
	.reg_addr_mcss_controller_target_sel = 0x3050,
	.reg_addr_mcss_xvs_io_ctrl = 0x3030,
	.reg_addr_mcss_extout_en = 0x3051,
	.reg_addr_mcss_sgmsync_sel = 0x404B,
	.reg_addr_mcss_swdio_io_ctrl = 0x303C,
	.reg_addr_mcss_gph_sync_mode = 0x3080,
	.reg_addr_mcss_complete_sleep_en = 0x385E,
	.reg_addr_mcss_mc_frm_lp_en = 0x306D,
	.reg_addr_mcss_frm_length_reflect_timing = 0x301C,
	.reg_addr_mcss_mc_frm_mask_num = 0x306C,

	.mcss_init = imx858_mcss_init,
	.mcss_update_subdrv_para = imx858_mcss_update_subdrv_para,
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
	.mcss_set_mask_frame = imx858_mcss_set_mask_frame,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	// the sam as imx758
	{HW_ID_MCLK, {24}, 0},
	{HW_ID_RST, {0}, 0},
	{HW_ID_MCLK_DRIVING_CURRENT, {4}, 1000},
	{HW_ID_OISVDD, {2800000, 2800000}, 0}, // pmic_ldo for oisvdd
	{HW_ID_OISEN, {2800000, 2800000}, 1000}, // gpio for oisen
	{HW_ID_AVDD, {2800000, 2800000}, 0}, // pmic_ldo for avdd
	{HW_ID_AFVDD, {2800000, 2800000}, 0}, // pmic_ldo for afvdd
	{HW_ID_DOVDD, {1800000, 1800000}, 0}, // pmic_ldo/gpio(1.8V ldo) for dovdd
	{HW_ID_DVDD, {1100000,1100000}, 1000}, // pmic_ldo/gpio(1.1V ldo) for dvdd
	{HW_ID_RST, {1}, 1000}
};


const struct subdrv_entry imx858_mipi_raw_entry = {
	.name = "imx858_mipi_raw",
	.id = IMX858_SENSOR_ID,
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

	/* QSC data */
	support = info[idx].qsc_support;
	pbuf = info[idx].preload_qsc_table;
	size = info[idx].qsc_size;
	addr = info[idx].sensor_reg_addr_qsc;
	if (support) {
		if (pbuf != NULL && addr > 0 && size > 0) {
			// subdrv_i2c_wr_u8(ctx, 0x86A9, 0x4E); // no such rg RD_QSC_KNOT_VALUE_OFFSET
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			subdrv_i2c_wr_u8(ctx, 0x3206, 0x01); // QSC_EN
			DRV_LOG(ctx, "set QSC calibration data done.");
		} else {
			subdrv_i2c_wr_u8(ctx, 0x3206, 0x00);
		}
	}
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
	return (1024 - (1024 * BASEGAIN) / gain);
}

static int imx858_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	enum SENSOR_SCENARIO_ID_ENUM scenario_id;
	struct mtk_hdr_ae *ae_ctrl = NULL;
	u64 *feature_data = (u64 *)para;
	u32 exp_cnt = 0;
	enum SENSOR_SCENARIO_ID_ENUM pre_seamless_scenario_id;

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

	exp_cnt = ctx->s_ctx.mode[scenario_id].exp_cnt;
	ctx->is_seamless = TRUE;
	pre_seamless_scenario_id = ctx->current_scenario_id;
	update_mode_info(ctx, scenario_id);

	subdrv_i2c_wr_u8(ctx, 0x0104, 0x01);
	subdrv_i2c_wr_u8(ctx, 0x3010, 0x02); //FAST_MODETRANSIT_CTL
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
		mcss_get_prsh_length_lines(ctx, ae_ctrl, pre_seamless_scenario_id, scenario_id);
	}

	if (ctx->s_ctx.seamless_switch_prsh_length_lc > 0) {
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_prsh_mode, 0x01);

		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[0],
				(ctx->s_ctx.seamless_switch_prsh_length_lc >> 24) & 0x07);
		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[1],
				(ctx->s_ctx.seamless_switch_prsh_length_lc >> 16)  & 0xFF);
		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[2],
				(ctx->s_ctx.seamless_switch_prsh_length_lc >> 8) & 0xFF);
		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[3],
				(ctx->s_ctx.seamless_switch_prsh_length_lc) & 0xFF);

		DRV_LOG_MUST(ctx, "seamless switch pre-shutter set(%u)\n", ctx->s_ctx.seamless_switch_prsh_length_lc);
	} else
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_prsh_mode, 0x00);

	subdrv_i2c_wr_u8(ctx, 0x0104, 0x00);

	ctx->fast_mode_on = TRUE;
	ctx->ref_sof_cnt = ctx->sof_cnt;
	ctx->is_seamless = FALSE;
	DRV_LOG(ctx, "X: set seamless switch done\n");
	return ERROR_NONE;
}

static int imx858_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
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
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_prsh_mode, 0x00);
		set_i2c_buffer(ctx, 0x3010, 0x00);
		commit_i2c_buffer(ctx);
	}

	if ((sof_cnt == 1) && (ctx->mcss_init_info.enable_mcss)) {
		DRV_LOG(ctx, "pre-shutter disabled.");
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_prsh_mode, 0x00);
		commit_i2c_buffer(ctx);
	}
	return 0;
}

static int imx858_mcss_init(void *arg)
{
	u32 prsh_length_lc = 0;
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	if (!(ctx->mcss_init_info.enable_mcss)) {
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_mcss_slave_add_en_2nd, 0x00);
		// low-power for deep sleep
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_mcss_mc_frm_lp_en, 0x00);
		// FLL N+1/N+2
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_mcss_frm_length_reflect_timing, 0x01);  //0:N+1, 1:N+2

		memset(&(ctx->mcss_init_info), 0, sizeof(struct mtk_fsync_hw_mcss_init_info));
		DRV_LOG_MUST(ctx, "Disable MCSS\n");
		return ERROR_NONE;
	}

	// master or slave
	if (ctx->mcss_init_info.is_mcss_master) {
		DRV_LOG_MUST(ctx, "common_mcss_init controller (ctx->s_ctx.sensor_id=0x%x)\n",ctx->s_ctx.sensor_id);
		subdrv_i2c_wr_u8(ctx,
			ctx->s_ctx.reg_addr_mcss_slave_add_en_2nd, 0x01);
		subdrv_i2c_wr_u8(ctx,
			ctx->s_ctx.reg_addr_mcss_slave_add_acken_2nd, 0x01);
		subdrv_i2c_wr_u8(ctx,
			ctx->s_ctx.reg_addr_mcss_controller_target_sel, 0x01); // controller mode is default
		subdrv_i2c_wr_u8(ctx,
			ctx->s_ctx.reg_addr_mcss_xvs_io_ctrl, 0x01);
		subdrv_i2c_wr_u8(ctx,
			ctx->s_ctx.reg_addr_mcss_extout_en, 0x01);
	} else {
		DRV_LOG_MUST(ctx, "common_mcss_init target (ctx->s_ctx.sensor_id=0x%x)\n",ctx->s_ctx.sensor_id);
		subdrv_i2c_wr_u8(ctx,
			ctx->s_ctx.reg_addr_mcss_slave_add_en_2nd, 0x01);
		subdrv_i2c_wr_u8(ctx,
			ctx->s_ctx.reg_addr_mcss_slave_add_acken_2nd, 0x00);
		subdrv_i2c_wr_u8(ctx,
			ctx->s_ctx.reg_addr_mcss_controller_target_sel, 0x00);
		subdrv_i2c_wr_u8(ctx,
			ctx->s_ctx.reg_addr_mcss_xvs_io_ctrl, 0x00);
		subdrv_i2c_wr_u8(ctx,
			ctx->s_ctx.reg_addr_mcss_extout_en, 0x00);
	}

	// low-power for deep sleep default enable
	subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_mcss_mc_frm_lp_en, 0x01);

	// FLL N+1/N+2
	subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_mcss_frm_length_reflect_timing, 0x00);  //0:N+1, 1:N+2

	// pre-shutter
	/* TODO How to find the pre-shutter length for slave? */
	prsh_length_lc = ((33000)
				* ctx->s_ctx.mode[ctx->current_scenario_id].pclk
				/ ctx->s_ctx.mode[ctx->current_scenario_id].linelength
				/ 1000000);

	DRV_LOG_MUST(ctx, "mcss slave using pre-shutter %llu/%u/%u\n",
				ctx->s_ctx.mode[ctx->current_scenario_id].pclk,
				ctx->s_ctx.mode[ctx->current_scenario_id].linelength,
				prsh_length_lc);

	subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_prsh_mode, 0x01);

	if (ctx->s_ctx.reg_addr_prsh_length_lines.addr[3]) {
		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[0],
				(prsh_length_lc >> 24) & 0x07);
		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[1],
				(prsh_length_lc >> 16) & 0xFF);
		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[2],
				(prsh_length_lc >> 8)  & 0xFF);
		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[3],
				(prsh_length_lc) & 0xFF);
	} else {
		subdrv_i2c_wr_u8(ctx,
			ctx->s_ctx.reg_addr_prsh_length_lines.addr[0],
			(prsh_length_lc >> 16) & 0xFF);
		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[1],
				(prsh_length_lc >> 8)  & 0xFF);
		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[2],
				(prsh_length_lc) & 0xFF);
	}

	DRV_LOG_MUST(ctx, "common_mcss_init pre-shutter set(%u)\n", prsh_length_lc);


	return ERROR_NONE;
}

static int imx858_mcss_update_subdrv_para(void *arg, int scenario_id)
{
	int i;
	u32 old_line_length;
	u64 origin_den;
	u64 origin_num;
	u64 new_den;
	u64 new_num;
	u64 update_linelength;
	u64 tmp1,tmp2;
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	u32 framerate = ctx->s_ctx.mode[scenario_id].max_framerate;

	if (ctx->s_ctx.sensor_mode_num <= scenario_id)
		return -EINVAL;

	if (!(ctx->mcss_init_info.enable_mcss))
		return ERROR_NONE;

	old_line_length = ctx->line_length;
	origin_den = ctx->s_ctx.mode[scenario_id].pclk / 1000000;
	origin_num = ctx->s_ctx.mode[scenario_id].linelength;
	tmp1 = (u64)ctx->s_ctx.mode[scenario_id].linelength * (u64)ctx->s_ctx.mclk;
	tmp2= ctx->s_ctx.mode[scenario_id].pclk / 1000000;
	new_num = 1+(tmp1/tmp2); // line_length_inck = round_up(tmp1, tmp2)
	new_den = ctx->s_ctx.mclk;
	update_linelength =  ctx->s_ctx.mode[scenario_id].linelength * (new_num*origin_den)/(new_den*origin_num);

	ctx->line_length = update_linelength;
	ctx->frame_length = ctx->s_ctx.mode[scenario_id].pclk / framerate * 10 / ctx->line_length;
	ctx->current_fps = ctx->pclk / ctx->frame_length * 10 / ctx->line_length;
	ctx->min_frame_length = ctx->frame_length;

	ctx->s_ctx.frame_time_delay_frame = 2;
	ctx->frame_time_delay_frame = 2;
	for (i = 0; i < ctx->s_ctx.sensor_mode_num; i++)
		ctx->s_ctx.mode[i].delay_frame = 2;

	DRV_LOG_MUST(ctx,
			"ctx->s_ctx.mode[%d].pclk:%llu ctx->s_ctx.mode[].linelength:%u,%llu/%llu/%llu/%llu, tmp:%llu/%llu, update_linelength:%llu update CALC_LINE_TIME_IN_NS=%u\n",
					scenario_id,
					ctx->s_ctx.mode[scenario_id].pclk,ctx->s_ctx.mode[scenario_id].linelength,
					origin_den, origin_num, new_den, new_num, tmp1,tmp2,update_linelength,
	CALC_LINE_TIME_IN_NS(ctx->s_ctx.mode[scenario_id].pclk, ctx->s_ctx.mode[scenario_id].linelength) );

	return ERROR_NONE;
}

static int imx858_get_sensor_sync_mode(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 *feature_data = (u32 *)para;

	if (!(ctx->mcss_init_info.enable_mcss))
		return ERROR_NONE;

	*feature_data = ctx->mcss_init_info.is_mcss_master? 1 : 2; // sync operate mode. none/master/slave
	(*(feature_data + 1)) = FS_HW_SYNC_GROUP_ID_MCSS;   // hw sync group ID
	(*(feature_data + 2)) = 1;     // legacy:0, MCSS:1
	return ERROR_NONE;
}

static int imx858_mcss_set_mask_frame(struct subdrv_ctx *ctx, u32 num, u32 is_critical)
{
	ctx->s_ctx.s_gph((void *)ctx, 1);
	set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_mcss_mc_frm_mask_num,  (0x7f & num));
	ctx->s_ctx.s_gph((void *)ctx, 0);

	if (is_critical)
		commit_i2c_buffer(ctx);

	DRV_LOG(ctx, "set mask frame num:%d\n", (0x7f & num));
	return 0;
}
