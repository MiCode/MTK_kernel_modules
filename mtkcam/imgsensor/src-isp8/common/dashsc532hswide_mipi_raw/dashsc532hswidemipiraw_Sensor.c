// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 dashsc532hswidemipiraw_Sensor.c
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
 ****************************************************************************/


#include "dashsc532hswidemipiraw_Sensor.h"
#define EEPROM_READY 1

#if EEPROM_READY
static void set_sensor_cali(void *arg);
#endif
static u16 get_gain2reg(u32 gain);
static int dashsc532hswide_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt);
static void mi_stream(void *arg,bool enable);
static int dashsc532hswide_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int dashsc532hswide_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static void set_shutter_seamless(struct subdrv_ctx *ctx, u64 shutter);
static int dashsc532hswide_set_multi_dig_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static void dashsc532hswide_set_multi_gain(struct subdrv_ctx *ctx, u32 *gains, u16 exp_cnt);
static int dashsc532hswide_set_hdr_tri_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int dashsc532hswide_get_sensor_temperature(void *arg);

#define SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE                 1
#define SEAMLESS_SWITCH_GROUP_HD_CAP_MODE                     3
#define SEAMLESS_SWITCH_GROUP_VIDEO_MODE                      2
#define SEAMLESS_SWITCH_GROUP_VIDEO_NIGHT_MODE                4
#define SEAMLESS_SWITCH_GROUP_BOKEH                           5

struct SET_SENSOR_AWB_GAIN dashsc532hswide_last_awb_gain = {0, 0, 0, 0};

/* STRUCT */
static struct mtk_sensor_saturation_info dashsc532hs_saturation_info_14bit = {
	.gain_ratio = 4000,
	.OB_pedestal = 1024,
	.saturation_level = 16368,
	.adc_bit = 10,
	.ob_bm = 64,
};

static struct mtk_sensor_saturation_info dashsc532hs_saturation_info_fake14bit = {
	.gain_ratio = 1000,
	.OB_pedestal = 1024,
	.saturation_level = 16383,
	.adc_bit = 14,
};

//static u32 dashsc532hs_dcg_ratio_table_14bit[] = {16000};

static struct mtk_sensor_saturation_info dashsc532hs_saturation_info_10bit = {
	.gain_ratio = 1000,
	.OB_pedestal = 64,
	.saturation_level = 1023,
};

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, dashsc532hswide_set_test_pattern},
	{SENSOR_FEATURE_SEAMLESS_SWITCH, dashsc532hswide_seamless_switch},
	{SENSOR_FEATURE_SET_AWB_GAIN, dashsc532hswide_set_awb_gain},
	{SENSOR_FEATURE_SET_MULTI_DIG_GAIN, dashsc532hswide_set_multi_dig_gain},
	{SENSOR_FEATURE_SET_DUAL_GAIN,dashsc532hswide_set_hdr_tri_gain},
};

static struct eeprom_info_struct eeprom_info[] = {
	{
		.header_id = 0x01, // vendor id : 0x01 == sunny
		.addr_header_id = 0x01, // vendor id addr
		.i2c_write_id = 0xA2,

		// QSC Calibration 1th
		.qsc_support = TRUE,
		.qsc_size = 3024,
		.addr_qsc = 0x2956,
		.sensor_reg_addr_qsc = 0xC000,
	},
	{
		.header_id = 0x07, // vendor id : 0x07 == ofilm
		.addr_header_id = 0x01, // vendor id addr
		.i2c_write_id = 0xA2,

		// QSC Calibration 1th
		.qsc_support = TRUE,
		.qsc_size = 3024,
		.addr_qsc = 0x1B7B,
		.sensor_reg_addr_qsc = 0xC000,
	},
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
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
	.i4FullRawW = 4096,
	.i4FullRawH = 3072,
	.i4ModeIndex = 3, /*HVBin 2; VBin 3*/
	.sPDMapInfo[0] = {
		.i4PDPattern = 1,//1: dense pd
		.i4BinFacX = 2,//
		.i4BinFacY = 4,
		.i4PDRepetition = 0,
		.i4PDOrder = {1},
	},
	.i4Crop = {
		// <pre> <cap> <normal_video> <hs_video> <slim_video>
		{0, 0}, {0, 0}, {0, 384}, {0, 0}, {0, 0},
		// <cust1> <cust2> <cust3> <cust4> <cust5>
		{0, 384}, {0, 0}, {0, 0}, {2048, 1536}, {0, 0},
		// <cust6> <cust7> <cust8> <cust9>< cust10>
		{2048, 1536}, {0, 0}, {0, 0}, {2048, 1536}, {0, 0},
		// <cust11> <cust12> cust13 <cust14> <cust15>
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 256},
		// <cust16> <cust17>
		{2048, 1792}, {0, 256}
	},
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_video_partial = {
	.i4OffsetX = 0,
	.i4OffsetY = 0,
	.i4PitchX = 16,
	.i4PitchY = 16,
	.i4PairNum = 8,
	.i4SubBlkW = 4,
	.i4SubBlkH = 8,
	.i4PosL = {{2, 1},{2, 13},{6, 1},{6, 13},{10, 1},{10, 13},{14, 1},{14, 13}},
	.i4PosR = {{2, 5},{2, 9},{6, 5},{6, 9},{10, 5},{10, 9},{14, 5},{14, 9}},
	.i4BlockNumX = 256,
	.i4BlockNumY = 192,
	.i4VCPackNum = 1,
	.PDAF_Support = PDAF_SUPPORT_CAMSV,
	.iMirrorFlip = 0,
	.i4FullRawW = 4096,
	.i4FullRawH = 3072,
	.i4ModeIndex = 0,
	/* VC's PD pattern description */
	.sPDMapInfo[0] = {
		.i4PDPattern = 2,
		.i4BinFacX = 0,
		.i4BinFacY = 0,
		.i4PDOrder = {1},
		.i4PDRepetition = 2,
	},
	.i4Crop = {
		// <pre> <cap> <normal_video> <hs_video> <slim_video>
		{0, 0}, {0, 0}, {0, 384}, {0, 0}, {0, 0},
		// <cust1> <cust2> <cust3> <cust4> <cust5>
		{0, 384}, {0, 0}, {0, 0}, {2048, 1536}, {0, 0},
		// <cust6> <cust7> <cust8> <cust9>< cust10>
		{2048, 1536}, {0, 0}, {0, 0}, {2048, 1536}, {0, 0},
		// <cust11> <cust12> cust13 <cust14> <cust15>
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 256},
		// <cust16> <cust17>
		{2048, 1792}, {0, 256}
	},
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
	.i4FullRawW = 8192,
	.i4FullRawH = 6144,
	.i4ModeIndex = 3, /*HVBin 2; VBin 3*/
	.sPDMapInfo[0] = {
		.i4PDPattern = 1, /*1: Dense; 2: Sparse LR interleaved; 3: Sparse LR non interleaved*/
		.i4BinFacX = 4, /*for Dense*/
		.i4BinFacY = 4,
		.i4PDRepetition = 4,
		.i4PDOrder = {1},
	},
	.i4Crop = {
		// <pre> <cap> <normal_video> <hs_video> <slim_video>
		{0, 0}, {0, 0}, {0, 384}, {0, 0}, {0, 0},
		// <cust1> <cust2> <cust3> <cust4> <cust5>
		{0, 384}, {0, 0}, {0, 0}, {2048, 1536}, {0, 0},
		// <cust6> <cust7> <cust8> <cust9>< cust10>
		{2048, 1536}, {0, 0}, {0, 0}, {2048, 1536}, {0, 0},
		// <cust11> <cust12> cust13 <cust14> <cust15>
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 256},
		// <cust16> <cust17>
		{2048, 1792}, {0, 256}
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
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
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
			.channel = 3,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 576,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_hs_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 1920,
			.vsize = 1080,
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
			.vsize = 2304,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 288,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 1920,
			.vsize = 1080,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
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

static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
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
			.channel = 3,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus5[] = {
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

static struct mtk_mbus_frame_desc_entry frame_desc_cus6[] = {
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
			.channel = 3,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus7[] = {
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

static struct mtk_mbus_frame_desc_entry frame_desc_cus8[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus9[] = {
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
			.channel = 3,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus11[] = {
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
			.channel = 3,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus12[] = {
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_ME,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus15[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 4096,
			.vsize = 2560,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 640,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus16[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 4096,
			.vsize = 2560,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 640,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus17[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 4096,
			.vsize = 2560,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 640,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
		},
	},
};

	// preview mode 0
	// V_2_RSS 4096x3072_binning_10bit_30fps_PD_DT_4096x768_Max_A_gain_64x
	// PD size = 4096x768
	// Tline = 8.2803us
	// VB = 7.88ms
	// version:2025-0820-v03
#define PRE_MODE  .frame_desc = frame_desc_prev,\
		.num_entries = ARRAY_SIZE(frame_desc_prev),\
		.mode_setting_table = dashsc532hswide_preview_setting,\
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_preview_setting),\
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_HD_CAP_MODE,\
		.seamless_switch_mode_setting_table = dashsc532hswide_preview_setting,\
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_preview_setting),\
		.hdr_mode = HDR_NONE,\
		.raw_cnt = 1,\
		.exp_cnt = 1,\
		.pclk = 132000000,\
		.linelength = 1093,\
		.framelength = 4024,\
		.max_framerate = 300,\
		.mipi_pixel_rate = 1181952000,\
		.readout_length = 0,\
		.read_margin = 24,\
		.framelength_step = 2,\
		.coarse_integ_step = 1,\
		.min_exposure_line = 5,\
		.mi_mode_type = 2, \
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,\
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,\
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN ,\
		.imgsensor_winsize_info = {\
			.full_w = 8192,\
			.full_h = 6144,\
			.x0_offset = 0,\
			.y0_offset = 0,\
			.w0_size = 8192,\
			.h0_size = 6144,\
			.scale_w = 4096,\
			.scale_h = 3072,\
			.x1_offset = 0,\
			.y1_offset = 0,\
			.w1_size = 4096,\
			.h1_size = 3072,\
			.x2_tg_offset = 0,\
			.y2_tg_offset = 0,\
			.w2_tg_size = 4096,\
			.h2_tg_size = 3072,\
		},\
		.pdaf_cap = TRUE,\
		.imgsensor_pd_info = &imgsensor_pd_info,\
		.ae_binning_ratio = 1000,\
		.fine_integ_line = 0,\
		.delay_frame = 2,

static struct subdrv_mode_struct mode_struct[] = {
	// preview mode 0
	{
		PRE_MODE
	},
	// capture mode 1: same as preview mode
	{
		PRE_MODE
	},
	// normal_video mode 2
	// V_8_RSS_crop 4096x2304_binning_crop_10bit_30fps_PDDT_4096x576_Max_A_gain_64x
	// PD size = 4096x576
	// Tline = 8.2803us
	// VB = 14.24 ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = dashsc532hswide_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_normal_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = dashsc532hswide_normal_video_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_normal_video_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 132000000,
		.linelength = 1093,
		.framelength = 4024,
		.max_framerate = 300,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 5,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 2,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// hs_video mode 3 : smvr 240fps
	// V_16_SUM_V2H2 1920x1080_binning_crop_10bit_240fps_no_PD_Max_A_gain_64x
	// no PD
	// Tline = 2.95us
	// VB = 0.97ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = dashsc532hswide_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = dashsc532hswide_hs_video_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_hs_video_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 108000000,
		.linelength = 319,
		.framelength = 1410,
		.max_framerate = 2400,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 256,
			.y0_offset = 912,
			.w0_size = 7680,
			.h0_size = 4320,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// slim_video mode 4: same as preview mode
	{
		PRE_MODE
	},
	// custom1 mode 5
	// V_9_SUM_CROP 4096x2304_binning_crop_10bit_60fps_PDDT_4096x576_Max_A_gain_64x
	// PD size = 2048x288
	// Tline = 5.2us
	// VB = 4.66 ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = dashsc532hswide_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom1_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom1_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom1_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 108000000,
		.linelength = 562,
		.framelength = 3200,
		.max_framerate = 600,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 5,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 2,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_video_partial,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// custom2 mode 6 : smvr 120fps
	// V_17_SUM_V2H2 1920x1080_binning_crop_10bit_120fps_no_pd_Max_A_gain_64x
	// PD Size = null
	// Tline = 2.95us
	// VB = 5.14ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = dashsc532hswide_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom2_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom2_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom2_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 108000000,
		.linelength = 319,
		.framelength = 2820,
		.max_framerate = 1200,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 256,
			.y0_offset = 912,
			.w0_size = 7680,
			.h0_size = 4320,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// custom3 mode 7 : fullsize 10bit quad
	// V_1 8192x6144_full_quad_bayer_10bit_30fps_no_PD_Max_A_gain_16x
	// Tline = 10.4us
	// VB = 2.66ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = dashsc532hswide_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom3_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom3_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom3_setting),
		.hdr_mode = PARAM_UNDEFINED,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 108000000,
		.linelength = 1124,
		.framelength = 6400,
		.max_framerate = 150,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 6,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 16,
		.ana_gain_min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_B,
	},
	// custom4 mode 8 : ISZ 10bit quad
	// V_4 4096x3072_full_crop_quad_bayer_10bit_30fps_PD_DT_2048x768_Max_A_gain_16x
	// PD Size = 2048x768
	// Tline = 10.4 us
	// VB = 1.33ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = dashsc532hswide_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom4_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom4_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom4_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 108000000,
		.linelength = 1124,
		.framelength = 3200,
		.max_framerate = 300,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 6,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 16,
		.ana_gain_min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_isz,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_B,
	},
	// custom5 mode 9 : full size bayer 10bit
	// V_0 8192x6144_full_bayer_10bit_15fps_no_PD_Max_A_gain_16x
	// no PD
	// Tline = 10.4 us
	// VB = 2.66ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = dashsc532hswide_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom5_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom5_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom5_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 108000000,
		.linelength = 1124,
		.framelength = 6400,
		.max_framerate = 150,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 6,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 16,
		.ana_gain_min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.awb_enabled = true,
	},
	// custom6 mode 10: ISZ bayer 10bit
	// V_3	4096x3072_full_crop_bayer_10bit_30fps_PD_DT_2048x768_Max_A_gain_16x
	// PD Size = 2048x768
	// Tline = 10.4us
	// VB = 1.33ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus6,
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.mode_setting_table = dashsc532hswide_custom6_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom6_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom6_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom6_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 108000000,
		.linelength = 1124,
		.framelength = 3200,
		.max_framerate = 300,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 6,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 16,
		.ana_gain_min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_isz,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.awb_enabled = true,
	},
	//custom7 mode 11 : full size quad for hd capture
	// V_1 8192x6144_full_bayer_quad_10bit_30fps_no_PD_Max_A_gain_16x
	// no PD
	// Tline = 10.4us
	// VB = 2.66ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus7,
		.num_entries = ARRAY_SIZE(frame_desc_cus7),
		.mode_setting_table = dashsc532hswide_custom7_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom7_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_HD_CAP_MODE,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom7_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom7_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 108000000,
		.linelength = 1124,
		.framelength = 6400,
		.max_framerate = 150,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 6,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 16,
		.ana_gain_min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_B,
	},
	//custom8 mode 12 : binning size 24fps for bokeh
	// V_6_RSS 4096x3072_binning_10bit_24fps_PD_4096x768_Max_A_gain_64x
	// PD Size = 4096x768
	// Tline = 8.28us
	// VB = 16.23ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus8,
		.num_entries = ARRAY_SIZE(frame_desc_cus8),
		.mode_setting_table = dashsc532hswide_custom8_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom8_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom8_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom8_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 132000000,
		.linelength = 1093,
		.framelength = 5032,
		.max_framerate = 240,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 5,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.delay_frame = 2,
	},
	// custom9 mode 13: 2X bokeh
	// V_7 4096x3072_full_crop_quad_10bit_24fps_PD_DT_2048x768_Max_A_gain_16x
	// PD Size = 2048x768
	// Tline = 10.4 us
	// VB = 9.66 ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus9,
		.num_entries = ARRAY_SIZE(frame_desc_cus9),
		.mode_setting_table = dashsc532hswide_custom9_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom9_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_BOKEH,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom9_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom9_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 108000000,
		.linelength = 1124,
		.framelength = 4000,
		.max_framerate = 240,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 6,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 16,
		.ana_gain_min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_isz,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_B,
	},
	// custom10 mode 14: bokeh slave(same as custom8)
	// V_6_RSS
	// PD size = 4096x768
	// Tline = 8.28us
	// VB = 16.23ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus8,
		.num_entries = ARRAY_SIZE(frame_desc_cus8),
		.mode_setting_table = dashsc532hswide_custom8_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom8_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_BOKEH,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom8_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom8_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 132000000,
		.linelength = 1093,
		.framelength = 5032,
		.max_framerate = 240,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 5,
		.mi_mode_type = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN ,
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
		.delay_frame = 2,
	},
	// custom11 mode 15: 4:3 binning 10bit
	// V_2_RSS 4096x3072_binning_10bit_30fps_PD_DT_4096x768_Max_A_gain_64x
	// PD size = 4096x768
	// Tline = 8.2803us
	// VB = 7.88ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus11,
		.num_entries = ARRAY_SIZE(frame_desc_cus11),
		.mode_setting_table = dashsc532hswide_custom11_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom11_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom11_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom11_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 132000000,
		.linelength = 1093,
		.framelength = 4024,
		.max_framerate = 300,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 5,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.delay_frame = 2,
	},
	// custom12 mode 16: stagger exp2 10bit
	// Mode_V_5_SUM_FDOL5 4096x3072_binning_STG2_HDR_10bit_30fps_no_pd_Max_A_gain_64x
	// PD Size = NULL
	// Tline = 5.25us
	// VB = 17.2ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus12,
		.num_entries = ARRAY_SIZE(frame_desc_cus12),
		.mode_setting_table = dashsc532hswide_custom12_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom12_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom12_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom12_setting),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 108000000,
		.linelength = 567,
		.framelength = 6348,
		.max_framerate = 300,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 768 * 2,
		.read_margin = 128,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// custom13 mode 17: unused
	{
		PRE_MODE
	},
	// custom14 mode 18: unused
	{
		PRE_MODE
	},
	// custom15 mode 19: video fake 14bit 16:10 
	// V_10_RSS_CROP 4096x2560_binning_crop_fake14bit_30fps_PD_DT_4096x640_Max_A_gain_64x
	// PD Size = 4096x640
	// Tline = 8.28us
	// VB = 12.12ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus15,
		.num_entries = ARRAY_SIZE(frame_desc_cus15),
		.mode_setting_table = dashsc532hswide_custom15_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom15_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_MODE,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom15_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom15_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 132000000,
		.linelength = 1093,
		.framelength = 4024,
		.max_framerate = 300,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 5,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &dashsc532hs_saturation_info_fake14bit,
	},
	// custom16 mode 20 14bit video
	// V_12_RSS_CROP 4096x2560_binning_4C2Plus_crop_DCG_combine_MSB_14bit_30fps_PD_DT_4096x640_Max_A_gain_64x
	// PD Size = 2048x640
	// Tline = 10.04us
	// VB = 6.66ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus16,
		.num_entries = ARRAY_SIZE(frame_desc_cus16),
		.mode_setting_table = dashsc532hswide_custom16_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom16_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_MODE,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom16_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom16_setting),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 132000000,
		.linelength = 830,
		.framelength = 5296,
		.max_framerate = 300,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.mi_mode_type = 3, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN * 1,
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
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_ratio_min = 4000,
			.dcg_gain_ratio_max = 16000,
			.dcg_gain_ratio_step = 0,
			//.dcg_ratio_group = {4096, 1024}, // HCG = 5.7*1024, LCG = 1024
		},
		.bit_align_type = IMGSENSOR_PIXEL_LSB_ALIGN,
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &dashsc532hs_saturation_info_14bit,
	},
	// custom17 mode 21 : full size crop bayer 16:10 fake14 bit
	// V_11_FULL_CROP 4096x2560_full_crop_bayer_fake14bit_30fps_PD_DT_2048x640_Max_A_gain_16x
	// PD Size = 4096x640
	// Tline = 6.28us
	// VB = 1.11ms
	// version:2025-0820-v03
	{
		.frame_desc = frame_desc_cus17,
		.num_entries = ARRAY_SIZE(frame_desc_cus17),
		.mode_setting_table = dashsc532hswide_custom17_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom17_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_MODE,
		.seamless_switch_mode_setting_table = dashsc532hswide_custom17_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(dashsc532hswide_custom17_setting),
		.hdr_mode = PARAM_UNDEFINED,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 108000000,
		.linelength = 1124,
		.framelength = 3200,
		.max_framerate = 300,
		.mipi_pixel_rate = 1181952000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.min_exposure_line = 6,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN ,
		.ana_gain_max = BASEGAIN * 16,
		.ana_gain_min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_isz,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &dashsc532hs_saturation_info_fake14bit,
		.awb_enabled = true,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = DASHSC532HSWIDE_SENSOR_ID,
	.reg_addr_sensor_id = {0x3107, 0x3108},
	.i2c_addr_table = {0x20,0xFF},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {8192, 6144},
	.mirror = IMAGE_NORMAL,

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_2MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_CPHY,
	.mipi_lane_num = SENSOR_MIPI_3_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
	.dig_gain_min = BASEGAIN * 1,
	.dig_gain_max = BASEGAIN * 16,
	.dig_gain_step = 4,  //If the value is 0, SENSOR_FEATURE_SET_MULTI_DIG_GAIN is disabled
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 64,
	.ana_gain_type = 0,
	.ana_gain_step = 1,
	.ana_gain_table = dashsc532hswide_ana_gain_table,
	.ana_gain_table_size = sizeof(dashsc532hswide_ana_gain_table),
	.tuning_iso_base = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 4,
	.exposure_max = 0xFFFFF0 - 16,
	.exposure_step = 1,
	.exposure_margin = 16,
	.saturation_info = &dashsc532hs_saturation_info_10bit,

	.frame_length_max = 0xFFFFF0,
	.ae_effective_frame = 3,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 1722000,

	.pdaf_type = PDAF_SUPPORT_CAMSV_QPD,
	.hdr_type = HDR_SUPPORT_STAGGER_DOL|HDR_SUPPORT_DCG,
	.seamless_switch_support = TRUE,
	.temperature_support = TRUE,

	.g_gain2reg = get_gain2reg,
	.s_mi_stream = mi_stream,
	//.s_mi_read_CGRatio = mi_read_CGRatio,
	.mi_hb_vb_cal = false,
#if EEPROM_READY
	.s_cali = set_sensor_cali,
#else
	.s_cali = NULL,
#endif
	.reg_addr_stream = 0x0100,
	// .reg_addr_mirror_flip = 0x3820,
	.reg_addr_exposure = {
		{0x3e00, 0x3e01, 0x3e02},
		{0x3e22, 0x3e04, 0x3e05},
		{0x3e50, 0x3e51, 0x3e52}
	},
	.long_exposure_support = TRUE,
	// .reg_addr_exposure_lshift = 0x3160,
	.reg_addr_ana_gain = {
		{0x3e08, 0x3e09},
		{0x3e12, 0x3e13},
		{0x3e82, 0x3e83},
	},
	.reg_addr_dig_gain = {
		{0x3e06, 0x3e07, 0x3e2c},
		{0x3e10, 0x3e11, 0x3e3c},
		{0x3e80, 0x3e81, 0x3e90},
	},
	.reg_addr_dcg_ratio = PARAM_UNDEFINED, //TBD
	.reg_addr_frame_length = {0x326d, 0x320e, 0x320f},
	.reg_addr_temp_en = PARAM_UNDEFINED,
	.reg_addr_temp_read = PARAM_UNDEFINED,
	.g_temp = dashsc532hswide_get_sensor_temperature,
	// .reg_addr_auto_extend = 0x0350,
	.reg_addr_frame_count = 0x59EE,//TBD 0x59EE,0x59Ef
	// .reg_addr_fast_mode = 0x3010,

	// .mi_disable_set_dummy = 1, // disable set dummy
	// .mi_evaluate_frame_rate_by_scenario = evaluate_frame_rate_by_scenario,
	.init_setting_table = dashsc532hswide_init_setting,
	.init_setting_len = ARRAY_SIZE(dashsc532hswide_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 0,
	.chk_s_off_end = 0,

	//TBD
	.checksum_value = 0xAF3E324E,
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
	{HW_ID_AFVDD, {3100000, 3100000},  1000},
	{HW_ID_MCLK,   {24},        0},
	{HW_ID_RST,    {0},         0},
	{HW_ID_DVDD,  {1800000, 1800000},  2000},
	{HW_ID_AVDD,  {2800000, 2800000},  2000},
	{HW_ID_DOVDD, {1800000, 1800000},  2000},
	{HW_ID_MCLK_DRIVING_CURRENT, {4},  2000},
	{HW_ID_RST,    {1},       10000},
};


const struct subdrv_entry dashsc532hswide_mipi_raw_entry = {
    .name = "dashsc532hswide_mipi_raw",
    .id = DASHSC532HSWIDE_SENSOR_ID,
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
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			DRV_LOG(ctx, "set QSC calibration data done.");
		} else {
			DRV_LOG(ctx, "pbuf(%p) addr(%d) size(%d) set QSC calibration data fail.",
				pbuf, addr, size);
		}
	}
}
#endif

void mi_stream_check_stream_off(struct subdrv_ctx *ctx)
{
	u32 i = 0, framecnt = 0;
	u32 timeout = 5;
	DRV_LOG_MUST(ctx, "mi_stream_check_stream_off enter");

	framecnt = subdrv_ixc_rd_u8(ctx, ctx->s_ctx.reg_addr_frame_count);

	if (ctx->is_seamless == FALSE)
		return;

	for (i = 0; i < timeout; i++) {
		usleep_range(20000,21000);
		DRV_LOG_MUST(ctx, "mi_stream_check_stream_off check");
		ctx->is_seamless = FALSE;
		if(framecnt != subdrv_ixc_rd_u8(ctx, ctx->s_ctx.reg_addr_frame_count))
			return;
	}

	DRV_LOG_MUST(ctx, "mi_stream_check_stream_off fail");
}

static void mi_stream(void *arg,bool enable)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	u32 len = 0;
	DRV_LOG_MUST(ctx, "Enter %s enable = %d \n", __FUNCTION__,enable);
	if(enable){
		dashsc532hswide_set_awb_gain(ctx, (u8 *)&dashsc532hswide_last_awb_gain, &len);
		i2c_table_write(ctx, dashsc532hswide_stream_on, ARRAY_SIZE(dashsc532hswide_stream_on));
		mdelay(5);
	}else{
		i2c_table_write(ctx, dashsc532hswide_stream_off, ARRAY_SIZE(dashsc532hswide_stream_off));
	}
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
		DRV_LOG(ctx, "seamless_switch finish.");
	}

	return 0;

}

static u16 get_gain2reg(u32 gain)
{
	return gain * 128 / BASEGAIN;
}

static int dashsc532hswide_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	switch (mode) {
	case 5:
		subdrv_i2c_wr_u8(ctx, 0x3271, 0x6d);
		break;
	default:
		subdrv_i2c_wr_u8(ctx, 0x3271, 0x6c);
		break;
	}

	ctx->test_pattern = mode;
	return ERROR_NONE;

}

static void set_tline_ns(struct subdrv_ctx *ctx, u32 curr_vts, u32 curr_shutter, u32 curr_hts, u32 curr_pclk)
{
	u32 tline_ns;
	u16 st_dummy_line=0x00;
	u32 shutter_15ms;
	u32 st_vts = curr_vts;

	tline_ns = curr_hts * 10000 / (curr_pclk / 100000);
	shutter_15ms = 15000000 / tline_ns;

	if (curr_shutter > curr_vts - 32)
		st_vts = curr_shutter + 32;

	if(curr_shutter > shutter_15ms)
		st_dummy_line = st_vts - curr_shutter - 12;
	else
		st_dummy_line = st_vts - shutter_15ms - 12;

	// TBD: 打开pd后影响seamless切换，暂时去掉，FAE优化后重新添加
	// subdrv_i2c_wr_u8(ctx, 0x3230, (st_dummy_line >> 8) & 0xff);
	// subdrv_i2c_wr_u8(ctx, 0x3231, (st_dummy_line) & 0xff);

	return;
}

void set_shutter_seamless(struct subdrv_ctx *ctx, u64 shutter)
{
	int fine_integ_line = 0;

	check_current_scenario_id_bound(ctx);
	/* check boundary of shutter */
	fine_integ_line = ctx->s_ctx.mode[ctx->current_scenario_id].fine_integ_line;
	shutter = FINE_INTEG_CONVERT(shutter, fine_integ_line);
	shutter = max_t(u64, shutter,
		(u64)ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_shutter_range[0].min);
	shutter = min_t(u64, shutter,
		(u64)ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_shutter_range[0].max);

	/* restore shutter */
	memset(ctx->exposure, 0, sizeof(ctx->exposure));
	ctx->exposure[0] = (u32) shutter;

	/* enable auto extend */
	if (ctx->s_ctx.reg_addr_auto_extend)
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_auto_extend, 0x01);

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

}

static int dashsc532hswide_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	enum SENSOR_SCENARIO_ID_ENUM scenario_id;
	enum IMGSENSOR_HDR_MODE_ENUM src_hdr;
	struct mtk_hdr_ae *ae_ctrl = NULL;
	u64 *feature_data = (u64 *)para;
	u32 exp_cnt = 0;
	u32 retLen = 0;

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

	ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_LE] = ae_ctrl->gain.le_gain;
	ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_ME] = ae_ctrl->gain.me_gain;

	exp_cnt = ctx->s_ctx.mode[scenario_id].exp_cnt;
	ctx->is_seamless = TRUE;
	src_hdr = ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode;

	update_mode_info(ctx, scenario_id);

	i2c_table_write(ctx, dashsc532hswide_seamless_switch_step1,
		ARRAY_SIZE(dashsc532hswide_seamless_switch_step1));

	dashsc532hswide_set_awb_gain(ctx, (u8 *)&dashsc532hswide_last_awb_gain, &retLen);

	i2c_table_write(ctx, ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table,
		ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_len);

	if (ae_ctrl) {
		switch (ctx->s_ctx.mode[scenario_id].hdr_mode) {
		case HDR_RAW_STAGGER:
			set_multi_shutter_frame_length(ctx, (u64 *)&ae_ctrl->exposure, exp_cnt, 0);
			set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			break;
		case HDR_RAW_DCG_COMPOSE:
			set_shutter_seamless(ctx, ae_ctrl->exposure.le_exposure);
			if (ctx->s_ctx.mode[scenario_id].dcg_info.dcg_gain_mode
				== IMGSENSOR_DCG_DIRECT_MODE)
				dashsc532hswide_set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			else
				set_gain(ctx, ae_ctrl->gain.le_gain);
			break;
		default:
			set_shutter_seamless(ctx, ae_ctrl->exposure.le_exposure);
			set_gain(ctx, ae_ctrl->gain.le_gain);
			break;
		}
	}
	set_tline_ns(ctx, ctx->s_ctx.mode[scenario_id].framelength, ae_ctrl->exposure.le_exposure, ctx->s_ctx.mode[scenario_id].linelength, ctx->s_ctx.mode[scenario_id].pclk);

	i2c_table_write(ctx, dashsc532hswide_seamless_switch_step2,
		ARRAY_SIZE(dashsc532hswide_seamless_switch_step2));

	ctx->fast_mode_on = TRUE;
	ctx->ref_sof_cnt = ctx->sof_cnt;
	DRV_LOG(ctx, "X: set seamless switch done\n");
	return ERROR_NONE;
}

static int dashsc532hswide_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *) para;
	struct SET_SENSOR_AWB_GAIN *awb_gain = (struct SET_SENSOR_AWB_GAIN *)feature_data;
	MUINT32 r_Gain = awb_gain->ABS_GAIN_R;
	MUINT32 b_Gain = awb_gain->ABS_GAIN_B;
	MUINT32 gr_Gain = awb_gain->ABS_GAIN_GR;
	MUINT32 gb_Gain = awb_gain->ABS_GAIN_GB;
	UINT32 r_gain_int = 0;
	UINT32 b_gain_int = 0;
	UINT32 gr_gain_int = 0;
	UINT32 gb_gain_int = 0;
	UINT32 gain_base = 0x81;

	dashsc532hswide_last_awb_gain = *awb_gain;

	if (r_Gain == 0 || gr_Gain == 0 || gb_Gain == 0 || b_Gain == 0) {
		DRV_LOG(ctx, "error awb gain [r/gr/gb/b]: 0x%x 0x%x 0x%x 0x%x\n",
			r_Gain, gr_Gain, gb_Gain, b_Gain);
		return 0;
	}

	DRV_LOG(ctx, "feedback_awbgain: r_gain = %d b_gain = %d\n",r_Gain, b_Gain);
	DRV_LOG(ctx, "feedback_awbgain: gr_gain = %d gb_gain = %d\n",gr_Gain, gb_Gain);
	r_gain_int = r_Gain * gain_base/ 512;
	b_gain_int = b_Gain * gain_base/ 512;
	gr_gain_int = gr_Gain * gain_base/ 512;
	gb_gain_int = gb_Gain * gain_base/ 512;

	r_gain_int = (r_gain_int < gain_base) ? gain_base : r_gain_int;
	b_gain_int = (b_gain_int < gain_base) ? gain_base : b_gain_int;
	gr_gain_int = (gr_gain_int < gain_base) ? gain_base : gr_gain_int;
	gb_gain_int = (gb_gain_int < gain_base) ? gain_base : gb_gain_int;

	DRV_LOG(ctx, "r_gain_final = 0x%x b_gain_final = 0x%x\n", r_gain_int, b_gain_int);
	DRV_LOG(ctx, "gr_gain_final = 0x%x gb_gain_final = 0x%x\n", gr_gain_int, gb_gain_int);
	//setGPHon(KAL_TRUE);
	subdrv_i2c_wr_u8(ctx, 0x5415, (r_gain_int>>8)&0xff);		//R_H   1x=0x81
	subdrv_i2c_wr_u8(ctx, 0x5416, r_gain_int&0xff); //R_L
	subdrv_i2c_wr_u8(ctx, 0x5417, (gb_gain_int>>8)&0xff);       //GB_H
	subdrv_i2c_wr_u8(ctx, 0x5418, gb_gain_int&0xff);  //GB_L
	subdrv_i2c_wr_u8(ctx, 0x569c, (gr_gain_int>>8)&0xff);       //GR_H
	subdrv_i2c_wr_u8(ctx, 0x569d, gr_gain_int&0xff);  //GR_L
	subdrv_i2c_wr_u8(ctx, 0x5413, (b_gain_int>>8)&0xff);       //B_H
	subdrv_i2c_wr_u8(ctx, 0x5414, b_gain_int&0xff); //B_L

	return ERROR_NONE;
}

static int dashsc532hswide_set_multi_dig_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	int i = 0;
	u16 rg_gains[IMGSENSOR_STAGGER_EXPOSURE_CNT] = {0};
	bool gph = !ctx->is_seamless && (ctx->s_ctx.s_gph != NULL);
	u64 *feature_data = (u64 *)para;
	u32 *gains = (u32 *)(*feature_data);
	u16 exp_cnt = (u16) (*(feature_data + 1));

	if (ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_LBMF) {
		set_multi_dig_gain_in_lut(ctx, gains, exp_cnt);
		return 0;
	}
	// skip if no porting digital gain
	if (!ctx->s_ctx.reg_addr_dig_gain[0].addr[0])
		return 0;

	if (exp_cnt > ARRAY_SIZE(ctx->dig_gain)) {
		DRV_LOGE(ctx, "invalid exp_cnt:%u>%lu\n", exp_cnt, ARRAY_SIZE(ctx->dig_gain));
		exp_cnt = ARRAY_SIZE(ctx->dig_gain);
	}
	for (i = 0; i < exp_cnt; i++) {
		/* check boundary of gain */
		gains[i] = max(gains[i], ctx->s_ctx.dig_gain_min);
		gains[i] = min(gains[i], ctx->s_ctx.dig_gain_max);
		gains[i] = gains[i] / BASE_DGAIN * 1024;
	}

	/* restore gain */
	memset(ctx->dig_gain, 0, sizeof(ctx->dig_gain));
	for (i = 0; i < exp_cnt; i++)
		ctx->dig_gain[i] = gains[i];

	/* group hold start */
	if (gph && !ctx->ae_ctrl_gph_en)
		ctx->s_ctx.s_gph((void *)ctx, 1);

	/* write gain */
	switch (exp_cnt) {
	case 1:
		rg_gains[0] = gains[0];
		break;
	case 2:
		rg_gains[0] = gains[0];
		rg_gains[2] = gains[1];
		break;
	case 3:
		rg_gains[0] = gains[0];
		rg_gains[1] = gains[1];
		rg_gains[2] = gains[2];
		break;
	default:
		break;
	}
	if(ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_DCG_COMPOSE){
		rg_gains[2] = gains[0];
	}
	for (i = 0;
	     (i < ARRAY_SIZE(rg_gains)) && (i < ARRAY_SIZE(ctx->s_ctx.reg_addr_dig_gain));
	     i++) {
		if (!rg_gains[i])
			continue; // skip zero gain setting

		if (ctx->s_ctx.reg_addr_dig_gain[i].addr[0]) {
			set_i2c_buffer(ctx,
				ctx->s_ctx.reg_addr_dig_gain[i].addr[0],
				(rg_gains[i] >> 10) & 0x0F);
		}
		if (ctx->s_ctx.reg_addr_dig_gain[i].addr[1]) {
			set_i2c_buffer(ctx,
				ctx->s_ctx.reg_addr_dig_gain[i].addr[1],
				(rg_gains[i] >> 2) & 0xFF);
		}
		if (ctx->s_ctx.reg_addr_dig_gain[i].addr[2]) {
			set_i2c_buffer(ctx,
				ctx->s_ctx.reg_addr_dig_gain[i].addr[2],
				rg_gains[i] & 0x03);
		}
	}

	if (!ctx->ae_ctrl_gph_en) {
		if (gph)
			ctx->s_ctx.s_gph((void *)ctx, 0);
		commit_i2c_buffer(ctx);
	}

	DRV_LOG(ctx, "dgain reg[lg/mg/sg]: 0x%x 0x%x 0x%x\n",
		rg_gains[0], rg_gains[1], rg_gains[2]);

	return 0;
}

static int dashsc532hswide_set_hdr_tri_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	int i = 0;
	int exp_cnt = 2;
	u32 values[3] = {0};
	u64 *gains = (u64 *)para;

	if (gains != NULL) {
		for (i = 0; i < 3; i++)
			values[i] = (u32) *(gains + i);
	}
	if (ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_LBMF) {
		set_multi_gain_in_lut(ctx, values, exp_cnt);
		return 0;
	}
	dashsc532hswide_set_multi_gain(ctx,values, exp_cnt);
	return 0;
}

static void dashsc532hswide_set_multi_gain(struct subdrv_ctx *ctx, u32 *gains, u16 exp_cnt)
{
	int i = 0;
	u16 rg_gains[3] = {0};
	u8 has_gains[3] = {0};
	bool gph = !ctx->is_seamless && (ctx->s_ctx.s_gph != NULL);

	if (exp_cnt > ARRAY_SIZE(ctx->ana_gain)) {
		DRV_LOGE(ctx, "invalid exp_cnt:%u>%lu\n", exp_cnt, ARRAY_SIZE(ctx->ana_gain));
		exp_cnt = ARRAY_SIZE(ctx->ana_gain);
	}
	for (i = 0; i < exp_cnt; i++) {
		/* check boundary of gain */
		gains[i] = max(gains[i],
			ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_ana_gain_range[i].min);
		gains[i] = min(gains[i],
			ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_ana_gain_range[i].max);
		/* mapping of gain to register value */
		if (ctx->s_ctx.g_gain2reg != NULL)
			gains[i] = ctx->s_ctx.g_gain2reg(gains[i]);
		else
			gains[i] = gain2reg(gains[i]);
	}
	/* restore gain */
	memset(ctx->ana_gain, 0, sizeof(ctx->ana_gain));
	for (i = 0; i < exp_cnt; i++)
		ctx->ana_gain[i] = gains[i];
	/* group hold start */
	if (gph && !ctx->ae_ctrl_gph_en)
		ctx->s_ctx.s_gph((void *)ctx, 1);
	/* write gain */
	memset(has_gains, 1, sizeof(has_gains));
	switch (exp_cnt) {
	case 2:
		rg_gains[0] = gains[0];
		has_gains[1] = 0;
		rg_gains[2] = gains[1];
		break;
	case 3:
		rg_gains[0] = gains[0];
		rg_gains[1] = gains[1];
		rg_gains[2] = gains[2];
		break;
	default:
		has_gains[0] = 0;
		has_gains[1] = 0;
		has_gains[2] = 0;
		break;
	}
	for (i = 0; i < 3; i++) {
		if (has_gains[i]) {
			set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_ana_gain[i].addr[0],
				(rg_gains[i] >> 8) & 0xFF);
			set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_ana_gain[i].addr[1],
				rg_gains[i] & 0xFF);
		}
	}
	DRV_LOG(ctx, "reg[lg/mg/sg]: 0x%x 0x%x 0x%x\n", rg_gains[0], rg_gains[1], rg_gains[2]);
	if (gph)
		ctx->s_ctx.s_gph((void *)ctx, 0);
	commit_i2c_buffer(ctx);
	/* group hold end */
}

static int dashsc532hswide_get_sensor_temperature(void *arg){
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	u16 temperature_enable = 0;
	u8 temperature = 0, temperature_last = 0;

	temperature_enable = subdrv_i2c_rd_u8(ctx, 0x4c00);
	temperature = subdrv_i2c_rd_u8(ctx, 0x0010);

	if(temperature_enable != 0x2a){
		DRV_LOG_MUST(ctx, "current temperature_enable is 0x%x, not support gettemp, set temp 0!\n", temperature_enable);
		return 0;
	}

	temperature = subdrv_i2c_rd_u8(ctx, 0x4c10);
	temperature_last = subdrv_i2c_rd_u8(ctx, 0x4c11);
	temperature_last = (temperature_last >> 2) & 0x1;
	temperature = (temperature << 1) | temperature_last;
	temperature -= 273;

	DRV_LOG(ctx, "temperature: %d degrees\n", temperature);

	return temperature;
}
