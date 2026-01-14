// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 lapisimx882widemipiraw_Sensor.c
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
#include "lapisimx882widemipiraw_Sensor.h"

static void set_sensor_cali(void *arg);
static int get_sensor_temperature(void *arg);
static void set_group_hold(void *arg, u8 en);
static u16 get_gain2reg(u32 gain);
static int lapisimx882wide_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapisimx882wide_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapisimx882wide_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapisimx882wide_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx, struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);
static int lapisimx882wide_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static void lapisimx82wide_s_mi_init_setting(void* arg);
static void lapisimx82wide_s_mi_mode_setting(void *arg, enum SENSOR_SCENARIO_ID_ENUM scenario_id);

static int longexposure_times = 0;
static int long_exposure_status;
int lapisimx882_vendor_id = 0x01;

#define SENEOR_QSC_ADDR  0xC000
#define SENEOR_LRC0_ADDR 0xD200
#define SENEOR_LRC1_ADDR 0xD300

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, lapisimx882wide_set_test_pattern},
	{SENSOR_FEATURE_SEAMLESS_SWITCH, lapisimx882wide_seamless_switch},
	{SENSOR_FEATURE_SET_ESHUTTER, lapisimx882wide_set_shutter},
	{SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME, lapisimx882wide_set_shutter_frame_length},
	{SENSOR_FEATURE_SET_AWB_GAIN, lapisimx882wide_set_awb_gain},
};
static struct eeprom_info_struct eeprom_info[] = {
	{
		.header_id = 0x00000093,
		.addr_header_id = 0x0000000C,
		.i2c_write_id = 0xA2,

		.qsc_support = TRUE,
		.qsc_size = 0x0C00,
		.addr_qsc = 0x2EFF,
		.sensor_reg_addr_qsc = SENEOR_QSC_ADDR,

		.lrc_support = TRUE,
		.lrc_size = 0x0180,
		.addr_lrc = 0x3B04,
		.sensor_reg_addr_lrc = SENEOR_LRC0_ADDR,
	},
};
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
    .i4OffsetX = 0,
    .i4OffsetY = 0,
    .i4PitchX  = 0,
    .i4PitchY  = 0,
    .i4PairNum = 0,
    .i4SubBlkW = 0,
    .i4SubBlkH = 0,
    .i4PosL    = {{0, 0} },
    .i4PosR    = {{0, 0} },
    .i4BlockNumX = 0,
    .i4BlockNumY = 0,
    .i4LeFirst   = 0,
    .i4FullRawW = 4096,
    .i4FullRawH = 3072,
    .i4VCPackNum = 1,
    .PDAF_Support = PDAF_SUPPORT_CAMSV_QPD,
    .i4ModeIndex = 2,
    .sPDMapInfo[0] = {
        .i4PDPattern = 1,
        .i4BinFacX = 2,
        .i4BinFacY = 4,
	.i4PDRepetition = 0,
        .i4PDOrder = {0},
        },
        .i4Crop = {
	    {0, 0}, {0, 0}, {0, 384}, {0, 0}, {0, 768},
        {0, 0}, {0, 0}, {2048, 1536},
		},
        .iMirrorFlip = IMAGE_HV_MIRROR,
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_isz = {
	.i4OffsetX	= 0,
	.i4OffsetY	= 0,
	.i4PitchX	= 0,
	.i4PitchY	= 0,
	.i4PairNum	= 0,
	.i4SubBlkW	= 0,
	.i4SubBlkH	= 0,
	.iMirrorFlip = 0,
	.i4PosL = {{0, 0}},
	.i4PosR = {{0, 0}},
	.i4BlockNumX = 0,
	.i4BlockNumY = 0,
	.i4LeFirst   = 0,
	.i4FullRawW = 8192,
	.i4FullRawH = 6144,
	.i4VCPackNum = 1,
	.i4ModeIndex = 2, /*HVBin 2; VBin 3*/
	.sPDMapInfo[0] = {
		.i4PDPattern = 1, /*1: Dense; 2: Sparse LR interleaved; 3: Sparse LR non interleaved*/
		.i4BinFacX = 4, /*for Dense*/
		.i4BinFacY = 2,
		.i4PDRepetition = 0,//?
		.i4PDOrder = {0},//L fist 0 R fist 1
	},
	.i4Crop = {
		// <pre> <cap> <normal_video> <hs_video> <slim_video>
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		// <cust1> <cust2> <cust3> <cust4> <cust5>
		{0, 0}, {0, 0}, {2048, 1536}, {0, 0}, {0, 0},
	},
	.iMirrorFlip = IMAGE_HV_MIRROR,
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_partial = {
    .i4OffsetX = 16,
    .i4OffsetY = 32,
    .i4PitchX  = 8,
    .i4PitchY  = 16,
    .i4PairNum = 4,
    .i4SubBlkW = 8,
    .i4SubBlkH = 4,
    .i4PosL    = {{16, 35}, {20, 37}, {19, 42}, {23, 44}},
    .i4PosR    = {{18, 33}, {22, 39}, {17, 40}, {21, 46}},
    .i4BlockNumX = 508,
    .i4BlockNumY = 144,
    .i4LeFirst   = 0,
    .i4FullRawW = 4096,
    .i4FullRawH = 3072,
    .i4VCPackNum = 1,
    .PDAF_Support = PDAF_SUPPORT_CAMSV,
    .i4ModeIndex = 0,
    .sPDMapInfo[0] = {
        .i4PDPattern = 3,
        //.i4BinFacX = 2,
        //.i4BinFacY = 4,
        .i4PDRepetition = 4,
        .i4PDOrder = {1,0,0,1},
        },
        .i4Crop = {
	    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 384},
        {0, 0}, {0, 0}, {0, 0},
		},
        .iMirrorFlip = IMAGE_HV_MIRROR,
};

static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0xc00,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x300,
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
			.hsize = 0x1000,
			.vsize = 0xc00,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_hs_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x780,
			.vsize = 0x438,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x200,
			.vsize = 0x480,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
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
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x780,
			.vsize = 0x438,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
	    .bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x800,
			.vsize = 0x600,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			//.user_data_desc = VC_STAGGER_NE,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
    },
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x500,
			.vsize = 0x2d0,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus5[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x500,
			.vsize = 0x2d0,
			.user_data_desc = VC_STAGGER_NE,
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
		},

	},
};
static struct subdrv_mode_struct mode_struct[] = {
	{
		.frame_desc = frame_desc_prev,//4096x3072@30fps
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = lapisimx882wide_preview_setting,
		.mode_setting_len = ARRAY_SIZE(lapisimx882wide_preview_setting),
		.mode_second_setting_table = lapisimx882wide_preview_ofilm_setting,
		.mode_second_setting_len = ARRAY_SIZE(lapisimx882wide_preview_ofilm_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = lapisimx882wide_seamless_preview,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapisimx882wide_seamless_preview),
		.seamless_switch_mode_second_setting_table = lapisimx882wide_seamless_ofilm_preview,
		.seamless_switch_mode_second_setting_len = ARRAY_SIZE(lapisimx882wide_seamless_ofilm_preview),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 7500,
		.framelength = 3880,
		.max_framerate = 300,
		.mipi_pixel_rate = 1714290000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.ana_gain_max = BASEGAIN * 64,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
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
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true, /* reg 0x0b06 */
	},
	{
		.frame_desc = frame_desc_cap,//4096x3072@30fps
		.num_entries = ARRAY_SIZE(frame_desc_cap),
		.mode_setting_table = lapisimx882wide_capture_setting,
		.mode_setting_len = ARRAY_SIZE(lapisimx882wide_capture_setting),
		.mode_second_setting_table = lapisimx882wide_preview_ofilm_setting,
		.mode_second_setting_len = ARRAY_SIZE(lapisimx882wide_preview_ofilm_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 7500,
		.framelength = 3880,
		.max_framerate = 300,
		.mipi_pixel_rate = 1714290000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.ana_gain_max = BASEGAIN * 64,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
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
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_vid,//4096x2304@30fps
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = lapisimx882wide_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(lapisimx882wide_normal_video_setting),
		.mode_second_setting_table = lapisimx882wide_normal_video_ofilm_setting,
		.mode_second_setting_len = ARRAY_SIZE(lapisimx882wide_normal_video_ofilm_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 7500,
		.framelength = 3880,
		.max_framerate = 300,
		.mipi_pixel_rate = 1714290000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.ana_gain_max = BASEGAIN * 64,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_hs_vid,//1080p@240fps
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = lapisimx882wide_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(lapisimx882wide_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 2468,
		.framelength = 1470,
		.max_framerate = 2400,
		.mipi_pixel_rate = 1714290000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 64,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 896,
			.w0_size = 8192,
			.h0_size = 4352,
			.scale_w = 2048,
			.scale_h = 1088,
			.x1_offset = 64,
			.y1_offset = 4,
			.w1_size = 1920,
			.h1_size = 1080,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1920,
			.h2_tg_size = 1080,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 876,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_slim_vid,//4096x2304@60fps
		.num_entries = ARRAY_SIZE(frame_desc_slim_vid),
		.mode_setting_table = lapisimx882wide_slim_video_setting,
		.mode_setting_len = ARRAY_SIZE(lapisimx882wide_slim_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 4616,
		.framelength = 3149,
		.max_framerate = 600,
		.mipi_pixel_rate = 1714290000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.ana_gain_max = BASEGAIN * 64,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
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
		.imgsensor_pd_info = &imgsensor_pd_info_partial,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus1,//4096x3072@24fps
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = lapisimx882wide_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(lapisimx882wide_custom1_setting),
		.mode_second_setting_table = lapisimx882wide_custom1_ofilm_setting,
		.mode_second_setting_len = ARRAY_SIZE(lapisimx882wide_custom1_ofilm_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 7500,
		.framelength = 4852,
		.max_framerate = 240,
		.mipi_pixel_rate = 1714290000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.ana_gain_max = BASEGAIN * 64,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
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
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus2,//1080p@120fps
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = lapisimx882wide_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(lapisimx882wide_custom2_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 2468,
		.framelength = 2940,
		.max_framerate = 1200,
		.mipi_pixel_rate = 1714290000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 64,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 896,
			.w0_size = 8192,
			.h0_size = 4352,
			.scale_w = 2048,
			.scale_h = 1088,
			.x1_offset = 64,
			.y1_offset = 4,
			.w1_size = 1920,
			.h1_size = 1080,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1920,
			.h2_tg_size = 1080,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 65,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus3,//seamless 2X
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = lapisimx882wide_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(lapisimx882wide_custom3_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = lapisimx882wide_seamless_custom3,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapisimx882wide_seamless_custom3),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 8960,
		.framelength = 3252,
		.max_framerate = 300,
		.mipi_pixel_rate = 1714290000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 10,
		.ana_gain_max = BASEGAIN * 16,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 10,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 2048,
			.y0_offset = 1536,
			.w0_size = 4096,
			.h0_size = 3072,
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
		.imgsensor_pd_info = &imgsensor_pd_info_isz,
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
		.frame_desc = frame_desc_cus4,//720p@240fps
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = lapisimx882wide_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(lapisimx882wide_custom4_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 2468,
		.framelength = 1470,
		.max_framerate = 2400,
		.mipi_pixel_rate = 1714290000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 64,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 1632,
			.w0_size = 8192,
			.h0_size = 2880,
			.scale_w = 2048,
			.scale_h = 720,
			.x1_offset = 384,
			.y1_offset = 0,
			.w1_size = 1280,
			.h1_size = 720,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1280,
			.h2_tg_size = 720,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus5,//720p@120fps
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = lapisimx882wide_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(lapisimx882wide_custom5_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 2468,
		.framelength = 2940,
		.max_framerate = 1200,
		.mipi_pixel_rate = 1714290000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 64,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 1632,
			.w0_size = 8192,
			.h0_size = 2880,
			.scale_w = 2048,
			.scale_h = 720,
			.x1_offset = 384,
			.y1_offset = 0,
			.w1_size = 1280,
			.h1_size = 720,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1280,
			.h2_tg_size = 720,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 335,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus6,//full size
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.mode_setting_table = lapisimx882wide_custom6_setting,
		.mode_setting_len = ARRAY_SIZE(lapisimx882wide_custom6_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = lapisimx882wide_seamless_custom6,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapisimx882wide_seamless_custom6),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 8960,
		.framelength = 9756,
		.max_framerate = 300,
		.mipi_pixel_rate = 1714290000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 10,
		.ana_gain_max = BASEGAIN * 16,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 10,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
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
		.rgbw_output_mode = IMGSENSOR_NORMAL_BAYER,
		.csi_param = {
			.cphy_settle = 69,
		},
		.dpc_enabled = true,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = LAPISIMX882WIDE_SENSOR_ID,
	.reg_addr_sensor_id = {0x0016, 0x0017},
	.i2c_addr_table = {0x20, 0xff},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.resolution = {8192, 6144},
	.mirror = IMAGE_HV_MIRROR,

	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),

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
	.ana_gain_table = lapisimx882wide_ana_gain_table,
	.ana_gain_table_size = sizeof(lapisimx882wide_ana_gain_table),
	.min_gain_iso = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 6,
	.exposure_max = 128 * (0xFFFC - 56),
	.exposure_step = 4,
	.exposure_margin = 56,

	.frame_length_max = 0xFFFC,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 3,
	.start_exposure_offset = 2400000,

	.pdaf_type = PDAF_SUPPORT_CAMSV_QPD,
	.hdr_type = HDR_SUPPORT_STAGGER_FDOL,
	.rgbw_support = FALSE,
	.seamless_switch_support = TRUE,
	.temperature_support = TRUE,

	.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,
	.s_cali = set_sensor_cali,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = 0x0101,
	.reg_addr_exposure = {
			{0x0202, 0x0203},
			{0x313A, 0x313B},
			{0x0224, 0x0225},
	},
	.long_exposure_support = TRUE,
	.reg_addr_exposure_lshift = 0x3160,
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

	.init_setting_table = lapisimx882wide_init_setting,
	.init_setting_len = ARRAY_SIZE(lapisimx882wide_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 1,
	.chk_s_off_end = 0,
	.s_mi_init_setting = lapisimx82wide_s_mi_init_setting,
	.s_mi_mode_setting = lapisimx82wide_s_mi_mode_setting,
	.mi_enable_async = 1,

	.checksum_value = 0x8ac2d94a,
};

static struct subdrv_ops ops = {
	.get_vendr_id = common_get_vendor_id,
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
	{HW_ID_MCLK_DRIVING_CURRENT, 4, 1},
	{HW_ID_RST, 0, 1},
	{HW_ID_AVDD, 2800000, 1},
	{HW_ID_DOVDD, 1800000, 1},
	{HW_ID_DVDD, 1100000, 1},
	{HW_ID_RST, 1, 5},
	{HW_ID_AFVDD, 2800000, 1},
};

const struct subdrv_entry lapisimx882wide_mipi_raw_entry = {
	.name = "lapisimx882wide_mipi_raw",
	.id = LAPISIMX882WIDE_SENSOR_ID,
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

struct SET_SENSOR_AWB_GAIN g_last_awb_gain = {0, 0, 0, 0};
static int lapisimx882wide_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *) para;
	struct SET_SENSOR_AWB_GAIN *awb_gain = (struct SET_SENSOR_AWB_GAIN *)feature_data;
	MUINT32 r_Gain = awb_gain->ABS_GAIN_R << 1;
	MUINT32 gr_Gain = awb_gain->ABS_GAIN_GR << 1;
	MUINT32 gb_Gain = awb_gain->ABS_GAIN_GB << 1;
	MUINT32 b_Gain = awb_gain->ABS_GAIN_B << 1;

	DRV_LOG(ctx, "sunrey awb gain [r/gr/gb/b]: 0x%x 0x%x 0x%x 0x%x\n",
		r_Gain, gr_Gain, gb_Gain, b_Gain);
	g_last_awb_gain = *awb_gain;

	if ((ctx->current_scenario_id != SENSOR_SCENARIO_ID_CUSTOM3) && (ctx->current_scenario_id != SENSOR_SCENARIO_ID_CUSTOM6)) {
		DRV_LOG(ctx, "skip awb gain [r/gr/gb/b]: 0x%x 0x%x 0x%x 0x%x\n",
			r_Gain, gr_Gain, gb_Gain, b_Gain);
		return 0;
	}

	if (r_Gain == 0 || gr_Gain == 0|| gb_Gain == 0 || b_Gain == 0) {
		DRV_LOG(ctx, "skip awb gain [r/gr/gb/b]: 0x%x 0x%x 0x%x 0x%x\n",
			r_Gain, gr_Gain, gb_Gain, b_Gain);
		return 0;
	}

	// set awb gain
	subdrv_i2c_wr_u8(ctx, 0x0B8E, (gr_Gain / 1024) & 0x7F); //Gr Gain [15:8]
	subdrv_i2c_wr_u8(ctx, 0x0B8F, ((gr_Gain % 1024) / 4) & 0xFF); //Gr Gain [7:0]
	subdrv_i2c_wr_u8(ctx, 0x0B90, (r_Gain / 1024) & 0x7F); //R Gain [15:8]
	subdrv_i2c_wr_u8(ctx, 0x0B91, ((r_Gain % 1024) / 4) & 0xFF); //R Gain [7:0]
	subdrv_i2c_wr_u8(ctx, 0x0B92, (b_Gain / 1024) & 0x7F); //B Gain [15:8]
	subdrv_i2c_wr_u8(ctx, 0x0B93, ((b_Gain % 1024) / 4) & 0xFF); //B Gain [7:0]
	subdrv_i2c_wr_u8(ctx, 0x0B94, (gb_Gain / 1024) & 0xFF); //Gb Gain [15:9]
	subdrv_i2c_wr_u8(ctx, 0x0B95, ((gb_Gain % 1024) / 4) & 0xFF); //Gb Gain [7:0]

	return 0;
}

static int lapisimx882wide_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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

	lapisimx882wide_set_awb_gain(ctx, (u8 *)&g_last_awb_gain, len);

	subdrv_i2c_wr_u8(ctx, 0x0104, 0x01);
	subdrv_i2c_wr_u8(ctx, 0x3010, 0x02);
	if (ctx->s_ctx.mode[scenario_id].seamless_switch_mode_second_setting_table != NULL && lapisimx882_vendor_id == OFILM){
		subdrv_i2c_wr_regs_u8_burst(ctx,
			ctx->s_ctx.mode[scenario_id].seamless_switch_mode_second_setting_table,
			ctx->s_ctx.mode[scenario_id].seamless_switch_mode_second_setting_len);
	}else{
		subdrv_i2c_wr_regs_u8_burst(ctx,
			ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table,
			ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_len);
	}

	if (ae_ctrl) {
		switch (ctx->s_ctx.mode[scenario_id].hdr_mode) {
		case HDR_RAW_STAGGER:
			set_multi_shutter_frame_length(ctx, (u64 *)&ae_ctrl->exposure, exp_cnt, 0);
			set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			break;
		default:
			lapisimx882wide_set_shutter(ctx, (u8 *)(&ae_ctrl->exposure.le_exposure), len); //use sensor's set_shutter
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

static int lapisimx882wide_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	switch (mode) {
	case 5:
		subdrv_i2c_wr_u8(ctx, 0x0601, 0x01); /* Solid Color */
		break;
	default:
		subdrv_i2c_wr_u8(ctx, 0x0601, mode);
		break;
	}

	ctx->test_pattern = mode;
	return ERROR_NONE;
}

static int lapisimx882wide_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
	/* enable auto extend */
	if (ctx->s_ctx.reg_addr_auto_extend)
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_auto_extend, 0x01);

	/* long exposure */
	longexposure_times = 0;
	while (ctx->exposure[0] >= 65535) {
		ctx->exposure[0] = ctx->exposure[0] / 2;
		longexposure_times += 1;
	}
	if (longexposure_times > 0) {
		DRV_LOG_MUST(ctx, "enter long exposure mode, time is %d", longexposure_times);
		long_exposure_status = 1;
		ctx->frame_length = ctx->min_frame_length;
		set_i2c_buffer(ctx, 0x3160, longexposure_times & 0x07);
	} else if (long_exposure_status == 1) {
		long_exposure_status = 0;
		set_i2c_buffer(ctx, 0x3160, longexposure_times & 0xf8);
		DRV_LOG_MUST(ctx, "exit long exposure mode");
	}
	/* write framelength */
	if (set_auto_flicker(ctx, 0) || frame_length || !ctx->s_ctx.reg_addr_auto_extend)
		write_frame_length(ctx,ctx->frame_length);
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
	DRV_LOG(ctx, "exp[0x%x], fll(input/output):%u/%u, flick_en:%d\n",
		ctx->exposure[0], frame_length, ctx->frame_length, ctx->autoflicker_en);
	if (!ctx->ae_ctrl_gph_en) {
		if (gph)
			ctx->s_ctx.s_gph((void *)ctx, 0);
		commit_i2c_buffer(ctx);
	}
	/* group hold end */
	return 0;
}

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
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			subdrv_i2c_wr_u8(ctx, 0x3206, 0x01);
			DRV_LOG(ctx, "set QSC calibration data done.");
		} else {
			subdrv_i2c_wr_u8(ctx, 0x3206, 0x00);
		}
	}

	/* LRC data */
	support = info[idx].lrc_support;
	pbuf = info[idx].preload_lrc_table;
	size = info[idx].lrc_size;
	addr = info[idx].sensor_reg_addr_lrc;
	if (support) {
		if (pbuf != NULL && addr > 0 && size > 0) {
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size/2);
			subdrv_i2c_wr_seq_p8(ctx, SENEOR_LRC1_ADDR, pbuf+size/2, size/2);
			DRV_LOG(ctx, "set LRC calibration data done.");
		}
	}
}

static int lapisimx882wide_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	lapisimx882wide_set_shutter_frame_length(ctx, para, len);
	return 0;
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

static void lapisimx82wide_s_mi_init_setting(void* arg){
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	subdrv_i2c_wr_regs_u8_burst(ctx, ctx->s_ctx.init_setting_table, ctx->s_ctx.init_setting_len);
}

static void lapisimx82wide_s_mi_mode_setting(void *arg, enum SENSOR_SCENARIO_ID_ENUM scenario_id){
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	DRV_LOG(ctx, "vendor_id(0x%x)", lapisimx882_vendor_id);
	if (ctx->s_ctx.mode[scenario_id].mode_second_setting_table != NULL && lapisimx882_vendor_id == OFILM){
		subdrv_i2c_wr_regs_u8_burst(ctx, ctx->s_ctx.mode[scenario_id].mode_second_setting_table,
			ctx->s_ctx.mode[scenario_id].mode_second_setting_len);
	}else{
		subdrv_i2c_wr_regs_u8_burst(ctx, ctx->s_ctx.mode[scenario_id].mode_setting_table,
			ctx->s_ctx.mode[scenario_id].mode_setting_len);
	}
}