
// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *     klimts5kkdsfrontmipiraw_Sensor.c
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
#include "klimts5kkdsfrontmipiraw_Sensor.h"
#include <linux/timer.h>
#include "adaptor-ctrls.h"
#define DEBUG_LOG_EN 0
#define PFX "KLIMTS5KKDSFRONT_camera_sensor"
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
static void set_mi_init_setting_seq(void *arg);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);
static u16 get_gain2reg(u32 gain);
static void set_group_hold(void *arg, u8 en);
static int get_sensor_temperature(void *arg);
#if EEPROM_READY
static void set_sensor_cali(void *arg);
#endif
static int set_streaming_control(void *arg, bool enable);
static int klimts5kkdsfront_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
/* STRUCT */
static struct subdrv_feature_control feature_control_list[] = {
	{
		.feature_id = SENSOR_FEATURE_SET_TEST_PATTERN,
		.func = klimts5kkdsfront_set_test_pattern
	},
};
static struct eeprom_info_struct eeprom_info[] = {
#if EEPROM_READY
	{
		// .header_id = 0x0059000B,
		// .addr_header_id = 0x00000006,
		.i2c_write_id = 0xA2,
		.qsc_support = FALSE,
		.qsc_size = 1560,
		.addr_qsc = 0x1900,
		.sensor_reg_addr_qsc = 0xC800, // useless
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
			.hsize = 3280,
			.vsize = 2464,
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
			.hsize = 3280,
			.vsize = 2464,
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
			.hsize = 3280,
			.vsize = 1848,
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
			.hsize = 1640,
			.vsize = 928,
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
			.hsize = 3280,
			.vsize = 2464,
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
			.hsize = 3280,
			.vsize = 1848,
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
			.hsize = 816,
			.vsize = 612,
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
			.hsize = 816,
			.vsize = 612,
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
			.hsize = 816,
			.vsize = 612,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	}
};
static struct subdrv_mode_struct mode_struct[] = {
	{	/* mode 0 : preview 3280x2464@30fps */
		.mode_setting_table = klimts5kkdsfront_preview_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kkdsfront_preview_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560400000,
		.linelength = 3712,
		.framelength = 5024,
		.max_framerate = 300,
		.mipi_pixel_rate = 546000000,
		.framelength_step = 2,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 6560,
			.h0_size = 4928,
			.scale_w = 3280,
			.scale_h = 2464,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3280,
			.h1_size = 2464,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3280,
			.h2_tg_size = 2464,
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
	{	/* mode 1 : same as preview 3280x2464@30fps */
		.mode_setting_table = klimts5kkdsfront_preview_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kkdsfront_preview_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560400000,
		.linelength = 3712,
		.framelength = 5024,
		.max_framerate = 300,
		.mipi_pixel_rate = 546000000,
		.framelength_step = 2,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 6560,
			.h0_size = 4928,
			.scale_w = 3280,
			.scale_h = 2464,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3280,
			.h1_size = 2464,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3280,
			.h2_tg_size = 2464,
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
	{	/* mode 2 : normal video 3280x1848@30fps */
		.mode_setting_table = klimts5kkdsfront_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kkdsfront_normal_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560400000,
		.linelength = 3712,
		.framelength = 5024,
		.max_framerate = 300,
		.mipi_pixel_rate = 546000000,
		.framelength_step = 2,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 6560,
			.h0_size = 4928,
			.scale_w = 3280,
			.scale_h = 2464,
			.x1_offset = 0,
			.y1_offset = 308,
			.w1_size = 3280,
			.h1_size = 1848,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3280,
			.h2_tg_size = 1848,
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
	{	/* mode : 3 hs_video 1080p@120fps */
		.mode_setting_table = klimts5kkdsfront_hs_video_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kkdsfront_hs_video_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560400000,
		.linelength = 3728,
		.framelength = 1250,
		.max_framerate = 1200,
		.mipi_pixel_rate = 546000000,
		.framelength_step = 4,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 6560,
			.h0_size = 4928,
			.scale_w = 1640,
			.scale_h = 1232,
			.x1_offset = 0,
			.y1_offset = 152,
			.w1_size = 1640,
			.h1_size = 928,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1640,
			.h2_tg_size = 928,
		},
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
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
	{	/* mode 4 : same as preview 3280x2464@30fps */
		.mode_setting_table = klimts5kkdsfront_preview_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kkdsfront_preview_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560400000,
		.linelength = 3712,
		.framelength = 5024,
		.max_framerate = 300,
		.mipi_pixel_rate = 546000000,
		.framelength_step = 2,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 6560,
			.h0_size = 4928,
			.scale_w = 3280,
			.scale_h = 2464,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3280,
			.h1_size = 2464,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3280,
			.h2_tg_size = 2464,
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
	{	/* mode 5 : video 60fps 3280x1848@60FPS */
		.mode_setting_table = klimts5kkdsfront_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kkdsfront_custom1_setting),
		.mode_setting_table_for_md = PARAM_UNDEFINED,
		.mode_setting_len_for_md = PARAM_UNDEFINED,
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560400000,
		.linelength = 3712,
		.framelength = 2512,
		.max_framerate = 600,
		.mipi_pixel_rate = 546000000,
		.framelength_step = 2,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 6560,
			.h0_size = 4928,
			.scale_w = 3280,
			.scale_h = 2464,
			.x1_offset = 0,
			.y1_offset = 308,
			.w1_size = 3280,
			.h1_size = 1848,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3280,
			.h2_tg_size = 1848,
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
	{	/* mode : 6  812x612@10FPS */
		.mode_setting_table = klimts5kkdsfront_custom2_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kkdsfront_custom2_setting),
		.mode_setting_table_for_md = klimts5kkdsfront_md1_setting,
		.mode_setting_len_for_md = ARRAY_SIZE(klimts5kkdsfront_md1_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560400000,
		.linelength = 3728,
		.framelength = 15008,
		.max_framerate = 100,
		.mipi_pixel_rate = 227520000,
		.framelength_step = 2,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 6560,
			.h0_size = 4928,
			.scale_w = 820,
			.scale_h = 616,
			.x1_offset = 2,
			.y1_offset = 2,
			.w1_size = 816,
			.h1_size = 612,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 816,
			.h2_tg_size = 612,
		},
        .csi_param = {
 			.not_fixed_trail_settle = 1,
			.dphy_trail = 13,
        },
		.aov_mode = 1,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.s_dummy_support = 0,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
	},
	{	/* mode : 7  812x612@20FPS */
		.mode_setting_table = klimts5kkdsfront_custom3_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kkdsfront_custom3_setting),
		.mode_setting_table_for_md = klimts5kkdsfront_md1_setting,
		.mode_setting_len_for_md = ARRAY_SIZE(klimts5kkdsfront_md1_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560400000,
		.linelength = 3728,
		.framelength = 7504,
		.max_framerate = 200,
		.mipi_pixel_rate = 227520000,
		.framelength_step = 2,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 6560,
			.h0_size = 4928,
			.scale_w = 820,
			.scale_h = 616,
			.x1_offset = 2,
			.y1_offset = 2,
			.w1_size = 816,
			.h1_size = 612,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 816,
			.h2_tg_size = 612,
		},
        .csi_param = {
 			.not_fixed_trail_settle = 1,
			.dphy_trail = 13,
        },
		.aov_mode = 1,
		.s_dummy_support = 0,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
	},
	{	/* mode : 8  812x612@30FPS */
		.mode_setting_table = klimts5kkdsfront_custom4_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kkdsfront_custom4_setting),
		.mode_setting_table_for_md = klimts5kkdsfront_md1_setting,
		.mode_setting_len_for_md = ARRAY_SIZE(klimts5kkdsfront_md1_setting),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560400000,
		.linelength = 3728,
		.framelength = 5002,
		.max_framerate = 300,
		.mipi_pixel_rate = 227520000,
		.framelength_step = 2,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 6560,
			.h0_size = 4928,
			.scale_w = 820,
			.scale_h = 616,
			.x1_offset = 2,
			.y1_offset = 2,
			.w1_size = 816,
			.h1_size = 612,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 816,
			.h2_tg_size = 612,
		},
        .csi_param = {
 			.not_fixed_trail_settle = 1,
			.dphy_trail = 13,
        },
		.aov_mode = 1,
		.s_dummy_support = 0,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].max = BASEGAIN * 16,
		.multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_LE].min = BASEGAIN,
		.ae_ctrl_support = IMGSENSOR_AE_CONTROL_SUPPORT_SENSING_MODE,
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_cus4,
		.num_entries = ARRAY_SIZE(frame_desc_cus4),
	},
	{	/* mode 9 : video 60fps 3280x1848@60FPS as mode 5*/
		.mode_setting_table = klimts5kkdsfront_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(klimts5kkdsfront_custom1_setting),
		.mode_setting_table_for_md = PARAM_UNDEFINED,
		.mode_setting_len_for_md = PARAM_UNDEFINED,
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 560400000,
		.linelength = 3712,
		.framelength = 2512,
		.max_framerate = 600,
		.mipi_pixel_rate = 546000000,
		.framelength_step = 2,
		.imgsensor_winsize_info = {
			.full_w = 6560,
			.full_h = 4928,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 6560,
			.h0_size = 4928,
			.scale_w = 3280,
			.scale_h = 2464,
			.x1_offset = 0,
			.y1_offset = 308,
			.w1_size = 3280,
			.h1_size = 1848,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3280,
			.h2_tg_size = 1848,
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
};
static struct subdrv_static_ctx static_ctx = {
	.sensor_id = KLIMTS5KKDSFRONT_SENSOR_ID,
	.reg_addr_sensor_id = {0x0000, 0x0001},
	.i2c_addr_table = {0x20, 0xFF},
	.i2c_burst_write_support = FALSE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_16,
	.eeprom_info = eeprom_info,
	.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {6560, 4928},
	.mirror = IMAGE_NORMAL,
	.mclk = 26,
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.ob_pedestal = 0x40,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 128,
	.ana_gain_type = 2,
	.ana_gain_step = 32,
	.ana_gain_table = klimts5kkdsfront_ana_gain_table,
	.ana_gain_table_size = sizeof(klimts5kkdsfront_ana_gain_table),
	.tuning_iso_base = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 5,
	.exposure_max = 0xFFFF- 11,
	.exposure_step = 2,
	.exposure_margin = 11,
	.frame_length_max = 0xFFFF,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 1866600,
	.pdaf_type = PDAF_SUPPORT_NA,
	.hdr_type = HDR_SUPPORT_NA,
	.seamless_switch_support = FALSE,
	.temperature_support = FALSE,
	.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	.s_gph = set_group_hold,
#if EEPROM_READY
	.s_cali = set_sensor_cali,
#else
	.s_cali = NULL,
#endif
	.s_streaming_control = set_streaming_control,
	.reg_addr_exposure = {{0x0202, 0x0203},},
	.reg_addr_ana_gain = {{0x0204, 0x0205},},
	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = PARAM_UNDEFINED,
	.long_exposure_support = FALSE,
	.reg_addr_exposure_lshift = PARAM_UNDEFINED,
	.reg_addr_temp_en = PARAM_UNDEFINED,
	.reg_addr_temp_read = PARAM_UNDEFINED,
	.reg_addr_auto_extend = PARAM_UNDEFINED,
	.reg_addr_frame_count = 0x0005,
	.s_mi_init_seq = set_mi_init_setting_seq,
	.init_setting_table = klimts5kkdsfront_init_setting,
	.init_setting_len = ARRAY_SIZE(klimts5kkdsfront_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.checksum_value = 0xAF3E324F,
	.aov_sensor_support = TRUE,
	.sensor_mode_ops = 0,
	.sensor_debug_sensing_ut_on_scp = TRUE,
	.sensor_debug_dphy_global_timing_continuous_clk = FALSE,
	.init_in_open = FALSE,
	.streaming_ctrl_imp = FALSE,
	.chk_s_off_sta = 1,
	.chk_s_off_end = 0,
	/* custom stream control delay timing for hw limitation (ms) */
	.custom_stream_ctrl_delay = 5,
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

static struct subdrv_pw_seq_entry aov_pw_seq[] = {
	{HW_ID_DVDD1,  {1300000, 1300000},  5000}, //ldo supply
	{HW_ID_RST, {0}, 1000},
	{HW_ID_DOVDD, {1800000, 1800000}, 0},
	{HW_ID_DVDD, {1050000, 1050000}, 1000},
	{HW_ID_AVDD, {2800000, 2800000}, 1000},
	{HW_ID_RST, {1}, 4000},
	{HW_ID_MCLK, {26, MCLK_ULPOSC}, 0},
	{HW_ID_MCLK_DRIVING_CURRENT, {4}, 9000},
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_DVDD1,  {1300000, 1300000},  5000}, //ldo supply
	{HW_ID_RST, {0}, 1000},
	{HW_ID_DOVDD, {1800000, 1800000}, 0},
	{HW_ID_DVDD, {1050000, 1050000}, 1000},
	{HW_ID_AVDD, {2800000, 2800000}, 1000},
	{HW_ID_RST, {1}, 4000},
	{HW_ID_MCLK, {26}, 0},
	{HW_ID_MCLK_DRIVING_CURRENT, {4}, 9000},
};
const struct subdrv_entry klimts5kkdsfront_mipi_raw_entry = {
	.name = SENSOR_DRVNAME_KLIMTS5KKDSFRONT_MIPI_RAW,
	.id = KLIMTS5KKDSFRONT_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
	.aov_pw_seq = aov_pw_seq,
	.aov_pw_seq_cnt = ARRAY_SIZE(aov_pw_seq),
};
 static void set_mi_init_setting_seq(void *arg)
{
  	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	DRV_LOG_MUST(ctx, "Enter %s\n", __FUNCTION__);
	subdrv_i2c_wr_u16(ctx, 0xFCFC, 0x4000);
	subdrv_i2c_wr_u16(ctx, 0x0000, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0x0000, 0x484F);
	subdrv_i2c_wr_u16(ctx, 0x6010, 0x0001);
	mdelay(8);
	subdrv_i2c_wr_u16(ctx, 0x6214, 0xFF7D);
	subdrv_i2c_wr_u16(ctx, 0x6218, 0x0000);
	subdrv_i2c_wr_u16(ctx, 0x6226, 0x0001);
	subdrv_i2c_wr_u16(ctx, 0x0A02, 0X0078);
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
	return gain * 32 / BASEGAIN;
}
static void set_group_hold(void *arg, u8 en)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	if (en)
		set_i2c_buffer(ctx, 0x0104, 0x01);
	else
		set_i2c_buffer(ctx, 0x0104, 0x00);
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
	u64 stream_ctrl_delay_timing = 0;
	u64 stream_ctrl_delay = 0;
	struct adaptor_ctx *_adaptor_ctx = NULL;
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
	int ret = 0;
	struct v4l2_subdev *sd = NULL;
	DRV_LOG(ctx, "E!\n");
	if (ctx->i2c_client)
		sd = i2c_get_clientdata(ctx->i2c_client);
	if (sd)
		_adaptor_ctx = to_ctx(sd);
	if (!_adaptor_ctx) {
		DRV_LOGE(ctx, "null _adaptor_ctx\n");
		return -1;
	}
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
		ctx->stream_ctrl_start_time = ktime_get_boottime_ns();
	} else {
		ctx->stream_ctrl_end_time = ktime_get_boottime_ns();
		if (ctx->s_ctx.custom_stream_ctrl_delay &&
			ctx->stream_ctrl_start_time && ctx->stream_ctrl_end_time) {
			stream_ctrl_delay_timing =
				(ctx->stream_ctrl_end_time - ctx->stream_ctrl_start_time) / 1000000;
			stream_ctrl_delay = (u64)get_sof_timeout(_adaptor_ctx, _adaptor_ctx->cur_mode) / 1000;
			DRV_LOG_MUST(ctx,
				"stream_ctrl_delay/stream_ctrl_delay_timing:%llums/%llums\n",
				stream_ctrl_delay,
				stream_ctrl_delay_timing);
			if (stream_ctrl_delay_timing < stream_ctrl_delay)
				mdelay(stream_ctrl_delay - stream_ctrl_delay_timing);
		}
		subdrv_i2c_wr_u8(ctx, 0x0100, 0X00);// stream off
	}
	return ret;
}
static int klimts5kkdsfront_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
       u32 mode = *((u32 *)para);
       if (mode != ctx->test_pattern)
               DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
       /* 1:Solid Color 2:Color Bar 5:Black */
       switch (mode){
       case 5:
               subdrv_i2c_wr_u16(ctx, 0x0600, 0x01); /*100% Color bar*/
               break;
       default:
               subdrv_i2c_wr_u16(ctx, 0x0600, 0x00); /*No pattern*/
               break;
       }
       ctx->test_pattern = mode;
       return ERROR_NONE;
}
