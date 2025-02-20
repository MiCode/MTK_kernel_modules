// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 s5k3p9spmipiraw_Sensor.c
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
#include "s5k3p9spmipiraw_Sensor.h"

#define USING_DPHY_N_LANE 4

static void set_group_hold(void *arg, u8 en);
static u16 get_gain2reg(u32 gain);
static int s5k3p9sp_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int s5k3p9sp_set_test_pattern_data(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static void s5k3p9sp_sensor_init(struct subdrv_ctx *ctx);
static int open(struct subdrv_ctx *ctx);
static int s5k3p9sp_set_ctrl_locker(struct subdrv_ctx *ctx, u32 cid, bool *is_lock);

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, s5k3p9sp_set_test_pattern},
	{SENSOR_FEATURE_SET_TEST_PATTERN_DATA, s5k3p9sp_set_test_pattern_data},
};

static struct eeprom_info_struct eeprom_info[] = {
	{
		.header_id = 0x010B00FF,
		.addr_header_id = 0x00000001,
		.i2c_write_id = 0xA0,

		.xtalk_support = TRUE,
		.xtalk_size = 2048,
		.addr_xtalk = 0x150F,
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0910,
			.vsize = 0x06d0,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1220,
			.vsize = 0x0da0,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0910,
			.vsize = 0x051C,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_hs_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0780,
			.vsize = 0x0438,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0280,
			.vsize = 0x01e0,
		},
	},
};

#if  USING_DPHY_N_LANE == 1
static struct mtk_mbus_frame_desc_entry frame_desc_cust1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x780,
			.vsize = 0x438,
		},
	},
};

#elif USING_DPHY_N_LANE == 2
static struct mtk_mbus_frame_desc_entry frame_desc_cust1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x910,
			.vsize = 0x6d0,
		},
	},
};
#endif

static struct mtk_mbus_frame_desc_entry frame_desc_cust2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0280,
			.vsize = 0x01e0,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cust3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0280,
			.vsize = 0x01e0,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cust4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0280,
			.vsize = 0x01e0,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cust5[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0140,
			.vsize = 0x00F0,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cust6[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0140,
			.vsize = 0x00F0,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cust7[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0140,
			.vsize = 0x00F0,
		},
	},
};

static struct subdrv_mode_struct mode_struct[] = {
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = addr_data_pair_preview,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_preview),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560000000,
		.linelength = 7152,
		.framelength = 2608,
		.max_framerate = 300,
		.mipi_pixel_rate = 269400000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 4640,
			.h0_size = 3488,
			.scale_w = 2320,
			.scale_h = 1744,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2320,
			.h1_size = 1744,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2320,
			.h2_tg_size = 1744,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{
		.frame_desc = frame_desc_cap,
		.num_entries = ARRAY_SIZE(frame_desc_cap),
		.mode_setting_table = addr_data_pair_capture,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_capture),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560000000,
		.linelength = 5088,
		.framelength = 3668,
		.max_framerate = 300,
		.mipi_pixel_rate = 586000000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 4640,
			.h0_size = 3488,
			.scale_w = 4640,
			.scale_h = 3488,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4640,
			.h1_size = 3488,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4640,
			.h2_tg_size = 3488,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = addr_data_pair_normal_video,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_normal_video),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560000000,
		.linelength = 7152,
		.framelength = 2608,
		.max_framerate = 300,
		.mipi_pixel_rate = 269400000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 0,
			.y0_offset = 436,
			.w0_size = 4640,
			.h0_size = 2616,
			.scale_w = 2320,
			.scale_h = 1308,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2320,
			.h1_size = 1308,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2320,
			.h2_tg_size = 1308,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = addr_data_pair_hs_video,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_hs_video),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560000000,
		.linelength = 12960,
		.framelength = 1440,
		.max_framerate = 300,
		.mipi_pixel_rate = 216666667,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 400,
			.y0_offset = 664,
			.w0_size = 3840,
			.h0_size = 2160,
			.scale_w = 1920,
			.scale_h = 1080,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 1920,
			.h1_size = 1080,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1920,
			.h2_tg_size = 1080,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{
		.frame_desc = frame_desc_slim_vid,
		.num_entries = ARRAY_SIZE(frame_desc_slim_vid),
		.mode_setting_table = addr_data_pair_slim_video,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_slim_video),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560000000,
		.linelength = 5088,
		.framelength = 917,
		.max_framerate = 1200,
		.mipi_pixel_rate = 278400000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 1024,
			.y0_offset = 784,
			.w0_size = 2592,
			.h0_size = 1920,
			.scale_w = 1296,
			.scale_h = 960,
			.x1_offset = 328,
			.y1_offset = 240,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},

#if USING_DPHY_N_LANE == 1
	{
		.frame_desc = frame_desc_cust1,
		.num_entries = ARRAY_SIZE(frame_desc_cust1),
		.mode_setting_table = addr_data_pair_cust1,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_cust1),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560000000,
		.linelength = 20352,
		.framelength = 1158,
		.max_framerate = 237,
		.mipi_pixel_rate = 139200000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 400,
			.y0_offset = 664,
			.w0_size = 3840,
			.h0_size = 2160,
			.scale_w = 1920,
			.scale_h = 1080,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 1920,
			.h1_size = 1080,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1920,
			.h2_tg_size = 1080,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
#elif USING_DPHY_N_LANE == 2
	{
		.frame_desc = frame_desc_cust1,
		.num_entries = ARRAY_SIZE(frame_desc_cust1),
		.mode_setting_table = addr_data_pair_cust1_2_lane,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_cust1_2_lane),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560000000,
		.linelength = 5088,
		.framelength = 1834,
		.max_framerate = 300,
		.mipi_pixel_rate = 210000000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 2320,
			.h0_size = 1744,
			.scale_w = 2320,
			.scale_h = 1744,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2320,
			.h1_size = 1744,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2320,
			.h2_tg_size = 1744,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
#endif
	{
		.frame_desc = frame_desc_cust2,
		.num_entries = ARRAY_SIZE(frame_desc_cust2),
		.mode_setting_table = addr_data_pair_cust2,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_cust2),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 555286000,
		.linelength =10036,
		.framelength = 5520,
		.max_framerate = 100,
		.mipi_pixel_rate = 215800000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 1024,
			.y0_offset = 784,
			.w0_size = 2592,
			.h0_size = 1920,
			.scale_w = 1296,
			.scale_h = 960,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{
		.frame_desc = frame_desc_cust3,
		.num_entries = ARRAY_SIZE(frame_desc_cust3),
		.mode_setting_table = addr_data_pair_cust3,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_cust3),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 555286000,
		.linelength = 10036,
		.framelength = 11040,
		.max_framerate = 50,
		.mipi_pixel_rate = 215800000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 1024,
			.y0_offset = 784,
			.w0_size = 2592,
			.h0_size = 1920,
			.scale_w = 1296,
			.scale_h = 960,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{
		.frame_desc = frame_desc_cust4,
		.num_entries = ARRAY_SIZE(frame_desc_cust4),
		.mode_setting_table = addr_data_pair_cust4,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_cust4),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 555286000,
		.linelength = 10036,
		.framelength = 27600,
		.max_framerate = 20,
		.mipi_pixel_rate = 215800000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 1024,
			.y0_offset = 784,
			.w0_size = 2592,
			.h0_size = 1920,
			.scale_w = 1296,
			.scale_h = 960,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{
		.frame_desc = frame_desc_cust5,
		.num_entries = ARRAY_SIZE(frame_desc_cust5),
		.mode_setting_table = addr_data_pair_cust5,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_cust5),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 555286000,
		.linelength = 10036,
		.framelength = 5520,
		.max_framerate = 100,
		.mipi_pixel_rate = 215800000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 1024,
			.y0_offset = 784,
			.w0_size = 2592,
			.h0_size = 1920,
			.scale_w = 648,
			.scale_h = 480,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 320,
			.h1_size = 240,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 320,
			.h2_tg_size = 240,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{
		.frame_desc = frame_desc_cust6,
		.num_entries = ARRAY_SIZE(frame_desc_cust6),
		.mode_setting_table = addr_data_pair_cust6,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_cust6),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 555286000,
		.linelength = 10036,
		.framelength = 11040,
		.max_framerate = 50,
		.mipi_pixel_rate = 215800000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 1024,
			.y0_offset = 784,
			.w0_size = 2592,
			.h0_size = 1920,
			.scale_w = 648,
			.scale_h = 480,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 320,
			.h1_size = 240,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 320,
			.h2_tg_size = 240,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{
		.frame_desc = frame_desc_cust7,
		.num_entries = ARRAY_SIZE(frame_desc_cust7),
		.mode_setting_table = addr_data_pair_cust7,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_cust7),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 555286000,
		.linelength = 10036,
		.framelength = 27600,
		.max_framerate = 20,
		.mipi_pixel_rate = 215800000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = 4640,
			.full_h = 3488,
			.x0_offset = 1024,
			.y0_offset = 784,
			.w0_size = 2592,
			.h0_size = 1920,
			.scale_w = 648,
			.scale_h = 480,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 320,
			.h1_size = 240,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 320,
			.h2_tg_size = 240,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 4,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = S5K3P9SP_SENSOR_ID,
	.reg_addr_sensor_id = {0x0000, 0x0001},
	.i2c_addr_table = {0x20, 0xFF},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_16,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {4640, 3488},
	.mirror = IMAGE_NORMAL,

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_2MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
#if USING_DPHY_N_LANE == 1
	.mipi_lane_num = SENSOR_MIPI_1_LANE,
#elif USING_DPHY_N_LANE == 2
	.mipi_lane_num = SENSOR_MIPI_2_LANE,
#else
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
#endif
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_BAYER_Gr,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 16,
	.ana_gain_type = 2,
	.ana_gain_step = 32,
	.ana_gain_table = s5k3p9sp_ana_gain_table,
	.ana_gain_table_size = sizeof(s5k3p9sp_ana_gain_table),
	.min_gain_iso = 100,
	.exposure_def = 0x3D0,
	.exposure_min = 3,
	.exposure_max = 0xFFFF - 3,
	.exposure_step = 1,
	.exposure_margin = 3,

	.frame_length_max = 0xFFFF,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 500000,
#ifdef IMGSENSOR_FUSION_TEST_WORKAROUND
	.start_exposure_offset_custom = 5500000,
#endif
	.pdaf_type = PDAF_SUPPORT_NA,
	.hdr_type = HDR_SUPPORT_NA,
	.seamless_switch_support = FALSE,
	.temperature_support = FALSE,
	.g_temp = PARAM_UNDEFINED,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = 0x0101,
	.reg_addr_exposure = {{0x0202, 0x0203},},
	.long_exposure_support = FALSE,
	.reg_addr_exposure_lshift = PARAM_UNDEFINED,
	.reg_addr_ana_gain = {{0x0204, 0x0205},},
	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_temp_en = PARAM_UNDEFINED,
	.reg_addr_temp_read = PARAM_UNDEFINED,
	.reg_addr_auto_extend = PARAM_UNDEFINED,
	.reg_addr_frame_count = 0x0005,

	.init_setting_table = PARAM_UNDEFINED,
	.init_setting_len = PARAM_UNDEFINED,
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 1,
	.chk_s_off_end = 1,

	.checksum_value = 0x31E3FBE2,

	/* custom stream control to mipi delay time for hw limitation */
	.custom_stream_ctrl_delay = TRUE,
};

static struct subdrv_ops ops = {
	.get_id = common_get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = open,
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = common_control,
	.feature_control = common_feature_control,
	.close = common_close,
	.get_frame_desc = common_get_frame_desc,
	.get_csi_param = common_get_csi_param,
	.update_sof_cnt = common_update_sof_cnt,
	.set_ctrl_locker = s5k3p9sp_set_ctrl_locker,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_MCLK, 24, 0},
	{HW_ID_RST, 0, 1},
	{HW_ID_DVDD, 1100000, 1}, // pmic_ldo for dvdd
	{HW_ID_DVDD1, 1800000, 1}, // pmic_gpo(1.1V ldo) for dvdd
	{HW_ID_AVDD, 2800000, 1}, // pmic_ldo for avdd
	{HW_ID_DOVDD, 1800000, 3}, // pmic_ldo/gpio(1.8V ldo) for dovdd
	{HW_ID_MCLK_DRIVING_CURRENT, 2, 0},
	{HW_ID_RST, 1, 2}
};

const struct subdrv_entry s5k3p9sp_mipi_raw_entry = {
	.name = "s5k3p9sp_mipi_raw",
	.id = S5K3P9SP_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

/* FUNCTION */

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
	return gain * 32 / BASEGAIN;
}

static int s5k3p9sp_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	if (mode)
		subdrv_i2c_wr_u16(ctx, 0x0600, mode); /*100% Color bar*/
	else if (ctx->test_pattern)
		subdrv_i2c_wr_u16(ctx, 0x0600, 0x0000); /*No pattern*/

	ctx->test_pattern = mode;
	return ERROR_NONE;
}

static int s5k3p9sp_set_test_pattern_data(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	struct mtk_test_pattern_data *data = (struct mtk_test_pattern_data *)para;
	u16 R = (data->Channel_R >> 22) & 0x3ff;
	u16 Gr = (data->Channel_Gr >> 22) & 0x3ff;
	u16 Gb = (data->Channel_Gb >> 22) & 0x3ff;
	u16 B = (data->Channel_B >> 22) & 0x3ff;

	subdrv_i2c_wr_u16(ctx, 0x0602, R);
	subdrv_i2c_wr_u16(ctx, 0x0604, Gr);
	subdrv_i2c_wr_u16(ctx, 0x0606, B);
	subdrv_i2c_wr_u16(ctx, 0x0608, Gb);

	DRV_LOG(ctx, "mode(%u) R/Gr/Gb/B = 0x%04x/0x%04x/0x%04x/0x%04x\n",
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

static void s5k3p9sp_sensor_init(struct subdrv_ctx *ctx)
{
	DRV_LOG(ctx, "E\n");
#if USING_DPHY_N_LANE == 4
	DRV_LOG(ctx, "Dphy-4lane setting\n");
	subdrv_i2c_wr_u16(ctx, 0x6028, 0x4000);
	subdrv_i2c_wr_u16(ctx, 0x6010, 0x0001);
	mdelay(3);
	subdrv_i2c_wr_u16(ctx, 0x6214, 0x7970);
	subdrv_i2c_wr_u16(ctx, 0x6218, 0x7150);
	subdrv_i2c_wr_u16(ctx, 0x0A02, 0x007E);
	subdrv_i2c_wr_u16(ctx, 0x6028, 0x4000); //TNP burst start
	subdrv_i2c_wr_u16(ctx, 0x6004, 0x0001);
	subdrv_i2c_wr_u16(ctx, 0x6028, 0x2000);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x3F4C);

	subdrv_i2c_wr_p8(ctx, 0x6F12, (u8 *)uTnpArrayInit, sizeof(uTnpArrayInit));

	subdrv_i2c_wr_u16(ctx, 0x6028, 0x4000);
	subdrv_i2c_wr_u16(ctx, 0x6004, 0x0000); //TNP burst end
	subdrv_i2c_wr_u16(ctx, 0x6028, 0x2000); // global
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x16F0);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x2929);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x16F2);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x2929);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x16FA);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0029);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x16FC);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0029);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1708);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0029);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x170A);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0029);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1712);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x2929);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1714);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x2929);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1716);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x2929);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1722);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x152A);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1724);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x152A);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x172C);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x002A);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x172E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x002A);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1736);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x1500);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1738);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x1500);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1740);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x152A);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1742);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x152A);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x16BE);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x1515);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x1515);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x16C8);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0029);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0029);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x16D6);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0015);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0015);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x16E0);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x2929);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x2929);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x2929);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x19B8);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0100);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x2224);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0100);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x0DF8);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x1001);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1EDA);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x16A0);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x3D09);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x10A8);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1198);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x002B);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1002);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0001);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x0F70);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0101);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x002F);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x007F);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0030);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0080);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000B);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0009);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0xF46E);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x0FAA);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x000D);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0003);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0xF464);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x1698);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0D05);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x20A0);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0001);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0203);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x4A74);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0101);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x1F80);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0x602A, 0x0FF4);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x0100);
	subdrv_i2c_wr_u16(ctx, 0x6F12, 0x1800);
	subdrv_i2c_wr_u16(ctx, 0x6028, 0x4000);
	subdrv_i2c_wr_u16(ctx, 0x0FEA, 0x1440);
	subdrv_i2c_wr_u16(ctx, 0x0B06, 0x0101);
	subdrv_i2c_wr_u16(ctx, 0xF44A, 0x0007);
	subdrv_i2c_wr_u16(ctx, 0xF456, 0x000A);
	subdrv_i2c_wr_u16(ctx, 0xF46A, 0xBFA0);
	subdrv_i2c_wr_u16(ctx, 0x0D80, 0x1388);
	subdrv_i2c_wr_u16(ctx, 0xB134, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0xB136, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0xB138, 0x0000);
#else
	DRV_LOG(ctx, "Dphy-2lane setting\n");
	subdrv_i2c_wr_u16(ctx, 0x6028, 0x4000);
	subdrv_i2c_wr_u16(ctx, 0x0000, 0x1000);
	subdrv_i2c_wr_u16(ctx, 0x0000, 0x3109);
	subdrv_i2c_wr_u16(ctx, 0x6010, 0x0001);

	mdelay(3);
	i2c_table_write(ctx, uTnpArrayInit_1lane,
			ARRAY_SIZE(uTnpArrayInit_1lane));


#endif
	DRV_LOG(ctx, "X\n");
}

static int open(struct subdrv_ctx *ctx)
{
	u32 sensor_id = 0;
	u32 scenario_id = 0;

	/* get sensor id */
	if (common_get_imgsensor_id(ctx, &sensor_id) != ERROR_NONE)
		return ERROR_SENSOR_CONNECT_FAIL;

	/* initail setting */
	s5k3p9sp_sensor_init(ctx);

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
} /* open */

static int s5k3p9sp_set_ctrl_locker(struct subdrv_ctx *ctx,
		u32 cid, bool *is_lock)
{
	bool lock_set_ctrl = false;

	if (unlikely(is_lock == NULL)) {
		pr_info("[%s][ERROR] is_lock %p is NULL\n", __func__, is_lock);
		return -EINVAL;
	}

	switch (cid) {
	case V4L2_CID_MTK_STAGGER_AE_CTRL:
	case V4L2_CID_MTK_MAX_FPS:
		if ((ctx->sof_no == 0) && (ctx->is_streaming)) {
			lock_set_ctrl = true;
			DRV_LOG(ctx,
				"[%s] Target lock cid(%u) lock_set_ctrl(%d), sof_no(%d) is_streaming(%d)\n",
				__func__,
				cid,
				lock_set_ctrl,
				ctx->sof_no,
				ctx->is_streaming);
		}
		break;
	default:
		break;
	}

	*is_lock = lock_set_ctrl;
	return ERROR_NONE;
} /* s5k3p9sp_set_ctrl_locker */
