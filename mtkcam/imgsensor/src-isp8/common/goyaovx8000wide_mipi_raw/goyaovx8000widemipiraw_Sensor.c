// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 goyaovx8000widemipiraw_Sensor.c
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


#include "goyaovx8000widemipiraw_Sensor.h"
#define EEPROM_READY 1

#if EEPROM_READY
static void set_sensor_cali(void *arg);
#endif
static int get_sensor_temperature(void *arg);
static void set_group_hold(void *arg, u8 en);
static u16 get_gain2reg(u32 gain);
static int goyaovx8000wide_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int goyaovx8000wide_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt);
static int goyaovx8000wide_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int goyaovx8000wide_set_multi_dig_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int goyaovx8000wide_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static void goyaovx8000wide_set_multi_gain(struct subdrv_ctx *ctx, u32 *gains, u16 exp_cnt);
static int goyaovx8000wide_set_hdr_tri_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int goyaovx8000wide_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int goyaovx8000wide_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static void mi_stream(void *arg,bool enable);
static void mi_read_CGRatio(void *arg);
static int goyaovx8000wide_set_hdr_tri_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int goyaovx8000wide_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int get_csi_param(struct subdrv_ctx *ctx,	enum SENSOR_SCENARIO_ID_ENUM scenario_id,struct mtk_csi_param *csi_param);
struct subdrv_ctx *ctx_ovx8000 = NULL;

#define SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE                 1
#define SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA           2
#define SEAMLESS_SWITCH_GROUP_HD_CAP_MODE                     4
#define SEAMLESS_SWITCH_GROUP_VIDEO_MODE                      3
#define SEAMLESS_SWITCH_GROUP_VIDEO_NIGHT_MODE                5

/* STRUCT */
static struct mtk_sensor_saturation_info goyaovx8000_saturation_info_14bit = {
	.gain_ratio = 3400,
	.OB_pedestal = 64,
	.saturation_level = 16383,
	.adc_bit = 10,
	.ob_bm = 64,
};

static struct mtk_sensor_saturation_info goyaovx8000_saturation_info_fake14bit = {
	.gain_ratio = 1000,
	.OB_pedestal = 1024,
	.saturation_level = 16368,
	.adc_bit = 10,
};

//static u32 goyaovx8000_dcg_ratio_table_14bit[] = {16000};

static struct mtk_sensor_saturation_info goyaovx8000_saturation_info_10bit = {
	.gain_ratio = 1000,
	.OB_pedestal = 64,
	.saturation_level = 1023,
};

struct SET_SENSOR_AWB_GAIN g_last_awb_gain = {0, 0, 0, 0};

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, goyaovx8000wide_set_test_pattern},
	{SENSOR_FEATURE_SEAMLESS_SWITCH, goyaovx8000wide_seamless_switch},
	{SENSOR_FEATURE_SET_AWB_GAIN, goyaovx8000wide_set_awb_gain},
	{SENSOR_FEATURE_SET_MULTI_DIG_GAIN, goyaovx8000wide_set_multi_dig_gain},
	{SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME,goyaovx8000wide_set_multi_shutter_frame_length},
	{SENSOR_FEATURE_SET_ESHUTTER, goyaovx8000wide_set_shutter},
	{SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME, goyaovx8000wide_set_shutter_frame_length},
	{SENSOR_FEATURE_SET_DUAL_GAIN,goyaovx8000wide_set_hdr_tri_gain},
	{SENSOR_FEATURE_SET_HDR_SHUTTER,goyaovx8000wide_set_hdr_tri_shutter},
	{SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO, goyaovx8000wide_set_max_framerate_by_scenario},
};

static struct eeprom_info_struct eeprom_info[] = {
	{
		//.header_id = 0x01, // vendor id : 0x01 == sunny
		//.addr_header_id = 0x01, // vendor id addr
		.i2c_write_id = 0xA2,

		// PDC Calibration 1th
		.pdc_support = TRUE,
		.pdc_size = 32,
		.addr_pdc = 0x2956,
		.sensor_reg_addr_pdc = 0x5A20,

		// LRC Calibration 2th
		.lrc_support = TRUE,
		.lrc_size = 3536,
		.addr_lrc = 0x2976,
		.sensor_reg_addr_lrc = 0x5AC0,

		// LRC Calibration 3th
		.xtalk_support = TRUE,
		.xtalk_size = 16,
		.addr_xtalk = 0x3746,
		.sensor_reg_addr_xtalk = 0x68AE,
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
	.i4ModeIndex = 2, /*HVBin 2; VBin 3*/
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
		{128, 456}, {0, 192}, {0, 0}, {0, 0}, {2048, 1536},
		// <cust6> <cust7> <cust8> <cust9>< cust10>
		{0, 0}, {2048, 1536}, {0, 0}, {2048, 1536}, {0, 0},
		// cust11 cust12 cust13 <cust14> <cust15>
		{608, 456}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		// <cust16> <cust17> cust18 <cust19> <cust20>
		{0, 0}, {0, 256}, {0, 256}, {2048, 1792}, {256, 912},
		// <cust21> <cust22> cust23 <cust24> <cust25>
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 256},
		// <cust26> <cust27> cust28 <cust29> <cust30>
		{0, 256}, {2048, 1792}, {0, 0}, {0, 0}, {0, 0},
		// <cust31> <cust32> <cust33> <cust34> <cust35>
		{0, 0}, {0, 0}, {2048, 1536}, {0, 0}, {2048, 1536},
		// <cust36> <cust37> <cust38>
		{0, 0}, {608, 456}, {0, 0},
	},
};


static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_smvr = {
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
	.i4FullRawW = 2048,
	.i4FullRawH = 1536,
	.i4ModeIndex = 2, /*HVBin 2; VBin 3*/
	.sPDMapInfo[0] = {
		.i4PDPattern = 1,//1: dense pd
		.i4BinFacX = 2,
		.i4BinFacY = 2,
		.i4PDRepetition = 2,
		.i4PDOrder = {1},
	},
	.i4Crop = {
		// <pre> <cap> <normal_video> <hs_video> <slim_video>
		{0, 0}, {0, 0}, {0, 384}, {0, 0}, {0, 0},
		// <cust1> <cust2> <cust3> <cust4> <cust5>
		{128, 456}, {0, 192}, {0, 0}, {0, 0}, {2048, 1536},
		// <cust6> <cust7> <cust8> <cust9>< cust10>
		{0, 0}, {2048, 1536}, {0, 0}, {2048, 1536}, {0, 0},
		// cust11 cust12 cust13 <cust14> <cust15>
		{608, 456}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		// <cust16> <cust17> cust18 <cust19> <cust20>
		{0, 0}, {0, 256}, {0, 256}, {2048, 1792}, {256, 912},
		// <cust21> <cust22> cust23 <cust24> <cust25>
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 256},
		// <cust26> <cust27> cust28 <cust29> <cust30>
		{0, 256}, {2048, 1792}, {0, 0}, {0, 0}, {0, 0},
		// <cust31> <cust32> <cust33> <cust34> <cust35>
		{0, 0}, {0, 0}, {2048, 1536}, {0, 0}, {2048, 1536},
		// <cust36> <cust37> <cust38>
		{0, 0}, {608, 456}, {0, 0},
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
	.i4ModeIndex = 2, /*HVBin 2; VBin 3*/
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
		{0, 384}, {0, 192}, {0, 0}, {0, 0}, {2048, 1536},
		// <cust6> <cust7> <cust8> <cust9>< cust10>
		{0, 0}, {2048, 1536}, {0, 0}, {2048, 1536}, {0, 0},
		// cust11 cust12 cust13 <cust14> <cust15>
		{608, 456}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		// <cust16> <cust17> cust18 <cust19> <cust20>
		{0, 0}, {0, 256}, {0, 256}, {2048, 1792}, {256, 912},
		// <cust21> <cust22> cust23 <cust24> <cust25>
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 256},
		// <cust26> <cust27> cust28 <cust29> <cust30>
		{0, 256}, {2048, 1792}, {0, 0}, {0, 0}, {0, 0},
		// <cust31> <cust32> <cust33> <cust34> <cust35>
		{0, 0}, {0, 0}, {2048, 1536}, {0, 0}, {2048, 1536},
		// <cust36> <cust37> <cust38>
		{0, 0}, {608, 456}, {0, 0},
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 768,
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 576,
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
			.hsize = 1088,
			.vsize = 612,
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
			.hsize = 3840,
			.vsize = 2160,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 3840,
			.vsize = 540,
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 2048,
			.vsize = 576,
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
			.hsize = 1920,
			.vsize = 1080,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 8192,
			.vsize = 6144,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus5[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 2048,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
	{
		.bus.csi2 = {
			.channel = 2,
			.data_type = 0x2b,
			.hsize = 2048,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_2,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus6[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 8192,
			.vsize = 6144,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus7[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 2048,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
	{
		.bus.csi2 = {
			.channel = 2,
			.data_type = 0x2b,
			.hsize = 2048,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_2,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus8[] = {
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 2048,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
	{
		.bus.csi2 = {
			.channel = 2,
			.data_type = 0x2b,
			.hsize = 2048,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_2,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus11[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 2880,
			.vsize = 2160,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 2880,
			.vsize = 540,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus13[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus14[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 768,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
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
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2d,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_ME,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 640,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus18[] = {
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 640,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus19[] = {
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 2048,
			.vsize = 640,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
	{
		.bus.csi2 = {
			.channel = 2,
			.data_type = 0x2b,
			.hsize = 2048,
			.vsize = 640,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_2,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus20[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 7680,
			.vsize = 4320,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 3840,
			.vsize = 1080,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
	{
		.bus.csi2 = {
			.channel = 2,
			.data_type = 0x2b,
			.hsize = 3840,
			.vsize = 1080,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_2,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_LAST,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus21[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 2304,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

#define PRE_MODE  .frame_desc = frame_desc_prev,\
		.num_entries = ARRAY_SIZE(frame_desc_prev),\
		.mode_setting_table = goyaovx8000wide_preview_setting,\
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_preview_setting),\
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_HD_CAP_MODE,\
		.seamless_switch_mode_setting_table = goyaovx8000wide_preview_setting,\
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_preview_setting),\
		.hdr_mode = HDR_NONE,\
		.raw_cnt = 1,\
		.exp_cnt = 1,\
		.pclk = 75190000,\
		.linelength = 400,\
		.framelength = 6256,\
		.max_framerate = 300,\
		.mipi_pixel_rate = 2394000000,\
		.readout_length = 0,\
		.read_margin = 24,\
		.framelength_step = 2,\
		.coarse_integ_step = 2,\
		.min_exposure_line = 4,\
		.mi_mode_type = 2, \
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,\
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,\
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
	// mode_1 4096x3072_binning_4C2PlusSCG_10bit_60fps_PD_DT_4096x768_Max_A_gain_63.75x
	// PD size = 4096x768
	// Tline = 5.333us
	// VB = 16.949ms
	// version:2025-0416_v1.1
	{
		PRE_MODE
	},
	// capture mode 1: same as preview mode
	{
		PRE_MODE
	},
	// normal_video mode 2
	// Mode_26 4096x2304_binning_4C2PlusSCG_crop_10bit_30fps_PDDT_4096x576_Max_A_gain_63.75x
	// PD size = 4096x576
	// Tline = 5.333us
	// VB = 21.045 ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = goyaovx8000wide_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_normal_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_normal_video_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_normal_video_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6248,
		.max_framerate = 300,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 2,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
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
	// hs_video mode 3 : smvr 480fps
	// Mode_24 1088x612_binning_4C1SCG_crop_10bit_480fps_no_PD_Max_A_gain_63.75x
	// no PD
	// Tline = 3.16us
	// VB = 0.145ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = goyaovx8000wide_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_hs_video_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_hs_video_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 237,
		.framelength = 658,
		.max_framerate = 4800,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 1920,
			.y0_offset = 1848,
			.w0_size = 4352,
			.h0_size = 2448,
			.scale_w = 1088,
			.scale_h = 612,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 1088,
			.h1_size = 612,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1088,
			.h2_tg_size = 612,
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
	// Mode_29 3840x2160_binning_4C2PlusSCG_crop_10bit_30fps_PDDT_4096x576_Max_A_gain_63.75x
	// PD size = 3840x540
	// Tline = 5.333us
	// VB = 4.395 ms
	// version:2025-0620_v1.6
	{
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = goyaovx8000wide_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom1_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom1_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom1_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 452,
		.framelength = 2760,
		.max_framerate = 600,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 2,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// custom2 mode 6 : smvr 120fps
	// Mode_22 2048x1152_binning_4C2PlusSCG_crop_10bit_120fps_PDDT_2048x576_Max_A_gain_63.75x
	// PD Size = 2048*576
	// Tline = 5.333us
	// VB = 2.187ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = goyaovx8000wide_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom2_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom2_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom2_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 1562,
		.max_framerate = 1200,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_smvr,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// custom3 mode 7 : 240fps
	// Mode_23 1920x1080_binning_4C1SCG_crop_10bit_240fps_no_PD_Max_A_gain_63.75x
	// no PD
	// Tline = 3.16us
	// VB = 0.752ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = goyaovx8000wide_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom3_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom3_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom3_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 237,
		.framelength = 1318,
		.max_framerate = 2400,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
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
	// custom4 mode 8 : fullsize fake 14 quad
	// Mode_6 8192x6144_full_quad_bayer_fake14bit_30fps_no_PD_Max_A_gain_15.9375x
	// Tline = 5.333us
	// VB = 0.576ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = goyaovx8000wide_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom4_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom4_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom4_setting),
		.hdr_mode = PARAM_UNDEFINED,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75150000,
		.linelength = 400,
		.framelength = 6252,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_4CELL_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
		.awb_enabled = true,
	},
	// custom5 mode 9 : ISZ
	// Mode_7 4096x3072_full_crop_quad_bayer_fake14bit_30fps_PD_DT_2048x768_Max_A_gain_15.9375x
	// PD Size = 2048x768
	// Tline = 5.333 us
	// VB = 16.96ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = goyaovx8000wide_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom5_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom5_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom5_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6248,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_4CELL_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
		.awb_enabled = true,
	},
	// custom6 mode 10 : full size bayer fake 14
	// Mode_4 8192x6144_full_bayer_fake14bit_30fps_no_PD_Max_A_gain_15.9375x
	// no PD
	// Tline = 5.333 us
	// VB = 0.576ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus6,
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.mode_setting_table = goyaovx8000wide_custom6_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom6_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom6_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom6_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6252,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom7 mode 11: ISZ bayer fake 14
	// Mode_5	4096x3072_full_crop_bayer_fake14bit_30fps_PD_DT_2048x768_Max_A_gain_15.9375x
	// PD Size = 2048x768
	// Tline = 5.333us
	// VB = 16.96ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus7,
		.num_entries = ARRAY_SIZE(frame_desc_cus7),
		.mode_setting_table = goyaovx8000wide_custom7_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom7_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom7_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom7_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6248,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
		.awb_enabled = true,
	},
	//custom8 mode 12 : full size
	// Mode_2 8192x6144_full_bayer_10bit_30fps_no_PD_Max_A_gain_15.9375x
	// no PD
	// Tline = 5.333us
	// VB = 0.576ms
	// version:2025-0620_v1.6
	{
		.frame_desc = frame_desc_cus8,
		.num_entries = ARRAY_SIZE(frame_desc_cus8),
		.mode_setting_table = goyaovx8000wide_custom8_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom8_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_HD_CAP_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom8_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom8_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75150000,
		.linelength = 400,
		.framelength = 7822,
		.max_framerate = 240,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
	// custom9 mode 13: ISZ bayer raw10
	// Mode_3 4096x3072_full_crop_bayer_10bit_30fps_PD_DT_2048x768_Max_A_gain_15.9375x
	// PD Size = 2048x768
	// Tline = 5.333 us
	// VB = 16.96 ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus9,
		.num_entries = ARRAY_SIZE(frame_desc_cus9),
		.mode_setting_table = goyaovx8000wide_custom9_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom9_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom9_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom9_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6248,
		.max_framerate = 300,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
	// custom10 mode 14: 1X bokeh binning 24fps same as preview
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = goyaovx8000wide_preview_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_preview_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_preview_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 7820,
		.max_framerate = 240,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
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
	// custom11 mode 15: 2X bokeh
	// Mode_28 2880x2160_scg_binning_cropping_10bit_30fps_AG63.75x_PDVC_2880x540
	// PD Size = 2880x540
	// Tline = 5.333 us
	// VB = 21.803ms
	// version:2025-0319_v2.3
	{
		.frame_desc = frame_desc_cus11,
		.num_entries = ARRAY_SIZE(frame_desc_cus11),
		.mode_setting_table = goyaovx8000wide_custom11_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom11_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom11_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom11_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 7820,
		.max_framerate = 240,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 1216,
			.y0_offset = 912,
			.w0_size = 5760,
			.h0_size = 4320,
			.scale_w = 2880,
			.scale_h = 2160,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2880,
			.h1_size = 2160,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2880,
			.h2_tg_size = 2160,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1240,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// custom12 mode 16: bokeh slave
	// Mode_2 
	// Tline = 10.88us
	// VB = 0.0.661ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = goyaovx8000wide_preview_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_preview_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_preview_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 7820,
		.max_framerate = 240,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
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
	// custom13 mode 17: 4:3 binning fake 14
	// Mode_8 4096x3072_binning_4C2PlusSCG_fake14bit_60fps_PD_DT_4096x768_Max_A_gain_63.75x
	// Tline = 5.333us
	// VB = 16.384ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus13,
		.num_entries = ARRAY_SIZE(frame_desc_cus13),
		.mode_setting_table = goyaovx8000wide_custom13_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom13_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom13_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom13_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75190000,
		.linelength = 400,
		.framelength = 3185,
		.max_framerate = 590,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom14 mode 18: DXG 14bit
	// Mode_9 4096x3072_binning_4C2Plus_DCG_Combine_MSB_14bit_30fps_PD_DT_4096x768_Max_A_gain_63.75x
	// PD Size = 4096x768
	// Tline = 10.333us
	// VB = 1.633ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus14,
		.num_entries = ARRAY_SIZE(frame_desc_cus14),
		.mode_setting_table = goyaovx8000wide_custom14_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom14_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom14_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom14_setting),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 75000000,
		.linelength = 775,
		.framelength = 3224,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 3, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 63.75f,
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
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_DIRECT_MODE,
			.dcg_gain_ratio_min = 1000,
			.dcg_gain_ratio_max = 16000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = PARAM_UNDEFINED,
			.dcg_gain_table_size = PARAM_UNDEFINED,
			.dcg_ratio_group = {4362, 1024}, // HCG = 5.7*1024, LCG = 1024
		},
		.bit_align_type = IMGSENSOR_PIXEL_MSB_ALIGN,
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_14bit,
	},
	// custom15 mode 19: stagger exp2 fake 14
	// Mode_10 4096x3072_binning_4C2PlusSCG_STG2_HDR_14bit_30fps_PD_DT_4096x768_Max_A_gain_63.75x
	// PD Size = 4096x768
	// Tline = 25.493us
	// VB = 13.715ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus15,
		.num_entries = ARRAY_SIZE(frame_desc_cus15),
		.mode_setting_table = goyaovx8000wide_custom15_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom15_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom15_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom15_setting),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 75190000,
		.linelength = 800,
		.framelength = 3128,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 768 * 2,
		.read_margin = 128,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 63.75f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom16 mode 20: unused
	{
		PRE_MODE
	},
	// custom17 mode 21: video fake 14bit 16:10 
	// Mode_12 4096x2560_binning_4C2PlusSCG_crop_fake14bit_30fps_PD_DT_4096x640_Max_A_gain_63.75x
	// PD Size = 4096x640
	// Tline = 5.333us
	// VB = 19.669ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus17,
		.num_entries = ARRAY_SIZE(frame_desc_cus17),
		.mode_setting_table = goyaovx8000wide_custom17_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom17_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom17_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom17_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6228,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75,
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
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom18 version:2025-0416_v1.1 22: 14bit  video DCG
	// Mode_13	4096x2560_binning_4C2Plus_crop_DCG_combine_MSB_14bit_30fps_PD_DT_4096x640_Max_A_gain_63.75x
	// PD Size = 4096x640
	// Tline = 5.333us
	// VB = 6.923ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus18,
		.num_entries = ARRAY_SIZE(frame_desc_cus18),
		.mode_setting_table = goyaovx8000wide_custom18_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom18_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom18_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom18_setting),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 75000000,
		.linelength = 775,
		.framelength = 3214,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 3, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN,
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
			.dcg_gain_mode = IMGSENSOR_DCG_DIRECT_MODE,
			.dcg_gain_ratio_min = 1000,
			.dcg_gain_ratio_max = 16000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = PARAM_UNDEFINED,
			.dcg_gain_table_size = PARAM_UNDEFINED,
			.dcg_ratio_group = {4362, 1024}, // HCG = 5.7*1024, LCG = 1024
		},
		.bit_align_type = IMGSENSOR_PIXEL_MSB_ALIGN,
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_14bit,
	},
	// custom19 mode 23 : full size crop bayer 16:10 fake14 bit
	// Mode_14 4096x2560_full_crop_bayer_fake14bit_30fps_PD_DT_2048x640_Max_A_gain_15.9375x
	// PD Size = 2048x640
	// Tline = 5.333us
	// VB = 19.691ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus19,
		.num_entries = ARRAY_SIZE(frame_desc_cus19),
		.mode_setting_table = goyaovx8000wide_custom19_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom19_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom19_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom19_setting),
		.hdr_mode = PARAM_UNDEFINED,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6228,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN ,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
		.awb_enabled = true,
	},
	// custom20 mode 24: 8k video
	// Mode_20 7680x4320_full_crop_bayer_10bit_30fps_PD_DT_3840x1080_Max_A_gain_15.9375x
	// PD Size = 3840x1080
	// Tline = 5.333us
	// VB = 10.304ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus20,
		.num_entries = ARRAY_SIZE(frame_desc_cus20),
		.mode_setting_table = goyaovx8000wide_custom20_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom20_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom20_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom20_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6248,
		.max_framerate = 300,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.937f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 256,
			.y0_offset = 912,
			.w0_size = 7680,
			.h0_size = 4320,
			.scale_w = 7680,
			.scale_h = 4320,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 7680,
			.h1_size = 4320,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 7680,
			.h2_tg_size = 4320,
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
	// custom21 mode 25: 4k 120fps video
	// Mode_21 4096x2304_binning_4C2PlusSCG_crop_10bit_120fps_no_PD_Max_A_gain_63.75x
	// no PD 
	// Tline = 3.16us
	// VB = 1.049ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus21,
		.num_entries = ARRAY_SIZE(frame_desc_cus21),
		.mode_setting_table = goyaovx8000wide_custom21_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom21_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom21_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom21_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 237,
		.framelength = 2636,
		.max_framerate = 1200,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// custom22 mode 26 : same as preview
	{
		PRE_MODE
	},
	// custom23 mode 27 : same as preview
	{
		PRE_MODE
	},
	// custom24 mode 28 : same as preview
	{
		PRE_MODE
	},
	// custom25 mode 29: video fake 14bit 16:10 24fps same as csutom17
	// Mode_12 4096x2560_binning_4C2PlusSCG_crop_fake14bit_30fps_PD_DT_4096x640_Max_A_gain_63.75x
	// PD Size = 4096x640
	// Tline = 5.333us
	// VB = 19.669ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus17,
		.num_entries = ARRAY_SIZE(frame_desc_cus17),
		.mode_setting_table = goyaovx8000wide_custom17_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom17_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_NIGHT_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom17_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom17_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6228,
		.max_framerate = 240,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75,
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
		.cms_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom26 mode 30: fake 14bit  video DCG 24 pfs same custom18
	// Mode_13	4096x2560_binning_4C2Plus_crop_DCG_combine_MSB_14bit_30fps_PD_DT_4096x640_Max_A_gain_63.75x
	// PD Size = 4096x640
	// Tline = 5.333us
	// VB = 6.923ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus18,
		.num_entries = ARRAY_SIZE(frame_desc_cus18),
		.mode_setting_table = goyaovx8000wide_custom18_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom18_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_NIGHT_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom18_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom18_setting),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 75000000,
		.linelength = 775,
		.framelength = 3214,
		.max_framerate = 240,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 3, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN,
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
			.dcg_gain_mode = IMGSENSOR_DCG_DIRECT_MODE,
			.dcg_gain_ratio_min = 1000,
			.dcg_gain_ratio_max = 16000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = PARAM_UNDEFINED,
			.dcg_gain_table_size = PARAM_UNDEFINED,
			.dcg_ratio_group = {4362, 1024}, // HCG = 5.7*1024, LCG = 1024
		},
		.bit_align_type = IMGSENSOR_PIXEL_MSB_ALIGN,
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_14bit,
	},
	// custom27 mode 31 : full size crop bayer 16:10 fake14 bit 24fps same as custom19
	// Mode_15 4096x2560_full_crop_bayer_fake14bit_30fps_PD_DT_2048x640_Max_A_gain_15.9375x
	// PD Size = 2048x640
	// Tline = 5.333us
	// VB = 19.691ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus19,
		.num_entries = ARRAY_SIZE(frame_desc_cus19),
		.mode_setting_table = goyaovx8000wide_custom19_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom19_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_NIGHT_MODE,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom19_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom19_setting),
		.hdr_mode = PARAM_UNDEFINED,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6228,
		.max_framerate = 240,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN ,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom28 mode 32: same as preview mode
	{
		PRE_MODE
	},
	// custom29 mode 33: 4:3 binning fake 14 LEICA same as custom13
	// Mode_8 4096x3072_binning_4C2PlusSCG_fake14bit_60fps_PD_DT_4096x768_Max_A_gain_63.75x
	// Tline = 5.333us
	// VB = 16.384ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus13,
		.num_entries = ARRAY_SIZE(frame_desc_cus13),
		.mode_setting_table = goyaovx8000wide_custom13_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom13_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom13_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom13_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75190000,
		.linelength = 400,
		.framelength = 3185,
		.max_framerate = 590,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom30 mode 34: DXG 14bit LEICA same as custom14
	// Mode_9 4096x3072_binning_4C2Plus_DCG_Combine_MSB_14bit_30fps_PD_DT_4096x768_Max_A_gain_63.75x
	// PD Size = 4096x768
	// Tline = 10.333us
	// VB = 1.633ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus14,
		.num_entries = ARRAY_SIZE(frame_desc_cus14),
		.mode_setting_table = goyaovx8000wide_custom14_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom14_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom14_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom14_setting),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 75000000,
		.linelength = 775,
		.framelength = 3224,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 3, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 63.75f,
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
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_DIRECT_MODE,
			.dcg_gain_ratio_min = 1000,
			.dcg_gain_ratio_max = 16000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = PARAM_UNDEFINED,
			.dcg_gain_table_size = PARAM_UNDEFINED,
			.dcg_ratio_group = {4362, 1024}, // HCG = 5.7*1024, LCG = 1024
		},
		.bit_align_type = IMGSENSOR_PIXEL_MSB_ALIGN,
		.dpc_enabled = true,
		.pdc_enabled = true,
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_14bit,
	},
	// custom31 mode 35: stagger exp2 fake 14 LEICA same as custom15
	// Mode_10 4096x3072_binning_4C2PlusSCG_STG2_HDR_14bit_30fps_PD_DT_4096x768_Max_A_gain_63.75x
	// PD Size = 4096x768
	// Tline = 25.493us
	// VB = 13.715ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus15,
		.num_entries = ARRAY_SIZE(frame_desc_cus15),
		.mode_setting_table = goyaovx8000wide_custom15_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom15_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom15_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom15_setting),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 75000000,
		.linelength = 800,
		.framelength = 3128,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 768 * 2,
		.read_margin = 128,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 63.75f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom32 mode 36 : fullsize fake 14 quad LEICA same as custom4
	// Mode_6 8192x6144_full_quad_bayer_fake14bit_30fps_no_PD_Max_A_gain_15.9375x
	// Tline = 5.333us
	// VB = 0.576ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = goyaovx8000wide_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom4_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom4_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom4_setting),
		.hdr_mode = PARAM_UNDEFINED,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6252,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_4CELL_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom33 mode 37 : ISZ LEICA same as custom5
	// Mode_7 4096x3072_full_crop_quad_bayer_fake14bit_30fps_PD_DT_2048x768_Max_A_gain_15.9375x
	// PD Size = 2048x768
	// Tline = 5.333 us
	// VB = 16.96ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = goyaovx8000wide_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom5_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom5_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom5_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6248,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_4CELL_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom34 mode 38 : full size bayer fake 14 LEICA same as custom6
	// Mode_4 8192x6144_full_bayer_fake14bit_30fps_no_PD_Max_A_gain_15.9375x
	// no PD
	// Tline = 5.333 us
	// VB = 0.576ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus6,
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.mode_setting_table = goyaovx8000wide_custom6_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom6_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom6_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom6_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75150000,
		.linelength = 400,
		.framelength = 6252,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom35 mode 39: ISZ bayer fake 14 LEICA same as custom7
	// Mode_5	4096x3072_full_crop_bayer_fake14bit_30fps_PD_DT_2048x768_Max_A_gain_15.9375x
	// PD Size = 2048x768
	// Tline = 5.333us
	// VB = 16.96ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus7,
		.num_entries = ARRAY_SIZE(frame_desc_cus7),
		.mode_setting_table = goyaovx8000wide_custom7_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom7_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom18_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom18_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 6248,
		.max_framerate = 300,
		.mipi_pixel_rate = 1710000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.min_exposure_line = 8,
		.mi_mode_type = 1, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 15.9375f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ana_gain_max = BASEGAIN * 15.9375f,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_B,
		.saturation_info = &goyaovx8000_saturation_info_fake14bit,
	},
	// custom36 mode 40: 1X bokeh binning 24fps LEICA same as preview
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = goyaovx8000wide_preview_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_preview_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_preview_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 7820,
		.max_framerate = 240,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
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
	// custom37 mode 41: 2X bokeh LEICA
	// Mode_3 4096x3072_full_crop_bayer_10bit_30fps_PD_DT_2048x768_Max_A_gain_15.9375x
	// PD Size = 2048x768
	// Tline = 5.333 us
	// VB = 16.96 ms
	// version:2025-0416_v1.1
	{
		.frame_desc = frame_desc_cus11,
		.num_entries = ARRAY_SIZE(frame_desc_cus11),
		.mode_setting_table = goyaovx8000wide_custom11_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom11_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_custom11_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_custom11_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 7820,
		.max_framerate = 240,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 1216,
			.y0_offset = 912,
			.w0_size = 5760,
			.h0_size = 4320,
			.scale_w = 2880,
			.scale_h = 2160,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2880,
			.h1_size = 2160,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2880,
			.h2_tg_size = 2160,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1240,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// custom38 mode 42: bokeh fall back binning 24fps LEICA same as preview(as mode 16)
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = goyaovx8000wide_preview_setting,
		.mode_setting_len = ARRAY_SIZE(goyaovx8000wide_preview_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = goyaovx8000wide_preview_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(goyaovx8000wide_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 75000000,
		.linelength = 400,
		.framelength = 7820,
		.max_framerate = 240,
		.mipi_pixel_rate = 2394000000,
		.readout_length = 0,
		.read_margin = 24,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.min_exposure_line = 4,
		.mi_mode_type = 2, //defu :0 ; full size: 1; bining 2; DXG: 3; CMS: 4;
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 63.75f,
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
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = GOYAOVX8000WIDE_SENSOR_ID,
	.reg_addr_sensor_id = {0x300A, 0x300B},
	.i2c_addr_table = {0x20, 0xFF}, // TBD
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
	.dig_gain_max = BASEGAIN * 32,
	.dig_gain_step = 4,  //If the value is 0, SENSOR_FEATURE_SET_MULTI_DIG_GAIN is disabled
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 63.75,
	.ana_gain_type = 0,
	.ana_gain_step = 1,
	.ana_gain_table = goyaovx8000wide_ana_gain_table,
	.ana_gain_table_size = sizeof(goyaovx8000wide_ana_gain_table),
	.tuning_iso_base = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 8,
	.exposure_max = 0xFFFFFF - 32,
	.exposure_step = 4,
	.exposure_margin = 32,
	.saturation_info = &goyaovx8000_saturation_info_10bit,

	.frame_length_max = 0xFFFFFF,
	.ae_effective_frame = 3,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 1704166,

	.pdaf_type = PDAF_SUPPORT_CAMSV_QPD,
	.hdr_type = HDR_SUPPORT_STAGGER_DOL|HDR_SUPPORT_DCG,
	.seamless_switch_support = TRUE,
	.temperature_support = TRUE,

	.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,
	.s_mi_stream = mi_stream,
	.s_mi_read_CGRatio = mi_read_CGRatio,
	.mi_hb_vb_cal = false,
	.mi_dxg_reg = 0x5060,
#if EEPROM_READY
	.s_cali = set_sensor_cali,
#else
	.s_cali = NULL,
#endif	

	 .reg_addr_stream = 0x0100,
	// .reg_addr_mirror_flip = 0x3820,
	.reg_addr_exposure = {
		{0x3500, 0x3501, 0x3502},
		{0x3580, 0x3581, 0x3582},
		{0x3540, 0x3541, 0x3542}
	},
	.long_exposure_support = TRUE,
	// .reg_addr_exposure_lshift = 0x3160,
	.reg_addr_ana_gain = {
		{0x3508, 0x3509},
		{0x3588, 0x3589},
		{0x3548, 0x3549}
	},
	.reg_addr_dig_gain = {
		{0x350A, 0x350B, 0x350C},
		{0x358A, 0x358B, 0x358C},
		{0x354A, 0x354B, 0x354C}
	},
	.reg_addr_dcg_ratio = PARAM_UNDEFINED, //TBD
	.reg_addr_frame_length = {0x3840, 0x380E, 0x380F},
	.reg_addr_temp_en = 0x4D12,
	.reg_addr_temp_read = 0x4D13,
	// .reg_addr_auto_extend = 0x0350,
	.reg_addr_frame_count = 0x387F,
	// .reg_addr_fast_mode = 0x3010,

	// .mi_disable_set_dummy = 1, // disable set dummy
	// .mi_evaluate_frame_rate_by_scenario = evaluate_frame_rate_by_scenario,
	.init_setting_table = goyaovx8000wide_init_setting,
	.init_setting_len = ARRAY_SIZE(goyaovx8000wide_init_setting),
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
	.get_csi_param = get_csi_param,
	.update_sof_cnt = common_update_sof_cnt,
	.vsync_notify = vsync_notify,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_DVDD1,  {1800000, 1800000},  5000}, //ldo supply
	{HW_ID_AFVDD, {3100000, 3100000},  1000},
	{HW_ID_DVDD,  {1100000, 1100000},  2000},
	{HW_ID_MCLK,   {24},        0},
	{HW_ID_RST,    {0},         0},
	{HW_ID_AVDD,  {2800000, 2800000},  0},
	{HW_ID_DOVDD, {1800000, 1800000},  1000},
	{HW_ID_MCLK_DRIVING_CURRENT, {4},  5000},
	{HW_ID_RST,    {1},       5000},
};


const struct subdrv_entry goyaovx8000wide_mipi_raw_entry = {
    .name = "goyaovx8000wide_mipi_raw",
    .id = GOYAOVX8000WIDE_SENSOR_ID,
    .pw_seq = pw_seq,
    .pw_seq_cnt = ARRAY_SIZE(pw_seq),
    .ops = &ops,
};


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

	/* PDC data 1th */
	support = info[idx].pdc_support;
	pbuf = info[idx].preload_pdc_table;
	size = info[idx].pdc_size;
	addr = info[idx].sensor_reg_addr_pdc;
	if (support) {
		if (pbuf != NULL && addr > 0 && size > 0) {
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			DRV_LOG(ctx, "set 1th PDC calibration data done.");
		} else {
			DRV_LOG(ctx, "pbuf(%p) addr(%d) size(%d) set 1th PDC calibration data fail.",
				pbuf, addr, size);
		}
	}

	/* PDC data 2th */
	support = info[idx].lrc_support;
	pbuf = info[idx].preload_lrc_table;
	size = info[idx].lrc_size;
	addr = info[idx].sensor_reg_addr_lrc;
	if (support) {
		if (pbuf != NULL && addr > 0 && size > 0) {
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			DRV_LOG(ctx, "set 2th PDC calibration data done.");
		} else {
			DRV_LOG(ctx, "pbuf(%p) addr(%d) size(%d) set 2th PDC calibration data fail.",
				pbuf, addr, size);
		}
	}

	/* PDC data 3th */
	support = info[idx].xtalk_support;
	pbuf = info[idx].preload_xtalk_table;
	size = info[idx].xtalk_size;
	addr = info[idx].sensor_reg_addr_xtalk;
	if (support) {
		if (pbuf != NULL && addr > 0 && size > 0) {
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			DRV_LOG(ctx, "set 3th PDC calibration data done.");
		} else {
			DRV_LOG(ctx, "pbuf(%p) addr(%d) size(%d) set 3th PDC calibration data fail.",
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
		goyaovx8000wide_set_awb_gain(ctx, (u8 *)&g_last_awb_gain, &len);
		i2c_table_write(ctx,goyaovx8000wide_stream_on,ARRAY_SIZE(goyaovx8000wide_stream_on));
	}else{
		mi_stream_check_stream_off(ctx);
		i2c_table_write(ctx,goyaovx8000wide_stream_off,ARRAY_SIZE(goyaovx8000wide_stream_off));
	}
}

static void mi_read_CGRatio(void *arg){
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	int i = 0;
	DRV_LOG_MUST(ctx, "Enter %s\n", __FUNCTION__);
	if(ctx->s_ctx.mi_dxg_reg){
		subdrv_i2c_wr_u8(ctx,0x3d8c,0x9b);
		subdrv_i2c_wr_u8(ctx,0x3d8d,0xa0);
		subdrv_i2c_wr_u8(ctx,0x3016,0xf1);
		subdrv_i2c_wr_u8(ctx,0x0100,0x01);
		mdelay(4);
		ctx->s_ctx.mi_dxg_ratio = subdrv_i2c_rd_u16(ctx,ctx->s_ctx.mi_dxg_reg);
		subdrv_i2c_wr_u8(ctx,0x0100,0x00);
		DRV_LOG(ctx, "mi_dxg_ratio(%d) \n",ctx->s_ctx.mi_dxg_ratio);
	}
	if(ctx->s_ctx.mi_dxg_ratio < 3174 || ctx->s_ctx.mi_dxg_ratio > 3788){
		ctx->s_ctx.mi_dxg_ratio = 3482;
		DRV_LOGE(ctx, "E: mi_dxg_ratio(%d) ratio need (3174,3788) \n",ctx->s_ctx.mi_dxg_ratio);
	}
	for (i = 0; i < ctx->s_ctx.sensor_mode_num ; i++) {
		if(ctx->s_ctx.mode[i].hdr_mode == HDR_RAW_DCG_COMPOSE){
			ctx->s_ctx.mode[i].dcg_info.dcg_ratio_group[0] = ctx->s_ctx.mi_dxg_ratio;
			DRV_LOG(ctx, "mode(%d) dcg_ratio_group[0](%d) \n",i,ctx->s_ctx.mode[i].dcg_info.dcg_ratio_group[0]);
		}

		if(ctx->s_ctx.mode[i].mi_mode_type == 1){//full size / full size crop mode
			if(ctx->s_ctx.mi_dxg_ratio > 4491)//CG ratio * 1000 / 1024 > 4386
				ctx->s_ctx.mode[i].ae_binning_ratio = (4386 * 1024) / ctx->s_ctx.mi_dxg_ratio;
			else
				ctx->s_ctx.mode[i].ae_binning_ratio = 1000;
		} else {
			if(ctx->s_ctx.mi_dxg_ratio > 4491)//CG ratio * 1000 / 1024 > 4386
				ctx->s_ctx.mode[i].ae_binning_ratio = 1000;
			else
				ctx->s_ctx.mode[i].ae_binning_ratio = (4386 * 1024) / ctx->s_ctx.mi_dxg_ratio;
		}
		DRV_LOG(ctx,"mode(%d) mi_dxg_ratio(%d) ae_binning_ratio = %d\n",
				i,ctx->s_ctx.mi_dxg_ratio,ctx->s_ctx.mode[i].ae_binning_ratio);
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

	if (ctx->sof_cnt < 3 && -1 == temperature)
		temperature = 0;

	DRV_LOG(ctx, "temperature: %d degrees\n", temperature);
	return temperature;

}

static void set_group_hold(void *arg, u8 en)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	return;

	if (en)
		set_i2c_buffer(ctx, 0x3208, 0x00);
	else{
		set_i2c_buffer(ctx, 0x3208, 0x10);
		set_i2c_buffer(ctx, 0x3208, 0xA0);
	}
}

static void gain_control(u16 reg_gain)
{
	if (reg_gain < 1024) {
		set_i2c_buffer(ctx_ovx8000, 0x3506, ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[13]);
		set_i2c_buffer(ctx_ovx8000, 0x3546, ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[19]);
	} else {
		set_i2c_buffer(ctx_ovx8000, 0x3506, ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[13] + 0x02);
		set_i2c_buffer(ctx_ovx8000, 0x3546, ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[19] + 0x02);
	}
	DRV_LOG_MUST(ctx_ovx8000, "reg_gain(%d) current_scenario_id(%d) mode_setting_table[13](%x) mode_setting_table[19](%x)\n",
	reg_gain,
	ctx_ovx8000->current_scenario_id,
	ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[13],
	ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[19]);
}

static u16 get_gain2reg(u32 gain)
{
	u16 reg_gain = 0;

	reg_gain = gain * 256 / BASEGAIN;
	if(ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mi_mode_type == 2)
		gain_control(reg_gain);

	return reg_gain;
}

static void gain_control_dol(u16 reg_gain,u32 exp_cnt)
{
	if (exp_cnt == 0) {
		if (reg_gain < 1024) {
			set_i2c_buffer(ctx_ovx8000, 0x3506, ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[13]);
		} else {
			set_i2c_buffer(ctx_ovx8000, 0x3506, ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[13] + 0x02);
		}
	} else {
		if (reg_gain < 1024) {
			set_i2c_buffer(ctx_ovx8000, 0x3546, ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[19]);
		} else {
			set_i2c_buffer(ctx_ovx8000, 0x3546, ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[19] + 0x02);
		}
	}
	DRV_LOG_MUST(ctx_ovx8000, "exp_cnt(%d) reg_gain(%d) current_scenario_id(%d) mode_setting_table[13](%x) mode_setting_table[19](%x)\n",
	exp_cnt,
	reg_gain,
	ctx_ovx8000->current_scenario_id,
	ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[13],
	ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mode_setting_table[19]);
}

static u16 get_gain2reg_dol(u32 gain,u32 exp_cnt)
{
	u16 reg_gain = 0;

	reg_gain = gain * 256 / BASEGAIN;

	if(ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].mi_mode_type == 2)
		gain_control_dol(reg_gain,exp_cnt);

	return reg_gain;
}

static int covert_seamless_setting_table(enum SENSOR_SCENARIO_ID_ENUM scenario_id, u16* seamless_setting)
{
	u32 i = 0;
	u32 len = 0;
	DRV_LOG(ctx_ovx8000, "seamless setting convert begin\n");
	for (i = 0; i< ctx_ovx8000->s_ctx.mode[scenario_id].seamless_switch_mode_setting_len/2; i++){
		if ( 2*i < ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].seamless_switch_mode_setting_len &&
		ctx_ovx8000->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table[2*i+1] !=
		ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].seamless_switch_mode_setting_table[2*i+1]) {
			seamless_setting[2*len] = ctx_ovx8000->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table[2*i];
			seamless_setting[2*len+1] = ctx_ovx8000->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table[2*i + 1];
			len++;
		} else if (2*i >= ctx_ovx8000->s_ctx.mode[ctx_ovx8000->current_scenario_id].seamless_switch_mode_setting_len) {
			seamless_setting[2*len] = ctx_ovx8000->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table[2*i];
			seamless_setting[2*len+1] = ctx_ovx8000->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table[2*i + 1];
			len++;
		}
	}

	DRV_LOG(ctx_ovx8000, "seamless setting convert end len(%d)\n",len);

	return len;
}

static int goyaovx8000wide_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *)para;
	enum SENSOR_SCENARIO_ID_ENUM scenario_id = (enum SENSOR_SCENARIO_ID_ENUM)*feature_data;
	u32 framerate = *(feature_data + 1);
	u32 frame_length;
	u32 frame_length_step;
	u32 frame_length_min;
	u32 frame_length_max;

	if (scenario_id >= ctx->s_ctx.sensor_mode_num) {
		DRV_LOGE(ctx, "invalid sid:%u, mode_num:%u\n",
			scenario_id, ctx->s_ctx.sensor_mode_num);
		scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW;
	}
	if (!framerate) {
		DRV_LOGE(ctx, "framerate (%u) is invalid\n", framerate);
		return ERROR_NONE;
	}
	if (!ctx->s_ctx.mode[scenario_id].linelength) {
		DRV_LOGE(ctx, "linelength (%u) is invalid\n",
			ctx->s_ctx.mode[scenario_id].linelength);
		return ERROR_NONE;
	}

	if (framerate > ctx->s_ctx.mode[scenario_id].max_framerate) {
		DRV_LOGE(ctx, "framerate (%u) is greater than max_framerate (%u)\n",
			framerate, ctx->s_ctx.mode[scenario_id].max_framerate);
		framerate = ctx->s_ctx.mode[scenario_id].max_framerate;
	}

	frame_length_step = ctx->s_ctx.mode[scenario_id].framelength_step;
	/* set on the step of frame length */
	frame_length = ctx->s_ctx.mode[scenario_id].pclk / framerate * 10
		/ ctx->s_ctx.mode[scenario_id].linelength;
	frame_length = frame_length_step ?
		(frame_length - (frame_length % frame_length_step)) : frame_length;
	frame_length_min = ctx->s_ctx.mode[scenario_id].framelength;
	frame_length_max = ctx->s_ctx.frame_length_max;
	frame_length_max = frame_length_step ?
		(frame_length_max - (frame_length_max % frame_length_step)) : frame_length_max;


	/* set in the range of frame length */
	ctx->frame_length = max(frame_length, frame_length_min);
	ctx->frame_length = min(ctx->frame_length, frame_length_max);
	ctx->frame_length = frame_length_step ?
		roundup(ctx->frame_length,frame_length_step) : ctx->frame_length;

	/* set default frame length if given default framerate */
	if (framerate == ctx->s_ctx.mode[scenario_id].max_framerate)
		ctx->frame_length = ctx->s_ctx.mode[scenario_id].framelength;

	ctx->current_fps = ctx->s_ctx.mode[scenario_id].pclk /
						ctx->frame_length * 10 /
						ctx->s_ctx.mode[scenario_id].linelength;
	ctx->min_frame_length = ctx->frame_length;
	DRV_LOG(ctx, "max_fps(input/output):%u/%u(sid:%u), min_fl_en:1, ctx->frame_length:%u\n",
		framerate, ctx->current_fps, scenario_id, ctx->frame_length);

	return ERROR_NONE;
}

void goyaovx8000wide_update_mode_info(struct subdrv_ctx *ctx, enum SENSOR_SCENARIO_ID_ENUM scenario_id)
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

void goyaovx8000wide_write_frame_length(struct subdrv_ctx *ctx, u32 fll)
{
	u32 addr_h = ctx->s_ctx.reg_addr_frame_length.addr[0];
	u32 addr_l = ctx->s_ctx.reg_addr_frame_length.addr[1];
	u32 addr_ll = ctx->s_ctx.reg_addr_frame_length.addr[2];
	u32 fll_step = 0;
	u32 dol_cnt = 1;

	if (ctx->s_ctx.reg_addr_frame_length.addr[0] == 0 &&
		ctx->s_ctx.reg_addr_frame_length.addr[1] == 0 &&
		ctx->s_ctx.reg_addr_frame_length.addr[2] == 0)
		return;

	check_current_scenario_id_bound(ctx);
	fll_step = ctx->s_ctx.mode[ctx->current_scenario_id].framelength_step;
	if (fll_step)
		fll = roundup(fll, fll_step);
	ctx->frame_length = fll;

//	if (ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_STAGGER)
//		dol_cnt = ctx->s_ctx.mode[ctx->current_scenario_id].exp_cnt;

	fll = fll / dol_cnt;

	if (addr_ll) {
		set_i2c_buffer(ctx,	addr_h,	(fll >> 16) & 0xFF);
		set_i2c_buffer(ctx,	addr_l, (fll >> 8) & 0xFF);
		set_i2c_buffer(ctx,	addr_ll, fll & 0xFF);
	} else {
		set_i2c_buffer(ctx,	addr_h, (fll >> 8) & 0xFF);
		set_i2c_buffer(ctx,	addr_l, fll & 0xFF);
	}
	/* update FL RG value after setting buffer for writing RG */
	ctx->frame_length_rg = ctx->frame_length;

		DRV_LOG(ctx,
			"ctx:(fl(RG):%u), fll[0x%x] multiply %u, fll_step:%u\n",
			ctx->frame_length_rg, fll, dol_cnt, fll_step);

}

void goyaovx8000wide_set_multi_shutter_frame_length_dol(struct subdrv_ctx *ctx,
	u64 *shutters, u16 exp_cnt,	u16 frame_length)
{
int i = 0;
int fine_integ_line = 0;
u16 last_exp_cnt = 1;
u32 calc_fl[4] = {0};
bool gph = !ctx->is_seamless && (ctx->s_ctx.s_gph != NULL);
u32 rg_shutters[3] = {0};
u32 cit_step = 0;
u32 fll = 0, fll_temp = 0, s_fll;

fll = frame_length ? frame_length : ctx->min_frame_length;
if (exp_cnt > ARRAY_SIZE(ctx->exposure)) {
	DRV_LOGE(ctx, "invalid exp_cnt:%u>%lu\n", exp_cnt, ARRAY_SIZE(ctx->exposure));
	exp_cnt = ARRAY_SIZE(ctx->exposure);
}
check_current_scenario_id_bound(ctx);

/* check boundary of shutter */
for (i = 1; i < ARRAY_SIZE(ctx->exposure); i++)
	last_exp_cnt += ctx->exposure[i] ? 1 : 0;
fine_integ_line = ctx->s_ctx.mode[ctx->current_scenario_id].fine_integ_line;
cit_step = ctx->s_ctx.mode[ctx->current_scenario_id].coarse_integ_step;
for (i = 0; i < exp_cnt; i++) {
	shutters[i] = FINE_INTEG_CONVERT(shutters[i], fine_integ_line);
	shutters[i] = max_t(u64, shutters[i],
		(u64)ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_shutter_range[i].min);
	shutters[i] = min_t(u64, shutters[i],
		(u64)ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_shutter_range[i].max);
	if (cit_step)
		shutters[i] = roundup(shutters[i], cit_step);
}

/* check boundary of framelength */
/* - (1) previous Me + current le + 128 */
if (ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_STAGGER) {
	calc_fl[0] = (u32) shutters[0] + ctx->exposure[1]
	+ ctx->s_ctx.mode[ctx->current_scenario_id].read_margin;

	/* - (2) previous Me + readout_length */
	if (ctx->exposure[1] + 3072 > (u32) shutters[1])
	calc_fl[1] = ctx->exposure[1] + 3072 - (u32) shutters[1] ;

	calc_fl[2] = (u32) shutters[0] + (u32) shutters[1]
	+ ctx->s_ctx.mode[ctx->current_scenario_id].read_margin;

	calc_fl[3] = ctx->s_ctx.mode[ctx->current_scenario_id].framelength
	+ ctx->s_ctx.mode[ctx->current_scenario_id].read_margin;
	DRV_LOG(ctx,
		"calc_fl[0](%u) calc_fl[1]%u, calc_fl[2]%u, calc_fl[3]%u,exp-LE/ME/SE (%u/%u/%u),shutter-LE/ME/SE (%u/%u/%u)\
		read_margin(%u)\
		readout_length(%d)\n",\
		calc_fl[0],calc_fl[1],calc_fl[2],calc_fl[3],ctx->exposure[0],ctx->exposure[1],ctx->exposure[2],\
		(u32)shutters[0], (u32)shutters[1], (u32)shutters[2],\
		ctx->s_ctx.mode[ctx->current_scenario_id].read_margin,\
		ctx->s_ctx.mode[ctx->current_scenario_id].readout_length
	);
}

for (i = 0; i < ARRAY_SIZE(calc_fl); i++)
	fll = max(fll, calc_fl[i]);
fll =	max(fll, ctx->min_frame_length);
fll =	min(fll, ctx->s_ctx.frame_length_max);
/* restore shutter */
memset(ctx->exposure, 0, sizeof(ctx->exposure));
for (i = 0; i < exp_cnt; i++)
	ctx->exposure[i] = (u32) shutters[i];
/* group hold start */
if (gph)
	ctx->s_ctx.s_gph((void *)ctx, 1);
/* enable auto extend */
if (ctx->s_ctx.reg_addr_auto_extend)
	set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_auto_extend, 0x01);

if (ctx->s_ctx.mode[ctx->current_scenario_id].sw_fl_delay) {
	fll_temp = ctx->frame_length_next;
	ctx->frame_length_next = fll;
	s_fll = calc_fl[0];
	for (i = 1; i < ARRAY_SIZE(calc_fl); i++)
		s_fll = max(s_fll, calc_fl[i]);
	if (s_fll > frame_length) {
		fll = s_fll;
		fll = max(fll, ctx->min_frame_length);
		fll = min(fll, ctx->s_ctx.frame_length_max);
		if(fll<fll_temp)
			fll = fll_temp;
	} else {
		if (s_fll > fll_temp)
			fll = s_fll;
		else
			fll = fll_temp;
	}
	DRV_LOG(ctx, "fll:%u, s_fll:%u, fll_temp:%u, frame_length:%u\n",
		fll, s_fll, fll_temp, frame_length);
}
ctx->frame_length = fll;
if (ctx->extraVB) {
	ctx->frame_length += ctx->extraVB * (ctx->pclk / 1000)  / ctx->line_length + ctx->s_ctx.mode[ctx->current_scenario_id].imgsensor_winsize_info.h2_tg_size;
	DRV_LOG(ctx, "ctx->exposure[0] = %d, framelength = %d, extraVB(%d), hSize(%d)", ctx->exposure[0], ctx->frame_length, ctx->extraVB, ctx->s_ctx.mode[ctx->current_scenario_id].imgsensor_winsize_info.h2_tg_size);
}
/* write framelength */
if (set_auto_flicker(ctx, 0) || frame_length || !ctx->s_ctx.reg_addr_auto_extend)
	goyaovx8000wide_write_frame_length(ctx, ctx->frame_length);
else if (ctx->s_ctx.reg_addr_auto_extend)
	goyaovx8000wide_write_frame_length(ctx, ctx->min_frame_length);
/* write shutter */
switch (exp_cnt) {
case 1:
	rg_shutters[0] = (u32) shutters[0];
	break;
case 2:
	rg_shutters[0] = (u32) shutters[0];
	rg_shutters[2] = (u32) shutters[1];
	break;
case 3:
	rg_shutters[0] = (u32) shutters[0];
	rg_shutters[1] = (u32) shutters[1];
	rg_shutters[2] = (u32) shutters[2];
	break;
default:
	break;
}
if (ctx->s_ctx.reg_addr_exposure_lshift != PARAM_UNDEFINED) {
	set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_exposure_lshift, 0);
	ctx->l_shift = 0;
}
for (i = 0; i < 3; i++) {
	if (rg_shutters[i]) {
		if (ctx->s_ctx.reg_addr_exposure[i].addr[2]) {
			set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[0],
				(rg_shutters[i] >> 16) & 0xFF);
			set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[1],
				(rg_shutters[i] >> 8) & 0xFF);
			set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[2],
				rg_shutters[i] & 0xFF);

			DRV_LOG(ctx, "mode id = %d hdr_mode = %d",ctx->current_scenario_id,ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode);
			if(ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_DCG_COMPOSE ){
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[2].addr[0],
					(rg_shutters[i] >> 16) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[2].addr[1],
					(rg_shutters[i] >> 8) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[2].addr[2],
					rg_shutters[i] & 0xFF);
			}
		} else {
			set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[0],
				(rg_shutters[i] >> 8) & 0xFF);
			set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[1],
				rg_shutters[i] & 0xFF);
		}
	}
}
DRV_LOG(ctx, "exp[0x%d/0x%d/0x%d], fll(input/output):%u/%u, flick_en:%d, extraVB:%d\n",
	rg_shutters[0], rg_shutters[1], rg_shutters[2],
	frame_length, ctx->frame_length, ctx->autoflicker_en, ctx->extraVB);
if (!ctx->ae_ctrl_gph_en) {
	if (gph)
		ctx->s_ctx.s_gph((void *)ctx, 0);
	commit_i2c_buffer(ctx);
}
/* group hold end */
}

static int goyaovx8000wide_set_hdr_tri_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	int i = 0;
	u64 values[3] = {0};
	u32 frame_length_in_lut[IMGSENSOR_STAGGER_EXPOSURE_CNT] = {0};
	u64 *shutters = (u64 *)para;
	int exp_cnt = 2;

	if (shutters != NULL) {
		for (i = 0; i < 3; i++)
			values[i] = (u64) *(shutters + i);
	}
	if (ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_LBMF) {
		set_multi_shutter_frame_length_in_lut(ctx,
			values, exp_cnt, 0, frame_length_in_lut);
		return 0;
	}
	DRV_LOG(ctx, "goyaovx8000wide_set_hdr_tri_shutter");

	goyaovx8000wide_set_multi_shutter_frame_length_dol(ctx, values, exp_cnt, 0);
	return 0;
}

static int goyaovx8000wide_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	enum SENSOR_SCENARIO_ID_ENUM scenario_id;
	enum IMGSENSOR_HDR_MODE_ENUM src_hdr;
	struct mtk_hdr_ae *ae_ctrl = NULL;
	u64 *feature_data = (u64 *)para;
	u32 frame_length_in_lut[IMGSENSOR_STAGGER_EXPOSURE_CNT] = {0};
	u32 exp_cnt = 0;
	u16 goyaovx8000wide_setting_seamless[700] = {0};
	u32 setting_seamless_len = 0;
	u16 is_dol = false;
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

	setting_seamless_len = covert_seamless_setting_table(scenario_id, goyaovx8000wide_setting_seamless);

	if (scenario_id >= ctx->s_ctx.sensor_mode_num) {
		DRV_LOGE(ctx, "invalid sid:%u, mode_num:%u\n",
			scenario_id, ctx->s_ctx.sensor_mode_num);
		return ERROR_NONE;
	}

	if (ctx->current_scenario_id == SENSOR_SCENARIO_ID_CUSTOM15
	|| ctx->current_scenario_id == SENSOR_SCENARIO_ID_CUSTOM31)
		is_dol  = true;

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

	goyaovx8000wide_update_mode_info(ctx, scenario_id);

	if (is_dol) {
		i2c_table_write(ctx,goyaovx8000wide_seamless_switch_step1_dol,
			ARRAY_SIZE(goyaovx8000wide_seamless_switch_step1_dol));
	} else {
		i2c_table_write(ctx,goyaovx8000wide_seamless_switch_step1,
			ARRAY_SIZE(goyaovx8000wide_seamless_switch_step1));
	}

	if (ctx->s_ctx.mode[scenario_id].seamless_switch_group == 1 ||
		ctx->s_ctx.mode[scenario_id].seamless_switch_group == 2) {
		i2c_table_write(ctx,
			ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table,
			ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_len);
	 } else {
		i2c_table_write(ctx,goyaovx8000wide_setting_seamless, 2*setting_seamless_len);
	 }

	if (ctx->s_ctx.mode[scenario_id].mi_mode_type == 1)
		goyaovx8000wide_set_awb_gain(ctx, (u8 *)&g_last_awb_gain, &retLen);

	if ((ctx->current_fps > 290) && (scenario_id == SENSOR_SCENARIO_ID_CUSTOM13 ||
		scenario_id == SENSOR_SCENARIO_ID_CUSTOM29) ) {
		set_max_framerate_by_scenario(ctx, scenario_id, 300);
	}

	if (ae_ctrl) {
		switch (ctx->s_ctx.mode[scenario_id].hdr_mode) {
		case HDR_RAW_STAGGER:
			goyaovx8000wide_set_multi_shutter_frame_length_dol(ctx, (u64 *)&ae_ctrl->exposure, exp_cnt, 0);
			set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			break;
		case HDR_RAW_LBMF:
			set_multi_shutter_frame_length_in_lut(ctx,
				(u64 *)&ae_ctrl->exposure, exp_cnt, 0, frame_length_in_lut);
			set_multi_gain_in_lut(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			break;
		case HDR_RAW_DCG_COMPOSE:
			goyaovx8000wide_set_shutter_frame_length(ctx, (u8 *)&ae_ctrl->exposure, len);
			if (ctx->s_ctx.mode[scenario_id].dcg_info.dcg_gain_mode == IMGSENSOR_DCG_DIRECT_MODE) {
				goyaovx8000wide_set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			}
			else
				set_gain(ctx, ae_ctrl->gain.le_gain);
			break;
		default:
			goyaovx8000wide_set_shutter(ctx, (u8 *)&ae_ctrl->exposure.le_exposure,len);
			set_gain(ctx, ae_ctrl->gain.le_gain);
			break;
		}
	}

	if (is_dol) {
		i2c_table_write(ctx,goyaovx8000wide_seamless_switch_step2_dol,
			ARRAY_SIZE(goyaovx8000wide_seamless_switch_step2_dol));
	} else {
		i2c_table_write(ctx,goyaovx8000wide_seamless_switch_step2,
			ARRAY_SIZE(goyaovx8000wide_seamless_switch_step2));
	}

	ctx->fast_mode_on = TRUE;
	ctx->ref_sof_cnt = ctx->sof_cnt;
	DRV_LOG(ctx, "X: set seamless switch done\n");
	return ERROR_NONE;
}

static u16 goyaovx8000wide_dgain2reg(struct subdrv_ctx *ctx, u32 dgain)
{
	return dgain; // digitalRealGain * 1024
}
static int goyaovx8000wide_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *) para;
	u64 *shutters = (u64 *)(* feature_data);
	u16 exp_cnt = (u16) (*(feature_data + 1));
	u16 frame_length = (u16) (*(feature_data + 2));	
	int i = 0;
	int fine_integ_line = 0;
	u16 last_exp_cnt = 1;
	u32 calc_fl[4] = {0};
	bool gph = !ctx->is_seamless && (ctx->s_ctx.s_gph != NULL);
	u32 rg_shutters[3] = {0};
	u32 cit_step = 0;
	u32 fll = 0, fll_temp = 0, s_fll;

	fll = frame_length ? frame_length : ctx->min_frame_length;
	if (exp_cnt > ARRAY_SIZE(ctx->exposure)) {
		DRV_LOGE(ctx, "invalid exp_cnt:%u>%lu\n", exp_cnt, ARRAY_SIZE(ctx->exposure));
		exp_cnt = ARRAY_SIZE(ctx->exposure);
	}
	check_current_scenario_id_bound(ctx);

	/* check boundary of shutter */
	for (i = 1; i < ARRAY_SIZE(ctx->exposure); i++)
		last_exp_cnt += ctx->exposure[i] ? 1 : 0;
	fine_integ_line = ctx->s_ctx.mode[ctx->current_scenario_id].fine_integ_line;
	cit_step = ctx->s_ctx.mode[ctx->current_scenario_id].coarse_integ_step;
	for (i = 0; i < exp_cnt; i++) {
		shutters[i] = FINE_INTEG_CONVERT(shutters[i], fine_integ_line);
		shutters[i] = max_t(u64, shutters[i],
			(u64)ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_shutter_range[i].min);
		shutters[i] = min_t(u64, shutters[i],
			(u64)ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_shutter_range[i].max);
		if (cit_step)
			shutters[i] = roundup(shutters[i], cit_step);
	}

	//frame need > shutter + magin
	for (i = 0; i < exp_cnt; i++)
	calc_fl[3] += (u32) shutters[i];
	calc_fl[3] += ctx->s_ctx.exposure_margin*exp_cnt*exp_cnt;

	/* check boundary of framelength */
	/* - (1) previous Me + current le + 128 */
	if (ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_STAGGER) {
		calc_fl[0] = (u32) shutters[0] + ctx->exposure[1]
		+ ctx->s_ctx.mode[ctx->current_scenario_id].read_margin;

		/* - (2) previous Me + readout_length */
		if (ctx->exposure[1] + 3072 > (u32) shutters[1])
		calc_fl[1] = ctx->exposure[1] + 3072 - (u32) shutters[1] ;

		calc_fl[2] = (u32) shutters[0] + (u32) shutters[1] + (u32) shutters[0]/2
		+ ctx->s_ctx.mode[ctx->current_scenario_id].read_margin*4;

		calc_fl[3] = ctx->s_ctx.mode[ctx->current_scenario_id].framelength
		+ ctx->s_ctx.mode[ctx->current_scenario_id].read_margin*6;
		DRV_LOG(ctx,
			"calc_fl[0](%u) calc_fl[1]%u, calc_fl[2]%u, calc_fl[3]%u,exp-LE/ME/SE (%u/%u/%u),shutter-LE/ME/SE (%u/%u/%u)\
			read_margin(%u)\
			readout_length(%d)\n",\
			calc_fl[0],calc_fl[1],calc_fl[2],calc_fl[3],ctx->exposure[0],ctx->exposure[1],ctx->exposure[2],\
			(u32)shutters[0], (u32)shutters[1], (u32)shutters[2],\
			ctx->s_ctx.mode[ctx->current_scenario_id].read_margin,\
			ctx->s_ctx.mode[ctx->current_scenario_id].readout_length
		);
	}

	for (i = 0; i < ARRAY_SIZE(calc_fl); i++)
		fll = max(fll, calc_fl[i]);
	fll =	max(fll, ctx->min_frame_length);
	fll =	min(fll, ctx->s_ctx.frame_length_max);
	/* restore shutter */
	memset(ctx->exposure, 0, sizeof(ctx->exposure));
	for (i = 0; i < exp_cnt; i++)
		ctx->exposure[i] = (u32) shutters[i];
	/* group hold start */
	if (gph)
		ctx->s_ctx.s_gph((void *)ctx, 1);
	/* enable auto extend */
	if (ctx->s_ctx.reg_addr_auto_extend)
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_auto_extend, 0x01);

	if (ctx->s_ctx.mode[ctx->current_scenario_id].sw_fl_delay) {
		fll_temp = ctx->frame_length_next;
		ctx->frame_length_next = fll;
		s_fll = calc_fl[0];
		for (i = 1; i < ARRAY_SIZE(calc_fl); i++)
			s_fll = max(s_fll, calc_fl[i]);
		if (s_fll > frame_length) {
			fll = s_fll;
			fll = max(fll, ctx->min_frame_length);
			fll = min(fll, ctx->s_ctx.frame_length_max);
			if(fll<fll_temp)
				fll = fll_temp;
		} else {
			if (s_fll > fll_temp)
				fll = s_fll;
			else
				fll = fll_temp;
		}
		DRV_LOG(ctx, "fll:%u, s_fll:%u, fll_temp:%u, frame_length:%u\n",
			fll, s_fll, fll_temp, frame_length);
	}
	ctx->frame_length = fll;
	if (ctx->extraVB) {
		ctx->frame_length += ctx->extraVB * (ctx->pclk / 1000)  / ctx->line_length + ctx->s_ctx.mode[ctx->current_scenario_id].imgsensor_winsize_info.h2_tg_size;
		DRV_LOG(ctx, "ctx->exposure[0] = %d, framelength = %d, extraVB(%d), hsize(%d)", ctx->exposure[0], ctx->frame_length, ctx->extraVB, ctx->s_ctx.mode[ctx->current_scenario_id].imgsensor_winsize_info.h2_tg_size);
	}
	/* write framelength */
	if (set_auto_flicker(ctx, 0) || frame_length || !ctx->s_ctx.reg_addr_auto_extend)
		goyaovx8000wide_write_frame_length(ctx, ctx->frame_length);
	else if (ctx->s_ctx.reg_addr_auto_extend)
		write_frame_length(ctx, ctx->min_frame_length);
	/* write shutter */
	switch (exp_cnt) {
	case 1:
		rg_shutters[0] = (u32) shutters[0];
		break;
	case 2:
		rg_shutters[0] = (u32) shutters[0];
		rg_shutters[2] = (u32) shutters[1];
		break;
	case 3:
		rg_shutters[0] = (u32) shutters[0];
		rg_shutters[1] = (u32) shutters[1];
		rg_shutters[2] = (u32) shutters[2];
		break;
	default:
		break;
	}
	if (ctx->s_ctx.reg_addr_exposure_lshift != PARAM_UNDEFINED) {
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_exposure_lshift, 0);
		ctx->l_shift = 0;
	}
	for (i = 0; i < 3; i++) {
		if (rg_shutters[i]) {
			if (ctx->s_ctx.reg_addr_exposure[i].addr[2]) {
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[0],
					(rg_shutters[i] >> 16) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[1],
					(rg_shutters[i] >> 8) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[2],
					rg_shutters[i] & 0xFF);

				DRV_LOG(ctx, "mode id = %d hdr_mode = %d",ctx->current_scenario_id,ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode);
				if(ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_DCG_COMPOSE ){
					set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[2].addr[0],
						(rg_shutters[i] >> 16) & 0xFF);
					set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[2].addr[1],
						(rg_shutters[i] >> 8) & 0xFF);
					set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[2].addr[2],
						rg_shutters[i] & 0xFF);
				}
			} else {
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[0],
					(rg_shutters[i] >> 8) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[1],
					rg_shutters[i] & 0xFF);
			}
		}
	}
	DRV_LOG(ctx, "exp[0d%x/0d%x/0d%x], fll(input/output):%u/%u, flick_en:%d, extraVB:%d\n",
		rg_shutters[0], rg_shutters[1], rg_shutters[2],
		frame_length, ctx->frame_length, ctx->autoflicker_en, ctx->extraVB);
	if (!ctx->ae_ctrl_gph_en) {
		if (gph)
			ctx->s_ctx.s_gph((void *)ctx, 0);
		commit_i2c_buffer(ctx);
	}
	/* group hold end */
	return 0;
}

static int goyaovx8000wide_set_multi_dig_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
		gains[i] = goyaovx8000wide_dgain2reg(ctx, gains[i]);
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
				(rg_gains[i] << 6) & 0xC0);
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

static int goyaovx8000wide_set_hdr_tri_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
	goyaovx8000wide_set_multi_gain(ctx,values, exp_cnt);
	return 0;
}

static void goyaovx8000wide_set_multi_gain(struct subdrv_ctx *ctx, u32 *gains, u16 exp_cnt)
{
	int i = 0;
	u32 rg_gains[3] = {0};
	u8 has_gains[3] = {0};
	u32 ration = 0, ration_back;
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

		DRV_LOG(ctx, "mode[%d].multi_exposure_ana_gain_range[%d], max: 0x%x, min:0x%x \n",
						ctx->current_scenario_id,i,ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_ana_gain_range[i].max,
						ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_ana_gain_range[i].min);
		/* mapping of gain to register value */
		gains[i] = get_gain2reg_dol(gains[i], i);

	}
		/*check DXG gain*/
	if(ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_DCG_COMPOSE ){
		u32 le_gain = ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_LE];
		u32 me_gain = ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_ME];

		if((me_gain *1050 /le_gain * 1024 ) > (ctx->s_ctx.mi_dxg_ratio *1000)){
			DRV_LOG_MUST(ctx,"dxg ratio error! before LE %d ,ME %d",le_gain,me_gain);
			le_gain = (me_gain *1024/ctx->s_ctx.mi_dxg_ratio)*1050/1000;
			DRV_LOG_MUST(ctx,"dxg ratio error! after LE 0x%x ,ME 0x%x",le_gain,me_gain);
			ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_LE]=le_gain;
			ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_ME]=me_gain;
			gains[0] = get_gain2reg_dol(le_gain, 0);
			gains[1] = get_gain2reg_dol(me_gain, 1);
		}
	}
	/* restore gain */
	memset(ctx->ana_gain, 0, sizeof(ctx->ana_gain));
	for (i = 0; i < exp_cnt; i++)
		ctx->ana_gain[i] = gains[i];
	/* group hold start */
	if (gph && !ctx->ae_ctrl_gph_en)
		ctx->s_ctx.s_gph((void *)ctx, 1);
	/* write gain */
	if(ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_DCG_COMPOSE )
		set_i2c_buffer(ctx, 0x5006, 0x02);
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
			set_i2c_buffer(ctx,ctx->s_ctx.reg_addr_ana_gain[i].addr[0],(rg_gains[i] >> 8) & 0xFF);
			set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_ana_gain[i].addr[1],rg_gains[i] & 0xFF);
		}
	}
	if(ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_DCG_COMPOSE ){
		if(ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_LE] > 0 &&
			ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_ME] > 0){
			set_i2c_buffer(ctx, 0x5019, (ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_LE] >> 16)& 0xFF);
			set_i2c_buffer(ctx, 0x501a, (ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_LE] >> 8)& 0xFF);
			set_i2c_buffer(ctx, 0x501b, ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_LE] & 0xFF);
			set_i2c_buffer(ctx, 0x501c, (ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_ME] >> 16)& 0xFF);
			set_i2c_buffer(ctx, 0x501d, (ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_ME] >> 8)& 0xFF);
			set_i2c_buffer(ctx, 0x501e, ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_ME] & 0xFF);
		}
	}
	DRV_LOG(ctx, "total gain[lg/mg]: 0x%x 0x%x\n", ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_LE],
				ctx->s_ctx.mi_dcg_gain[IMGSENSOR_STAGGER_EXPOSURE_ME]);
	DRV_LOG(ctx, "reg[lg/mg/sg]: 0x%x 0x%x 0x%x\n", rg_gains[0], rg_gains[1], rg_gains[2]);

	ration = subdrv_i2c_rd_u16(ctx, 0x6a02);
	ration_back = subdrv_i2c_rd_u16(ctx, 0x5060);

	DRV_LOG(ctx, "ration:0x%x ration_back:0x%x \n",ration,ration_back);


	if (gph)
		ctx->s_ctx.s_gph((void *)ctx, 0);
	commit_i2c_buffer(ctx);
	/* group hold end */
}

static int goyaovx8000wide_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
	/* write framelength */
	if (set_auto_flicker(ctx, 0) || frame_length || !ctx->s_ctx.reg_addr_auto_extend)
		goyaovx8000wide_write_frame_length(ctx, ctx->frame_length);
	/* write shutter */
	set_long_exposure(ctx);
	if (ctx->s_ctx.reg_addr_exposure[0].addr[2]) {
		set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[0].addr[0],
			(ctx->exposure[0] >> 16) & 0xFF);
		set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[0].addr[1],
			(ctx->exposure[0] >> 8) & 0xFF);
		set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[0].addr[2],
			ctx->exposure[0] & 0xFF);
		//DXG
		DRV_LOG(ctx, "mode id = %d hdr_mode = %d",ctx->current_scenario_id,ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode);
		if(ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_DCG_COMPOSE ){
			set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[2].addr[0],
				(ctx->exposure[0] >> 16) & 0xFF);
			set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[2].addr[1],
				(ctx->exposure[0] >> 8) & 0xFF);
			set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[2].addr[2],
				ctx->exposure[0] & 0xFF);
		}
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
	}/* group hold end */
	return 0;
}


static int goyaovx8000wide_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	goyaovx8000wide_set_shutter_frame_length(ctx, para, len);
	return 0;
}

static int goyaovx8000wide_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	switch (mode) {
	case 5:
		subdrv_i2c_wr_u8(ctx, 0x50c1, 0x01);
		subdrv_i2c_wr_u8(ctx, 0x50c2, 0x01);
		break;
	default:
		subdrv_i2c_wr_u8(ctx, 0x50c1, 0x00);
		subdrv_i2c_wr_u8(ctx, 0x50c2, 0x00);
		break;
	}

	ctx->test_pattern = mode;
	return ERROR_NONE;
}

static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id) {
	memcpy(&(ctx->s_ctx), &static_ctx, sizeof(struct subdrv_static_ctx));
	subdrv_ctx_init(ctx);
	ctx_ovx8000 = ctx;
	ctx->i2c_client = i2c_client;
	ctx->i2c_write_id = i2c_write_id;
	return 0;
}

static int vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt)
{
	kal_uint16 sensor_output_cnt;

	sensor_output_cnt = subdrv_i2c_rd_u8(ctx, ctx->s_ctx.reg_addr_frame_count);
	DRV_LOG_MUST(ctx, "sensormode(%d) sof_cnt(%d) sensor_output_cnt(%d)\n",
		ctx->current_scenario_id, sof_cnt, sensor_output_cnt);

	ctx->is_seamless = FALSE;

	if (ctx->fast_mode_on && (sof_cnt > ctx->ref_sof_cnt)) {
		ctx->fast_mode_on = FALSE;
		ctx->ref_sof_cnt = 0;
		DRV_LOG(ctx, "seamless_switch disabled.");
		//set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_fast_mode, 0x00);
		//commit_i2c_buffer(ctx);
	}

	return 0;
};

static int goyaovx8000wide_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *) para;
	struct SET_SENSOR_AWB_GAIN *awb_gain = (struct SET_SENSOR_AWB_GAIN *)feature_data;
	MUINT32 r_Gain = awb_gain->ABS_GAIN_R << 1;
	MUINT32 g_Gain = awb_gain->ABS_GAIN_GR << 1;
	MUINT32 b_Gain = awb_gain->ABS_GAIN_B << 1;
	u8 data = 0;
	u16 i = 0;

	g_last_awb_gain = *awb_gain;

	if (r_Gain == 0 || g_Gain == 0 || b_Gain == 0) {
		DRV_LOG(ctx, "error awb gain [r/g/b]: 0x%x 0x%x 0x%x\n",
			r_Gain, g_Gain, b_Gain);
		return 0;
	}

	// en manual awb gain
	data = subdrv_i2c_rd_u8(ctx, 0x5001);
	if(ctx->is_seamless){
		for (i = 0; i< ctx->s_ctx.mode[ctx->current_scenario_id].mode_setting_len/2; i++){
			if (ctx->s_ctx.mode[ctx->current_scenario_id].seamless_switch_mode_setting_table[2*i] == 0x5001)
			data = ctx->s_ctx.mode[ctx->current_scenario_id].seamless_switch_mode_setting_table[2*i+1];
		}
	}
	data &= ~0x08; // bit[3] = 0
	subdrv_i2c_wr_u8(ctx, 0x5001, data); // Simple AWB enable

	data = subdrv_i2c_rd_u8(ctx, 0x58b5);
	data |= 0x01; // bit[0] = 1
	subdrv_i2c_wr_u8(ctx, 0x58b5, data); // man_qpdwb_gain_enable

	data = subdrv_i2c_rd_u8(ctx, 0x5ab0);
	data |= 0x02; // bit[1] = 1
	subdrv_i2c_wr_u8(ctx, 0x5ab0, data);

	// set awb gain
	subdrv_i2c_wr_u8(ctx, 0x58ac, (b_Gain >> 8) & 0x7F); //B Gain [14:8]
	subdrv_i2c_wr_u8(ctx, 0x58ad, (b_Gain >> 0) & 0xFF); //B Gain [7:0]
	subdrv_i2c_wr_u8(ctx, 0x58ae, (g_Gain >> 8) & 0x7F); //Gb Gain [14:8]
	subdrv_i2c_wr_u8(ctx, 0x58af, (g_Gain >> 0) & 0xFF); //Gb Gain [7:0]
	subdrv_i2c_wr_u8(ctx, 0x58b0, (g_Gain >> 8) & 0x7F); //Gr Gain [14:8]
	subdrv_i2c_wr_u8(ctx, 0x58b1, (g_Gain >> 0) & 0xFF); //Gr Gain [7:0]
	subdrv_i2c_wr_u8(ctx, 0x58b2, (r_Gain >> 8) & 0x7F); //R Gain [14:8]
	subdrv_i2c_wr_u8(ctx, 0x58b3, (r_Gain >> 0) & 0xFF); //R Gain [7:0]

	DRV_LOG(ctx, "awb gain [r/g/b]: 0x%x 0x%x 0x%x\n",
		r_Gain, g_Gain, b_Gain);

	return 0;
}

static int get_csi_param(struct subdrv_ctx *ctx,
	enum SENSOR_SCENARIO_ID_ENUM scenario_id,
	struct mtk_csi_param *csi_param)
{
	switch (scenario_id) {
	default:
		csi_param->cdr_delay_enable = 1;
		csi_param->cdr_delay        = 8;
		break;
	}
	DRV_LOG(ctx, "scenario_id:%u, cdr param custom:%d/%d\n", scenario_id, csi_param->cdr_delay_enable, csi_param->cdr_delay);
	return 0;
}