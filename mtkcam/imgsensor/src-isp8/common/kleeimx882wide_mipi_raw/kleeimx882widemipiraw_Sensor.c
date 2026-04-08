// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 kleeimx882widemipiraw_Sensor.c
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
#include "kleeimx882widemipiraw_Sensor.h"

#define KLEEIMX882WIDE_EMBEDDED_DATA_EN 0

static void set_sensor_cali(void *arg);
static int get_sensor_temperature(void *arg);
static void set_group_hold(void *arg, u8 en);
static u16 get_gain2reg(u32 gain);
static int kleeimx882wide_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int kleeimx882wide_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int kleeimx882wide_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int kleeimx882wide_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);

#define SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE                 1
#define SEAMLESS_SWITCH_GROUP_NORMAL_VIDEO_MODE               2

/* STRUCT */
static struct SET_SENSOR_AWB_GAIN g_last_awb_gain = {0, 0, 0, 0};
static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, kleeimx882wide_set_test_pattern},
	{SENSOR_FEATURE_SEAMLESS_SWITCH, kleeimx882wide_seamless_switch},
	{SENSOR_FEATURE_SET_AWB_GAIN, kleeimx882wide_set_awb_gain},
};

static struct eeprom_info_struct eeprom_info[] = {
	{
		.header_id = 0x01, //sunny
		.addr_header_id = 0x01,
		.i2c_write_id = 0xA2,

		// LRC/SPC part1
		.lrc_support = true,
		.lrc_size = 192,
		.addr_lrc = 0x3B04,
		.sensor_reg_addr_lrc = 0xD200,
		// LRC/SPC part2
		.pdc_support = true,
		.pdc_size = 192,
		.addr_pdc = 0x3BC3,
		.sensor_reg_addr_pdc = 0xD300,

		// QSC Calibration
		.qsc_support = true,
		.qsc_size = 3072,
		.addr_qsc = 0x2EFF,
		.sensor_reg_addr_qsc = 0xC000,
	},
	{
		.header_id = 0x10, //aac
		.addr_header_id = 0x01,
		.i2c_write_id = 0xA2,

		// LRC/SPC part1
		.lrc_support = true,
		.lrc_size = 192,
		.addr_lrc = 0x3B04,
		.sensor_reg_addr_lrc = 0xD200,
		// LRC/SPC part2
		.pdc_support = true,
		.pdc_size = 192,
		.addr_pdc = 0x3BC3,
		.sensor_reg_addr_pdc = 0xD300,

		// QSC Calibration
		.qsc_support = true,
		.qsc_size = 3072,
		.addr_qsc = 0x2EFF,
		.sensor_reg_addr_qsc = 0xC000,
	},
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 0,
	.i4OffsetY = 0,
	.i4PitchX = 0,
	.i4PitchY = 0,
	.i4PairNum = 0,
	.i4SubBlkW = 0,
	.i4SubBlkH = 0,
	.i4PosL = {{0, 0}},
	.i4PosR = {{0, 0}},
	.i4BlockNumX = 0,
	.i4BlockNumY = 0,
	.i4LeFirst = 0,
	.i4Crop = {
		// <pre> <cap> <normal_video> <hs_video> <<slim_video>>
		{0, 0}, {0, 0}, {0, 384}, {0, 192}, {0, 0},
		// <<cust1>> <<cust2>> <<cust3>> <cust4> <cust5>
		{2048, 1536}, {0, 192}, {0, 384}, {0,0}, {2048, 1536},
		// <cust6> <cust7> <cust8> <cust9> <cust10>
		{0, 0}, {0, 256}, {0, 256}, {0, 0}, {0, 0},
		// <cust11> <cust12>
		{2048, 1792}, {128, 456}
	},
	.iMirrorFlip = 0,
	.i4FullRawW = 4096,
	.i4FullRawH = 3072,
	.i4ModeIndex = 2,
	/* VC's PD pattern description */
	.sPDMapInfo[0] = {
		.i4PDPattern = 1,
		.i4BinFacX = 2,
		.i4BinFacY = 4,
		.i4PDOrder = {1},
	},
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_isz = {
	.i4OffsetX = 0,
	.i4OffsetY = 0,
	.i4PitchX = 0,
	.i4PitchY = 0,
	.i4PairNum = 0,
	.i4SubBlkW = 0,
	.i4SubBlkH = 0,
	.i4PosL = {{0, 0}},
	.i4PosR = {{0, 0}},
	.i4BlockNumX = 0,
	.i4BlockNumY = 0,
	.i4LeFirst = 0,
	.i4Crop = {
		// <pre> <cap> <normal_video> <hs_video> <<slim_video>>
		{0, 0}, {0, 0}, {0, 384}, {0, 192}, {0, 0},
		// <<cust1>> <<cust2>> <<cust3>> <cust4> <cust5>
		{2048, 1536}, {0, 192}, {0, 384}, {0,0}, {2048, 1536},
		// <cust6> <cust7> <cust8> <cust9> <cust10>
		{0, 0}, {0, 256}, {0, 256}, {0, 0}, {0, 0},
		// <cust11> <cust12>
		{2048, 1792}, {128, 456}
	},
	.iMirrorFlip = 0,
	.i4FullRawW = 8192,
	.i4FullRawH = 6144,
	.i4ModeIndex = 2,
	/* VC's PD pattern description */
	.sPDMapInfo[0] = {
		.i4PDPattern = 1,
		.i4BinFacX = 4,
		.i4BinFacY = 2,
		.i4PDOrder = {1},
	},
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_video_partial = {
	.i4OffsetX = 16,
	.i4OffsetY = 32,
	.i4PitchX = 8,
	.i4PitchY = 16,
	.i4PairNum = 4,
	.i4SubBlkW = 8,
	.i4SubBlkH = 4,
	.i4PosL = {{16, 35},{20, 37},{19, 42},{23, 44}},
	.i4PosR = {{18, 33},{22, 39},{17, 40},{21, 46}},
	.i4BlockNumX = 508,
	.i4BlockNumY = 140,
	.i4VCPackNum = 1,
	.PDAF_Support = PDAF_SUPPORT_CAMSV,
	.i4Crop = {
		// <pre> <cap> <normal_video> <hs_video> <<slim_video>>
		{0, 0}, {0, 0}, {0, 384}, {0, 192}, {0, 0},
		// <<cust1>> <<cust2>> <<cust3>> <cust4> <cust5>
		{2048, 1536}, {0, 192}, {0, 384}, {0,0}, {2048, 1536},
		// <cust6> <cust7> <cust8> <cust9> <cust10>
		{0, 0}, {0, 256}, {0, 256}, {0, 0}, {0, 0},
		// <cust11> <cust12>
		{2048, 1792}, {128, 456}
	},
	.iMirrorFlip = 0,
	.i4FullRawW = 4096,
	.i4FullRawH = 3072,
	.i4ModeIndex = 0,
	/* VC's PD pattern description */
	.sPDMapInfo[0] = {
		.i4PDPattern = 3,
		.i4BinFacX = 0,
		.i4BinFacY = 0,
		.i4PDOrder = {1,0,0,1},
		.i4PDRepetition = 4,
	}
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_smvr = {
	.i4OffsetX = 0,
	.i4OffsetY = 0,
	.i4PitchX = 0,
	.i4PitchY = 0,
	.i4PairNum = 0,
	.i4SubBlkW = 0,
	.i4SubBlkH = 0,
	.i4PosL = {{0, 0}},
	.i4PosR = {{0, 0}},
	.i4BlockNumX = 0,
	.i4BlockNumY = 0,
	.i4LeFirst = 0,
	.i4Crop = {
		// <pre> <cap> <normal_video> <hs_video> <<slim_video>>
		{0, 0}, {0, 0}, {0, 384}, {0, 192}, {0, 0},
		// <<cust1>> <<cust2>> <<cust3>> <cust4> <cust5>
		{2048, 1536}, {0, 192}, {0, 384}, {0,0}, {2048, 1536},
		// <cust6> <cust7> <cust8> <cust9> <cust10>
		{0, 0}, {0, 256}, {0, 256}, {0, 0}, {0, 0},
		// <cust11> <cust12>
		{2048, 1792}, {128, 456}
	},
	.iMirrorFlip = 0,
	.i4FullRawW = 2048,
	.i4FullRawH = 1536,
	.i4ModeIndex = 2,
	/* VC's PD pattern description */
	.sPDMapInfo[0] = {
		.i4PDPattern = 1,
		.i4BinFacX = 2,
		.i4BinFacY = 4,
		.i4PDOrder = {1},
	},
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_LBMF_Partical = {
	.i4OffsetX = 16,
	.i4OffsetY = 32,
	.i4PitchX = 8,
	.i4PitchY = 16,
	.i4PairNum = 4,
	.i4SubBlkW = 8,
	.i4SubBlkH = 4,
	.i4PosL = {{16, 35},{20, 37},{19, 42},{23, 44}},
	.i4PosR = {{18, 33},{22, 39},{17, 40},{21, 46}},
	.i4BlockNumX = 508,
	.i4BlockNumY = 160,
	.i4VCPackNum = 1,
	.PDAF_Support = PDAF_SUPPORT_CAMSV,
	.i4Crop = {
		// <pre> <cap> <normal_video> <hs_video> <<slim_video>>
		{0, 0}, {0, 0}, {0, 384}, {0, 192}, {0, 0},
		// <<cust1>> <<cust2>> <<cust3>> <cust4> <cust5>
		{2048, 1536}, {0, 192}, {0, 384}, {0,0}, {2048, 1536},
		// <cust6> <cust7> <cust8> <cust9> <cust10>
		{0, 0}, {0, 256}, {0, 256}, {0, 0}, {0, 0},
		// <cust11> <cust12>
		{2048, 1792}, {128, 456}
	},
	.iMirrorFlip = 0,
	.i4FullRawW = 4096,
	.i4FullRawH = 3072,
	.i4ModeIndex = 0,
	/* VC's PD pattern description */
	.sPDMapInfo[0] = {
		.i4PDPattern = 3,
		.i4BinFacX = 0,
		.i4BinFacY = 0,
		.i4PDOrder = {1,0,0,1},
		.i4PDRepetition = 4,
	},
};


static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 768,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 2304,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 576,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_hs_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 2048,
			.vsize = 1152,
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
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 1536,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 2048,
			.vsize = 1152,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 288,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 2304,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 508,
			.vsize = 1152,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 8192,
			.vsize = 6144,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus5[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 1536,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus7[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 2560,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 640,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus8[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 2560,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 2560,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 508,
			.vsize = 1280,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x30,
			.hsize = 508,
			.vsize = 1280,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_ME_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus10[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 8192,
			.vsize = 6144,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus11[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 2560,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 1280,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus12[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3840,
			.vsize = 2160,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 480,
			.vsize = 1072,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct subdrv_mode_struct mode_struct[] = {
	// preview mode 0
	// mode P10-3
	// PD size = 4096x768
	// Tline = 8.58us
	// VB = 6.72ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = kleeimx882wide_preview_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_preview_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 873800000,
		.linelength = 7500,
		.framelength = 3883,
		.max_framerate = 300,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
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
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true, /* reg 0x0b06 */
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
	// capture mode 1: same as preview mode
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = kleeimx882wide_preview_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_preview_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 873800000,
		.linelength = 7500,
		.framelength = 3883,
		.max_framerate = 300,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
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
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true, /* reg 0x0b06 */
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
	// normal_video mode 2(16:9)
	// mode P10_7
	// PD size = 4096x576
	// Tline = 8.58us
	// VB = 13.32ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = kleeimx882wide_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_normal_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 873000000,
		.linelength = 7500,
		.framelength = 3880,
		.max_framerate = 300,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
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
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
	// hs_video mode 3 : smvr 240fps
	// Mode_P10_15 without PD
	// PD Size = null
	// Tline = 2.83us
	// VB = 0.85ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = kleeimx882wide_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 2468,
		.framelength = 1464,
		.max_framerate = 2400,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 8,
		.coarse_integ_step = 8,
		.min_exposure_line = 12,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 12,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
	// slim_video mode 4: same as preview mode
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = kleeimx882wide_preview_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_preview_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 873800000,
		.linelength = 7500,
		.framelength = 3883,
		.max_framerate = 300,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
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
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true, /* reg 0x0b06 */
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
	// custom1 mode 5 : on ISZ(fullsize crop bayer)
	// Mode_P10_6
	// PD Size = 2048x1536
	// Tline = 10.25us
	// VB = 1.35ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = kleeimx882wide_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_custom1_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 874137600,
		.linelength = 8960,
		.framelength = 3252,
		.max_framerate = 300,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 10,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 10,
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
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true,
		.awb_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_BAYER_R,
	},
	// custom2 mode 6 : smvr 120fps
	// Mode_P10_14
	// PD Size = 2048x288
	// Tline = 4.62us
	// VB = 2.92ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = kleeimx882wide_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_custom2_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 4024,
		.framelength = 1800,
		.max_framerate = 1200,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 8,
		.coarse_integ_step = 8,
		.min_exposure_line = 12,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 12,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
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
		.imgsensor_pd_info = &imgsensor_pd_info_smvr,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
	// custom3 mode 7 : 60fps video(16:9)
	// Mode_P10_8
	// PD Size = 508x1152
	// Tline = 5.29us
	// VB = 4.34ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = kleeimx882wide_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_custom3_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 872200000,
		.linelength = 4616,
		.framelength = 3144,
		.max_framerate = 600,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 1,
		.coarse_integ_step = 1,
		.min_exposure_line = 5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 5,
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
		.imgsensor_pd_info = &imgsensor_pd_info_video_partial,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
	// custom4 mode 8 : fullsize bayer
	// Mode_P10_1
	// Tline = 10.25us
	// VB = 3.19ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = kleeimx882wide_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_custom4_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 8960,
		.framelength = 6504,
		.max_framerate = 150,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 10,
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
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true,
		.awb_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	},
	// custom5 mode 9 : ISZ(full size crop quad)
	// Mode_P10_5
	// PD Size = 2048x1536
	// Tline = 10.25us
	// VB = 1.35ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = kleeimx882wide_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_custom5_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 8960,
		.framelength = 3252,
		.max_framerate = 300,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 10,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 10,
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
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true,
		.awb_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_R,
	},
	// custom6 mode 10: same as preview mode
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = kleeimx882wide_preview_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_preview_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 873800000,
		.linelength = 7500,
		.framelength = 3883,
		.max_framerate = 300,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
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
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true, /* reg 0x0b06 */
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
	// custom7 mode 11: binning
	// Mode_P10_11
	// PD Size = 4096x640
	// Tline = 5.29us
	// VB = 4.34ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_cus7,
		.num_entries = ARRAY_SIZE(frame_desc_cus7),
		.mode_setting_table = kleeimx882wide_custom7_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_custom7_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_VIDEO_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 873000000,
		.linelength = 7500,
		.framelength = 3868,
		.max_framerate = 300,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 2304,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].max = 0xFFC7,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 512,
			.w0_size = 8192,
			.h0_size = 5120,
			.scale_w = 4096,
			.scale_h = 2560,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 2560,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 2560,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
	// custom8 mode 12: 2exp LBMF(binning)
	// Mode_P10_12-2exp
	// PD Size = 508x1280
	// Tline = 5.29us
	// VB = 4.34ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_cus8,
		.num_entries = ARRAY_SIZE(frame_desc_cus8),
		.mode_setting_table = kleeimx882wide_custom8_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_custom8_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_VIDEO_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_RAW_LBMF,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 872285520,
		.linelength = 4616,
		.framelength = 6299,
		.max_framerate = 300,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 2560,
		.read_margin = 56,
		.framelength_step = 1,
		.coarse_integ_step = 1,
		.min_exposure_line = 5,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN * 1.42867f,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 5,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 5,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].max = 0xFFC7,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].max = 0xFFC7,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 512,
			.w0_size = 8192,
			.h0_size = 5120,
			.scale_w = 4096,
			.scale_h = 2560,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 2560,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 2560,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_LBMF_Partical,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.exposure_order_in_lbmf = IMGSENSOR_LBMF_EXPOSURE_SE_FIRST,
		.mode_type_in_lbmf = IMGSENSOR_LBMF_MODE_MANUAL,
		.dpc_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
	// custom9 mode 13: same as preview
	// mode P10-3
	// 1x bokeh
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = kleeimx882wide_preview_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_preview_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 873800000,
		.linelength = 7500,
		.framelength = 4854,
		.max_framerate = 240,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
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
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true, /* reg 0x0b06 */
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
	// custom10 mode 14: fullsize 50M quad bayer RawSR(quad)
	// Mode_P10_2
	// Tline = 10.25us
	// VB = 3.19ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_cus10,
		.num_entries = ARRAY_SIZE(frame_desc_cus10),
		.mode_setting_table = kleeimx882wide_custom10_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_custom10_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 8960,
		.framelength = 6504,
		.max_framerate = 150,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 10,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 10,
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
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true,
		.awb_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_R,
	},
	// custom11 mode 15: video ISZ(fullsize crop bayer)
	// Mode_P10_13
	// PD Size = 2048x1280
	// Tline = 10.25us
	// VB = 9.22ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_cus11,
		.num_entries = ARRAY_SIZE(frame_desc_cus11),
		.mode_setting_table = kleeimx882wide_custom11_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_custom11_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_VIDEO_MODE,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 876800000,
		.linelength = 8960,
		.framelength = 3252,
		.max_framerate = 300,
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 10,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 10,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 2048,
			.y0_offset = 1792,
			.w0_size = 4096,
			.h0_size = 2560,
			.scale_w = 4096,
			.scale_h = 2560,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 2560,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 2560,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_isz,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true,
		.awb_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_BAYER_R,
	},
	// custom12 mode 16: 4k 60 video(3840*2160)
	// Mode_P10_10
	// PD Size = 480x1072
	// Tline = 10.25us
	// VB = 9.22ms
	// ver1.00-7.00-250618
	{
		.frame_desc = frame_desc_cus12,
		.num_entries = ARRAY_SIZE(frame_desc_cus12),
		.mode_setting_table = kleeimx882wide_custom12_setting,
		.mode_setting_len = ARRAY_SIZE(kleeimx882wide_custom12_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 873600000,
		.linelength = 4616,
		.framelength = 3149,
		.max_framerate = 600,//60.1fps
		.mipi_pixel_rate = 1026000000,
		.readout_length = 0,
		.read_margin = 56,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1.42867f,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 256,
			.y0_offset = 912,
			.w0_size = 7680,
			.h0_size = 4320,
			.scale_w = 3840,
			.scale_h = 2160,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3840,
			.h1_size = 2160,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3840,
			.h2_tg_size = 2160,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_video_partial,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 3,
		.dpc_enabled = true,
		.awb_enabled = true,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = KLEEIMX882WIDE_SENSOR_ID,
	.reg_addr_sensor_id = {0x0016, 0x0017},
	.i2c_addr_table = {0x20, 0xFF},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {8192, 6144},
	.mirror = IMAGE_NORMAL,

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_CPHY,
	.mipi_lane_num = SENSOR_MIPI_3_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1.42867f,
	.ana_gain_max = BASEGAIN * 64,
	.ana_gain_type = 0,
	.ana_gain_step = 1,
	.ana_gain_table = kleeimx882wide_ana_gain_table,
	.ana_gain_table_size = sizeof(kleeimx882wide_ana_gain_table),
	.tuning_iso_base = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 24,
	.exposure_max = (0xFFFC - 56) << 9,
	.exposure_step = 4,
	.exposure_margin = 56,

	.frame_length_max = 0xFFFC,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 3,
	.start_exposure_offset = 1332000,

	.pdaf_type = PDAF_SUPPORT_CAMSV_QPD,
	.hdr_type = HDR_SUPPORT_LBMF,
	.seamless_switch_support = TRUE,
	.seamless_switch_type = SEAMLESS_SWITCH_CUT_VB_INIT_SHUT,
	.seamless_switch_hw_re_init_time_ns = 2750000, //fixme
	.seamless_switch_prsh_hw_fixed_value = 56,
	.seamless_switch_prsh_length_lc = 0,
	.reg_addr_prsh_length_lines = {0x3059, 0x305a, 0x305b},
	.reg_addr_prsh_mode = 0x3056,

	.temperature_support = TRUE,

	.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,
	.s_cali = set_sensor_cali,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = 0x0101,
	.reg_addr_exposure = {
		{0x0202, 0x0203}
	},
	.reg_addr_exposure_in_lut = {
		{0x0E20, 0x0E21},
		{0x0E40, 0x0E41},
		{0x0E60, 0x0E61},
	},
	.long_exposure_support = TRUE,
	.reg_addr_exposure_lshift = 0x3160,
	.reg_addr_ana_gain = {
		{0x0204, 0x0205}
	},
	.reg_addr_ana_gain_in_lut = {
		{0x0E22, 0x0E23},
		{0x0E42, 0x0E43},
		{0x0E62, 0x0E63},
	},

	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_frame_length_in_lut = {
		{0x0E28, 0x0E29},
		{0x0E48, 0x0E49},
		{0x0E68, 0x0E69},
	},
	.reg_addr_temp_en = 0x0138,
	.reg_addr_temp_read = 0x013A,
	.reg_addr_auto_extend = 0x0350,
	.reg_addr_frame_count = 0x0005,
	.reg_addr_fast_mode = 0x3010,
	.reg_addr_fast_mode_in_lbmf = 0x31A7,

	.init_setting_table = kleeimx882wide_init_setting,
	.init_setting_len = ARRAY_SIZE(kleeimx882wide_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 1,
	.chk_s_off_end = 0,

	.checksum_value = 0xAF3E324F,
};

static struct subdrv_ops ops = {
	.get_id = kleeimx882wide_get_imgsensor_id,
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
	.parse_ebd_line = common_parse_ebd_line,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_MCLK, {24}, 0},
	{HW_ID_RST, {0}, 0},
	{HW_ID_MCLK_DRIVING_CURRENT, {4}, 1000},
	{HW_ID_AVDD1, {1800000, 1800000}, 0},
	{HW_ID_AVDD, {2800000, 2800000}, 0},
	{HW_ID_AFVDD, {3100000, 3100000}, 0},
	{HW_ID_DVDD, {1100000, 1100000}, 0},
	{HW_ID_DOVDD, {1800000, 1800000}, 0},
	{HW_ID_RST, {1}, 1000}
};

const struct subdrv_entry kleeimx882wide_mipi_raw_entry = {
	.name = "kleeimx882wide_mipi_raw",
	.id = KLEEIMX882WIDE_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

/* FUNCTION */
int kleeimx882wide_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id)
{
	u8 i = 0;
	u8 retry = 2;
	u32 addr_h = ctx->s_ctx.reg_addr_sensor_id.addr[0];
	u32 addr_l = ctx->s_ctx.reg_addr_sensor_id.addr[1];
	u32 addr_ll = ctx->s_ctx.reg_addr_sensor_id.addr[2];

	while (ctx->s_ctx.i2c_addr_table[i] != 0xFF) {
		ctx->i2c_write_id = ctx->s_ctx.i2c_addr_table[i];
		do {
			*sensor_id = (subdrv_ixc_rd_u8(ctx, addr_h) << 8) |
				subdrv_ixc_rd_u8(ctx, addr_l);
			if (addr_ll) {
				*sensor_id = ((*sensor_id) << 8) | subdrv_ixc_rd_u8(ctx, addr_ll);
			}

			*sensor_id = (*sensor_id) + 1;

			DRV_LOG_MUST(ctx, "i2c_write_id:0x%x sensor_id(cur/exp):0x%x/0x%x\n",
				ctx->i2c_write_id, *sensor_id, ctx->s_ctx.sensor_id);
			printk("i2c_write_id:0x%x sensor_id(cur/exp):0x%x/0x%x\n",
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
			DRV_LOG(ctx, "set QSC calibration data done.");
		}
	}

	// LRC/SPC part1
	support = info[idx].lrc_support;
	pbuf = info[idx].preload_lrc_table;
	size = info[idx].lrc_size;
	addr = info[idx].sensor_reg_addr_lrc;
	if (support) {
		if (pbuf != NULL && addr > 0 && size > 0) {
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			DRV_LOG(ctx, "set LRC/SPC part1 calibration data done.");
		}
	}

	// LRC/SPC part2
	support = info[idx].pdc_support;
	pbuf = info[idx].preload_pdc_table;
	size = info[idx].pdc_size;
	addr = info[idx].sensor_reg_addr_pdc;
	if (support) {
		if (pbuf != NULL && addr > 0 && size > 0) {
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			DRV_LOG(ctx, "set LRC/SPC part2 calibration data done.");
		}
	}
}

static int get_sensor_temperature(void *arg)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	u8 temperature = 0;
	int temperature_convert = 0;

	temperature = subdrv_i2c_rd_u8(ctx, ctx->s_ctx.reg_addr_temp_read);

	if (temperature < 0x55)
		temperature_convert = temperature;
	else if (temperature < 0x80)
		temperature_convert = 85;
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
	return (16384 - (16384 * BASEGAIN) / gain);
}

void kleeimx882wide_update_mode_info(struct subdrv_ctx *ctx, enum SENSOR_SCENARIO_ID_ENUM scenario_id)
{
	if (scenario_id >= ctx->s_ctx.sensor_mode_num)
	{
		DRV_LOGE(ctx, "invalid sid:%u, mode_num:%u\n",
				 scenario_id, ctx->s_ctx.sensor_mode_num);
		return;
	}
	ctx->current_scenario_id = scenario_id;
	ctx->pclk = ctx->s_ctx.mode[scenario_id].pclk;
	ctx->line_length = ctx->s_ctx.mode[scenario_id].linelength;
	if (ctx->current_fps < (ctx->s_ctx.mode[scenario_id].max_framerate - 20))
	{
		ctx->frame_length = ctx->pclk / ctx->line_length / (ctx->current_fps / 10);
		ctx->frame_length_rg = ctx->frame_length;
	}
	else
	{
		ctx->frame_length = ctx->s_ctx.mode[scenario_id].framelength;
		ctx->frame_length_rg = ctx->frame_length;
		ctx->current_fps = ctx->pclk / ctx->line_length / ctx->frame_length * 10;
	}

	ctx->readout_length = ctx->s_ctx.mode[scenario_id].readout_length;
	ctx->read_margin = ctx->s_ctx.mode[scenario_id].read_margin;
	ctx->min_frame_length = ctx->frame_length;
	ctx->autoflicker_en = FALSE;
	ctx->l_shift = 0;
	ctx->min_vblanking_line = ctx->s_ctx.mode[scenario_id].min_vblanking_line;
	if (ctx->s_ctx.mode[scenario_id].hdr_mode == HDR_RAW_LBMF)
	{
		memset(ctx->frame_length_in_lut, 0,
			   sizeof(ctx->frame_length_in_lut));

		switch (ctx->s_ctx.mode[scenario_id].exp_cnt)
		{
		case 2:
			ctx->frame_length_in_lut[0] = ctx->readout_length + ctx->read_margin;
			ctx->frame_length_in_lut[1] = ctx->frame_length -
										  ctx->frame_length_in_lut[0];
			break;
		case 3:
			ctx->frame_length_in_lut[0] = ctx->readout_length + ctx->read_margin;
			ctx->frame_length_in_lut[1] = ctx->readout_length + ctx->read_margin;
			ctx->frame_length_in_lut[2] = ctx->frame_length -
										  ctx->frame_length_in_lut[1] - ctx->frame_length_in_lut[0];
			break;
		default:
			break;
		}
		memcpy(ctx->frame_length_in_lut_rg, ctx->frame_length_in_lut,
			   sizeof(ctx->frame_length_in_lut_rg));
	}

	/* MCSS low power mode update para */
	if (ctx->s_ctx.mcss_update_subdrv_para != NULL)
		ctx->s_ctx.mcss_update_subdrv_para((void *)ctx, scenario_id);
}

static int kleeimx882wide_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	enum SENSOR_SCENARIO_ID_ENUM scenario_id;
	enum SENSOR_SCENARIO_ID_ENUM current_scenario_id;
	struct mtk_hdr_ae *ae_ctrl = NULL;
	u64 *feature_data = (u64 *)para;
	u32 frame_length_in_lut[IMGSENSOR_STAGGER_EXPOSURE_CNT] = {0};
	u32 exp_cnt = 0;
	u16 kleeimx882wide_setting_seamless[300] = {0};
	u32 setting_seamless_len = 0;
	u32 i = 0;
	u32 retLen = 0;

	current_scenario_id = ctx->current_scenario_id;
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

	DRV_LOG(ctx, "seamless switch setting convert begin\n");
	for (i = 0; i< ctx->s_ctx.mode[scenario_id].mode_setting_len/2; i++){
		if (ctx->s_ctx.mode[scenario_id].mode_setting_table[2*i+1] !=
			ctx->s_ctx.mode[ctx->current_scenario_id].mode_setting_table[2*i+1]) {
			kleeimx882wide_setting_seamless[2*setting_seamless_len] = ctx->s_ctx.mode[scenario_id].mode_setting_table[2*i];
			kleeimx882wide_setting_seamless[2*setting_seamless_len+1] = ctx->s_ctx.mode[scenario_id].mode_setting_table[2*i + 1];
			setting_seamless_len++;
		}
	}

	DRV_LOG(ctx, "seamless switch setting convert end setting_seamless_len(%d)\n",setting_seamless_len);

	exp_cnt = ctx->s_ctx.mode[scenario_id].exp_cnt;
	ctx->is_seamless = TRUE;

	subdrv_i2c_wr_u8(ctx, 0x0104, 0x01);
	subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_fast_mode, 0x02);

	kleeimx882wide_update_mode_info(ctx, scenario_id);
	// i2c_table_write(ctx, ctx->s_ctx.mode[scenario_id].mode_setting_table, ctx->s_ctx.mode[scenario_id].mode_setting_len);
	i2c_table_write(ctx, kleeimx882wide_setting_seamless, 2 * setting_seamless_len);
	// if (ctx->s_ctx.reg_addr_fast_mode_in_lbmf &&
	// 	ctx->s_ctx.mode[scenario_id].hdr_mode == HDR_RAW_LBMF)
	// 	subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_fast_mode_in_lbmf, 0x4);
	kleeimx882wide_set_awb_gain(ctx, (u8 *)&g_last_awb_gain, &retLen);

	if (ae_ctrl) {
		switch (ctx->s_ctx.mode[scenario_id].hdr_mode) {
		case HDR_RAW_STAGGER:
			set_multi_shutter_frame_length(ctx, (u64 *)&ae_ctrl->exposure, exp_cnt, 0);
			set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			break;
		case HDR_RAW_LBMF:
			set_multi_shutter_frame_length_in_lut(ctx,
				(u64 *)&ae_ctrl->exposure, exp_cnt, 0, frame_length_in_lut);
			set_multi_gain_in_lut(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			break;
		case HDR_RAW_DCG_RAW:
			set_multi_shutter_frame_length(ctx, (u64 *)&ae_ctrl->exposure, 1, 0);
			if (ctx->s_ctx.mode[scenario_id].dcg_info.dcg_gain_mode
				== IMGSENSOR_DCG_DIRECT_MODE)
				set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			else
				set_gain(ctx, ae_ctrl->gain.le_gain);
			break;
		default:
			set_shutter_frame_length(ctx, ae_ctrl->exposure.le_exposure, 0);
			set_gain(ctx, ae_ctrl->gain.le_gain);
			break;
		}
		/* the time between the end of last frame readout and the next vsync need greater than 10ms */
		common_get_prsh_length_lines_by_time(ctx, ae_ctrl, current_scenario_id, scenario_id, 10);
	}

	if (ctx->s_ctx.seamless_switch_prsh_length_lc > 0) {
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_prsh_mode, 0x01);

		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[0],
				(ctx->s_ctx.seamless_switch_prsh_length_lc >> 16) & 0xFF);
		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[1],
				(ctx->s_ctx.seamless_switch_prsh_length_lc >> 8)  & 0xFF);
		subdrv_i2c_wr_u8(ctx,
				ctx->s_ctx.reg_addr_prsh_length_lines.addr[2],
				(ctx->s_ctx.seamless_switch_prsh_length_lc) & 0xFF);

		DRV_LOG_MUST(ctx, "seamless switch pre-shutter set(%u)\n",
			ctx->s_ctx.seamless_switch_prsh_length_lc);
	} else
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_prsh_mode, 0x00);

	subdrv_i2c_wr_u8(ctx, 0x0104, 0x00);

	ctx->fast_mode_on = TRUE;
	ctx->ref_sof_cnt = ctx->sof_cnt;
	ctx->is_seamless = FALSE;
	DRV_LOG(ctx, "X: set seamless switch done\n");
	return ERROR_NONE;
}

static int kleeimx882wide_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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

static int kleeimx882wide_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *) para;
	struct SET_SENSOR_AWB_GAIN *awb_gain = (struct SET_SENSOR_AWB_GAIN *)feature_data;
	MUINT32 r_Gain = awb_gain->ABS_GAIN_R >> 1;
	MUINT32 g_Gain = awb_gain->ABS_GAIN_GR >> 1;
	MUINT32 b_Gain = awb_gain->ABS_GAIN_B >> 1;

	g_last_awb_gain = *awb_gain;

	if (r_Gain == 0 || g_Gain == 0 || b_Gain == 0) {
		DRV_LOG(ctx, "error awb gain [r/g/b]: 0x%x 0x%x 0x%x\n",
			r_Gain, g_Gain, b_Gain);
		return 0;
	}

	// set awb gain
	subdrv_i2c_wr_u8(ctx, 0x0B8E, (g_Gain >> 8) & 0xFF); //Gr Gain [15:8]
	subdrv_i2c_wr_u8(ctx, 0x0B8F, (g_Gain >> 0) & 0xFF); //Gr Gain [7:0]
	subdrv_i2c_wr_u8(ctx, 0x0B90, (r_Gain >> 8) & 0xFF); //R Gain [15:8]
	subdrv_i2c_wr_u8(ctx, 0x0B91, (r_Gain >> 0) & 0xFF); //R Gain [7:0]
	subdrv_i2c_wr_u8(ctx, 0x0B92, (b_Gain >> 8) & 0xFF); //B Gain [15:8]
	subdrv_i2c_wr_u8(ctx, 0x0B93, (b_Gain >> 0) & 0xFF); //B Gain [7:0]
	subdrv_i2c_wr_u8(ctx, 0x0B94, (g_Gain >> 8) & 0xFF); //Gb Gain [15:8]
	subdrv_i2c_wr_u8(ctx, 0x0B95, (g_Gain >> 0) & 0xFF); //Gb Gain [7:0]

	DRV_LOG(ctx, "awb gain [r/g/b]: 0x%x 0x%x 0x%x\n",
		r_Gain, g_Gain, b_Gain);

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
	kal_uint16 sensor_output_cnt;

	sensor_output_cnt = subdrv_i2c_rd_u8(ctx, ctx->s_ctx.reg_addr_frame_count);
	DRV_LOG_MUST(ctx, "sensormode(%d) sof_cnt(%d) sensor_output_cnt(%d)\n",
		ctx->current_scenario_id, sof_cnt, sensor_output_cnt);

	if (ctx->fast_mode_on && (sof_cnt > ctx->ref_sof_cnt)) {
		ctx->fast_mode_on = FALSE;
		ctx->ref_sof_cnt = 0;
		DRV_LOG(ctx, "seamless_switch disabled.");
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_prsh_mode, 0x00);
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_fast_mode, 0x00);
		commit_i2c_buffer(ctx);
	}
	return 0;
}
