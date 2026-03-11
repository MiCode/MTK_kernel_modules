// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 klimts5kjn5telemipiraw_Sensor.c
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
#include "klimts5kjn5telemipiraw_Sensor.h"

static int get_sensor_temperature(void *arg);
static void set_group_hold(void *arg, u8 en);
static int klimts5kjn5tele_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static u16 get_gain2reg(u32 gain);
static int klimts5kjn5tele_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id);
static int klimts5kjn5tele_control(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data);
static int klimts5kjn5tele_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);
static void set_mi_init_setting_seq(void *arg);
static int get_csi_param(struct subdrv_ctx *ctx, enum SENSOR_SCENARIO_ID_ENUM scenario_id,struct mtk_csi_param *csi_param);
static void set_mi_pre_init(void *arg);
static int tele_vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt);
static int klimts5kjn5tele_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int klimts5kjn5tele_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int klimts5kjn5tele_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
void klimts5kjn5tele_set_long_exposure(struct subdrv_ctx *ctx);
void klimts5kjn5tele_write_frame_length(struct subdrv_ctx *ctx, u32 fll);
static int klimts5kjn5tele_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int klimts5kjn5tele_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int klimts5kjn5tele_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int klimts5kjn5tele_set_multi_dig_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);

#define SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE                 1
#define SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA_AUTHENTIC 2
#define SEAMLESS_SWITCH_GROUP_VIDEO_1610_MODE                 3
#define SEAMLESS_SWITCH_GROUP_HDR_CAP_MODE                    4
#define SEAMLESS_SWITCH_GROUP_VIDEO_1609_MODE                 5
#define MAX_FL_REG_VALUE (0xFFFF)

/* STRUCT */
static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, klimts5kjn5tele_set_test_pattern},
	{SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO, klimts5kjn5tele_set_max_framerate_by_scenario},
	{SENSOR_FEATURE_SEAMLESS_SWITCH, klimts5kjn5tele_seamless_switch},
	{SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME,klimts5kjn5tele_set_multi_shutter_frame_length},
	{SENSOR_FEATURE_SET_GAIN, klimts5kjn5tele_set_gain},
	{SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME, klimts5kjn5tele_set_shutter_frame_length},
	{SENSOR_FEATURE_SET_ESHUTTER, klimts5kjn5tele_set_shutter},
	{SENSOR_FEATURE_SET_MULTI_DIG_GAIN, klimts5kjn5tele_set_multi_dig_gain},
	{SENSOR_FEATURE_SET_AWB_GAIN, klimts5kjn5tele_set_awb_gain},
};

static struct eeprom_info_struct eeprom_info[] = {
	{
		// .header_id = 0x010B00FF,
		.addr_header_id = 0x0001,
		.i2c_write_id = 0xA8,

		.pdc_support = TRUE,
		.pdc_size = 6,
		.addr_pdc = 0x002D,
		.sensor_reg_addr_pdc = 0x5F80,//need check

	},
};

static struct SET_SENSOR_AWB_GAIN g_last_awb_gain = {0, 0, 0, 0};

static struct mtk_sensor_saturation_info klimts5kjn5_saturation_info_10bit = {
	.gain_ratio = 1000,
	.OB_pedestal = 64,
	.saturation_level = 1023,
};

static struct mtk_sensor_saturation_info klimts5kjn5_saturation_info_12bit = {
	.gain_ratio = 4000,
	.OB_pedestal = 256, // Figure 8-77 Saturation level for DAG-HDR
	.saturation_level = 4095, // Figure 8-77 Saturation level for DAG-HDR
	.adc_bit = 10,
	.ob_bm = 64,
};

static struct mtk_sensor_saturation_info klimts5kjn5_saturation_info_fake12bit = {
	.gain_ratio = 1000,
	.OB_pedestal = 256,
	.saturation_level = 4092,
	.adc_bit = 10,
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
		{0, 0}, {0, 0}, {0, 384}, {0, 0}, {0, 0},
		// <<cust1>> <<cust2>> <<cust3>> <cust4> <cust5>
		{0, 384}, {0, 0}, {0, 0}, {0, 0}, {2048, 1536},
		// <cust6> <cust7> <cust8> <cust9> <cust10>
		{0, 0}, {2048, 1536}, {0, 0}, {2048, 1536}, {0, 0},
		// <cust11> <cust12> <cust13> <cust14> <cust15>
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		// <cust16> <cust17> <cust18> <cust19> <cust20>
		{0, 0}, {0, 256}, {0, 256}, {2048, 1792}, {0, 0},
		// <cust21> <cust22> <cust23> <cust24> <cust25>
		{0, 0}, {0, 0}, {0, 0}, {2048, 1536},  {0, 0},
		// <cust26> <cust27>
		{2048, 1536}, {0, 0},
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
		{0, 0}, {0, 0}, {0, 384}, {0, 0}, {0, 0},
		// <<cust1>> <<cust2>> <<cust3>> <cust4> <cust5>
		{0, 384}, {0, 0}, {0, 0}, {0, 0}, {2048, 1536},
		// <cust6> <cust7> <cust8> <cust9> <cust10>
		{0, 0}, {2048, 1536}, {0, 0}, {2048, 1536}, {0, 0},
		// <cust11> <cust12> <cust13> <cust14> <cust15>
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		// <cust16> <cust17> <cust18> <cust19> <cust20>
		{0, 0}, {0, 256}, {0, 256}, {2048, 1792},  {0, 0},
		// <cust21> <cust22> <cust23> <cust24> <cust25>
		{0, 0}, {0, 0}, {0, 0}, {2048, 1536},  {0, 0},
		// <cust26> <cust27>
		{2048, 1536}, {0, 0},
	},
	.iMirrorFlip = 0,
	.i4FullRawW = 8192,
	.i4FullRawH = 6144,
	.i4ModeIndex = 3,
	/* VC's PD pattern description */
	.sPDMapInfo[0] = {
		.i4PDPattern = 1,
		.i4BinFacX = 4,
		.i4BinFacY = 8,
		.i4PDOrder = {1},
	},
};


// mode 0: 4096*3072@30fps, normal preview + pd
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
			.channel = 0,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 768,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

//mode 2
static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
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
	//need verify
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 576,
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
			.data_type = 0x2c,
			.hsize = 8192,
			.vsize = 6144,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	//need verify
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 4096,//4096
			.vsize = 768,//768
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus5[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2c,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	//need verify
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 384,//768
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus6[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2c,
			.hsize = 8192,
			.vsize = 6144,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	//need verify
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 768,//768
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

// mode 8: full size crop bayer
static struct mtk_mbus_frame_desc_entry frame_desc_cus7[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2c,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	//need verify
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 384,//768
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
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
	//need verify
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 768,
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
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	//need verify
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 384,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus13[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2c,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	//need verify
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 768,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus14[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2c,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
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
		},
	},
};


static struct mtk_mbus_frame_desc_entry frame_desc_cus15[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2c,
			.hsize = 4096,
			.vsize = 3072,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
	//need verify
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2c,
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
			.data_type = 0x2c,
			.hsize = 4096,
			.vsize = 2560,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	//need verify
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 640,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus18[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2c,
			.hsize = 4096,
			.vsize = 2560,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	//need verify
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 640,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_NE_PIX_1,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus19[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2c,
			.hsize = 4096,
			.vsize = 2560,
			.user_data_desc = VC_STAGGER_NE,
			.valid_bit = 10,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
	//need verify
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x30,
			.hsize = 2048,
			.vsize = 320,
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
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x30,
			.hsize = 4096,
			.vsize = 768,
			.dt_remap_to_type = MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
			.user_data_desc = VC_PDAF_STATS_ME_PIX_1,
		},
	},
};

#define PRE_MODE	.frame_desc = frame_desc_prev,\
	.num_entries = ARRAY_SIZE(frame_desc_prev),\
	.mode_setting_table = klimts5kjn5tele_preview_setting,\
	.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_preview_setting),\
	.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,\
	.seamless_switch_mode_setting_table = klimts5kjn5tele_preview_setting,\
	.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_preview_setting),\
	.hdr_mode = HDR_NONE,\
	.raw_cnt = 1,\
	.exp_cnt = 1,\
	.pclk = 920000000,\
	.linelength = 4784,\
	.framelength = 6408,\
	.max_framerate = 300,\
	.mipi_pixel_rate = 1824000000,\
	.readout_length = 0,\
	.read_margin = 10,\
	.framelength_step = 1,\
	.min_exposure_line = 4,\
	.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,\
	.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,\
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
	// mode 0: 4096x3072@30fps, normal preview + pd
	//complete 2 (3->2) v1.6zf_0611
	{
		PRE_MODE
	},
	// mode 1: same as preview mode
	//
	{
		PRE_MODE
	},
	// normal_video mode 2
	//4096x 2304@30fps video
	// complete 12 (18->12) v1.6zf_0611
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = klimts5kjn5tele_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_normal_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 4784,
		.framelength = 6408,
		.max_framerate = 300,
		.mipi_pixel_rate = 1824000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
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
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	//hs_video mode 3 : smvr 240fps
	//complete 19
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = klimts5kjn5tele_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 3116,
		.framelength = 1228,
		.max_framerate = 2400,
		.mipi_pixel_rate = 1824000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// slim_video mode 4: same as preview mode
	//
	{
		PRE_MODE
	},
	// custom1 mode 5: video 60fps
	//30fps --> 60fps
	//complete 12 (18->12) v1.6zf_0611
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = klimts5kjn5tele_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_normal_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 4784,
		.framelength = 3204,
		.max_framerate = 600,
		.mipi_pixel_rate = 1824000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
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
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// custom2 mode 6 : 120 fps
	//
	//complete 19
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = klimts5kjn5tele_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 3116,
		.framelength = 2456,
		.max_framerate = 1200,
		.mipi_pixel_rate = 1824000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// custom3 mode 7 : 240fps video
	//same as hs_video mode 3 : smvr 240fps
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = klimts5kjn5tele_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 3116,
		.framelength = 1228,
		.max_framerate = 2400,
		.mipi_pixel_rate = 1824000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
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
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	// custom4 mode 8 
	//full size quad 4:3 
	//8192 6144
	//complete 3 (5->3) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = klimts5kjn5tele_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom4_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_HDR_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom4_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom4_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 9600,
		.framelength = 6346,
		.max_framerate = 151,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.imgsensor_pd_info = &imgsensor_pd_info_isz,
		.ae_binning_ratio = 1250,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_4CELL_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
		.awb_enabled = true,
	},
	// custom5 mode 9 : full size crop quad 4:3 fake12 bit
	//4096x3072
	//complete 5 (7->5) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = klimts5kjn5tele_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom5_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_HDR_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom5_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom5_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 9200,
		.framelength = 3332,
		.max_framerate = 300,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.ae_binning_ratio = 1250,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_4CELL_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
		.awb_enabled = true,
	},
	// custom6 mode 10 : full size bayer 4:3
	//8192x6144 not use
	//complete 4
	{
		.frame_desc = frame_desc_cus6,
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.mode_setting_table = klimts5kjn5tele_custom6_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom6_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_HDR_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom6_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom6_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 9600,
		.framelength = 6346,
		.max_framerate = 151,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.imgsensor_pd_info = &imgsensor_pd_info_isz,
		.ae_binning_ratio = 1250,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
		.awb_enabled = true,
	},
	//custom7 mode 11 : full size crop bayer 4:3 
	//4096x3072
	//complete 4 (6->4) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus7,
		.num_entries = ARRAY_SIZE(frame_desc_cus7),
		.mode_setting_table = klimts5kjn5tele_custom7_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom7_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_HDR_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom7_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom7_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 9200,
		.framelength = 3332,
		.max_framerate = 300,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.ae_binning_ratio = 1250,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
		.awb_enabled = true,
	},
	//custom8 mode 12: full size bayer 4:3  10bit
	//8192x6144
	//complete 0 (1->0) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus8,
		.num_entries = ARRAY_SIZE(frame_desc_cus8),
		.mode_setting_table = klimts5kjn5tele_custom8_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom8_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom8_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom8_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 9600,
		.framelength = 6346,
		.max_framerate = 151,
		.mipi_pixel_rate = 1824000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.imgsensor_pd_info = &imgsensor_pd_info_isz,
		.ae_binning_ratio = 1250,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.awb_enabled = true,
	},
	//custom9 mode 13: full size crop bayer 4:3  10 bit
	//4096x3072 not use seamless
	//complete 1 (2->1) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus9,
		.num_entries = ARRAY_SIZE(frame_desc_cus9),
		.mode_setting_table = klimts5kjn5tele_custom9_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom9_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 9200,
		.framelength = 3332,
		.max_framerate = 300,
		.mipi_pixel_rate = 1824000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1250,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.awb_enabled = true,
	},
	//custom10 mode 14: 1x bokeh
	//4096x3072
	//same as preview
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = klimts5kjn5tele_preview_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_preview_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_preview_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 4784,
		.framelength = 8010,
		.max_framerate = 240,
		.mipi_pixel_rate = 1824000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
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
	//custom11 mode 15: 2x bokeh 10bit 24fps
	//same as preview
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = klimts5kjn5tele_preview_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_preview_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_preview_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 4784,
		.framelength = 6408,
		.max_framerate = 300,
		.mipi_pixel_rate = 1824000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
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
		// .imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	//custom12 mode 16: T + W人像
	//4096x3072
	//same as preview
	{
		PRE_MODE
	},
	//custom13 mode 17: binning
	//fake 12bit
	//complete 7 (9->7) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus13,
		.num_entries = ARRAY_SIZE(frame_desc_cus13),
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom13_setting),
		.mode_setting_table = klimts5kjn5tele_custom13_setting,
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_HDR_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom13_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom13_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 4784,
		.framelength = 3264,
		.max_framerate = 589,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
	},
	//custom14 mode 18:  DXG mode
	//complete 6 (8->6)  v1.6zf_0611
	{
		.frame_desc = frame_desc_cus14,
		.num_entries = ARRAY_SIZE(frame_desc_cus14),
		.mode_setting_table = klimts5kjn5tele_custom14_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom14_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_HDR_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom14_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom14_setting),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 920000000,
		.linelength = 9568,
		.framelength = 3204,
		.max_framerate = 300,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 20,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN * 1,
		.bit_align_type = IMGSENSOR_PIXEL_MSB_ALIGN,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_ratio_min = 4000,
			.dcg_gain_ratio_max = 4000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = PARAM_UNDEFINED,
			.dcg_gain_table_size = PARAM_UNDEFINED,
			.dcg_ratio_group = {1024, 1024}, // HCG = 5.7*1024, LCG = 1024
		},
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
		.saturation_info = &klimts5kjn5_saturation_info_12bit,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
	},
	//custom15 mode 19: 2exp stagger
	//fake 12bit
	//complete 8 (10->8) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus15,
		.num_entries = ARRAY_SIZE(frame_desc_cus15),
		.mode_setting_table = klimts5kjn5tele_custom15_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom15_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_HDR_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom15_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom15_setting),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 920000000,
		.linelength = 6248,
		.framelength = 6386,
		.max_framerate = 230,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 96,
		.framelength_step = 2,
		.min_exposure_line = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 80,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
	},
	//custom16 mode 20: same as preview
	{
		PRE_MODE
	},
	//custom 17 mode 21: 
	//4096x2560
	//complete 9 (11->9) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus17,
		.num_entries = ARRAY_SIZE(frame_desc_cus17),
		.mode_setting_table = klimts5kjn5tele_custom17_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom17_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_1610_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom17_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom17_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 5184,
		.framelength = 5897,
		.max_framerate = 300,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
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
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = &imgsensor_pd_info,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
	},
	//custom 18 mode 22:
	//4096x2560
	//complete 10(12->10) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus18,
		.num_entries = ARRAY_SIZE(frame_desc_cus18),
		.mode_setting_table = klimts5kjn5tele_custom18_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom18_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_1610_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom18_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom18_setting),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 920000000,
		.linelength = 9568,
		.framelength = 3194,
		.max_framerate = 300,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 20,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN * 1,
		.bit_align_type = IMGSENSOR_PIXEL_MSB_ALIGN,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_ratio_min = 4000,
			.dcg_gain_ratio_max = 4000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = PARAM_UNDEFINED,
			.dcg_gain_table_size = PARAM_UNDEFINED,
			.dcg_ratio_group = {1024, 1024}, // HCG = 5.7*1024, LCG = 1024
		},
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
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
		.saturation_info = &klimts5kjn5_saturation_info_12bit,
	},
	//custom 19 mode 23:
	//4096x2560
	//complete 11 (13->11) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus19,
		.num_entries = ARRAY_SIZE(frame_desc_cus19),
		.mode_setting_table = klimts5kjn5tele_custom19_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom19_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_VIDEO_1610_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom19_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom19_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 9200,
		.framelength = 3322,
		.max_framerate = 300,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.ae_binning_ratio = 1250,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
		.awb_enabled = true,
	},
	//custom20 mode 24: binning
	//fake 12bit
	//complete 7 (9->7) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus13,
		.num_entries = ARRAY_SIZE(frame_desc_cus13),
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom13_setting),
		.mode_setting_table = klimts5kjn5tele_custom13_setting,
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA_AUTHENTIC,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom13_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom13_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 4784,
		.framelength = 3264,
		.max_framerate = 589,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
	},
	//custom21 mode 25:  DXG mode
	//complete 6 (8->6) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus14,
		.num_entries = ARRAY_SIZE(frame_desc_cus14),
		.mode_setting_table = klimts5kjn5tele_custom14_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom14_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA_AUTHENTIC,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom14_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom14_setting),
		.hdr_mode = HDR_RAW_DCG_COMPOSE,
		.raw_cnt = 1,
		.exp_cnt = 2,
		.pclk = 920000000,
		.linelength = 9568,
		.framelength = 3204,
		.max_framerate = 300,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 20,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN * 1,
		.bit_align_type = IMGSENSOR_PIXEL_MSB_ALIGN,
		.dcg_info = {
			.dcg_mode = IMGSENSOR_DCG_COMPOSE,
			.dcg_gain_mode = IMGSENSOR_DCG_RATIO_MODE,
			.dcg_gain_ratio_min = 4000,
			.dcg_gain_ratio_max = 4000,
			.dcg_gain_ratio_step = 0,
			.dcg_gain_table = PARAM_UNDEFINED,
			.dcg_gain_table_size = PARAM_UNDEFINED,
			.dcg_ratio_group = {1024, 1024}, // HCG = 5.7*1024, LCG = 1024
		},
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
		.saturation_info = &klimts5kjn5_saturation_info_12bit,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
	},
	//custom22 mode 26: 2exp stagger
	//fake 12bit
	//complete 8 (10->8) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus15,
		.num_entries = ARRAY_SIZE(frame_desc_cus15),
		.mode_setting_table = klimts5kjn5tele_custom15_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom15_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA_AUTHENTIC,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom15_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom15_setting),
		.hdr_mode = HDR_RAW_STAGGER,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 920000000,
		.linelength = 6248,
		.framelength = 6386,
		.max_framerate = 230,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 96,
		.framelength_step = 2,
		.min_exposure_line = 8,
		.coarse_integ_step = 2,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 80,
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
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
	},
	// custom23 mode 27 
	//full size quad 4:3
	//8192 6144
	//complete 3 (5->3) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.mode_setting_table = klimts5kjn5tele_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom4_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA_AUTHENTIC,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom4_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom4_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 9600,
		.framelength = 6346,
		.max_framerate = 151,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.imgsensor_pd_info = &imgsensor_pd_info_isz,
		.ae_binning_ratio = 1250,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_4CELL_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
		.awb_enabled = true,
	},
	// custom24 mode 28 : full size crop quad 4:3 fake12 bit 
	//4096x3072
	//complete 5 (7->5) v1.6zf_0611
	{
		.frame_desc = frame_desc_cus5,
		.num_entries = ARRAY_SIZE(frame_desc_cus5),
		.mode_setting_table = klimts5kjn5tele_custom5_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom5_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA_AUTHENTIC,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom5_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom5_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 9200,
		.framelength = 3332,
		.max_framerate = 300,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.ae_binning_ratio = 1250,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_4CELL_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
		.awb_enabled = true,
	},
	// custom25 mode 29 : full size bayer 4:3
	//8192x6144
	//complete 4
	{
		.frame_desc = frame_desc_cus6,
		.num_entries = ARRAY_SIZE(frame_desc_cus6),
		.mode_setting_table = klimts5kjn5tele_custom6_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom6_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA_AUTHENTIC,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom6_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom6_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 9600,
		.framelength = 6346,
		.max_framerate = 151,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.imgsensor_pd_info = &imgsensor_pd_info_isz,
		.ae_binning_ratio = 1250,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
		.awb_enabled = true,
	},
	//custom26 mode 30 : full size crop bayer 4:3 
	//4096x3072
	//complete 6
	{
		.frame_desc = frame_desc_cus7,
		.num_entries = ARRAY_SIZE(frame_desc_cus7),
		.mode_setting_table = klimts5kjn5tele_custom7_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom7_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA_AUTHENTIC,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom7_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom7_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 9200,
		.framelength = 3332,
		.max_framerate = 300,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
		.ae_binning_ratio = 1250,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW12_B,
		.saturation_info = &klimts5kjn5_saturation_info_fake12bit,
		.awb_enabled = true,
	},
	//custom27 mode 31: 1x bokeh leica
	//4096x3072
	//same as preview
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = klimts5kjn5tele_preview_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_preview_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_preview_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 920000000,
		.linelength = 4784,
		.framelength = 8010,
		.max_framerate = 240,
		.mipi_pixel_rate = 1824000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 1,
		.min_exposure_line = 4,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
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
		// .imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
	},
	//LBMF mode 32:
	{
		.frame_desc = frame_desc_cus28,
		.num_entries = ARRAY_SIZE(frame_desc_cus28),
		.mode_setting_table = klimts5kjn5tele_custom28_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom28_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom28_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom28_setting),
		.hdr_mode = HDR_RAW_LBMF,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 920000000,
		.linelength = 4784,
		.framelength = 3204 * 2,
		.max_framerate = 300,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 3192,
		.read_margin = 12,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 80,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN * 1,
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
		.exposure_order_in_lbmf = IMGSENSOR_LBMF_EXPOSURE_SE_FIRST,
		.mode_type_in_lbmf = IMGSENSOR_LBMF_MODE_MANUAL,
	},
	//LBMF mode 33: leica
	{
		.frame_desc = frame_desc_cus28,
		.num_entries = ARRAY_SIZE(frame_desc_cus28),
		.mode_setting_table = klimts5kjn5tele_custom28_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom28_setting),
		.seamless_switch_group = SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA_AUTHENTIC,
		.seamless_switch_mode_setting_table = klimts5kjn5tele_custom28_setting,
		.seamless_switch_mode_setting_len = ARRAY_SIZE(klimts5kjn5tele_custom28_setting),
		.hdr_mode = HDR_RAW_LBMF,
		.raw_cnt = 2,
		.exp_cnt = 2,
		.pclk = 920000000,
		.linelength = 4784,
		.framelength = 3204 * 2,
		.max_framerate = 300,
		.mipi_pixel_rate = 1520000000,
		.readout_length = 3192,
		.read_margin = 12,
		.framelength_step = 1,
		.min_exposure_line = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_ME].min = 8,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 80,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN * 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].max = BASEGAIN * 80,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_ME].min = BASEGAIN * 1,
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
		.exposure_order_in_lbmf = IMGSENSOR_LBMF_EXPOSURE_SE_FIRST,
		.mode_type_in_lbmf = IMGSENSOR_LBMF_MODE_MANUAL,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = KLIMTS5KJN5TELE_SENSOR_ID,
	.reg_addr_sensor_id = {0x0000, 0x0001},
	.i2c_addr_table = {0x5A, 0xFF}, // TBD
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_16,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {8192, 6144},
	.mirror = IMAGE_NORMAL, // TBD

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_CPHY,
	.mipi_lane_num = SENSOR_MIPI_3_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
	.dig_gain_min = BASEGAIN * 1,
	.dig_gain_max = BASEGAIN * 32,
	.dig_gain_step = 1,  //If the value is 0, SENSOR_FEATURE_SET_MULTI_DIG_GAIN is disabled
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 64,
	.ana_gain_type = 1,
	.ana_gain_step = 1,
	.ana_gain_table = klimts5kjn5tele_ana_gain_table,
	.ana_gain_table_size = sizeof(klimts5kjn5tele_ana_gain_table),
	.tuning_iso_base = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 4,
	.exposure_max = 0x7FFF800-12,
	.exposure_step = 2,
	.exposure_margin = 12,
	.saturation_info = &klimts5kjn5_saturation_info_10bit,

	.frame_length_max = 0x7FFF800,
	.ae_effective_frame = 3,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 1436800,

	.pdaf_type = PDAF_SUPPORT_CAMSV_QPD,
	.hdr_type = HDR_SUPPORT_STAGGER_DOL|HDR_SUPPORT_DCG,
	.seamless_switch_support = TRUE,
	.temperature_support = TRUE,

	.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,
	.s_mi_init_seq = set_mi_init_setting_seq,
	.s_mi_pre_init = set_mi_pre_init,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = 0x0101, // TBD
	.reg_addr_exposure = {
		{0x0202, 0x0203},
		{0x0226, 0x0227},
		{0x0226, 0x0227}
	},
	.long_exposure_support = TRUE,
	.mi_long_exposure_type = 1,
	.reg_addr_exposure_lshift = 0x0704,
	.reg_addr_framelength_lshift = 0x0702,
	.reg_addr_ana_gain = {
		{0x0204, 0x0205},
		{0x0206, 0x0207},
		{0x0206, 0x0207}
	},
	.reg_addr_frame_length_in_lut = {
		{0x0E14, 0x0E15},
		{0x0E20, 0x0E21},
		{0x0E20, 0x0E21},
	},
	.reg_addr_exposure_in_lut = {
		{0x0E10, 0x0E11},
		{0x0E1c, 0x0E1D},
		{0x0E1c, 0x0E1D},
	},
	.reg_addr_ana_gain_in_lut = {
		{0x0E12, 0x0E13},
		{0x0E1E, 0x0E1F},
		{0x0E1E, 0x0E1F},
	},
	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_temp_read = 0x0020,
//	.reg_addr_auto_extend = PARAM_UNDEFINED,
	.reg_addr_frame_count = 0x0005, // To be verified

	.init_setting_table = klimts5kjn5tele_init_setting,
	.init_setting_len = ARRAY_SIZE(klimts5kjn5tele_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 1,
	.chk_s_off_end = 0,
	/* custom stream control delay timing for hw limitation (ms) */
	.custom_stream_ctrl_delay = 5,
	//TBD
	.checksum_value = 0xAF3E324F,
};

static struct subdrv_ops ops = {
	.get_id = klimts5kjn5tele_get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = common_open,
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = klimts5kjn5tele_control,
	.feature_control = common_feature_control,
	.close = common_close,
	.get_frame_desc = common_get_frame_desc,
	.get_temp = common_get_temp,
	.get_csi_param = get_csi_param,
	.update_sof_cnt = common_update_sof_cnt,
	.vsync_notify = tele_vsync_notify,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_DVDD1,  {1300000, 1300000},  5000}, //ldo supply
	{HW_ID_AFVDD, {3300000, 3300000}, 1000},
	{HW_ID_RST,   {0},       0},
	{HW_ID_DOVDD, {1800000, 1800000}, 2000},
	{HW_ID_AVDD,  {2200000, 2200000}, 2000},
	{HW_ID_DVDD, {1800000, 1800000}, 1000},
	{HW_ID_RST,   {1},       0},
	{HW_ID_MCLK,  {24},	   0},
	{HW_ID_MCLK_DRIVING_CURRENT, {4}, 10000},
};

const struct subdrv_entry klimts5kjn5tele_mipi_raw_entry = {
	.name = "klimts5kjn5tele_mipi_raw",
	.id = KLIMTS5KJN5TELE_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

/* FUNCTION */
static int get_sensor_temperature(void *arg)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	int temperature = 0;

	/*TEMP_SEN_CTL */
	temperature = subdrv_i2c_rd_u16(ctx, ctx->s_ctx.reg_addr_temp_read);
	temperature = temperature / 256;

	DRV_LOG(ctx, "temperature: %d degrees\n", temperature);
	return temperature;
}

static void set_group_hold(void *arg, u8 en)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	if (en) {
		set_i2c_buffer(ctx, 0x0104, 0x01);
	} else {
		set_i2c_buffer(ctx, 0x0104, 0x00);
	}
}

static int klimts5kjn5tele_control(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	int ret = ERROR_NONE;
	u16 idx = 0;
	u8 support = FALSE;
	u8 *pbuf = NULL;
	u16 size = 0;
	u16 addr = 0;
	u32 switch_group;
	u16 *list;
	u32 len;
	struct eeprom_info_struct *info = ctx->s_ctx.eeprom_info;
	struct adaptor_ctx *_adaptor_ctx = NULL;
	struct v4l2_subdev *sd = NULL;

	if (ctx->i2c_client)
		sd = i2c_get_clientdata(ctx->i2c_client);
	if (ctx->ixc_client.protocol)
		sd = adaptor_ixc_get_clientdata(&ctx->ixc_client);
	if (sd)
		_adaptor_ctx = to_ctx(sd);
	if (!_adaptor_ctx) {
		DRV_LOGE(ctx, "null _adaptor_ctx\n");
		return -ENODEV;
	}

	DRV_LOG_MUST(ctx, "scenario_id = %d\n", scenario_id);

	if (scenario_id >= ctx->s_ctx.sensor_mode_num) {
		DRV_LOGE(ctx, "invalid sid:%u, mode_num:%u\n",
			scenario_id, ctx->s_ctx.sensor_mode_num);
		scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW;
		ret = ERROR_INVALID_SCENARIO_ID;
	}
	if (ctx->s_ctx.chk_s_off_sta)
		check_stream_off(ctx);
	update_mode_info(ctx, scenario_id);

	ctx->current_scenario_id = scenario_id;
	switch_group = ctx->s_ctx.mode[scenario_id].seamless_switch_group;
	list = ctx->s_ctx.mode[scenario_id].mode_setting_table;
	len = ctx->s_ctx.mode[scenario_id].mode_setting_len;

	switch (switch_group)
	{
		case SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE:
		case SEAMLESS_SWITCH_GROUP_NORMAL_CAP_MODE_LCICA_AUTHENTIC:
		case SEAMLESS_SWITCH_GROUP_HDR_CAP_MODE:
		case SEAMLESS_SWITCH_GROUP_VIDEO_1610_MODE:
		case SEAMLESS_SWITCH_GROUP_VIDEO_1609_MODE:
			break;
		default:
			DRV_LOG(ctx, "fcm is no \n");
			subdrv_i2c_wr_u16(ctx, 0x0B30, 0x01FF);
			break;
	}

	i2c_table_write(ctx, list, len);
	DRV_LOG(ctx, " write mode setting end \n");

	//vertical flip
	subdrv_i2c_wr_u16(ctx, ctx->s_ctx.reg_addr_mirror_flip, 0x0200);

	DRV_LOG_MUST(ctx, "-\n");

	if (check_is_no_crop(ctx, scenario_id) && probe_eeprom(ctx)) {
		idx = ctx->eeprom_index;
		support = info[idx].xtalk_support;
		pbuf = info[idx].preload_xtalk_table;
		size = info[idx].xtalk_size;
		addr = info[idx].sensor_reg_addr_xtalk;
		if (support) {
			if (pbuf != NULL && addr > 0 && size > 0) {
				subdrv_ixc_wr_seq_p8(ctx, addr, pbuf, size);
				DRV_LOG(ctx, "set XTALK calibration data done.");
			}
		}
	}

	return ret;
}


static int klimts5kjn5tele_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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

static u16 get_gain2reg(u32 gain)
{
	return gain * 32 / BASEGAIN;
}

static int klimts5kjn5tele_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id)
{
	u8 i = 0;
	u8 retry = 2;
	u8 vendor_id;
	u16 idx = 0;
	u32 addr_h = ctx->s_ctx.reg_addr_sensor_id.addr[0];
	u32 addr_l = ctx->s_ctx.reg_addr_sensor_id.addr[1];
	u32 addr_ll = ctx->s_ctx.reg_addr_sensor_id.addr[2];
	struct eeprom_info_struct *info = ctx->s_ctx.eeprom_info;

	while (ctx->s_ctx.i2c_addr_table[i] != 0xFF) {
		ctx->i2c_write_id = ctx->s_ctx.i2c_addr_table[i];
		do {
			*sensor_id = (subdrv_ixc_rd_u8(ctx, addr_h) << 8) |
				subdrv_ixc_rd_u8(ctx, addr_l);
			if (addr_ll) {
				*sensor_id = ((*sensor_id) << 8) | subdrv_ixc_rd_u8(ctx, addr_ll);
			}

			if (*sensor_id == ctx->s_ctx.sensor_id){
				ctx->i2c_write_id = info[idx].i2c_write_id;
				vendor_id = subdrv_ixc_rd_u8(ctx, info[idx].addr_header_id);
				ctx->i2c_write_id = ctx->s_ctx.i2c_addr_table[i];
				if (vendor_id == 0x01) {
					*sensor_id = 0xFFFFFFFF;
					return ERROR_SENSOR_CONNECT_FAIL;
				}else{
					DRV_LOG_MUST(ctx, "i2c_write_id:0x%x sensor_id(cur/exp):0x%x/0x%x vendor_id 0x%x\n",
						ctx->i2c_write_id, *sensor_id, ctx->s_ctx.sensor_id, vendor_id);
					return ERROR_NONE;
				}
			}
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

static int klimts5kjn5tele_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	switch (mode) {
	case 5:
		subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);
		subdrv_i2c_wr_u16(ctx, 0x0600, 0x0001);
		subdrv_i2c_wr_u16(ctx, 0x0602, 0x0000);
		subdrv_i2c_wr_u16(ctx, 0x0604, 0x0000);
		subdrv_i2c_wr_u16(ctx, 0x0606, 0x0000);
		subdrv_i2c_wr_u16(ctx, 0x0608, 0x0000);
		break;
	default:
		subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);
		subdrv_i2c_wr_u16(ctx, 0x0600, 0x0000);
		break;
	}

	ctx->test_pattern = mode;
	return ERROR_NONE;
}

static int klimts5kjn5tele_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
	ctx->frame_length = max(
		(u32)shutter + ctx->s_ctx.mode[ctx->current_scenario_id].exposure_margin,
			ctx->min_frame_length);
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

	if (ctx->extraVB) {
		ctx->frame_length += ctx->extraVB * (ctx->pclk / 1000)  / ctx->line_length + ctx->s_ctx.mode[ctx->current_scenario_id].imgsensor_winsize_info.h2_tg_size;
		DRV_LOG(ctx, "ctx->exposure[0] = %d, framelength = %d, extraVB(%d), hSize(%d)", ctx->exposure[0], ctx->frame_length, ctx->extraVB, ctx->s_ctx.mode[ctx->current_scenario_id].imgsensor_winsize_info.h2_tg_size);
	}

	klimts5kjn5tele_set_long_exposure(ctx);
	/* write framelength */
	if (set_auto_flicker(ctx, 0) || frame_length || !ctx->s_ctx.reg_addr_auto_extend)
		klimts5kjn5tele_write_frame_length(ctx, ctx->frame_length);
	else if (ctx->s_ctx.reg_addr_auto_extend)
		klimts5kjn5tele_write_frame_length(ctx, ctx->min_frame_length);
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

static int klimts5kjn5tele_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	klimts5kjn5tele_set_shutter_frame_length(ctx, para, len);
	return 0;
}

static u32 dgain2reg(struct subdrv_ctx *ctx, u32 dgain)
{
	DRV_LOG_MUST(ctx, "dgain is 0x%u\n", dgain);
	u32 ret = dgain * 256 / 1024;
	DRV_LOG_MUST(ctx, "dgain reg = 0x%x\n", ret);
	return ret;
}

static int klimts5kjn5tele_set_multi_dig_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *) para;
	u32 *gains  = (u32 *)(*feature_data);
	u16 exp_cnt = (u16) (*(feature_data + 1));

	int i = 0;
	u32 rg_gains[IMGSENSOR_STAGGER_EXPOSURE_CNT] = {0};
	// bool gph = !ctx->is_seamless && (ctx->s_ctx.s_gph != NULL);

	if (exp_cnt > ARRAY_SIZE(ctx->dig_gain)) {
		DRV_LOGE(ctx, "invalid exp_cnt:%u>%lu\n", exp_cnt, ARRAY_SIZE(ctx->dig_gain));
		exp_cnt = ARRAY_SIZE(ctx->dig_gain);
	}
	for (i = 0; i < exp_cnt; i++) {
		/* check boundary of gain */
		gains[i] = max(gains[i], ctx->s_ctx.dig_gain_min);
		gains[i] = min(gains[i], ctx->s_ctx.dig_gain_max);
		gains[i] = dgain2reg(ctx, gains[i]);
	}

	/* restore gain */
	memset(ctx->dig_gain, 0, sizeof(ctx->dig_gain));
	for (i = 0; i < exp_cnt; i++)
		ctx->dig_gain[i] = gains[i];

	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);

	// /* group hold start */
	// if (gph && !ctx->ae_ctrl_gph_en)
	// 	ctx->s_ctx.s_gph((void *)ctx, 1);

	/* write gain */
	switch (exp_cnt) {
	case 1:
		rg_gains[0] = gains[0];
		break;
	case 2:
		rg_gains[0] = gains[0];
		rg_gains[1] = gains[1];
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
			// set_i2c_buffer(ctx,
			// 	ctx->s_ctx.reg_addr_dig_gain[i].addr[0],
			// 	(rg_gains[i]) & 0xFF);
			DRV_LOG_MUST(ctx, "yjw_write_reg %d times, %4x gain\n", i, rg_gains[i]);
			subdrv_i2c_wr_u16(ctx, ctx->s_ctx.reg_addr_dig_gain[i].addr[0], (rg_gains[i]) & 0xFFFF);
		}
	}

	// if (!ctx->ae_ctrl_gph_en) {
	// 	if (gph)
	// 		ctx->s_ctx.s_gph((void *)ctx, 0);
	// 	commit_i2c_buffer(ctx);
	// }

	DRV_LOG(ctx, "yjw_dgain reg[lg/mg/sg]: 0x%x 0x%x 0x%x\n",
		rg_gains[0], rg_gains[1], rg_gains[2]);
	return 0;
}

void klimts5kjn5tele_update_mode_info(struct subdrv_ctx *ctx, enum SENSOR_SCENARIO_ID_ENUM scenario_id)
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

void klimts5kjn5tele_write_frame_length(struct subdrv_ctx *ctx, u32 fll)
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

	if (ctx->extend_frame_length_en == FALSE) {
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
}

void klimts5kjn5tele_set_multi_shutter_frame_length_dol(struct subdrv_ctx *ctx,
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

		calc_fl[1] = (u32) shutters[0] + (u32) shutters[1]
		+ ctx->s_ctx.mode[ctx->current_scenario_id].read_margin;

		DRV_LOG(ctx,
			"calc_fl[0](%u) calc_fl[1]%u,exp-LE/ME/SE (%u/%u/%u),shutter-LE/ME/SE (%u/%u/%u)\
			read_margin(%u)\
			readout_length(%d)\n",\
			calc_fl[0],calc_fl[1],ctx->exposure[0],ctx->exposure[1],ctx->exposure[2],\
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
		klimts5kjn5tele_write_frame_length(ctx, ctx->frame_length);
	else if (ctx->s_ctx.reg_addr_auto_extend)
		klimts5kjn5tele_write_frame_length(ctx, ctx->min_frame_length);
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
	DRV_LOG(ctx, "exp[0x%x/0x%x/0x%x], fll(input/output):%u/%u, flick_en:%d, extraVB:%d\n",
		rg_shutters[0], rg_shutters[1], rg_shutters[2],
		frame_length, ctx->frame_length, ctx->autoflicker_en, ctx->extraVB);
	if (!ctx->ae_ctrl_gph_en) {
		if (gph)
			ctx->s_ctx.s_gph((void *)ctx, 0);
		commit_i2c_buffer(ctx);
	}
}

static int klimts5kjn5tele_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
		calc_fl[0] = max(5000, (u32) shutters[0]) +  max(5000, (u32) ctx->exposure[1])
		+ ctx->s_ctx.mode[ctx->current_scenario_id].read_margin;

		calc_fl[1] = (u32) shutters[0] + (u32) shutters[1]
		+ ctx->s_ctx.mode[ctx->current_scenario_id].read_margin;

		DRV_LOG(ctx,
			"calc_fl[0](%u) calc_fl[1]%u,exp-LE/ME/SE (%u/%u/%u),shutter-LE/ME/SE (%u/%u/%u)\
			read_margin(%u)\
			readout_length(%d)\n",\
			calc_fl[0],calc_fl[1],ctx->exposure[0],ctx->exposure[1],ctx->exposure[2],\
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
		DRV_LOG(ctx, "ctx->exposure[0] = %d, framelength = %d, extraVB(%d) hsize(%d)", ctx->exposure[0], ctx->frame_length, ctx->extraVB, ctx->s_ctx.mode[ctx->current_scenario_id].imgsensor_winsize_info.h2_tg_size);
	}

	klimts5kjn5tele_set_long_exposure(ctx);

	/* write framelength */
	if (set_auto_flicker(ctx, 0) || frame_length || !ctx->s_ctx.reg_addr_auto_extend)
	klimts5kjn5tele_write_frame_length(ctx, ctx->frame_length);
	else if (ctx->s_ctx.reg_addr_auto_extend)
	klimts5kjn5tele_write_frame_length(ctx, ctx->min_frame_length);
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

	for (i = 0; i < 3; i++) {
		if (rg_shutters[i]) {
			if (ctx->s_ctx.reg_addr_exposure[i].addr[2]) {
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[0],
					(rg_shutters[i] >> 16) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[1],
					(rg_shutters[i] >> 8) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[2],
					rg_shutters[i] & 0xFF);
			} else {
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[0],
					(rg_shutters[i] >> 8) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[1],
					rg_shutters[i] & 0xFF);
			}
		}
	}
	DRV_LOG(ctx, "exp[0x%x/0x%x/0x%x], fll(input/output):%u/%u, flick_en:%d\n",
		rg_shutters[0], rg_shutters[1], rg_shutters[2],
		frame_length, ctx->frame_length, ctx->autoflicker_en);
	if (!ctx->ae_ctrl_gph_en) {
		if (gph)
			ctx->s_ctx.s_gph((void *)ctx, 0);
		commit_i2c_buffer(ctx);
	}
	/* group hold end */

	return 0;
}

void klimts5kjn5tele_set_long_exposure(struct subdrv_ctx *ctx)
{
	u32 shutter = ctx->exposure[IMGSENSOR_STAGGER_EXPOSURE_LE];
	u32 max_shutter_reg_value = MAX_FL_REG_VALUE - ctx->s_ctx.exposure_margin;
	u16 l_shift = 0;
	u16 f_shift = 0;

	if (shutter > max_shutter_reg_value) {
		if (ctx->s_ctx.long_exposure_support == FALSE) {
			DRV_LOGE(ctx, "sensor no support of exposure lshift!\n");
			return;
		}
		if (ctx->s_ctx.reg_addr_exposure_lshift == PARAM_UNDEFINED) {
			DRV_LOGE(ctx, "please implement lshift register address\n");
			return;
		}
		while (shutter >= max_shutter_reg_value) {
			shutter >>= 1;
			l_shift += 1;
		}
		ctx->frame_length = shutter + ctx->s_ctx.exposure_margin;
		DRV_LOG(ctx, "jn5_long exposure lshift %u times, ctx->frame_length is %d\n", l_shift, ctx->frame_length);
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_exposure_lshift, l_shift);
		if (ctx->s_ctx.reg_addr_framelength_lshift != PARAM_UNDEFINED) {
			set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_framelength_lshift, l_shift);
		}
		ctx->l_shift = l_shift;
		/* Frame exposure mode customization for LE*/
		ctx->ae_frm_mode.frame_mode_1 = IMGSENSOR_AE_MODE_SE;
		ctx->ae_frm_mode.frame_mode_2 = IMGSENSOR_AE_MODE_SE;
		ctx->current_ae_effective_frame = 2;
	} else if (ctx->frame_length > MAX_FL_REG_VALUE) {
		while (ctx->frame_length >= MAX_FL_REG_VALUE) {
			ctx->frame_length >>= 1;
			f_shift += 1;
		}
			set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_framelength_lshift, f_shift);
		DRV_LOG(ctx, "jn5 f_shift %u times, ctx->frame_length is %d\n", f_shift, ctx->frame_length);
	} else {
		DRV_LOG(ctx, "jn5 lshift %u times, ctx->frame_length is %d\n", l_shift, ctx->frame_length);
		if (ctx->s_ctx.reg_addr_exposure_lshift != PARAM_UNDEFINED) {
			set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_exposure_lshift, l_shift);
			ctx->l_shift = l_shift;
		}
		if (ctx->s_ctx.reg_addr_framelength_lshift != PARAM_UNDEFINED) {
			set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_framelength_lshift, f_shift);
		}
	}
	ctx->exposure[IMGSENSOR_STAGGER_EXPOSURE_LE] = shutter;
}

void klimts5kjn5tele_write_lens(struct subdrv_ctx *ctx)
{
	u16 len_macro = 0;
	u16 len_inf = 0;
	u8 *pbuf = NULL;
	u16 len_position = 0;
	struct eeprom_info_struct *info = ctx->s_ctx.eeprom_info;

	pbuf = info[0].preload_pdc_table;
	len_macro=pbuf[0]<<8|pbuf[1];
	len_inf=pbuf[4]<<8|pbuf[5];
	DRV_LOG_MUST(ctx,"len_macro(%d),len_inf(%d)\n",len_macro,len_inf);

	ctx->i2c_client->addr = 0x18;
	ctx->i2c_write_id = 0x18;

	len_position = subdrv_i2c_rd_u8_u8(ctx, 0x84) << 8 | subdrv_i2c_rd_u8_u8(ctx, 0x85);
	len_position = (len_position >> 2) & 0x3fff;
	DRV_LOG_MUST(ctx,"len_position(%d)\n",len_position);

	ctx->i2c_client->addr = 0x5A;
	ctx->i2c_write_id = 0x5A;

	if(len_position > len_inf) {
		len_position = ((len_position - len_inf) * 1023) / (len_macro - len_inf);
	} else {
		DRV_LOG_MUST(ctx,"len_position(%d),len_inf(%d)\n",len_position,len_inf);
		return;
	}

	DRV_LOG_MUST(ctx,"len_position(0x%04x)\n",len_position);

	len_position = ((len_position & 0xff) << 8 )|((len_position & 0xff00) >> 8);

	DRV_LOG_MUST(ctx,"len_position(0x%04x)\n",len_position);

	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x2001);
	subdrv_i2c_wr_u16(ctx, 0x2566, len_position);
	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);

}

static int klimts5kjn5tele_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	enum SENSOR_SCENARIO_ID_ENUM scenario_id;
	enum SENSOR_SCENARIO_ID_ENUM current_scenario_id;
	struct mtk_hdr_ae *ae_ctrl = NULL;
	u64 *feature_data = (u64 *)para;
	u32 exp_cnt = 0;
	u32 retLen = 0;
	u32 frame_length_in_lut[IMGSENSOR_STAGGER_EXPOSURE_CNT] = {0};

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
	DRV_LOG_MUST(ctx, "E: set seamless switch %u %u\n", ctx->current_scenario_id, scenario_id);
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

	klimts5kjn5tele_update_mode_info(ctx, scenario_id);

	if (ctx->s_ctx.mode[scenario_id].hdr_mode == HDR_RAW_STAGGER){
		ctx->s_ctx.reg_addr_exposure[0].addr[0] = 0x0226;
		ctx->s_ctx.reg_addr_exposure[0].addr[1] = 0x0227;
		ctx->s_ctx.reg_addr_exposure[2].addr[0] = 0x0202;
		ctx->s_ctx.reg_addr_exposure[2].addr[1] = 0x0203;
		ctx->s_ctx.reg_addr_ana_gain[0].addr[0] = 0x0206;
		ctx->s_ctx.reg_addr_ana_gain[0].addr[1] = 0x0207;
		ctx->s_ctx.reg_addr_ana_gain[2].addr[0] = 0x0204;
		ctx->s_ctx.reg_addr_ana_gain[2].addr[1] = 0x0205;
	} else {
		ctx->s_ctx.reg_addr_exposure[0].addr[0] = 0x0202;
		ctx->s_ctx.reg_addr_exposure[0].addr[1] = 0x0203;
		ctx->s_ctx.reg_addr_exposure[2].addr[0] = 0x0226;
		ctx->s_ctx.reg_addr_exposure[2].addr[1] = 0x0227;
		ctx->s_ctx.reg_addr_ana_gain[0].addr[0] = 0x0204;
		ctx->s_ctx.reg_addr_ana_gain[0].addr[1] = 0x0205;
		ctx->s_ctx.reg_addr_ana_gain[2].addr[0] = 0x0206;
		ctx->s_ctx.reg_addr_ana_gain[2].addr[1] = 0x0207;
	}

	subdrv_i2c_wr_u8(ctx, 0x0104, 0x01);

	klimts5kjn5tele_set_awb_gain(ctx, (u8 *)&g_last_awb_gain, &retLen);
	i2c_table_write(ctx,
		ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table,
		ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_len);

	if ((ctx->current_fps > 290) && (scenario_id == SENSOR_SCENARIO_ID_CUSTOM13 ||
		scenario_id == SENSOR_SCENARIO_ID_CUSTOM20) ) {
		set_max_framerate_by_scenario(ctx, scenario_id, 300);
	}

	if (ae_ctrl) {
		switch (ctx->s_ctx.mode[scenario_id].hdr_mode) {
		case HDR_RAW_LBMF:
			set_multi_shutter_frame_length_in_lut(ctx,
				(u64 *)&ae_ctrl->exposure, exp_cnt, 0, frame_length_in_lut);
			set_multi_gain_in_lut(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			break;
		case HDR_RAW_STAGGER:
			klimts5kjn5tele_set_multi_shutter_frame_length_dol(ctx, (u64 *)&ae_ctrl->exposure, exp_cnt, 0);
			set_multi_gain(ctx, (u32 *)&ae_ctrl->gain, exp_cnt);
			break;
		case HDR_RAW_DCG_COMPOSE:
			set_multi_shutter_frame_length(ctx, (u64 *)&ae_ctrl->exposure, 1, 0);
			klimts5kjn5tele_set_gain(ctx, (u8 *)&ae_ctrl->gain.le_gain,len);
			break;
		default:
			klimts5kjn5tele_set_shutter_frame_length(ctx, (u8 *)&ae_ctrl->exposure.le_exposure,len);
			set_gain(ctx, ae_ctrl->gain.le_gain);
			break;
		}
	}

	subdrv_i2c_wr_u8(ctx, 0x0104, 0x00);

	if(ctx->s_ctx.mode[scenario_id].imgsensor_winsize_info.w2_tg_size == 8192)
		klimts5kjn5tele_write_lens(ctx);

	ctx->fast_mode_on = TRUE;
	ctx->ref_sof_cnt = ctx->sof_cnt;
	ctx->is_seamless = FALSE;
	DRV_LOG_MUST(ctx, "X: set seamless switch done\n");
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

static void set_mi_pre_init(void *arg)
{
  	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

// add for af start
	ctx->i2c_client->addr = 0x18;
	ctx->i2c_write_id = 0x18;

	subdrv_i2c_wr_u8_u8(ctx, 0x02, 0x40);// sleep mode

	ctx->i2c_client->addr = 0x5A;
	ctx->i2c_write_id = 0x5A;
// add for af end

	DRV_LOG_MUST(ctx, "Enter %s\n", __FUNCTION__);


}

static void set_mi_init_setting_seq(void *arg)
{
  	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	DRV_LOG_MUST(ctx, "Enter %s\n", __FUNCTION__);
	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);
	subdrv_i2c_wr_u16(ctx, 0x0000, 0x000E);
	subdrv_i2c_wr_u16(ctx, 0x0000, 0x38E5);
	subdrv_i2c_wr_u16(ctx, 0x6018, 0x0001);
	subdrv_i2c_wr_u16(ctx, 0x7002, 0x0408);
	subdrv_i2c_wr_u16(ctx, 0x6014, 0x0001);
	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x2002);
	subdrv_i2c_wr_u16(ctx, 0x1E92, 0x8000);
	subdrv_i2c_wr_u16(ctx, 0x1E84, 0x282B);
	subdrv_i2c_wr_u16(ctx, 0x1E86, 0x0320);
	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);
	subdrv_i2c_wr_u16(ctx, 0x7002, 0x0008);
	mdelay(5);
	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x2000);
	subdrv_i2c_wr_u16(ctx, 0xDCF0, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x2001);
	subdrv_i2c_wr_u16(ctx, 0x3C60, 0x0000);

}

static int get_csi_param(struct subdrv_ctx *ctx,
	enum SENSOR_SCENARIO_ID_ENUM scenario_id,
	struct mtk_csi_param *csi_param)
{
	switch (scenario_id) {
	default:
		csi_param->cdr_delay_enable = 1;
		csi_param->cdr_delay        = 12;
		break;
	}
	DRV_LOG(ctx, "scenario_id:%u, cdr param custom:%d/%d\n", scenario_id, csi_param->cdr_delay_enable, csi_param->cdr_delay);
	return 0;
}

static int tele_vsync_notify(struct subdrv_ctx *ctx, unsigned int sof_cnt)
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
};


static int klimts5kjn5tele_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 gain = *((u32 *)para);
	u32 rg_gain;
	bool gph = !ctx->is_seamless && (ctx->s_ctx.s_gph != NULL);
	DRV_LOG(ctx, "gain_total[0x%x]\n", gain);
	/* check boundary of gain */
	gain = max(gain,
		ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_ana_gain_range[0].min);
	gain = min(gain,
		ctx->s_ctx.mode[ctx->current_scenario_id].multi_exposure_ana_gain_range[0].max);

	DRV_LOG(ctx, "mode id = %d hdr_mode = %d",ctx->current_scenario_id,ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode);
	if(ctx->s_ctx.mode[ctx->current_scenario_id].hdr_mode == HDR_RAW_DCG_COMPOSE ){
		gain = gain/4;
	}
	/* mapping of gain to register value */
	rg_gain = get_gain2reg(gain);

	DRV_LOG(ctx, "rg_gain_total[0x%x]\n", rg_gain);
	/* restore gain */
	memset(ctx->ana_gain, 0, sizeof(ctx->ana_gain));
	ctx->ana_gain[0] = gain;
	/* group hold start */
	if (gph && !ctx->ae_ctrl_gph_en)
		ctx->s_ctx.s_gph((void *)ctx, 1);
	/* write gain */
	set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_ana_gain[0].addr[0],
		(rg_gain >> 8) & 0xFF);
	set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_ana_gain[0].addr[1],
		rg_gain & 0xFF);

	DRV_LOG(ctx, "gain[0x%x]\n", rg_gain);
	if (gph)
		ctx->s_ctx.s_gph((void *)ctx, 0);
	commit_i2c_buffer(ctx);
	/* group hold end */
	return 0;
}

static int klimts5kjn5tele_set_awb_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *) para;
	struct SET_SENSOR_AWB_GAIN *awb_gain = (struct SET_SENSOR_AWB_GAIN *)feature_data;
	MUINT32 r_Gain = awb_gain->ABS_GAIN_R << 1;
	MUINT32 g_Gain = awb_gain->ABS_GAIN_GR << 1;
	MUINT32 b_Gain = awb_gain->ABS_GAIN_B << 1;

	g_last_awb_gain = *awb_gain;

	if (r_Gain == 0 || g_Gain == 0 || b_Gain == 0) {
		DRV_LOG(ctx, "error awb gain [r/g/b]: 0x%x 0x%x 0x%x\n",
			r_Gain, g_Gain, b_Gain);
		return 0;
	}

	// set awb gain
	subdrv_i2c_wr_u16(ctx, 0x0D86, b_Gain); //B Gain [14:8]
	subdrv_i2c_wr_u16(ctx, 0x0D84, g_Gain); //G Gain [14:8]
	subdrv_i2c_wr_u16(ctx, 0x0D82, r_Gain); //R Gain [7:0]

	DRV_LOG(ctx, "awb gain [r/g/b]: 0x%x 0x%x 0x%x\n",
		r_Gain, g_Gain, b_Gain);

	return 0;
}

