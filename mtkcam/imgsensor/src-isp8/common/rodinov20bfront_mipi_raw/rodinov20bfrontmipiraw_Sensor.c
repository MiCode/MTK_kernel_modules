// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *     RODINOV20BFRONTmipiraw_Sensor.c
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include "rodinov20bfrontmipiraw_Sensor.h"


#define DEBUG_LOG_EN 0
#define PFX "RODINOV20BFRONT_camera_sensor"
#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_DEBUG(...) do { if ((DEBUG_LOG_EN)) LOG_INF(__VA_ARGS__); } while (0)

static const char * const clk_names[] = {
	ADAPTOR_CLK_NAMES
};

static const char * const reg_names[] = {
	ADAPTOR_REGULATOR_NAMES
};

static const char * const state_names[] = {
	ADAPTOR_STATE_NAMES
};

static int init_ctx(
	struct subdrv_ctx *ctx, struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);
static int get_sensor_temperature(void *arg);
static u16 get_gain2reg(u32 gain);
static void set_group_hold(void *arg, u8 en);
#if EEPROM_READY
static void set_sensor_cali(void *arg);
#endif
static int set_streaming_control(void *arg, bool enable);
static int rodinov20bfront_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{
		.feature_id = SENSOR_FEATURE_SET_TEST_PATTERN,
		.func = rodinov20bfront_set_test_pattern
	},
};

static struct eeprom_info_struct eeprom_info[] = {
#if EEPROM_READY
	{
		.header_id = 0x10, //aac
		.addr_header_id = 0x01,
		.i2c_write_id = 0xA2,
	},
	{
		.header_id = 0x01,
		.addr_header_id = 0x01,
		.i2c_write_id = 0xA2,
	},
#else
	{0},
#endif
};


static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 2592,
			.vsize = 1944,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 2592,
			.vsize = 1944,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 2560,
			.vsize = 1440,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_hs[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 1280,
			.vsize = 720,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_slim[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 2592,
			.vsize = 1944,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 2560,
			.vsize = 1440,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 640,
			.vsize = 480,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 640,
			.vsize = 480,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	}
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2a,
			.hsize = 640,
			.vsize = 480,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	}
};

static struct subdrv_mode_struct mode_struct[] = {
	{	/* mode 0 : preview 2592x1944@30fps */
		.mode_setting_table = rodinov20bfront_preview_setting,
		.mode_setting_len = ARRAY_SIZE(rodinov20bfront_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 50000000,
		.linelength = 500,
		.framelength = 3333,
		.max_framerate = 300,
		.mipi_pixel_rate = 334520000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 5184,
			.full_h = 3888,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 5184,
			.h0_size = 3888,
			.scale_w = 2592,
			.scale_h = 1944,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2592,
			.h1_size = 1944,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2592,
			.h2_tg_size = 1944,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
	},
	{	/* mode 1 : same as preview 2592x1944@30fps */
		.mode_setting_table = rodinov20bfront_preview_setting,
		.mode_setting_len = ARRAY_SIZE(rodinov20bfront_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 50000000,
		.linelength = 500,
		.framelength = 3333,
		.max_framerate = 300,
		.mipi_pixel_rate = 334520000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 5184,
			.full_h = 3888,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 5184,
			.h0_size = 3888,
			.scale_w = 2592,
			.scale_h = 1944,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2592,
			.h1_size = 1944,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2592,
			.h2_tg_size = 1944,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_cap,
		.num_entries = ARRAY_SIZE(frame_desc_cap),
	},
	{	/* mode 2 : normal video 2560x1440@30fps */
		.mode_setting_table = rodinov20bfront_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(rodinov20bfront_normal_video_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 50000000,
		.linelength = 500,
		.framelength = 3333,
		.max_framerate = 300,
		.mipi_pixel_rate = 334520000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 5184,
			.full_h = 3888,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 5184,
			.h0_size = 3888,
			.scale_w = 2592,
			.scale_h = 1944,
			.x1_offset = 16,
			.y1_offset = 252,
			.w1_size = 2560,
			.h1_size = 1440,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2560,
			.h2_tg_size = 1440,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
	},
	{	/* mode : 3 hs_video 720p@120fps */
		.mode_setting_table = rodinov20bfront_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(rodinov20bfront_hs_video_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 50000000,
		.linelength = 500,
		.framelength = 833,
		.max_framerate = 1200,
		.mipi_pixel_rate = 334520000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 5184,
			.full_h = 3888,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 5184,
			.h0_size = 3888,
			.scale_w = 1296,
			.scale_h = 972,
			.x1_offset = 8,
			.y1_offset = 126,
			.w1_size = 1280,
			.h1_size = 720,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1280,
			.h2_tg_size = 720,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_hs,
		.num_entries = ARRAY_SIZE(frame_desc_hs),
	},
	{	/* mode 4 : same as preview 2592x1944@30fps */
		.mode_setting_table = rodinov20bfront_preview_setting,
		.mode_setting_len = ARRAY_SIZE(rodinov20bfront_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 50000000,
		.linelength = 500,
		.framelength = 3333,
		.max_framerate = 300,
		.mipi_pixel_rate = 334520000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 5184,
			.full_h = 3888,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 5184,
			.h0_size = 3888,
			.scale_w = 2592,
			.scale_h = 1944,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2592,
			.h1_size = 1944,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2592,
			.h2_tg_size = 1944,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_slim,
		.num_entries = ARRAY_SIZE(frame_desc_slim),
	},
	{	/* mode 5 : video 60fps 2560x1440@60FPS */
		.mode_setting_table = rodinov20bfront_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(rodinov20bfront_custom1_setting),
		.mode_setting_table_for_md = PARAM_UNDEFINED,
		.mode_setting_len_for_md = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 50000000,
		.linelength = 500,
		.framelength = 1666,
		.max_framerate = 600,
		.mipi_pixel_rate = 334520000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 5184,
			.full_h = 3888,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 5184,
			.h0_size = 3888,
			.scale_w = 2592,
			.scale_h = 1944,
			.x1_offset = 16,
			.y1_offset = 252,
			.w1_size = 2560,
			.h1_size = 1440,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2560,
			.h2_tg_size = 1440,
		},
		.aov_mode = 0,
		.s_dummy_support = 0,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
	},
	{	/* mode : 6  640x480@10FPS */
		.mode_setting_table = rodinov20bfront_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(rodinov20bfront_custom2_setting),
		.mode_setting_table_for_md = rodinov20bfront_md1_setting,
		.mode_setting_len_for_md = ARRAY_SIZE(rodinov20bfront_md1_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 50000000,
		.linelength = 580,
		.framelength = 8620,
		.max_framerate = 100,
		.mipi_pixel_rate = 100000000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 5184,
			.full_h = 3888,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 5184,
			.h0_size = 3888,
			.scale_w = 648,
			.scale_h = 486,
			.x1_offset = 4,
			.y1_offset = 3,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x50,
			.dphy_clk_settle=0x5,
			.dphy_data_settle =0x5,
        },
		.aov_mode = 1,
		.rosc_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_VIEWING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_VIEWING_MODE_RAW_MONO,
	},
	{	/* mode : 7  640x480@20FPS */
		.mode_setting_table = rodinov20bfront_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(rodinov20bfront_custom3_setting),
		.mode_setting_table_for_md = rodinov20bfront_md1_setting,
		.mode_setting_len_for_md = ARRAY_SIZE(rodinov20bfront_md1_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 50000000,
		.linelength = 580,
		.framelength = 4310,
		.max_framerate = 200,
		.mipi_pixel_rate = 100000000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 5184,
			.full_h = 3888,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 5184,
			.h0_size = 3888,
			.scale_w = 648,
			.scale_h = 486,
			.x1_offset = 4,
			.y1_offset = 3,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x50,
			.dphy_clk_settle=0x5,
			.dphy_data_settle =0x5,
        },
		.aov_mode = 1,
		.rosc_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_VIEWING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_VIEWING_MODE_RAW_MONO,
	},
	{	/* mode : 8  640x480@30FPS */
		.mode_setting_table = rodinov20bfront_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(rodinov20bfront_custom4_setting),
		.mode_setting_table_for_md = rodinov20bfront_md1_setting,
		.mode_setting_len_for_md = ARRAY_SIZE(rodinov20bfront_md1_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 50000000,
		.linelength = 580,
		.framelength = 2873,
		.max_framerate = 300,
		.mipi_pixel_rate = 100000000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 5184,
			.full_h = 3888,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 5184,
			.h0_size = 3888,
			.scale_w = 648,
			.scale_h = 486,
			.x1_offset = 4,
			.y1_offset = 3,
			.w1_size = 640,
			.h1_size = 480,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 640,
			.h2_tg_size = 480,
		},
		.csi_param = {
			.not_fixed_trail_settle =1,
			.dphy_trail = 0x50,
			.dphy_clk_settle=0x5,
			.dphy_data_settle =0x5,
        },
		.aov_mode = 1,
		.rosc_mode = 1,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_VIEWING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_VIEWING_MODE_RAW_MONO,
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = RODINOV20BFRONT_SENSOR_ID,
	.reg_addr_sensor_id = {0x300B, 0x300C},
	.i2c_addr_table = {0x20, 0xFF},
	.i2c_burst_write_support = FALSE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {5184, 3888},
	.mirror = IMAGE_NORMAL,
	.mclk = 26,
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 15.96875,
	.ana_gain_type = 1,
	.ana_gain_step = 64,
	.ana_gain_table = rodinov20bfront_ana_gain_table,
	.ana_gain_table_size = sizeof(rodinov20bfront_ana_gain_table),
	.tuning_iso_base = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 2,
	.exposure_max = 0xFFFF- 4,
	.exposure_step = 1,
	.exposure_margin = 4,

	.frame_length_max = 0xFFFF ,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 1500000,

	.pdaf_type = PDAF_SUPPORT_NA,
	.hdr_type = HDR_SUPPORT_NA,
	.seamless_switch_support = FALSE,
	.temperature_support = TRUE,

	.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,
#if EEPROM_READY
	.s_cali = set_sensor_cali,
#else
	.s_cali = NULL,
#endif	

	.s_streaming_control = set_streaming_control,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = PARAM_UNDEFINED,
	.reg_addr_exposure = {
		{0x3500, 0x3501, 0x3502},
		{0x3540, 0x3541, 0x3542}
	},
	.long_exposure_support = FALSE,
	.reg_addr_exposure_lshift = PARAM_UNDEFINED,
	.reg_addr_ana_gain = {
		{0x3508, 0x3509},
		//{0x3548, 0x3549} for hdr
	},
	.reg_addr_frame_length = {0x380E, 0x380F},		

	.reg_addr_temp_en = 0x4D12,
	.reg_addr_temp_read = 0x4D13,
	.reg_addr_auto_extend = PARAM_UNDEFINED,
	.reg_addr_frame_count = 0x387F,

	.init_setting_table = rodinov20bfront_init_setting,
	.init_setting_len = ARRAY_SIZE(rodinov20bfront_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),

	.checksum_value = 0xAF3E324F,
	.aov_sensor_support = TRUE,
	.sensor_mode_ops = 0,
	.sensor_debug_sensing_ut_on_scp = TRUE,
	.sensor_debug_dphy_global_timing_continuous_clk = FALSE,
	.reg_addr_aov_mode_mirror_flip = 0x3874,//TODO
	.init_in_open = TRUE,
	.streaming_ctrl_imp = TRUE,
};

static struct subdrv_ops ops = {
	.init_ctx = init_ctx,
	.open = common_open,
	.get_id = common_get_imgsensor_id,
	.vsync_notify = vsync_notify,
	.get_csi_param = common_get_csi_param,
	.get_temp = common_get_temp,
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = common_control,
	.feature_control = common_feature_control,
	.close = common_close,
	.get_frame_desc = common_get_frame_desc,
	.update_sof_cnt = common_update_sof_cnt,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_MCLK, {26}, 0},
	{HW_ID_RST, {0}, 1000},
	{HW_ID_MCLK_DRIVING_CURRENT, {4}, 1000},
	{HW_ID_AVDD1, {1800000, 1800000}, 0},
	{HW_ID_DOVDD, {1800000}, 0},
	{HW_ID_AVDD, {2800000}, 0},
	{HW_ID_DVDD, {1200000}, 1000},
	{HW_ID_RST, {1}, 5000}
};


const struct subdrv_entry rodinov20bfront_mipi_raw_entry = {
	.name = "rodinov20bfront_mipi_raw",
	.id = RODINOV20BFRONT_SENSOR_ID,
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

	DRV_LOG_MUST(ctx, "temperature: %d degrees\n", temperature_convert);
	return temperature_convert;
}

static u16 get_gain2reg(u32 gain)
{
	return gain * 256 / BASEGAIN;
}

static void set_group_hold(void *arg, u8 en)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	if (en) {
		set_i2c_buffer(ctx, 0x3208, 0x00);
	} else {
		set_i2c_buffer(ctx, 0x3208, 0x10);
		set_i2c_buffer(ctx, 0x3208, 0xA0);
	}
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
			subdrv_i2c_wr_u8(ctx, 0x86A9, 0x4E);
			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
			subdrv_i2c_wr_u8(ctx, 0x32D2, 0x01);
			DRV_LOG(ctx, "set QSC calibration data done.");
		} else {
			subdrv_i2c_wr_u8(ctx, 0x32D2, 0x00);
		}
	}
}
#endif

static int set_streaming_control(void *arg, bool enable)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	int ret = 0;

	DRV_LOG(ctx, "E!\n");

	DRV_LOG_MUST(ctx,
		"streaming_enable(0=Sw Standby,1=streaming):(%d)\n", enable);

	if (ctx->s_ctx.mode[ctx->current_scenario_id].aov_mode) {
		DRV_LOG_MUST(ctx,
			"AOV mode(%d) streaming control on apmcu side\n",
			ctx->sensor_mode);
	}

	if (enable) {
		subdrv_i2c_wr_u8(ctx, 0x0100, 0X01);// stream on
		DRV_LOG_MUST(ctx,
			"MODE_SEL(%08x)\n", subdrv_i2c_rd_u8(ctx, 0x0100));
		ctx->test_pattern = 0;
	} else {
		subdrv_i2c_wr_u8(ctx, 0x0100, 0x00);// stream off
	}

	return ret;
}

static int rodinov20bfront_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	switch (mode) {
	case 2:
		subdrv_i2c_wr_u8(ctx, 0x5000, 0x81);
		subdrv_i2c_wr_u8(ctx, 0x5001, 0x00);
		subdrv_i2c_wr_u8(ctx, 0x5002, 0x92);
		subdrv_i2c_wr_u8(ctx, 0x5081, 0x01);
		break;
	case 5:
		subdrv_i2c_wr_u8(ctx, 0x3019, 0xF0);
		subdrv_i2c_wr_u8(ctx, 0x4308, 0x01);
		break;
	default:
		break;
	}

	if (mode != ctx->test_pattern)
		switch (ctx->test_pattern) {
		case 2:
			subdrv_i2c_wr_u8(ctx, 0x5000, 0xCB);
			subdrv_i2c_wr_u8(ctx, 0x5001, 0x43);
			subdrv_i2c_wr_u8(ctx, 0x5002, 0x9E);
			subdrv_i2c_wr_u8(ctx, 0x5081, 0x00);
			break;
		case 5:
			subdrv_i2c_wr_u8(ctx, 0x3019, 0xD2);
			subdrv_i2c_wr_u8(ctx, 0x4308, 0x00);
			break;
		default:
			break;
		}

	ctx->test_pattern = mode;
	return ERROR_NONE;

}
