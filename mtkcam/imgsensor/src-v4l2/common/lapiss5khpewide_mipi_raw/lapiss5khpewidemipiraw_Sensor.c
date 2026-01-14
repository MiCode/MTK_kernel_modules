// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

/********************************************************************
 *
 * Filename:
 * ---------
 *	 lapiss5khpewidemipiraw_Sensor.c
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
 *-------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *===================================================================
 *******************************************************************/
#include "lapiss5khpewidemipiraw_Sensor.h"
#define LAPISS5KHPEWIDE_LOG_INF(format, args...) pr_info(LOG_TAG "[%s] " format, __func__, ##args)
static void set_group_hold(void *arg, u8 en);
static u16 get_gain2reg(u32 gain);
static int lapiss5khpewide_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapiss5khpewide_set_test_pattern_data(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapiss5khpewide_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);
static void mi_stream(void *arg,bool enable);
static int lapiss5khpewide_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int lapiss5khpewide_get_sensor_temperature(void *arg);
static int lapiss5khpewide_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id);
static int s5khpe_get_feature_get_4cell_data(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static void lapiss5khpewide_s_mi_init_setting(void* arg);
static void lapiss5khpewide_s_mi_mode_setting(void *arg, enum SENSOR_SCENARIO_ID_ENUM scenario_id);
static int lapiss5khpewide_set_curr_lens_data(struct subdrv_ctx *ctx, u8 *para, u32 *len);

#define AWB_RED_GAIN_ADDR   0x0D82
#define AWB_GREEN_GAIN_ADDR 0x0D84
#define AWB_BLUE_GAIN_ADDR  0x0D86

/* STRUCT */
static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, lapiss5khpewide_set_test_pattern},
	{SENSOR_FEATURE_SET_TEST_PATTERN_DATA, lapiss5khpewide_set_test_pattern_data},
	{SENSOR_FEATURE_SEAMLESS_SWITCH, lapiss5khpewide_seamless_switch},
	{SENSOR_FEATURE_SET_AWB_GAIN, lapiss5khpewide_set_awb_gain},
	{SENSOR_FEATURE_GET_4CELL_DATA, s5khpe_get_feature_get_4cell_data},
	{SENSOR_FEATURE_SET_CURR_LENS_DATA, lapiss5khpewide_set_curr_lens_data},

};

//OTP FOR E2E
static void s5khpe_read_swxtc_data(struct subdrv_ctx *ctx);

//s5khpe 200m swxtc data from eeprom
#define S5KHPE_EEPROM_I2C_ADDR 0xA2
#define S5KHPE_SWXTC_DATA_ADDR_STAST 0x3956
#define S5KHPE_SWXTC_DATA_ADDR_END   0x5D55
#define S5KHPE_SWXTC_DATA_LEN  (S5KHPE_SWXTC_DATA_ADDR_END - S5KHPE_SWXTC_DATA_ADDR_STAST + 1)
static u8 s5khpe_swxtc_data[S5KHPE_SWXTC_DATA_LEN] = {0};

static void s5khpe_read_sensor_otp_data(struct subdrv_ctx *ctx);
#define SENSOR_OTP_DATA_LEN 39168
static unsigned char s5khpe_sensor_otp_data[SENSOR_OTP_DATA_LEN] = {0};

#define QXTC_END_FALG_BYTE 4


static u8 otp_readed = 0;

#define SEAMLESS_SWITCH_GROUP_NORMAL_14BIT_MODE 1
#define SEAMLESS_SWITCH_GROUP_NORMAL_FULL_MODE  2
#define SEAMLESS_SWITCH_GROUP_VIDEO_14BIT_MODE  3

static struct eeprom_info_struct eeprom_info[] = {
	{
		.header_id = 0x0083,
		.addr_header_id = 0x000C,
		.i2c_write_id = 0xA2,

		//.xtalk_support = TRUE,
		//.xtalk_size = 2048,
		//.addr_xtalk = 0x150F,
	},
};
#define USE_PDAF 1
#if USE_PDAF
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
		.i4FullRawW = 4080,
		.i4FullRawH = 3060,
		.i4VCPackNum = 1,
		.PDAF_Support = PDAF_SUPPORT_CAMSV_QPD,
		.i4ModeIndex = 3,
		.sPDMapInfo[0] = {
			.i4PDPattern = 1,
			.i4BinFacX = 2,
			.i4BinFacY = 4,
			.i4PDRepetition = 0,
			.i4PDOrder = {1},
		},
		.i4Crop = {
			{0, 0}, {0, 0}, {0, 378}, {0, 0}, {0, 378},
			{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
			{0, 378}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
			{0, 0}, {0, 0}
		},
		.iMirrorFlip = IMAGE_HV_MIRROR,
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_ISZ_2x = {
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
		.i4FullRawW = 8160,
		.i4FullRawH = 6144,
		.i4VCPackNum = 1,
		.PDAF_Support = PDAF_SUPPORT_CAMSV_QPD,
		.i4ModeIndex = 3,
		.sPDMapInfo[0] = {
			.i4PDPattern = 1,
			.i4BinFacX = 2,
			.i4BinFacY = 8,
			.i4PDRepetition = 0,
			.i4PDOrder = {0},
		},
		.i4Crop = {
			{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
			{0, 0}, {0, 0}, {2040, 1542}, {0, 0}, {0, 0},
			{0, 0}, {0, 0}, {0, 0}, {2040, 1920}, {0, 0},
			{0, 0}, {0, 0}
		},
		.iMirrorFlip = IMAGE_HV_MIRROR,
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_ISZ_4x = {
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
		.i4FullRawW = 16320,
		.i4FullRawH = 12288,
		.i4VCPackNum = 1,
		.PDAF_Support = PDAF_SUPPORT_CAMSV_QPD,
		.i4ModeIndex = 3,
		.sPDMapInfo[0] = {
			.i4PDPattern = 1,
			.i4BinFacX = 4,
			.i4BinFacY = 16,
			.i4PDRepetition = 0,
			.i4PDOrder = {0},
		},
		.i4Crop = {
			{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
			{0, 0}, {0, 0}, {0, 0}, {6120, 4614}, {0, 0},
			{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
			{6120, 4614}, {6120, 4992}
		},
		.iMirrorFlip = IMAGE_HV_MIRROR,
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_hs_video = {
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
		.i4FullRawW = 2040,
		.i4FullRawH = 1530,
		.i4VCPackNum = 1,
		.PDAF_Support = PDAF_SUPPORT_CAMSV_QPD,
		.i4ModeIndex = 3,
		.sPDMapInfo[0] = {
			.i4PDPattern = 1,
			.i4BinFacX = 2,
			.i4BinFacY = 4,
			.i4PDRepetition = 0,
			.i4PDOrder = {0},
		},
		.i4Crop = {
			{0, 0}, {0, 0}, {0, 0}, {60, 225}, {0, 0},
			{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
			{0, 0}, {0, 0}, {0, 0}, {0, 0}, {60, 225}
		},
		.iMirrorFlip = IMAGE_HV_MIRROR,
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_idcg = {
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
	.i4FullRawW = 4080,
	.i4FullRawH = 3060,
	.i4VCPackNum = 1,
	.PDAF_Support = PDAF_SUPPORT_CAMSV_QPD,
	.i4ModeIndex = 3,
	.sPDMapInfo[0] = {
		.i4PDPattern = 1,
		.i4BinFacX = 2,
		.i4BinFacY = 8,
		.i4PDRepetition = 0,
		.i4PDOrder = {1},
	},
	.i4Crop = {
		{0, 0}, {0, 0}, {0, 378}, {0, 0}, {0, 378},
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		{0, 378}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		{0, 0}, {0, 0}
	},
	.iMirrorFlip = IMAGE_HV_MIRROR,
};

#endif

static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0ff0,
			.vsize = 0x0bf4,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0xff0,
			.vsize = 0x2fc,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};

static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0ff0,
			.vsize = 0x0bf4,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 0x0ff0,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0xff0,
			.vsize = 0x240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};

static struct mtk_mbus_frame_desc_entry frame_desc_hs_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0780,
			.vsize = 0x0438,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x780,
			.vsize = 0x10e,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};

static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0ff0,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0xff0,
			.vsize = 0x240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0ff0,
			.vsize = 0x0bf4,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0xff0,
			.vsize = 0x2fc,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 0x0ff0,
			.vsize = 0x0bf4,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0xff0,
			.vsize = 0x2fc,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 0x0ff0,
			.vsize = 0x0bf4,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0ff0,
			.vsize = 0x17e,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 0x0ff0,
			.vsize = 0x0bf4,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x7f0,
			.vsize = 0xbe,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus5[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 0x0ff0,
			.vsize = 0x0bf4,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0xff0,
			.vsize = 0x17e,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus6[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 0x0ff0,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0xff0,
			.vsize = 0x120,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus7[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x3fc0,
			.vsize = 0x2fd0,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus8 [] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1fe0,
			.vsize = 0x17e8,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus9[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 0x0ff0,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0xff0,
			.vsize = 0x120,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus10[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0780,
			.vsize = 0x0438,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x780,
			.vsize = 0x10e,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus12[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 0x0ff0,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
#if USE_PDAF
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x7f0,
			.vsize = 0x90,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
#endif
};

//1000 base for dcg gain ratio
static u32 lapiss5khpewide_dcg_ratio_table_cus5[] = {16000};

static u32 lapiss5khpewide_dcg_ratio_table_cus6[] = {16000};

static u32 lapiss5khpewide_dcg_ratio_table_cus9[] = {16000};

static u32 lapiss5khpewide_dcg_ratio_table_cus12[] = {16000};

static struct mtk_sensor_saturation_info imgsensor_saturation_info = {
	.gain_ratio = 1000,
	.OB_pedestal = 64,
	.saturation_level = 1023,
	.adc_bit = 10,
};

static struct mtk_sensor_saturation_info imgsensor_saturation_info_video = {
	.gain_ratio = 1000,
	.OB_pedestal = 1024,
	.saturation_level = 16384,
	.adc_bit = 14,
};

static struct mtk_sensor_saturation_info imgsensor_saturation_info_cus2 = {
	.gain_ratio = 1000,
	.OB_pedestal = 1024,
	.saturation_level = 16384,
	.adc_bit = 14,
};

static struct mtk_sensor_saturation_info imgsensor_saturation_info_cus3 = {
	.gain_ratio = 1000,
	.OB_pedestal = 1024,
	.saturation_level = 16384,
	.adc_bit = 14,
};

static struct mtk_sensor_saturation_info imgsensor_saturation_info_cus4 = {
	.gain_ratio = 1000,
	.OB_pedestal = 1024,
	.saturation_level = 16384,
	.adc_bit = 14,
};

static struct mtk_sensor_saturation_info imgsensor_saturation_info_cus5 = {
	.gain_ratio = 16000,
	.OB_pedestal = 1024,
	.saturation_level = 16368,
	.adc_bit = 10,
	.ob_bm = 64,
};

static struct mtk_sensor_saturation_info imgsensor_saturation_info_cus6 = {
	.gain_ratio = 16000,
	.OB_pedestal = 1024,
	.saturation_level = 16368,
	.adc_bit = 10,
	.ob_bm = 64,
};

static struct mtk_sensor_saturation_info imgsensor_saturation_info_cus9 = {
	.gain_ratio = 16000,
	.OB_pedestal = 1024,
	.saturation_level = 16368,
	.adc_bit = 10,
	.ob_bm = 64,
};

static struct mtk_sensor_saturation_info imgsensor_saturation_info_cus12 = {
	.gain_ratio = 16000,
	.OB_pedestal = 1024,
	.saturation_level = 16368,
	.adc_bit = 10,
	.ob_bm = 64,
};

static struct subdrv_mode_struct mode_struct[] = {
	{//preview    0
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = addr_data_pair_preview,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_preview),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_FULL_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_preview,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_preview),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1760000000,
		.linelength = 7296,
		.framelength = 8040,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 34,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 256,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 256,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 16320,
			.h0_size = 12288,
			.scale_w = 4080,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 6,
			.w1_size = 4080,
			.h1_size = 3060,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 3060,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{//capture		1
		.frame_desc = frame_desc_cap,
		.num_entries = ARRAY_SIZE(frame_desc_cap),
		.mode_setting_table = addr_data_pair_preview,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_preview),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1760000000,
		.linelength = 7296,
		.framelength = 8040,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 34,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 256,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 256,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 16320,
			.h0_size = 12288,
			.scale_w = 4080,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 6,
			.w1_size = 4080,
			.h1_size = 3060,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 3060,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{//normal video		2
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = addr_data_pair_normal_video,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_normal_video),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_14BIT_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_video,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_video),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1760000000,
		.linelength = 9312,
		.framelength = 6300,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 34,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 256,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 256,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 16320,
			.h0_size = 12288,
			.scale_w = 4080,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 384,
			.w1_size = 4080,
			.h1_size = 2304,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 2304,
		},
		.pdaf_cap = TRUE,
#if USE_PDAF
		.imgsensor_pd_info = &imgsensor_pd_info,
#else
		.imgsensor_pd_info = PARAM_UNDEFINED,
#endif
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_Gb,
		.saturation_info = &imgsensor_saturation_info_video,
	},
	{//hs video		3
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
		.pclk = 1760000000,
		.linelength = 5720,
		.framelength = 1280,
		.max_framerate = 2400,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 14,
		.coarse_integ_step = 2,
		.min_exposure_line = 16,
		.ana_gain_max = BASEGAIN * 256,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 256,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 16320,
			.h0_size = 12288,
			.scale_w = 2040,
			.scale_h = 1536,
			.x1_offset = 60,
			.y1_offset = 228,
			.w1_size = 1920,
			.h1_size = 1080,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1920,
			.h2_tg_size = 1080,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_hs_video,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{//slim video		4
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
		.pclk = 1760000000,
		.linelength = 7296,
		.framelength = 4020,
		.max_framerate = 600,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 34,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 256,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 256,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 16320,
			.h0_size = 12288,
			.scale_w = 4080,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 384,
			.w1_size = 4080,
			.h1_size = 2304,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 2304,
		},
		.pdaf_cap = TRUE,
#if USE_PDAF
		.imgsensor_pd_info = &imgsensor_pd_info,
#else
		.imgsensor_pd_info = PARAM_UNDEFINED,
#endif
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{//custom1		5
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = addr_data_pair_custom1,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom1),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1760000000,
		.linelength = 7296,
		.framelength = 8040,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 34,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 256,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 256,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 16320,
			.h0_size = 12288,
			.scale_w = 4080,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 6,
			.w1_size = 4080,
			.h1_size = 3060,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 3060,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{//custom2 LN2		6
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = addr_data_pair_custom2,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom2),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_14BIT_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_custom2,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_custom2),
		.hdr_mode = PARAM_UNDEFINED,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1760000000,
		.linelength = 9152,
		.framelength = 6320,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 34,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 256,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 256,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 16320,
			.h0_size = 12288,
			.scale_w = 4080,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 6,
			.w1_size = 4080,
			.h1_size = 3060,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 3060,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_Gb,
		.saturation_info = &imgsensor_saturation_info_cus2,
	},
	{//custom3 2X		7
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = addr_data_pair_custom3,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom3),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_14BIT_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_custom3,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_custom3),
		.hdr_mode = PARAM_UNDEFINED,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1760000000,
		.linelength = 14336,
		.framelength = 4092,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 55,
		.coarse_integ_step = 1,
		.min_exposure_line = 6,
		.ana_gain_max = BASEGAIN * 64,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 4080,
			.y0_offset = 3080,
			.w0_size = 8160,
			.h0_size = 6128,
			.scale_w = 4080,
			.scale_h = 3064,
			.x1_offset = 0,
			.y1_offset = 2,
			.w1_size = 4080,
			.h1_size = 3060,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 3060,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_ISZ_2x,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_Gb,
		.saturation_info = &imgsensor_saturation_info_cus3,
	},
	{//custom4 4X		8
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = addr_data_pair_custom4,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom4),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_14BIT_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_custom4,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_custom4),
		.raw_cnt = 1,
		.exp_cnt = 1,
		.hdr_mode = HDR_NONE,
		.pclk = 1760000000,
		.linelength = 13776,
		.framelength = 4258,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 164,
		.coarse_integ_step = 1,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 16,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 6120,
			.y0_offset = 4608,
			.w0_size = 4080,
			.h0_size = 3072,
			.scale_w = 4080,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 6,
			.w1_size = 4080,
			.h1_size = 3060,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 3060,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_ISZ_4x,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_Gb,
		.saturation_info = &imgsensor_saturation_info_cus4,
	},
	{//custom5 capture iDCG + DSG		9
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = addr_data_pair_custom5,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom5),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_14BIT_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_custom5,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_custom5),
		.raw_cnt = 1,
		.exp_cnt = 1,
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.pclk = 1760000000,
		.linelength = 15552,
		.framelength = 3772,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 34,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 16 * 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 2,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 16320,
			.h0_size = 12288,
			.scale_w = 4080,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 6,
			.w1_size = 4080,
			.h1_size = 3060,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 3060,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_idcg,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_Gb,
		.saturation_info = &imgsensor_saturation_info_cus5,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_base = IMGSENSOR_DCG_GAIN_LCG_BASE,
			.dcg_gain_ratio_min = 16000,
			.dcg_gain_ratio_max = 16000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = lapiss5khpewide_dcg_ratio_table_cus5,
			.dcg_gain_table_size = sizeof(lapiss5khpewide_dcg_ratio_table_cus5),
		},
	},
	{//custom6 video iDCG + DSG		10
		.frame_desc = frame_desc_cus6,
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.mode_setting_table = addr_data_pair_custom6,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom6),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_14BIT_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_custom6,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_custom6),
		.raw_cnt = 1,
		.exp_cnt = 1,
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.pclk = 1760000000,
		.linelength = 15552,
		.framelength = 3772,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 34,
		.coarse_integ_step = 2,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 16 * 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 2,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 16320,
			.h0_size = 12288,
			.scale_w = 4080,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 384,
			.w1_size = 4080,
			.h1_size = 2304,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 2304,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_idcg,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_Gb,
		.saturation_info = &imgsensor_saturation_info_cus6,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_base = IMGSENSOR_DCG_GAIN_LCG_BASE,
			.dcg_gain_ratio_min = 16000,
			.dcg_gain_ratio_max = 16000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = lapiss5khpewide_dcg_ratio_table_cus6,
			.dcg_gain_table_size = sizeof(lapiss5khpewide_dcg_ratio_table_cus6),
		},
	},
	{//custom7 200m		11
		.frame_desc = frame_desc_cus7,
		.num_entries = ARRAY_SIZE(frame_desc_cus7),
		.mode_setting_table = addr_data_pair_custom7,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom7),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_FULL_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_custom7,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_custom7),
		.raw_cnt = 1,
		.exp_cnt = 1,
		.hdr_mode = HDR_NONE,
		.pclk = 1760000000,
		.linelength = 19904,
		.framelength = 17684,
		.max_framerate = 50,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 164,
		.coarse_integ_step = 1,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 16,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 24,
			.w0_size = 16320,
			.h0_size = 12240,
			.scale_w = 16320,
			.scale_h = 12240,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 16320,
			.h1_size = 12240,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 16320,
			.h2_tg_size = 12240,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
		//.sensor_output_dataformat_cell_type = SENSOR_OUTPUT_FORMAT_CELL_4X4,
	},
	{//custom 8 50M		12
		.frame_desc = frame_desc_cus8,
		.num_entries = ARRAY_SIZE(frame_desc_cus8),
		.mode_setting_table = addr_data_pair_custom8,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom8),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_FULL_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_custom8,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_custom8),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.cms_enabled = TRUE,
		.pclk = 1760000000,
		.linelength = 14256,
		.framelength = 12344,
		.max_framerate = 100,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 55,
		.coarse_integ_step = 1,
		.min_exposure_line = 6,
		.ana_gain_max = BASEGAIN * 64,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 24,
			.w0_size = 16320,
			.h0_size = 12240,
			.scale_w = 8160,
			.scale_h = 6120,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 8160,
			.h1_size = 6120,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 8160,
			.h2_tg_size = 6120,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{//custom9 video isz 2x dsg		13
		.frame_desc = frame_desc_cus9,
		.num_entries = ARRAY_SIZE(frame_desc_cus9),
		.mode_setting_table = addr_data_pair_custom9,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom9),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_14BIT_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_custom9,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_custom9),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1760000000,
		.linelength = 17600,
		.framelength = 3332,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 55,
		.coarse_integ_step = 1,
		.min_exposure_line = 6,
		.ana_gain_max = BASEGAIN * 16 * 16,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 64,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 4080,
			.y0_offset = 3072,
			.w0_size = 8160,
			.h0_size = 6144,
			.scale_w = 4080,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 384,
			.w1_size = 4080,
			.h1_size = 2304,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 2304,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_ISZ_2x,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_Gb,
		.saturation_info = &imgsensor_saturation_info_cus9,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_base = IMGSENSOR_DCG_GAIN_LCG_BASE,
			.dcg_gain_ratio_min = 16000,
			.dcg_gain_ratio_max = 16000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = lapiss5khpewide_dcg_ratio_table_cus9,
			.dcg_gain_table_size = sizeof(lapiss5khpewide_dcg_ratio_table_cus9),
		},
	},
	{//custom10 1080p@120fps		14
		.frame_desc = frame_desc_cus10,
		.num_entries = ARRAY_SIZE(frame_desc_cus10),
		.mode_setting_table = addr_data_pair_custom10,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom10),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1760000000,
		.linelength = 5720,
		.framelength = 2562,
		.max_framerate = 1200,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 14,
		.coarse_integ_step = 2,
		.min_exposure_line = 16,
		.ana_gain_max = BASEGAIN * 256,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 256,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 16320,
			.h0_size = 12288,
			.scale_w = 2040,
			.scale_h = 1536,
			.x1_offset = 60,
			.y1_offset = 228,
			.w1_size = 1920,
			.h1_size = 1080,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1920,
			.h2_tg_size = 1080,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_hs_video,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
	},
	{//custom11 4X E2E		15
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = addr_data_pair_custom11,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom11),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_14BIT_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_custom11,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_custom11),
		.raw_cnt = 1,
		.exp_cnt = 1,
		.hdr_mode = HDR_NONE,
		.pclk = 1760000000,
		.linelength = 13776,
		.framelength = 4258,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 164,
		.coarse_integ_step = 1,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 16,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 6120,
			.y0_offset = 4608,
			.w0_size = 4080,
			.h0_size = 3072,
			.scale_w = 4080,
			.scale_h = 3072,
			.x1_offset = 0,
			.y1_offset = 6,
			.w1_size = 4080,
			.h1_size = 3060,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 3060,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_ISZ_4x,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_4CELL_R,
		.saturation_info = &imgsensor_saturation_info_cus4,
		.sensor_output_dataformat_cell_type = SENSOR_OUTPUT_FORMAT_CELL_2X2,
	},
	{//custom12 video 4x dsg		16
		.frame_desc = frame_desc_cus12,
		.num_entries = ARRAY_SIZE(frame_desc_cus12),
		.mode_setting_table = addr_data_pair_custom12,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_custom12),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_14BIT_MODE,
		.seamless_switch_mode_setting_table = lapiss5khpewide_seamless_custom12,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(lapiss5khpewide_seamless_custom12),
		.raw_cnt = 1,
		.exp_cnt = 1,
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.pclk = 1760000000,
		.linelength = 17600,
		.framelength = 3333,
		.max_framerate = 300,
		.mipi_pixel_rate = 1456992000,
		.readout_length = 0,
		.read_margin = 164,
		.coarse_integ_step = 1,
		.min_exposure_line = 8,
		.ana_gain_max = BASEGAIN * 4 * 16,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN * 1,
		.imgsensor_winsize_info = {
			.full_w = 16320,
			.full_h = 12288,
			.x0_offset = 6120,
			.y0_offset = 4992,
			.w0_size = 4080,
			.h0_size = 2304,
			.scale_w = 4080,
			.scale_h = 2304,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 4080,
			.h1_size = 2304,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4080,
			.h2_tg_size = 2304,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_ISZ_4x,
		.ae_binning_ratio = 1200,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.csi_param = {0},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_Gb,
		.saturation_info = &imgsensor_saturation_info_cus12,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_base = IMGSENSOR_DCG_GAIN_LCG_BASE,
			.dcg_gain_ratio_min = 16000,
			.dcg_gain_ratio_max = 16000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = lapiss5khpewide_dcg_ratio_table_cus12,
			.dcg_gain_table_size = sizeof(lapiss5khpewide_dcg_ratio_table_cus12),
		},
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = LAPISS5KHPEWIDE_SENSOR_ID,
	.reg_addr_sensor_id = {0x0000, 0x0001},
	.i2c_addr_table = {0x20, 0xFF},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_16,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {16320, 12288},
	.mirror = IMAGE_HV_MIRROR,

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_CPHY,
	.mipi_lane_num = SENSOR_MIPI_3_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_Gb,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 256,
	.ana_gain_type = 2,
	.ana_gain_step = 1,
	.ana_gain_table = lapiss5khpewide_ana_gain_table,
	.ana_gain_table_size = sizeof(lapiss5khpewide_ana_gain_table),
	.min_gain_iso = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 16,
	.exposure_max = 0xFFFF * 128 - 3,
	.exposure_step = 2,
	.exposure_margin = 34,
	.mi_long_exposure_type = 1,
	.dig_gain_min = BASE_DGAIN * 1,
	.dig_gain_max = BASE_DGAIN * 16,
	.dig_gain_step = 4,
	.saturation_info = &imgsensor_saturation_info,

	.frame_length_max = 0xFFFF,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 1820000,

	.pdaf_type = PDAF_SUPPORT_CAMSV_QPD,
	.hdr_type = HDR_SUPPORT_DCG,
	.seamless_switch_support = TRUE,
	.temperature_support = TRUE,
	.g_temp = lapiss5khpewide_get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = 0x0101,
	.reg_addr_exposure = {{0x0202, 0x0203},},
	.long_exposure_support = TRUE,
	.reg_addr_exposure_lshift = 0x0704,
	.reg_addr_framelength_lshift = 0x0702,
	.reg_addr_ana_gain = {{0x0204, 0x0205},},
	.reg_addr_dig_gain = {{0x020e, 0x020f},},
	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_temp_en = PARAM_UNDEFINED,
	.reg_addr_temp_read = 0x0020,
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
	.s_mi_stream = mi_stream,
	.s_mi_init_setting = lapiss5khpewide_s_mi_init_setting,
	.s_mi_mode_setting = lapiss5khpewide_s_mi_mode_setting,
	.mi_enable_async = 1,

	.checksum_value = 0x47a75476,
};

static struct subdrv_ops ops = {
	.get_vendr_id = common_get_vendor_id,
	.get_id = lapiss5khpewide_get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = common_open,
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = common_control,
	.feature_control = common_feature_control,
	.close = common_close,
	.get_frame_desc = common_get_frame_desc,
	.get_csi_param = common_get_csi_param,
	.vsync_notify = vsync_notify,
	.update_sof_cnt = common_update_sof_cnt,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_RST, 0, 1},
	{HW_ID_DOVDD, 1800000, 1},
	{HW_ID_DVDD, 950000, 1},
	{HW_ID_AVDD, 2200000, 1},
	{HW_ID_RST, 1, 1},
	{HW_ID_MCLK, 24, 1},
	{HW_ID_MCLK_DRIVING_CURRENT, 6, 20},
	{HW_ID_AFVDD, 2800000, 1},
};
const struct subdrv_entry lapiss5khpewide_mipi_raw_entry = {
	.name = "lapiss5khpewide_mipi_raw",
	.id = LAPISS5KHPEWIDE_SENSOR_ID,
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

static int lapiss5khpewide_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	if (mode)
		subdrv_i2c_wr_u16(ctx, 0x0600, 0x0001); /*100% Color bar*/
	else if (ctx->test_pattern)
		subdrv_i2c_wr_u16(ctx, 0x0600, 0x0000); /*No pattern*/

	ctx->test_pattern = mode;

	return 0;
}

static int lapiss5khpewide_set_test_pattern_data(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	struct mtk_test_pattern_data *data = (struct mtk_test_pattern_data *)para;
	u16 R = (data->Channel_R >> 22) & 0x3ff;
	u16 Gr = (data->Channel_Gr >> 22) & 0x3ff;
	u16 Gb = (data->Channel_Gb >> 22) & 0x3ff;
	u16 B = (data->Channel_B >> 22) & 0x3ff;

	subdrv_i2c_wr_u16(ctx, 0x0602, Gr);
	subdrv_i2c_wr_u16(ctx, 0x0604, R);
	subdrv_i2c_wr_u16(ctx, 0x0606, B);
	subdrv_i2c_wr_u16(ctx, 0x0608, Gb);

	DRV_LOG(ctx, "mode(%u) R/Gr/Gb/B = 0x%04x/0x%04x/0x%04x/0x%04x\n",
		ctx->test_pattern, R, Gr, Gb, B);

	return 0;
}

struct SET_SENSOR_AWB_GAIN g_s5khpe_last_awb_gain = {0, 0, 0, 0};
static int lapiss5khpewide_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	struct SET_SENSOR_AWB_GAIN *awb_gain = (struct SET_SENSOR_AWB_GAIN *)para;
	g_s5khpe_last_awb_gain = *awb_gain;

	// set awb gain
	switch(ctx->current_scenario_id){
		case SENSOR_SCENARIO_ID_CUSTOM3://isz 2x
		case SENSOR_SCENARIO_ID_CUSTOM4://isz 4x
		case SENSOR_SCENARIO_ID_CUSTOM7://200m
		case SENSOR_SCENARIO_ID_CUSTOM8://50m
		case SENSOR_SCENARIO_ID_CUSTOM9://video isz 2x
		case SENSOR_SCENARIO_ID_CUSTOM12://video isz 4x
			subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);
			subdrv_i2c_wr_u16(ctx, AWB_RED_GAIN_ADDR, awb_gain->ABS_GAIN_R*2);
			subdrv_i2c_wr_u16(ctx, AWB_GREEN_GAIN_ADDR, awb_gain->ABS_GAIN_GR*2);
			subdrv_i2c_wr_u16(ctx, AWB_BLUE_GAIN_ADDR, awb_gain->ABS_GAIN_B*2);
			DRV_LOG(ctx, "%s awb_r:0x%x awb_gr:0x%x awb_b:0x%x\n", __func__, awb_gain->ABS_GAIN_R*2, awb_gain->ABS_GAIN_GR*2, awb_gain->ABS_GAIN_B*2);
			break;
		default:
			DRV_LOG(ctx, "%s current mode %d don't need awb gain\n", __func__, ctx->current_scenario_id);
			break;
	}

	return 0;
}

static int lapiss5khpewide_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	enum SENSOR_SCENARIO_ID_ENUM scenario_id;
	struct mtk_hdr_ae *ae_ctrl = NULL;
	u64 *feature_data = (u64 *)para;
	u32 frame_length_in_lut[IMGSENSOR_STAGGER_EXPOSURE_CNT] = {0};
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

	subdrv_i2c_wr_u8(ctx, 0x0104, 0x01);

	update_mode_info(ctx, scenario_id);

	lapiss5khpewide_set_awb_gain(ctx, (u8 *)&g_s5khpe_last_awb_gain, len);

	subdrv_i2c_wr_regs_u16_burst(ctx,
		ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table,
		ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_len);

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
		case HDR_RAW_DCG_COMPOSE:
			set_shutter(ctx, ae_ctrl->exposure.le_exposure);
			if (ctx->s_ctx.mode[scenario_id].dcg_info.dcg_gain_mode
				== IMGSENSOR_DCG_DIRECT_MODE)
				set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			else
				set_gain(ctx, ae_ctrl->gain.me_gain);  //使用me_gain避免使用le_gain导致的切过去过曝
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

static void mi_stream(void *arg,bool enable)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	DRV_LOG_MUST(ctx, "Enter %s enable = %d \n", __FUNCTION__,enable);
	if(enable){
		subdrv_i2c_wr_u8(ctx,0x0100, 0x01);
		mdelay(20);
	}else{
		subdrv_i2c_wr_u8(ctx,0x0100, 0x00);
	}
}

static int lapiss5khpewide_set_curr_lens_data(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	unsigned short *data = (unsigned short*)para;
	unsigned short lp = *data;
	if(ctx->is_streaming == 1) 
		if (ctx->current_scenario_id == SENSOR_SCENARIO_ID_CUSTOM4  ||
			ctx->current_scenario_id == SENSOR_SCENARIO_ID_CUSTOM11 ||
			ctx->current_scenario_id == SENSOR_SCENARIO_ID_CUSTOM12 ){
			subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x2401);
			subdrv_i2c_wr_u16(ctx, 0x2E4C, lp);
			DRV_LOG(ctx, "%s lp:0x%x\n", __func__, lp);
			subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);
	}
	return 0;
}

static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt)
{
	DRV_LOG(ctx, "sof_cnt(%u) ctx->ref_sof_cnt(%u) ctx->fast_mode_on(%d)",
		sof_cnt, ctx->ref_sof_cnt, ctx->fast_mode_on);
	if (ctx->fast_mode_on && (sof_cnt > ctx->ref_sof_cnt)) {
		ctx->fast_mode_on = FALSE;
		ctx->ref_sof_cnt = 0;
		DRV_LOG(ctx, "seamless_switch finished.");
	}
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

static void lapiss5khpewide_s_mi_init_setting(void* arg)
{
	u16 is_otp_written = 0x0000;
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);
	is_otp_written = subdrv_i2c_rd_u16(ctx, 0x0016);
	pr_err("is_otp_written:0x%x\n", is_otp_written);

	DRV_LOG(ctx, "E\n");
	if (is_otp_written == 0x0001){
		i2c_table_write(ctx, sensor_init_addr_data_otp_written, sizeof(sensor_init_addr_data_otp_written)/sizeof(u16));
		mdelay(10);
		subdrv_i2c_wr_regs_u16_burst(ctx, sensor_init_addr_data2_otp_written, sizeof(sensor_init_addr_data2_otp_written)/sizeof(u16));
		subdrv_i2c_wr_regs_u16_burst(ctx, addr_data_fmc_mode_otp_written, sizeof(addr_data_fmc_mode_otp_written)/sizeof(u16));
	} else {
		i2c_table_write(ctx, sensor_init_addr_data_non_otp, sizeof(sensor_init_addr_data_non_otp)/sizeof(u16));
		mdelay(10);
		subdrv_i2c_wr_regs_u16_burst(ctx, sensor_init_addr_data2_non_otp, sizeof(sensor_init_addr_data2_non_otp)/sizeof(u16));
		subdrv_i2c_wr_regs_u16_burst(ctx, addr_data_fmc_mode_non_otp, sizeof(addr_data_fmc_mode_non_otp)/sizeof(u16));
	}
	DRV_LOG(ctx, "X\n");

	return ;
}

static void lapiss5khpewide_s_mi_mode_setting(void *arg, enum SENSOR_SCENARIO_ID_ENUM scenario_id){
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	DRV_LOG_MUST(ctx, "%s current mode %d\n", __func__, ctx->current_scenario_id);
	subdrv_i2c_wr_regs_u16_burst(ctx, ctx->s_ctx.mode[scenario_id].mode_setting_table,
		ctx->s_ctx.mode[scenario_id].mode_setting_len);
	return ;
}

static int lapiss5khpewide_get_sensor_temperature(void *arg){
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	s8 temperature = 0;
	u16 tmc = 0;

	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);
	temperature = subdrv_i2c_rd_u8(ctx, ctx->s_ctx.reg_addr_temp_read);
	tmc = subdrv_i2c_rd_u16(ctx, 0x0010);

	DRV_LOG_MUST(ctx, "temperature: %d degrees\n", temperature);
	DRV_LOG(ctx, "tmc: 0x%x\n", tmc);

	if(tmc == 0x06 || tmc == 0x2e || tmc == 0x1e){
		DRV_LOG_MUST(ctx, "current sensor tmc is 0x%x, not support gettemp, set temp 0!\n", tmc);
		temperature = 0;
	}
	return temperature;
}

static int lapiss5khpewide_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id)
{
	u8 i = 0;
	u8 retry = 2;
	u32 addr_h = ctx->s_ctx.reg_addr_sensor_id.addr[0];
	u32 addr_l = ctx->s_ctx.reg_addr_sensor_id.addr[1];
	u32 addr_ll = ctx->s_ctx.reg_addr_sensor_id.addr[2];


	while (ctx->s_ctx.i2c_addr_table[i] != 0xFF) {
		ctx->i2c_write_id = ctx->s_ctx.i2c_addr_table[i];
		do {
			*sensor_id = (subdrv_i2c_rd_u8(ctx, addr_h) << 8) |
				subdrv_i2c_rd_u8(ctx, addr_l);
			if (addr_ll)
				*sensor_id = ((*sensor_id) << 8) | subdrv_i2c_rd_u8(ctx, addr_ll);
			DRV_LOG_MUST(ctx, "i2c_write_id:0x%x sensor_id(cur/exp):0x%x/0x%x\n",
				ctx->i2c_write_id, *sensor_id, ctx->s_ctx.sensor_id);
			if (ctx->s_ctx.s_mi_read_CGRatio) {
					ctx->s_ctx.s_mi_read_CGRatio((void *)ctx);
			}
			if (*sensor_id == ctx->s_ctx.sensor_id && !otp_readed){
				s5khpe_read_swxtc_data(ctx);
				s5khpe_read_sensor_otp_data(ctx);
				otp_readed = 1;
			}
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

static void s5khpe_read_swxtc_data(struct subdrv_ctx *ctx)
{
	u16 addr;
	int ret;
	addr = ctx->i2c_client->addr;
	ctx->i2c_client->addr = S5KHPE_EEPROM_I2C_ADDR;

	ret = adaptor_i2c_rd_p8(ctx->i2c_client, S5KHPE_EEPROM_I2C_ADDR >> 1, S5KHPE_SWXTC_DATA_ADDR_STAST, s5khpe_swxtc_data, S5KHPE_SWXTC_DATA_LEN);

	ctx->i2c_client->addr = addr;
}

#define S5KHPE_PAGE_HEAD_ADDR 0x0A04
#define S5KHPE_PAGE_LENGTH 64
#define S5KHPE_PAGE_TAIL_ADDR (S5KHPE_PAGE_HEAD_ADDR + S5KHPE_PAGE_LENGTH - 1)
static void read_sensor_otp_page(struct subdrv_ctx *ctx, u16 start_page, u16 start_idx, u16 len, unsigned char* buf)
{
	u16 page_index = 0, last_page = start_page;
	u16 idx = start_idx;
	u16 value_size = 0;
	u16 temp_data = 0;

	while(value_size < len){
		if(page_index != last_page){
			page_index = last_page;
			//set page
			subdrv_i2c_wr_u16(ctx, 0x0A02, page_index);
			//read start
			subdrv_i2c_wr_u16(ctx, 0x0A00 ,0x0100);
			mdelay(1);
		}
		temp_data = subdrv_i2c_rd_u16(ctx, idx);
		buf[value_size] = temp_data >> 8;
		value_size++;
		buf[value_size] = temp_data & 0xFF;
		value_size++;
		idx += 2;
		//pr_err("%s, page:%d, idx:0x%04X, value:0x%04x\n", __func__, page_index, idx, temp_data);
		if(idx >= S5KHPE_PAGE_TAIL_ADDR){
			idx = S5KHPE_PAGE_HEAD_ADDR;
			last_page++;
		}
	}
}

static void s5khpe_read_sensor_otp_data(struct subdrv_ctx *ctx)
{
	u8 sof_time = 20;
	u8 sof_check = 0;

	DRV_LOG_MUST(ctx, "E");

	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);
	subdrv_i2c_wr_u16(ctx, 0x0000, 0x0008);
	subdrv_i2c_wr_u16(ctx, 0x0000, 0x1B7E);
	subdrv_i2c_wr_u16(ctx, 0x6018, 0x0001);
	subdrv_i2c_wr_u16(ctx, 0x7002, 0x0008);
	subdrv_i2c_wr_u16(ctx, 0x7004, 0x1770);
	subdrv_i2c_wr_u16(ctx, 0x7028, 0x0CDC);
	subdrv_i2c_wr_u16(ctx, 0x6014, 0x0001);

	mdelay(10);
	subdrv_i2c_wr_u16(ctx, 0x0A02, 0x07F4);
	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x2400);
	subdrv_i2c_wr_u16(ctx, 0xA9B4, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x2401);
	subdrv_i2c_wr_u16(ctx, 0x60F0, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0x70A0, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);
	subdrv_i2c_wr_u16(ctx, 0x0100, 0x0100);

	while(sof_time > 0){
		sof_check = subdrv_i2c_rd_u8(ctx, 0x0005);
		if(sof_check >= 0x01 && sof_check <= 0xFE)
			break;
		sof_time--;
		mdelay(1);
	}
	if(sof_time == 0){
		pr_err("S5KHPE SENSOR OTP read fail, sof_check:0x%x\n", sof_check);
		subdrv_i2c_wr_u16(ctx, 0x0A00, 0x0000);
		subdrv_i2c_wr_u16(ctx, 0x0100 ,0x0000);
		return;
	}

	read_sensor_otp_page(ctx, 15, 0x0A04, 39168, s5khpe_sensor_otp_data);

	subdrv_i2c_wr_u16(ctx, 0x0A00, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0x0100 ,0x0000);

	DRV_LOG_MUST(ctx, "X");

}

static int s5khpe_get_feature_get_4cell_data(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *)para;
	u16 type = (u16)(*feature_data);
	char *data = (char *)(uintptr_t)(*(feature_data + 1));
	if (type == FOUR_CELL_CAL_TYPE_XTALK_CAL) {
			data[0] = S5KHPE_SWXTC_DATA_LEN & 0xFF;
			data[1] = (S5KHPE_SWXTC_DATA_LEN >> 8) & 0xFF;
			memcpy(data + 2, s5khpe_swxtc_data, S5KHPE_SWXTC_DATA_LEN);
			pr_err("Read FOUR_CELL_CAL_TYPE_XTALK_CAL = %02x %02x %02x %02x %02x %02x\n",
				(UINT16)data[0], (UINT16)data[1],
				(UINT16)data[2], (UINT16)data[3],
				(UINT16)data[4], (UINT16)data[5]);
	}else if (type == FOUR_CELL_CAL_TYPE_DPC){
		data[0] = SENSOR_OTP_DATA_LEN & 0xFF;
		data[1] = (SENSOR_OTP_DATA_LEN >> 8) & 0xFF;
		memcpy(data + 2, s5khpe_sensor_otp_data, SENSOR_OTP_DATA_LEN);
		pr_err("Read FOUR_CELL_CAL_TYPE_DPC = %02x %02x %02x %02x %02x %02x\n",
			(UINT16)data[0], (UINT16)data[1],
			(UINT16)data[2], (UINT16)data[3],
			(UINT16)data[4], (UINT16)data[5]);

	}

	return ERROR_NONE;
}