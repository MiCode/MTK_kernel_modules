// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 imx766mipiraw_Sensor.c
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
#include "imx766mipiraw_Sensor.h"

#define IMX766_EMBEDDED_DATA_EN 0

static void set_sensor_cali(void *arg);
static int get_sensor_temperature(void *arg);
static void set_group_hold(void *arg, u8 en);
static u16 get_gain2reg(u32 gain);
static int imx766_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int imx766_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, imx766_set_test_pattern},
	{SENSOR_FEATURE_SEAMLESS_SWITCH, imx766_seamless_switch},
};

static struct eeprom_info_struct eeprom_info[] = {
	{
		.header_id = 0x01480005,
		.addr_header_id = 0x00000006,
		.i2c_write_id = 0xA0,

		.qsc_support = TRUE,
		.qsc_size = 0x0C00,
		.addr_qsc = 0x1E30,
		.sensor_reg_addr_qsc = 0xC800,
	},
	{
		.header_id = 0x0148000E,
		.addr_header_id = 0x00000006,
		.i2c_write_id = 0xA0,

		.qsc_support = TRUE,
		.qsc_size = 0x0C00,
		.addr_qsc = 0x1E30,
		.sensor_reg_addr_qsc = 0xC800,
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
	.i4PosL = {{0, 0} },
	.i4PosR = {{0, 0} },
	.i4BlockNumX = 0,
	.i4BlockNumY = 0,
	.i4LeFirst = 0,
	.i4Crop = {
		{0, 0}, {0, 0}, {0, 384}, {0, 384}, {0, 0},
		{0, 0}, {0, 384}, {0, 0}, {0, 384}, {0, 384}
	},
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
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
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0800,
			.vsize = 0x0120,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x0800,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x2000,
			.vsize = 0x1800,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x2000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
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
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x1000,
			.vsize = 0x0240,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus5[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0800,
			.vsize = 0x0480,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 0x0800,
			.vsize = 0x0120,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x0800,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus6[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0500,
			.vsize = 0x02d0,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x0500,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus7[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x780,
			.vsize = 0x438,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x780,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus8[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus9[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus10[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus11[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x2000,
			.vsize = 0x1800,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x2000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus12[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_ME,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus13[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus14[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus15[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus16[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_ME,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus17[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0900,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus18[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x1000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus19[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x2000,
			.vsize = 0x1800,
			.user_data_desc = VC_STAGGER_NE,
			.is_active_line = TRUE,
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
			.is_active_line = TRUE,
		},
	},
#if IMX766_EMBEDDED_DATA_EN
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x12,
			.hsize = 0x2000,
			.vsize = 0x2,
			.user_data_desc = VC_GENERAL_EMBEDDED,
			.ebd_parsing_type = MTK_EBD_PARSING_TYPE_MIPI_RAW10,
		},
	},
#endif
};
static struct subdrv_mode_struct mode_struct[] = {
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = imx766_preview_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_preview_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1488000000,
		.linelength = 15616,
		.framelength = 3176,
		.max_framerate = 300,
		.mipi_pixel_rate = 787200000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true, /* reg 0x0b06 */
	},
	{
		.frame_desc = frame_desc_cap,
		.num_entries = ARRAY_SIZE(frame_desc_cap),
		.mode_setting_table = imx766_capture_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_capture_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1488000000,
		.linelength = 15616,
		.framelength = 3176,
		.max_framerate = 300,
		.mipi_pixel_rate = 787200000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = imx766_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_normal_video_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx766_seamless_normal_video,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_normal_video),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 2246400000,
		.linelength = 15616,
		.framelength = 4794,
		.max_framerate = 300,
		.mipi_pixel_rate = 938060000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = imx766_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1300800000,
		.linelength = 8816,
		.framelength = 1228,
		.max_framerate = 1200,
		.mipi_pixel_rate = 517030000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 2555,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 76,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_slim_vid,
		.num_entries = ARRAY_SIZE(frame_desc_slim_vid),
		.mode_setting_table = imx766_slim_video_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_slim_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1488000000,
		.linelength = 15616,
		.framelength = 3176,
		.max_framerate = 300,
		.mipi_pixel_rate = 787200000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = imx766_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom1_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 2380800000,
		.linelength = 31232,
		.framelength = 3176,
		.max_framerate = 240,
		.mipi_pixel_rate = 462170000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 413,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 84,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = imx766_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom2_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 2284800000,
		.linelength = 15616,
		.framelength = 2438,
		.max_framerate = 600,
		.mipi_pixel_rate = 894170000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 65,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = imx766_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom3_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1752000000,
		.linelength = 11552,
		.framelength = 6318,
		.max_framerate = 240,
		.mipi_pixel_rate = 1738970000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 1,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 502,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 69,
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = imx766_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom4_setting),
		.seamless_switch_group = 1,
		.seamless_switch_mode_setting_table = imx766_seamless_custom4,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_custom4),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 2246400000,
		.linelength = 15616,
		.framelength = 4792,
		.max_framerate = 300,
		.mipi_pixel_rate = 938060000,
		.readout_length = 2374*2,
		.read_margin = 10*2,
		.framelength_step = 4*2,
		.coarse_integ_step = 4 * 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8*2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 2826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 73,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = imx766_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom5_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 2716800000,
		.linelength = 8816,
		.framelength = 1284,
		.max_framerate = 2400,
		.mipi_pixel_rate = 1110860000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 2555,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 69,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus6,
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.mode_setting_table = imx766_custom6_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom6_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 2400000000,
		.linelength = 5568,
		.framelength = 896,
		.max_framerate = 4800,
		.mipi_pixel_rate = 842060000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 1709,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 69,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus7,
		.num_entries = ARRAY_SIZE(frame_desc_cus7),
		.mode_setting_table = imx766_custom7_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom7_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 3513600000,
		.linelength = 5568,
		.framelength = 1312,
		.max_framerate = 4800,
		.mipi_pixel_rate = 1848690000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 4,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = 8192,
			.full_h = 6144,
			.x0_offset = 0,
			.y0_offset = 768,
			.w0_size = 8192,
			.h0_size = 4608,
			.scale_w = 2048,
			.scale_h = 1152,
			.x1_offset = 64,
			.y1_offset = 36,
			.w1_size = 1920,
			.h1_size = 1080,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1920,
			.h2_tg_size = 1080,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 1709,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 68,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus8,
		.num_entries = ARRAY_SIZE(frame_desc_cus8),
		.mode_setting_table = imx766_custom8_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom8_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 3513600000,
		.linelength = 15616,
		.framelength = 3748,
		.max_framerate = 600,
		.mipi_pixel_rate = 2122970000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 64,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus9,
		.num_entries = ARRAY_SIZE(frame_desc_cus9),
		.mode_setting_table = imx766_custom9_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom9_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx766_seamless_custom9,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_custom9),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1785600000,
		.linelength = 15616,
		.framelength = 3810,
		.max_framerate = 300,
		.mipi_pixel_rate = 1782860000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 65,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus10,
		.num_entries = ARRAY_SIZE(frame_desc_cus10),
		.mode_setting_table = imx766_custom10_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom10_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx766_seamless_custom10,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_custom10),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1785600000,
		.linelength = 11552,
		.framelength = 5152,
		.max_framerate = 300,
		.mipi_pixel_rate = 1782860000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 502,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 65,
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus11,
		.num_entries = ARRAY_SIZE(frame_desc_cus11),
		.mode_setting_table = imx766_custom11_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom11_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx766_seamless_custom11,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_custom11),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1785600000,
		.linelength = 11552,
		.framelength = 6440,
		.max_framerate = 240,
		.mipi_pixel_rate = 1782860000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 502,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 65,
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus12,
		.num_entries = ARRAY_SIZE(frame_desc_cus12),
		.mode_setting_table = imx766_custom12_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom12_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx766_seamless_custom12,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_custom12),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 1785600000,
		.linelength = 15616,
		.framelength = 6352,
		.max_framerate = 180,
		.mipi_pixel_rate = 1782860000,
		.readout_length = 1554*2,
		.read_margin = 10*2,
		.framelength_step = 4*2,
		.coarse_integ_step = 4 * 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8*2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 2826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 65,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus13,
		.num_entries = ARRAY_SIZE(frame_desc_cus13),
		.mode_setting_table = imx766_custom13_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom13_setting),
		.seamless_switch_group = 3,
		.seamless_switch_mode_setting_table = imx766_seamless_custom13,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_custom13),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1785600000,
		.linelength = 15616,
		.framelength = 3810,
		.max_framerate = 300,
		.mipi_pixel_rate = 949030000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 71,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus14,
		.num_entries = ARRAY_SIZE(frame_desc_cus14),
		.mode_setting_table = imx766_custom14_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom14_setting),
		.seamless_switch_group = 3,
		.seamless_switch_mode_setting_table = imx766_seamless_custom14,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_custom14),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1785600000,
		.linelength = 23104,
		.framelength = 2576,
		.max_framerate = 300,
		.mipi_pixel_rate = 949030000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 751,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 71,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_Gb,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus15,
		.num_entries = ARRAY_SIZE(frame_desc_cus15),
		.mode_setting_table = imx766_custom15_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom15_setting),
		.seamless_switch_group = 4,
		.seamless_switch_mode_setting_table = imx766_seamless_custom15,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_custom15),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 3513600000,
		.linelength = 15616,
		.framelength = 7500,
		.max_framerate = 300,
		.mipi_pixel_rate = 2122970000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 64,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus16,
		.num_entries = ARRAY_SIZE(frame_desc_cus16),
		.mode_setting_table = imx766_custom16_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom16_setting),
		.seamless_switch_group = 4,
		.seamless_switch_mode_setting_table = imx766_seamless_custom16,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_custom16),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 3513600000,
		.linelength = 15616,
		.framelength = 7496,
		.max_framerate = 300,
		.mipi_pixel_rate = 2122970000,
		.readout_length = 1554*2,
		.read_margin = 10*2,
		.framelength_step = 4*2,
		.coarse_integ_step = 4 * 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8*2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8*2,
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
		.ae_binning_ratio = 1465,
		.fine_integ_line = 2826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 64,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus17,
		.num_entries = ARRAY_SIZE(frame_desc_cus17),
		.mode_setting_table = imx766_custom17_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom17_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 3513600000,
		.linelength = 9216,
		.framelength = 3176,
		.max_framerate = 1200,
		.mipi_pixel_rate = 2122970000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 64,
		},
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus18,
		.num_entries = ARRAY_SIZE(frame_desc_cus18),
		.mode_setting_table = imx766_custom18_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom18_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx766_seamless_custom18,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_custom18),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1785600000,
		.linelength = 11552,
		.framelength = 5152,
		.max_framerate = 300,
		.mipi_pixel_rate = 1782860000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 502,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 65,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_B,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
	{
		.frame_desc = frame_desc_cus19,
		.num_entries = ARRAY_SIZE(frame_desc_cus19),
		.mode_setting_table = imx766_custom19_setting,
		.mode_setting_len = ARRAY_SIZE(imx766_custom19_setting),
		.seamless_switch_group = 2,
		.seamless_switch_mode_setting_table = imx766_seamless_custom19,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(imx766_seamless_custom19),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 1785600000,
		.linelength = 11552,
		.framelength = 6440,
		.max_framerate = 240,
		.mipi_pixel_rate = 1782860000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 4,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 502,
		.delay_frame = 3,
		.csi_param = {
			.cphy_settle = 65,
		},
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_B,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.dpc_enabled = true,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = IMX766_SENSOR_ID,
	.reg_addr_sensor_id = {0x0016, 0x0017},
	.i2c_addr_table = {0x20, 0xFF},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {8192, 6144},
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
	.ana_gain_table = imx766_ana_gain_table,
	.ana_gain_table_size = sizeof(imx766_ana_gain_table),
	.min_gain_iso = 100,
	.exposure_def = 0x3D0,
	.exposure_min = 24,
	.exposure_max = 128 * (0xFFFC - 48),
	.exposure_step = 4,
	.exposure_margin = 48,
	.dig_gain_min = BASE_DGAIN * 1,
	.dig_gain_max = BASE_DGAIN * 16,
	.dig_gain_step = 4,

	.frame_length_max = 0xFFFC,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 3,
	.start_exposure_offset = 500000,

	.pdaf_type = PDAF_SUPPORT_CAMSV_QPD,
	.hdr_type = HDR_SUPPORT_STAGGER_FDOL,
	.seamless_switch_support = TRUE,
	.seamless_switch_type = SEAMLESS_SWITCH_CUT_VB_INIT_SHUT,
	.seamless_switch_hw_re_init_time_ns = 2750000,
	.seamless_switch_prsh_hw_fixed_value = 32,
	.seamless_switch_prsh_length_lc = 0,
	.reg_addr_prsh_length_lines = {0x3039, 0x303a, 0x303b},
	.reg_addr_prsh_mode = 0x3036,

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
	.reg_addr_exposure_lshift = 0x3128,
	.reg_addr_ana_gain = {
			{0x0204, 0x0205},
			{0x313C, 0x313D},
			{0x0216, 0x0217},
	},
	.reg_addr_dig_gain = {
			{0x020E, 0x020F},
			{0x313E, 0x313F},
			{0x0218, 0x0219},
	},
	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_temp_en = 0x0138,
	.reg_addr_temp_read = 0x013A,
	.reg_addr_auto_extend = 0x0350,
	.reg_addr_frame_count = 0x0005,
	.reg_addr_fast_mode = 0x3010,

	.init_setting_table = imx766_init_setting,
	.init_setting_len = ARRAY_SIZE(imx766_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 1,
	.chk_s_off_end = 0,

	.checksum_value = 0xAF3E324F,

	.ebd_info = {
		.frm_cnt_loc = {
			.loc_line = 1,
			.loc_pix = {7},
		},
		.coarse_integ_loc = {
			{  // NE
				.loc_line = 1,
				.loc_pix = {47, 49},
			},
			{  // ME
				.loc_line = 2,
				.loc_pix = {105, 107},
			},
			{  // SE
				.loc_line = 1,
				.loc_pix = {73, 75},
			},
		},
		.ana_gain_loc = {
			{  // NE
				.loc_line = 1,
				.loc_pix = {51, 53},
			},
			{  // ME
				.loc_line = 2,
				.loc_pix = {109, 111},
			},
			{  // SE
				.loc_line = 1,
				.loc_pix = {63, 65},
			},
		},
		.dig_gain_loc = {
			{  // NE
				.loc_line = 1,
				.loc_pix = {57, 59},
			},
			{  // ME
				.loc_line = 2,
				.loc_pix = {113, 115},
			},
			{  // SE
				.loc_line = 1,
				.loc_pix = {67, 69},
			},
		},
		.coarse_integ_shift_loc = {
			.loc_line = 2,
			.loc_pix = {97},
		},
		.dol_loc = {
			.loc_line = 2,
			.loc_pix = {145, 147}, // dol_en and dol_mode
		},
		.framelength_loc = {
			.loc_line = 1,
			.loc_pix = {111, 113},
		},
		.temperature_loc = {
			.loc_line = 1,
			.loc_pix = {37},
		},
	},
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
	.parse_ebd_line = common_parse_ebd_line,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_MCLK, 24, 0},
	{HW_ID_PDN, 0, 0},
	{HW_ID_RST, 0, 0},
	{HW_ID_MCLK_DRIVING_CURRENT, 4, 1},
	{HW_ID_AVDD2, 1800000, 0}, // power 1.8V to enable 2.8V ldo
	{HW_ID_AVDD, 2800000, 0},
	{HW_ID_AVDD1, 1800000, 0},
	{HW_ID_AFVDD1, 1800000, 0}, // power 1.8V to enable 2.8V ldo
	{HW_ID_AFVDD, 2800000, 0},
	{HW_ID_DVDD, 1100000, 0},
	{HW_ID_DOVDD, 1800000, 5},
	{HW_ID_PDN, 1, 0},
	{HW_ID_RST, 1, 2}
};

const struct subdrv_entry imx766_mipi_raw_entry = {
	.name = "imx766_mipi_raw",
	.id = IMX766_SENSOR_ID,
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
			subdrv_i2c_wr_u8(ctx, 0x86A9, 0x4E);
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			subdrv_i2c_wr_u8(ctx, 0x32D2, 0x01);
			DRV_LOG(ctx, "set QSC calibration data done.");
		} else {
			subdrv_i2c_wr_u8(ctx, 0x32D2, 0x00);
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
	return (16384 - (16384 * BASEGAIN) / gain);
}

static int imx766_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
		common_get_prsh_length_lines(ctx, ae_ctrl, pre_seamless_scenario_id, scenario_id);
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

static int imx766_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
	return 0;
}
