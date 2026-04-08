// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 imx866mipiraw_Sensor.c
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
#include "imx866mipiraw_Sensor.h"

static int get_sensor_temperature(void *arg);
static void set_group_hold(void *arg, u8 en);
static u16 get_gain2reg(u32 gain);
static int imx866_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int imx866_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, imx866_set_test_pattern},
	{SENSOR_FEATURE_SEAMLESS_SWITCH, imx866_seamless_switch},
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 0,
	.i4OffsetY = 0,
	.i4PitchX = 0,
	.i4PitchY = 0,
	.i4PairNum = 0,
	.i4SubBlkW = 0,
	.i4SubBlkH = 0,
	.i4PosL = {{0, 0} },
	.i4PosR = {{0, 0} },
	.i4BlockNumX = 0,
	.i4BlockNumY = 0,
	.i4LeFirst = 0,
	.i4Crop = {
		{0, 0}, {0, 0}, {0, 356}, {0, 356}, {0, 0},
		{0, 0}, {0, 356}, {0, 0}, {0, 356}, {0, 356}
	},  //{0, 1632}
	.iMirrorFlip = 3,
};

static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x2b,
			.hsize = 0x400,
			.vsize = 0x5F0,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x2b,
			.hsize = 0x400,
			.vsize = 0x5F0,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x2b,
			.hsize = 0x400,
			.vsize = 0x480,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_hs_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0800,
			.vsize = 0x0480,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x2b,
			.hsize = 0x0400,
			.vsize = 0x0240,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x2b,
			.hsize = 0x400,
			.vsize = 0x5f0,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_RAW_NE_W_DATA,
			.is_active_line = TRUE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x2b,
			.hsize = 0x400,
			.vsize = 0x5F0,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 5,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_RAW_NE_W_DATA,
			.is_active_line = TRUE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x2b,
			.hsize = 0x400,
			.vsize = 0x480,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 5,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_RAW_NE_W_DATA,
			.is_active_line = TRUE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_ME,
			.is_active_line = TRUE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 6,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_RAW_ME_W_DATA,
			.is_active_line = TRUE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x2b,
			.hsize = 0x400,
			.vsize = 0x480,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 4,
			.data_type = 0x2b,
			.hsize = 0x400,
			.vsize = 0x480,
			.user_data_desc = VC_PDAF_STATS_ME_PIX_1,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_ME,
			.is_active_line = TRUE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x2b,
			.hsize = 0x400,
			.vsize = 0x480,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 4,
			.data_type = 0x2b,
			.hsize = 0x400,
			.vsize = 0x480,
			.user_data_desc = VC_PDAF_STATS_ME_PIX_1,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus5[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1e00,
			.vsize = 0x10e0,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x2b,
			.hsize = 0x3c0,
			.vsize = 0x430,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus6[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x2000,
			.vsize = 0x1800,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},

	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x2b,
			.hsize = 0x400,
			.vsize = 0x5f0,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct subdrv_mode_struct mode_struct[] = {
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = imx866_preview_setting,
		.mode_setting_len = ARRAY_SIZE(imx866_preview_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1440000000,
		.linelength = 11536,
		.framelength = 4160,
		.max_framerate = 300,
		.mipi_pixel_rate = 3085710000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 4,
		.imgsensor_winsize_info = {
			.full_w = 8768,
			.full_h = 6144,
			.x0_offset = 288,
			.y0_offset = 0,
			.w0_size = 8192,
			.h0_size = 6144,
			.scale_w = 4096,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 3072,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 3072,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.rgbw_output_mode = IMGSENSOR_RGBW_HW_BAYER,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true, /* reg 0x0b06 */
	},
	{
		.frame_desc = frame_desc_cap,
		.num_entries = ARRAY_SIZE(frame_desc_cap),
		.mode_setting_table = imx866_capture_setting,
		.mode_setting_len = ARRAY_SIZE(imx866_capture_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1440000000,
		.linelength = 11536,
		.framelength = 4160,
		.max_framerate = 300,
		.mipi_pixel_rate = 3085710000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 4,
		.imgsensor_winsize_info = {
			.full_w = 8768,
			.full_h = 6144,
			.x0_offset = 288,
			.y0_offset = 0,
			.w0_size = 8192,
			.h0_size = 6144,
			.scale_w = 4096,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 3072,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 3072,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.rgbw_output_mode = IMGSENSOR_RGBW_HW_BAYER,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = imx866_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx866_normal_video_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx866_seamless_normal_video,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx866_seamless_normal_video),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1648000000,
		.linelength = 11536,
		.framelength = 4761,
		.max_framerate = 300,
		.mipi_pixel_rate = 3085710000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 4,
		.imgsensor_winsize_info = {
			.full_w = 8768,
			.full_h = 6144,
			.x0_offset = 288,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.rgbw_output_mode = IMGSENSOR_RGBW_HW_BAYER,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = imx866_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx866_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1440000000,
		.linelength = 7008,
		.framelength = 1712,
		.max_framerate = 1200,
		.mipi_pixel_rate = 3085710000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 4,
		.imgsensor_winsize_info = {
			.full_w = 8768,
			.full_h = 6144,
			.x0_offset = 288,
			.y0_offset = 768,
			.w0_size = 8192,
			.h0_size = 4608,
			.scale_w = 2048,
			.scale_h = 1152,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2048,
			.h1_size = 1152,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2048,
			.h2_tg_size = 1152,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 876,
		.delay_frame = 3,
		.rgbw_output_mode = IMGSENSOR_RGBW_HW_BAYER,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_slim_vid,
		.num_entries = ARRAY_SIZE(frame_desc_slim_vid),
		.mode_setting_table = imx866_slim_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx866_slim_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1440000000,
		.linelength = 11536,
		.framelength = 4160,
		.max_framerate = 300,
		.mipi_pixel_rate = 3085710000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 4,
		.imgsensor_winsize_info = {
			.full_w = 8768,
			.full_h = 6144,
			.x0_offset = 288,
			.y0_offset = 0,
			.w0_size = 8192,
			.h0_size = 6144,
			.scale_w = 4096,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 3072,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 3072,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.rgbw_output_mode = IMGSENSOR_RGBW_HW_BAYER,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = imx866_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(imx866_custom1_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1440000000,
		.linelength = 11536,
		.framelength = 4160,
		.max_framerate = 300,
		.mipi_pixel_rate = 3085710000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 4,
		.imgsensor_winsize_info = {
			.full_w = 8768,
			.full_h = 6144,
			.x0_offset = 288,
			.y0_offset = 0,
			.w0_size = 8192,
			.h0_size = 6144,
			.scale_w = 4096,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 3072,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 3072,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.rgbw_output_mode = IMGSENSOR_RGBW_TWO_PLANE,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = imx866_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(imx866_custom2_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx866_seamless_custom2,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx866_seamless_custom2),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1648000000,
		.linelength = 11536,
		.framelength = 4761,
		.max_framerate = 300,
		.mipi_pixel_rate = 3085710000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 4,
		.imgsensor_winsize_info = {
			.full_w = 8768,
			.full_h = 6144,
			.x0_offset = 288,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.rgbw_output_mode = IMGSENSOR_RGBW_TWO_PLANE,
		.csi_param = {
			.cphy_settle = 65,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = imx866_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(imx866_custom3_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx866_seamless_custom3,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx866_seamless_custom3),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 1648000000,
		.linelength = 11536,
		.framelength = 4760,
		.max_framerate = 300,
		.mipi_pixel_rate = 3085710000,
		.readout_length = 580 * 2,
		.read_margin = 10 * 2,
		.framelength_step = 4 * 2,
		.coarse_integ_step = 4 * 2,
		.min_exposure_line = 4 * 2,
		.imgsensor_winsize_info = {
			.full_w = 8768,
			.full_h = 6144,
			.x0_offset = 288,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.rgbw_output_mode = IMGSENSOR_RGBW_TWO_PLANE,
		.csi_param = {
			.cphy_settle = 69,
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = imx866_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(imx866_custom4_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx866_seamless_custom4,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx866_seamless_custom4),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 1648000000,
		.linelength = 11536,
		.framelength = 4760,
		.max_framerate = 300,
		.mipi_pixel_rate = 3085710000,
		.readout_length = 580 * 2,
		.read_margin = 10 * 2,
		.framelength_step = 4 * 2,
		.coarse_integ_step = 4 * 2,
		.min_exposure_line = 4 * 2,
		.imgsensor_winsize_info = {
			.full_w = 8768,
			.full_h = 6144,
			.x0_offset = 288,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.rgbw_output_mode = IMGSENSOR_RGBW_HW_BAYER,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = imx866_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(imx866_custom5_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1544000000,
		.linelength = 11536,
		.framelength = 4461,
		.max_framerate = 300,
		.mipi_pixel_rate = 3085710000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 4,
		.imgsensor_winsize_info = {
			.full_w = 8768,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 912,
			.w0_size = 8768,
			.h0_size = 4320,
			.scale_w = 8768,
			.scale_h = 4320,
			.x1_offset = 544,
			.y1_offset = 0,
			.w1_size = 7680,
			.h1_size = 4320,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 7680,
			.h2_tg_size = 4320,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.rgbw_output_mode = IMGSENSOR_NORMAL_BAYER,
		.csi_param = {
			.cphy_settle = 69,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus6,
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.mode_setting_table = imx866_custom6_setting,
		.mode_setting_len = ARRAY_SIZE(imx866_custom6_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 2184000000,
		.linelength = 11536,
		.framelength = 6310,
		.max_framerate = 300,
		.mipi_pixel_rate = 3085710000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 4,
		.imgsensor_winsize_info = {
			.full_w = 8768,
			.full_h = 6144,
			.x0_offset = 288,
			.y0_offset = 0,
			.w0_size = 8192,
			.h0_size = 6144,
			.scale_w = 8192,
			.scale_h = 6144,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 8192,
			.h1_size = 6144,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 8192,
			.h2_tg_size = 6144,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.rgbw_output_mode = IMGSENSOR_RGBW_HW_BAYER,
		.csi_param = {
			.cphy_settle = 34,
		},
		.dpc_enabled = true,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = IMX866_SENSOR_ID,
	.reg_addr_sensor_id = {0x0016, 0x0017},
	.i2c_addr_table = {0x34, 0xff},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.resolution = {8768, 6144},
	.mirror = IMAGE_HV_MIRROR,

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_CPHY,
	.mipi_lane_num = SENSOR_MIPI_3_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 64,
	.ana_gain_type = 0,
	.ana_gain_step = 1,
	.ana_gain_table = imx866_ana_gain_table,
	.ana_gain_table_size = sizeof(imx866_ana_gain_table),
	.tuning_iso_base = 100,
	.exposure_def = 0x3D0,
	.exposure_min = 24,
	.exposure_max = 128 * (0xFFFC - 48),
	.exposure_step = 4,
	.exposure_margin = 48,

	.frame_length_max = 0xFFFC,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 3,
	.start_exposure_offset = 3000000,

	.pdaf_type = PDAF_SUPPORT_CAMSV_QPD,
	.hdr_type = HDR_SUPPORT_STAGGER_FDOL,
	.rgbw_support = 1,
	.seamless_switch_support = TRUE,
	.temperature_support = TRUE,

	.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = 0x0101,
	.reg_addr_exposure = {
			{0x0202, 0x0203},
			{0x313A, 0x313B},
			{0x0224, 0x0225},
	},
	.long_exposure_support = TRUE,
	.reg_addr_exposure_lshift = 0x3150,
	.reg_addr_ana_gain = {
			{0x0204, 0x0205},
			{0x313C, 0x313D},
			{0x0216, 0x0217},
	},
	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_temp_en = 0x0138,
	.reg_addr_temp_read = 0x013A,
	.reg_addr_auto_extend = 0x0350,
	.reg_addr_frame_count = 0x0005,
	.reg_addr_fast_mode = 0x3010,

	.init_setting_table = imx866_init_setting,
	.init_setting_len = ARRAY_SIZE(imx866_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 1,
	.chk_s_off_end = 0,

	.checksum_value = 0x8ac2d94a,
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
	{HW_ID_RST, 0, 1000},
	{HW_ID_AVDD2, 1800000, 0}, // power AVDD 1.8V to enable 2.8V ldo
	{HW_ID_AVDD, 2800000, 1000},
	{HW_ID_AVDD3, 1800000, 1000}, // power AVDD2 1.8V to enable 2.8V ldo
	{HW_ID_AFVDD1, 1800000, 0}, // power  1.8V to enable 2.8V ldo
	{HW_ID_AFVDD, 2800000, 1000},
	{HW_ID_DOVDD, 1800000, 1000},
	{HW_ID_DVDD, 1100000, 1000},
	{HW_ID_DVDD1, 1090000, 1000},
	{HW_ID_MCLK_DRIVING_CURRENT, 4, 1000},
	{HW_ID_RST, 1, 5000}
};

const struct subdrv_entry imx866_mipi_raw_entry = {
	.name = "imx866_mipi_raw",
	.id = IMX866_SENSOR_ID,
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
	return (16384 - (16384 * BASEGAIN) / gain);
}

static int imx866_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	enum SENSOR_SCENARIO_ID_ENUM scenario_id;
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
	DRV_LOG(ctx, "X: set seamless switch done\n");
	return ERROR_NONE;
}

static int imx866_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
		set_i2c_buffer(ctx, 0x3010, 0x00);
		commit_i2c_buffer(ctx);
	}
	return 0;
}
