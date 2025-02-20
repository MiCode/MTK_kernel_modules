// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 imx989mipiraw_Sensor.c
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
#include "imx989mipiraw_Sensor.h"

static void set_sensor_cali(void *arg);
static int get_sensor_temperature(void *arg);
static void set_group_hold(void *arg, u8 en);
static u16 get_gain2reg(u32 gain);
static int imx989_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int imx989_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int imx989_set_test_pattern_data(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int imx989_cphy_lrte_mode(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, imx989_set_test_pattern},
	{SENSOR_FEATURE_SET_TEST_PATTERN_DATA, imx989_set_test_pattern_data},
	{SENSOR_FEATURE_SEAMLESS_SWITCH, imx989_seamless_switch},
	{SENSOR_FEATURE_SET_CPHY_LRTE_MODE, imx989_cphy_lrte_mode},
};

static struct eeprom_info_struct eeprom_info[] = {
	{
		.header_id = 0x010B00FF,
		.addr_header_id = 0x0000000B,
		.i2c_write_id = 0xA0,
		// QSC Calibration
		.qsc_support = TRUE,
		.qsc_size = 0x0C00,
		.addr_qsc = 0x1A3A,
		.sensor_reg_addr_qsc = 0x1000,
	},
};

/* fullsize <binning> <<2bining>> */
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_fullsize = {
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
		{0, 0}, {0, 0}, {0, 384}, {0, 384}, {0, 0},
		// <<cust1>> <<cust2>> <<cust3>> <cust4> <cust5>
		{0, 0}, {0, 0}, {0, 0}, {0, 384}, {0, 384},
		// <cust6> <cust7> <cust8> cust9 cust10
		{0, 384}, {0, 384}, {0, 384}, {2048, 1920}, {2048, 1536},
		// cust11 cust12 cust13 <cust14> <cust15>
		{2048, 1536}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		// <cust16> <cust17> cust18 <cust19> cust20
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {256, 912},
		// <cust21> <cust22> <cust23> <cust24> <cust25>
		{0, 0}, {0, 384}, {0, 384}, {0, 384}, {0, 0},
		// cust26 <cust27> cust28
		{1088, 996}, {0, 384}, {2048, 1536},
	},
	.iMirrorFlip = 0,
	.i4FullRawW = 8192,
	.i4FullRawH = 6144,
	.i4ModeIndex = 3,
	/* VC's PD pattern description */
	.sPDMapInfo[0] = {
		.i4PDPattern = 1,
		.i4BinFacX = 4,
		.i4BinFacY = 4,
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
		{0, 0}, {0, 0}, {0, 384}, {0, 384}, {0, 0},
		// <<cust1>> <<cust2>> <<cust3>> <cust4> <cust5>
		{0, 0}, {0, 0}, {0, 0}, {0, 384}, {0, 384},
		// <cust6> <cust7> <cust8> cust9 cust10
		{0, 384}, {0, 384}, {0, 384}, {2048, 1920}, {2048, 1536},
		// cust11 cust12 cust13 <cust14> <cust15>
		{2048, 1536}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		// <cust16> <cust17> cust18 <cust19> cust20
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {256, 912},
		// <cust21> <cust22> <cust23> <cust24> <cust25>
		{0, 0}, {0, 384}, {0, 384}, {0, 384}, {0, 0},
		// cust26 <cust27> cust28
		{1088, 996}, {0, 384}, {2048, 1536},
	},
	.iMirrorFlip = 0,
	.i4FullRawW = 4096,
	.i4FullRawH = 3072,
	.i4ModeIndex = 3,
	/* VC's PD pattern description */
	.sPDMapInfo[0] = {
		.i4PDPattern = 1,
		.i4BinFacX = 2,
		.i4BinFacY = 4,
	},
};

//1000 base for dcg gain ratio
static u32 imx989_dcg_ratio_table_12bit[] = {4000};

static u32 imx989_dcg_ratio_table_14bit[] = {16000};

static u32 imx989_dcg_ratio_table_ratio4[] = {4000};

static struct mtk_sensor_saturation_info imgsensor_saturation_info_10bit = {
	.gain_ratio = 1000,
	.OB_pedestal = 64,
	.saturation_level = 1023,
};

static struct mtk_sensor_saturation_info imgsensor_saturation_info_12bit = {
	.gain_ratio = 4000,
	.OB_pedestal = 64,
	.saturation_level = 3900,
};

static struct mtk_sensor_saturation_info imgsensor_saturation_info_14bit = {
	.gain_ratio = 16000,
	.OB_pedestal = 64,
	.saturation_level = 15408,
};

static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
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
			.vsize = 0x0300,
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
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0300,
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
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0240,
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
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0780,
			.vsize = 0x0438,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0500,
			.vsize = 0x02d0,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0780,
			.vsize = 0x0438,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0500,
			.vsize = 0x02d0,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2c,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x31,
			.hsize = 0x1000,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW12,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.valid_bit = 10,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus5[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x32,
			.hsize = 0x1000,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW14,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.valid_bit = 10,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus6[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
	// {
		// .bus.csi2 = {
			// .channel = 1,
			// .data_type = 0x30,
			// .hsize = 0x1000,
			// .vsize = 0x0240,
			// .dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			// .user_data_desc = VC_PDAF_STATS_ME_PIX_1,
		// },
	// },
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus7[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_ME_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus8[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus9[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0800,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus10[] = {
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
			.hsize = 0x0800,
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus11[] = {
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
			.hsize = 0x0800,
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus12[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x2000,
			.vsize = 0x1800,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0600,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus13[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x2000,
			.vsize = 0x1800,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0600,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus14[] = {
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_ME_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus15[] = {
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 2,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_SE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_ME_PIX_1,
		},
	},
// MRAW not support; HW number not enough
	// {
		// .bus.csi2 = {
			// .channel = 2,
			// .data_type = 0x30,
			// .hsize = 0x1000,
			// .vsize = 0x0300,
			// .dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			// .user_data_desc = VC_PDAF_STATS_SE_PIX_1,
		// },
	// },
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus16[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2c,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x31,
			.hsize = 0x1000,
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW12,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.valid_bit = 10,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus17[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2d,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x32,
			.hsize = 0x1000,
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW14,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.valid_bit = 10,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus18[] = {
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
	// {
		// .bus.csi2 = {
			// .channel = 1,
			// .data_type = 0x30,
			// .hsize = 0x1000,
			// .vsize = 0x0300,
			// .dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			// .user_data_desc = VC_PDAF_STATS_ME_PIX_1,
		// },
	// },
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus19[] = {
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
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus20[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1E00,
			.vsize = 0x10E0,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0F00,
			.vsize = 0x0438,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus21[] = {
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
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus22[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus23[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus24[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus25[] = {
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus26[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0780,
			.vsize = 0x0438,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x0780,
			.vsize = 0x0438,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0780,
			.vsize = 0x010E,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus27[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus28[] = {
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
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_ME,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0800,
			.vsize = 0x0300,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

#ifdef IMX989_CPHY_LRTE_DVT
static struct mtk_mbus_frame_desc_entry frame_desc_cus29[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 0x0500,
			.vsize = 0x02D0,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
#endif //IMX989_CPHY_LRTE_DVT

static struct subdrv_mode_struct mode_struct[] = {
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = imx989_preview_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_preview_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_preview,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_preview),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4070400000,
		.linelength = 11168,
		.framelength = 12144,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cap,
		.num_entries = ARRAY_SIZE(frame_desc_cap),
		.mode_setting_table = imx989_capture_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_capture_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_capture,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_capture),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4070400000,
		.linelength = 11168,
		.framelength = 12144,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = imx989_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_normal_video_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx989_seamless_normal_video,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_normal_video),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 3225600000,
		.linelength = 11168,
		.framelength = 9624,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = imx989_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4390400000,
		.linelength = 11168,
		.framelength = 3272,
		.max_framerate = 1200,
		.mipi_pixel_rate = 3085714286,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_slim_vid,
		.num_entries = ARRAY_SIZE(frame_desc_slim_vid),
		.mode_setting_table = imx989_slim_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_slim_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4352000000,
		.linelength = 7168,
		.framelength = 2528,
		.max_framerate = 2400,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 16,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 192,
			.y0_offset = 896,
			.w0_size = 7808,
			.h0_size = 4352,
			.scale_w = 1952,
			.scale_h = 1088,
			.x1_offset = 16,
			.y1_offset = 4,
			.w1_size = 1920,
			.h1_size = 1080,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1920,
			.h2_tg_size = 1080,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -1486,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = imx989_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom1_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 2982400000,
		.linelength = 3584,
		.framelength = 864,
		.max_framerate = 9600,
		.mipi_pixel_rate = 3085714286,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 16,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 192,
			.y0_offset = 1632,
			.w0_size = 7808,
			.h0_size = 2880,
			.scale_w = 1952,
			.scale_h = 720,
			.x1_offset = 336,
			.y1_offset = 0,
			.w1_size = 1280,
			.h1_size = 720,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1280,
			.h2_tg_size = 720,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -2973,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = imx989_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom2_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 3059200000,
		.linelength = 3584,
		.framelength = 1776,
		.max_framerate = 4800,
		.mipi_pixel_rate = 3085714286,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 16,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 192,
			.y0_offset = 896,
			.w0_size = 7808,
			.h0_size = 4352,
			.scale_w = 1952,
			.scale_h = 1088,
			.x1_offset = 16,
			.y1_offset = 4,
			.w1_size = 1920,
			.h1_size = 1080,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1920,
			.h2_tg_size = 1080,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -2973,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = imx989_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom3_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 2995200000,
		.linelength = 7168,
		.framelength = 864,
		.max_framerate = 4800,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 16,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 192,
			.y0_offset = 1632,
			.w0_size = 7808,
			.h0_size = 2880,
			.scale_w = 1952,
			.scale_h = 720,
			.x1_offset = 336,
			.y1_offset = 0,
			.w1_size = 1280,
			.h1_size = 720,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1280,
			.h2_tg_size = 720,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -1486,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = imx989_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom4_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx989_seamless_custom4,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom4),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 3225600000,
		.linelength = 42400,
		.framelength = 2528,
		.max_framerate = 300,
		.mipi_pixel_rate = 2037942857,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 12,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 12,
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
		.fine_integ_line = -436,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_R,
		.saturation_info = &imgsensor_saturation_info_12bit,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_ratio_min = 4000,
			.dcg_gain_ratio_max = 4000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = imx989_dcg_ratio_table_12bit,
			.dcg_gain_table_size = sizeof(imx989_dcg_ratio_table_12bit),
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = imx989_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom5_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx989_seamless_custom5,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom5),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 3225600000,
		.linelength = 42400,
		.framelength = 2528,
		.max_framerate = 300,
		.mipi_pixel_rate = 1746808163,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 12,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 12,
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
		.fine_integ_line = -436,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_R,
		.saturation_info = &imgsensor_saturation_info_14bit,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_ratio_min = 16000,
			.dcg_gain_ratio_max = 16000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = imx989_dcg_ratio_table_14bit,
			.dcg_gain_table_size = sizeof(imx989_dcg_ratio_table_14bit),
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 4,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus6,
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.mode_setting_table = imx989_custom6_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom6_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx989_seamless_custom6,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom6),
		.hdr_mode = HDR_RAW_LBMF,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 3225600000,
		.linelength = 11168,
		.framelength = 4808*2,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 2304,
		.read_margin = 64*2,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].max = 0xFFFC,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].max = 0xFFFC,
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
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.exposure_order_in_lbmf = IMGSENSOR_LBMF_EXPOSURE_SE_FIRST,
		.mode_type_in_lbmf = IMGSENSOR_LBMF_MODE_MANUAL,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus7,
		.num_entries = ARRAY_SIZE(frame_desc_cus7),
		.mode_setting_table = imx989_custom7_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom7_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx989_seamless_custom7,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom7),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 3225600000,
		.linelength = 11168,
		.framelength = 4808*2,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 2424*2, // 32+(112+Y_ADD_END-Y_ADD_STA+1+64)/2
		.read_margin = 64,
		.framelength_step = 8*2,
		.coarse_integ_step = 8*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].max = 0xFFF8*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].max = 0xFFF8*2,
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
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus8,
		.num_entries = ARRAY_SIZE(frame_desc_cus8),
		.mode_setting_table = imx989_custom8_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom8_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx989_seamless_custom8,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom8),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 3225600000,
		.linelength = 11168,
		.framelength = 4808,
		.max_framerate = 600,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus9,
		.num_entries = ARRAY_SIZE(frame_desc_cus9),
		.mode_setting_table = imx989_custom9_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom9_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx989_seamless_custom9,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom9),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 3225600000,
		.linelength = 21536,
		.framelength = 2496,
		.max_framerate = 600,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 1920,
			.w0_size = 8192,
			.h0_size = 2304,
			.scale_w = 8192,
			.scale_h = 2304,
			.x1_offset = 2048,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 2304,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 2304,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_fullsize,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -484,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_R,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus10,
		.num_entries = ARRAY_SIZE(frame_desc_cus10),
		.mode_setting_table = imx989_custom10_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom10_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom10,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom10),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4070400000,
		.linelength = 21536,
		.framelength = 6296,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 1536,
			.w0_size = 8192,
			.h0_size = 3072,
			.scale_w = 8192,
			.scale_h = 3072,
			.x1_offset = 2048,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 3072,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 3072,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_fullsize,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -484,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus11,
		.num_entries = ARRAY_SIZE(frame_desc_cus11),
		.mode_setting_table = imx989_custom11_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom11_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom11,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom11),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4070400000,
		.linelength = 21536,
		.framelength = 6296,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 1536,
			.w0_size = 8192,
			.h0_size = 3072,
			.scale_w = 8192,
			.scale_h = 3072,
			.x1_offset = 2048,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 3072,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 3072,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_fullsize,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -484,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_R,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus12,
		.num_entries = ARRAY_SIZE(frame_desc_cus12),
		.mode_setting_table = imx989_custom12_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom12_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom12,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom12),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4070400000,
		.linelength = 21536,
		.framelength = 7872,
		.max_framerate = 240,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
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
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_fullsize,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -484,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus13,
		.num_entries = ARRAY_SIZE(frame_desc_cus13),
		.mode_setting_table = imx989_custom13_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom13_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom13,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom13),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4070400000,
		.linelength = 21536,
		.framelength = 7872,
		.max_framerate = 240,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
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
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_fullsize,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -484,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_R,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus14,
		.num_entries = ARRAY_SIZE(frame_desc_cus14),
		.mode_setting_table = imx989_custom14_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom14_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom14,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom14),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 4070400000,
		.linelength = 11168,
		.framelength = 6072*2,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 3192*2, // 32+(112+Y_ADD_END-Y_ADD_STA+1+64)/2
		.read_margin = 64,
		.framelength_step = 8*2,
		.coarse_integ_step = 8*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].max = 0xFFF8*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].max = 0xFFF8*2,
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
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus15,
		.num_entries = ARRAY_SIZE(frame_desc_cus15),
		.mode_setting_table = imx989_custom15_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom15_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom15,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom15),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 3,
		.exp_cnt = 3,
		.pclk = 4070400000,
		.linelength = 11168,
		.framelength = 4048*3,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 3192*3, // 32+(112+Y_ADD_END-Y_ADD_STA+1+64)/2
		.read_margin = 64,
		.framelength_step = 8*3,
		.coarse_integ_step = 8*3,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8*3,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8*3,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_SE].min = 8*3,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].max = 0xFFF8*3,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].max = 0xFFF8*3,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_SE].max = 0xFFF8*3,
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
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus16,
		.num_entries = ARRAY_SIZE(frame_desc_cus16),
		.mode_setting_table = imx989_custom16_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom16_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom16,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom16),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 4070400000,
		.linelength = 42400,
		.framelength = 3200,
		.max_framerate = 300,
		.mipi_pixel_rate = 2037942857,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 12,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 12,
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
		.fine_integ_line = -436,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_R,
		.saturation_info = &imgsensor_saturation_info_12bit,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_ratio_min = 4000,
			.dcg_gain_ratio_max = 4000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = imx989_dcg_ratio_table_12bit,
			.dcg_gain_table_size = sizeof(imx989_dcg_ratio_table_12bit),
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus17,
		.num_entries = ARRAY_SIZE(frame_desc_cus17),
		.mode_setting_table = imx989_custom17_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom17_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom17,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom17),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 4070400000,
		.linelength = 42400,
		.framelength = 3200,
		.max_framerate = 300,
		.mipi_pixel_rate = 1746808163,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 12,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 12,
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
		.fine_integ_line = -436,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW14_R,
		.saturation_info = &imgsensor_saturation_info_14bit,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_ratio_min = 16000,
			.dcg_gain_ratio_max = 16000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = imx989_dcg_ratio_table_14bit,
			.dcg_gain_table_size = sizeof(imx989_dcg_ratio_table_14bit),
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 4,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus18,
		.num_entries = ARRAY_SIZE(frame_desc_cus18),
		.mode_setting_table = imx989_custom18_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom18_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom18,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom18),
		.hdr_mode = HDR_RAW_LBMF,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 4070400000,
		.linelength = 11168,
		.framelength = 6072*2,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 3072,
		.read_margin = 64*2,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].max = 0xFFFC,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].max = 0xFFFC,
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
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.exposure_order_in_lbmf = IMGSENSOR_LBMF_EXPOSURE_SE_FIRST,
		.mode_type_in_lbmf = IMGSENSOR_LBMF_MODE_MANUAL,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus19,
		.num_entries = ARRAY_SIZE(frame_desc_cus19),
		.mode_setting_table = imx989_custom19_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom19_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom19,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom19),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4070400000,
		.linelength = 11168,
		.framelength = 6072,
		.max_framerate = 600,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus20,
		.num_entries = ARRAY_SIZE(frame_desc_cus20),
		.mode_setting_table = imx989_custom20_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom20_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom20,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom20),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4339200000,
		.linelength = 21536,
		.framelength = 6712,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 896,
			.w0_size = 8192,
			.h0_size = 4352,
			.scale_w = 8192,
			.scale_h = 4352,
			.x1_offset = 256,
			.y1_offset = 16,
			.w1_size = 7680,
			.h1_size = 4320,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 7680,
			.h2_tg_size = 4320,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_fullsize,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -484,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_R,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus21,
		.num_entries = ARRAY_SIZE(frame_desc_cus21),
		.mode_setting_table = imx989_custom21_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom21_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 3276800000,
		.linelength = 22336,
		.framelength = 4888,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.fine_integ_line = -491,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus22,
		.num_entries = ARRAY_SIZE(frame_desc_cus22),
		.mode_setting_table = imx989_custom22_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom22_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 3276800000,
		.linelength = 22336,
		.framelength = 4888,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.fine_integ_line = -491,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus23,
		.num_entries = ARRAY_SIZE(frame_desc_cus23),
		.mode_setting_table = imx989_custom23_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom23_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 3328000000,
		.linelength = 22336,
		.framelength = 2480,
		.max_framerate = 600,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.fine_integ_line = -491,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
		{
		.frame_desc = frame_desc_cus24,
		.num_entries = ARRAY_SIZE(frame_desc_cus24),
		.mode_setting_table = imx989_custom24_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom24_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx989_seamless_custom24,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom24),
		.hdr_mode = HDR_RAW_DCG_RAW,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 3225600000,
		.linelength = 42400,
		.framelength = 2528,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
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
		.fine_integ_line = -436,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.saturation_info = &imgsensor_saturation_info_10bit,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_RAW,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_ratio_min = 4000,
			.dcg_gain_ratio_max = 4000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = imx989_dcg_ratio_table_ratio4,
			.dcg_gain_table_size = sizeof(imx989_dcg_ratio_table_ratio4),
		},
		.dpc_enabled = true,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 16,
	},
	{
		.frame_desc = frame_desc_cus25,
		.num_entries = ARRAY_SIZE(frame_desc_cus25),
		.mode_setting_table = imx989_custom25_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom25_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom25,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom25),
		.hdr_mode = HDR_RAW_DCG_RAW,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 4070400000,
		.linelength = 42400,
		.framelength = 3200,
		.max_framerate = 300,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
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
		.fine_integ_line = -436,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.saturation_info = &imgsensor_saturation_info_10bit,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_RAW,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_ratio_min = 4000,
			.dcg_gain_ratio_max = 4000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = imx989_dcg_ratio_table_ratio4,
			.dcg_gain_table_size = sizeof(imx989_dcg_ratio_table_ratio4),
		},
		.dpc_enabled = true,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 16,
	},
	{
		.frame_desc = frame_desc_cus26,
		.num_entries = ARRAY_SIZE(frame_desc_cus26),
		.mode_setting_table = imx989_custom26_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom26_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_RAW_DCG_RAW,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 4070400000,
		.linelength = 42400,
		.framelength = 1600,
		.max_framerate = 600,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 1984,
			.w0_size = 8192,
			.h0_size = 2176,
			.scale_w = 4096,
			.scale_h = 1088,
			.x1_offset = 1088,
			.y1_offset = 4,
			.w1_size = 1920,
			.h1_size = 1080,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1920,
			.h2_tg_size = 1080,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -436,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.saturation_info = &imgsensor_saturation_info_10bit,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_RAW,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_ratio_min = 4000,
			.dcg_gain_ratio_max = 4000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = imx989_dcg_ratio_table_ratio4,
			.dcg_gain_table_size = sizeof(imx989_dcg_ratio_table_ratio4),
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 16,
	},
	{
		.frame_desc = frame_desc_cus27,
		.num_entries = ARRAY_SIZE(frame_desc_cus27),
		.mode_setting_table = imx989_custom27_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom27_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4390400000,
		.linelength = 11168,
		.framelength = 6552,
		.max_framerate = 600,
		.mipi_pixel_rate = 3085714286,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.fine_integ_line = -982,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus28,
		.num_entries = ARRAY_SIZE(frame_desc_cus28),
		.mode_setting_table = imx989_custom28_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom28_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx989_seamless_custom28,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx989_seamless_custom28),
		.hdr_mode = HDR_RAW_LBMF,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 4070400000,
		.linelength = 21536,
		.framelength = 3264*2,
		.max_framerate = 289,
		.mipi_pixel_rate = 2445531428,
		.readout_length = 3136,
		.read_margin = 64*2,
		.framelength_step = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 6,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 6,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].max = 0xFFFC,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].max = 0xFFFC,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 1792,
			.y0_offset = 1536,
			.w0_size = 4608,
			.h0_size = 3072,
			.scale_w = 4608,
			.scale_h = 3072,
			.x1_offset = 256,
			.y1_offset = 0,
			.w1_size = 4096,
			.h1_size = 3072,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 4096,
			.h2_tg_size = 3072,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info_fullsize,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -484,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.exposure_order_in_lbmf = IMGSENSOR_LBMF_EXPOSURE_SE_FIRST,
		.mode_type_in_lbmf = IMGSENSOR_LBMF_MODE_MANUAL,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
#ifdef IMX989_CPHY_LRTE_DVT
	{
		.frame_desc = frame_desc_cus29,
		.num_entries = ARRAY_SIZE(frame_desc_cus29),
		.mode_setting_table = imx989_custom29_setting,
		.mode_setting_len = ARRAY_SIZE(imx989_custom29_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 4390400000,
		.linelength = 3584,
		.framelength = 1272,
		.max_framerate = 9600,
		.mipi_pixel_rate = 3850971428,
		.readout_length = 0,
		.read_margin = 64,
		.framelength_step = 8,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 192,
			.y0_offset = 1632,
			.w0_size = 7808,
			.h0_size = 2880,
			.scale_w = 1952,
			.scale_h = 720,
			.x1_offset = 336,
			.y1_offset = 0,
			.w1_size = 1280,
			.h1_size = 720,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1280,
			.h2_tg_size = 720,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW8_R,
		.ae_binning_ratio = 1000,
		.fine_integ_line = -2973,
		.delay_frame = 3,
		.csi_param = {
			.cphy_lrte_support = 1,
		},
		.dpc_enabled = true,
	},
#endif //IMX989_CPHY_LRTE_DVT
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = IMX989_SENSOR_ID,
	.reg_addr_sensor_id = {0x0016, 0x0017},
	.i2c_addr_table = {0x34, 0xFF},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {8192, 6144},
	.mirror = IMAGE_NORMAL,

	.mclk = 19,
	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_CPHY,
	.mipi_lane_num = SENSOR_MIPI_3_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 64,
	.ana_gain_type = 0,
	.ana_gain_step = 1,
	.ana_gain_table = imx989_ana_gain_table,
	.ana_gain_table_size = sizeof(imx989_ana_gain_table),
	.min_gain_iso = 100,
	.exposure_def = 0x3D0,
	.exposure_min = 8,
	.exposure_max = 128 * (0xFFFC - 48),
	.exposure_step = 4,
	.exposure_margin = 48,
	.dig_gain_min = BASE_DGAIN * 1,
	.dig_gain_max = BASE_DGAIN * 256,
	.dig_gain_step = 4,
	.saturation_info = &imgsensor_saturation_info_10bit,

	.frame_length_max = 0xFFF8,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 3,
	.start_exposure_offset = 5500000,

	.pdaf_type = PDAF_SUPPORT_CAMSV_DUALPD,
	.hdr_type = HDR_SUPPORT_STAGGER_FDOL|HDR_SUPPORT_DCG|HDR_SUPPORT_LBMF,
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
			{0x3162, 0x3163},
			{0x0224, 0x0225},
	},
	.reg_addr_exposure_in_lut = {
			{0x0E10, 0x0E11},
			{0x0E40, 0x0E41},
			{0x0E70, 0x0E71},
	},
	.long_exposure_support = TRUE,
	.reg_addr_exposure_lshift = 0x3150,
	.reg_addr_ana_gain = {
			{0x0204, 0x0205},
			{0x3164, 0x3165},
			{0x0216, 0x0217},
	},
	.reg_addr_ana_gain_in_lut = {
			{0x0E12, 0x0E13},
			{0x0E42, 0x0E43},
			{0x0E72, 0x0E73},
	},
	.reg_addr_dig_gain = {
			{0x020E, 0x020F},
			{0x3166, 0x3167},
			{0x0218, 0x0219},
	},
	.reg_addr_dig_gain_in_lut = {
			{0x0E14, 0x0E15},
			{0x0E44, 0x0E45},
			{0x0E74, 0x0E75},
	},
	.reg_addr_dcg_ratio = 0x3172,
	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_frame_length_in_lut = {
			{0x0E18, 0x0E19},
			{0x0E48, 0x0E49},
			{0x0E78, 0x0E79},
	},
	.reg_addr_temp_en = 0x0138,
	.reg_addr_temp_read = 0x013A,
	.reg_addr_auto_extend = 0x0350,
	.reg_addr_frame_count = 0x0005,
	.reg_addr_fast_mode = 0x3010,
	.reg_addr_fast_mode_in_lbmf = 0x3248,

	.init_setting_table = imx989_init_setting,
	.init_setting_len = ARRAY_SIZE(imx989_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 1,
	.chk_s_off_end = 0,

	.checksum_value = 0xd086e5a5,
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
	{HW_ID_MCLK, 19, 0},
	{HW_ID_RST, 0, 0},
	{HW_ID_MCLK_DRIVING_CURRENT, 4, 1},
	{HW_ID_AVDD, 2900000, 0}, // pmic_ldo for avdd
	{HW_ID_AVDD2, 1800000, 0}, // pmic_ldo/gpio(1.8V ldo) for avdd1
	{HW_ID_AFVDD, 3100000, 0}, // pmic_ldo for afvdd
	{HW_ID_AFVDD1, 1800000, 0}, // pmic_gpo(3.1V ldo) for afvdd
	{HW_ID_DOVDD, 1800000, 0}, // pmic_ldo/gpio(1.8V ldo) for dovdd
	{HW_ID_DVDD, 1100000, 1}, // pmic_ldo for dvdd
	{HW_ID_OISVDD, 3100000, 1},
	{HW_ID_RST, 1, 2}
};

const struct subdrv_entry imx989_mipi_raw_entry = {
	.name = "imx989_mipi_raw",
	.id = IMX989_SENSOR_ID,
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
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			subdrv_i2c_wr_u8(ctx, 0x3206, 0x01);
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

static int imx989_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
	subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_fast_mode, 0x02);
	if (ctx->s_ctx.reg_addr_fast_mode_in_lbmf &&
		(ctx->s_ctx.mode[scenario_id].hdr_mode == HDR_RAW_LBMF ||
		ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_LBMF))
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_fast_mode_in_lbmf, 0x4);

	update_mode_info(ctx, scenario_id);
	i2c_table_write(ctx,
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
		case HDR_RAW_DCG_RAW:
			set_shutter(ctx, ae_ctrl->exposure.le_exposure);
			if (ctx->s_ctx.mode[scenario_id].dcg_info.dcg_gain_mode
				== IMGSENSOR_DCG_DIRECT_MODE)
				set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			else
				set_gain(ctx, ae_ctrl->gain.le_gain);
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

static int imx989_cphy_lrte_mode(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	enum SENSOR_SCENARIO_ID_ENUM scenario_id;
	u8 cphy_lrte_en = 0;

	scenario_id = *((u64 *)para);
	cphy_lrte_en =
		ctx->s_ctx.mode[scenario_id].csi_param.cphy_lrte_support;

	if (cphy_lrte_en) {
		/*cphy lrte enable*/
		subdrv_i2c_wr_u8(ctx, 0x0860, 0x80);//enable cphy lrte and short packet 110 spacers
		subdrv_i2c_wr_u8(ctx, 0x0861, 0x6E);
		subdrv_i2c_wr_u8(ctx, 0x0862, 0x00);//long packet 40 spacers
		subdrv_i2c_wr_u8(ctx, 0x0863, 0x28);
	} else {
		/*cphy lrte disable*/
		subdrv_i2c_wr_u8(ctx, 0x0860, 0x00);//disable cphy lrte
		subdrv_i2c_wr_u8(ctx, 0x0861, 0x00);
		subdrv_i2c_wr_u8(ctx, 0x0862, 0x00);
		subdrv_i2c_wr_u8(ctx, 0x0863, 0x00);
	}

	DRV_LOG_MUST(ctx, "cphy_lrte_en = %d, scen = %u\n",
		cphy_lrte_en, scenario_id);
	return ERROR_NONE;
}

static int imx989_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	if (mode)
		subdrv_i2c_wr_u8(ctx, 0x0601, mode); /*100% Color bar*/
	else if (ctx->test_pattern)
		subdrv_i2c_wr_u8(ctx, 0x0601, 0x00); /*No pattern*/

	ctx->test_pattern = mode;
	return ERROR_NONE;
}

static int imx989_set_test_pattern_data(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	struct mtk_test_pattern_data *data = (struct mtk_test_pattern_data *)para;
	u16 R = (data->Channel_R >> 22) & 0x3ff;
	u16 Gr = (data->Channel_Gr >> 22) & 0x3ff;
	u16 Gb = (data->Channel_Gb >> 22) & 0x3ff;
	u16 B = (data->Channel_B >> 22) & 0x3ff;

	subdrv_i2c_wr_u8(ctx, 0x0602, (R >> 8));
	subdrv_i2c_wr_u8(ctx, 0x0603, (R & 0xff));
	subdrv_i2c_wr_u8(ctx, 0x0604, (Gr >> 8));
	subdrv_i2c_wr_u8(ctx, 0x0605, (Gr & 0xff));
	subdrv_i2c_wr_u8(ctx, 0x0606, (B >> 8));
	subdrv_i2c_wr_u8(ctx, 0x0606, (B & 0xff));
	subdrv_i2c_wr_u8(ctx, 0x0608, (Gb >> 8));
	subdrv_i2c_wr_u8(ctx, 0x0608, (Gb & 0xff));

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

static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt)
{
	DRV_LOG(ctx, "sof_cnt(%u) ctx->ref_sof_cnt(%u) ctx->fast_mode_on(%d)",
		sof_cnt, ctx->ref_sof_cnt, ctx->fast_mode_on);
	if (ctx->fast_mode_on && (sof_cnt > ctx->ref_sof_cnt)) {
		ctx->fast_mode_on = FALSE;
		ctx->ref_sof_cnt = 0;
		DRV_LOG(ctx, "seamless_switch disabled.");
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_fast_mode, 0x00);
		commit_i2c_buffer(ctx);
	}
	return 0;
}
