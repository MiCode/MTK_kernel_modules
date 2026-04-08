// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *     dashsc821csultramipiraw_Sensor.c
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
#include "dashsc821csultramipiraw_Sensor.h"


#define DEBUG_LOG_EN 0
#define PFX "DASHSC821CSULTRA_camera_sensor"
#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_DEBUG(...) do { if ((DEBUG_LOG_EN)) LOG_INF(__VA_ARGS__); } while (0)

static int init_ctx(
	struct subdrv_ctx *ctx, struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);
static u16 get_gain2reg(u32 gain);
static int set_streaming_control(void *arg, bool enable);
static int dashsc821csultra_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int dashsc821csultra_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int dashsc821csultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int dashsc821csultra_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int dashsc821csultra_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int dashsc821csultra_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id);


#define DASHSC821CSULTRA_SENSOR_GAIN_BASE           1024
#define DASHSC821CSULTRA_SENSOR_GAIN_MAX            15.75*1024
#define DASHSC821CSULTRA_SENSOR_GAIN_MAX_VALID_INDEX  6

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, dashsc821csultra_set_test_pattern},
	{SENSOR_FEATURE_SET_GAIN, dashsc821csultra_set_gain},
	{SENSOR_FEATURE_SET_ESHUTTER, dashsc821csultra_set_shutter},
	{SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME, dashsc821csultra_set_shutter_frame_length},
	{SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME,dashsc821csultra_set_multi_shutter_frame_length},
};

//mode 0 : preview 3264x2448@30fps
static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 2448,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	}
};

// mode 2 : normal video 3264x1840@30fps
static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 1840,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

// mode 3 : bokeh slave 3264x2448 @24fps
static struct mtk_mbus_frame_desc_entry frame_desc_bokeh[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 2448,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

// mode 4 : bokeh slave 2x 1632x1224 @24fps
static struct mtk_mbus_frame_desc_entry frame_desc_bokeh2x[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 1632,
			.vsize = 1224,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

// mode 5 : normal video  3264x2040@30fps
static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 3264,
			.vsize = 2040,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct subdrv_mode_struct mode_struct[] = {
	{	/* 0x12 mode 0 : preview 3264x2448@30fps */
		.mode_setting_table = dashsc821csultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc821csultra_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 132000000,
		.linelength = 1760,
		.framelength = 2500,
		.max_framerate = 300,
		.mipi_pixel_rate = 272000000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 3264,
			.h0_size = 2448,
			.scale_w = 3264,
			.scale_h = 2448,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 2448,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 2448,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
	},
	{	/* 0x12 mode 1 : preview 3264x2448@30fps */
		.mode_setting_table = dashsc821csultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc821csultra_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 132000000,
		.linelength = 1760,
		.framelength = 2500,
		.max_framerate = 300,
		.mipi_pixel_rate = 272000000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 3264,
			.h0_size = 2448,
			.scale_w = 3264,
			.scale_h = 2448,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 2448,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 2448,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
	},
	{	/* 0x15 mode 2 : normal video 3264x1840@30fps */
		.mode_setting_table = dashsc821csultra_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc821csultra_normal_video_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 132000000,
		.linelength = 1760,
		.framelength = 2500,
		.max_framerate = 300,
		.mipi_pixel_rate = 272000000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 0,
			.y0_offset = 304,
			.w0_size = 3264,
			.h0_size = 1840,
			.scale_w = 3264,
			.scale_h = 1840,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 1840,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 1840,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
	},
	{	/* 0x12 mode 3 : bokeh slave 3264x2448 @24fps */
		.mode_setting_table = dashsc821csultra_bokeh_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc821csultra_bokeh_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 132000000,
		.linelength = 1760,
		.framelength = 3125,
		.max_framerate = 240,
		.mipi_pixel_rate = 272000000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = 3264,
			.h0_size = 2448,
			.scale_w = 3264,
			.scale_h = 2448,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 2448,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 2448,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
		.frame_desc = frame_desc_bokeh,
		.num_entries = ARRAY_SIZE(frame_desc_bokeh),
	},
	{	/* 0x12 mode 4 : bokeh slave 2x 1632x1224 @24fps */
		.mode_setting_table = dashsc821csultra_bokeh2x_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc821csultra_bokeh2x_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 132000000,
		.linelength = 1760,
		.framelength = 3125,
		.max_framerate = 240,
		.mipi_pixel_rate = 272000000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 816,
			.y0_offset = 612,
			.w0_size = 1632,
			.h0_size = 1224,
			.scale_w = 1632,
			.scale_h = 1224,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 1632,
			.h1_size = 1224,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 1632,
			.h2_tg_size = 1224,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
		.frame_desc = frame_desc_bokeh2x,
		.num_entries = ARRAY_SIZE(frame_desc_bokeh2x),
	},
	{	/* 0x14 mode 5 : normal video  3264x2040@30fps */
		.mode_setting_table = dashsc821csultra_custom1_setting,
		.mode_setting_len = ARRAY_SIZE(dashsc821csultra_custom1_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 132000000,
		.linelength = 1760,
		.framelength = 2500,
		.max_framerate = 300,
		.mipi_pixel_rate = 272000000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 0,
			.y0_offset = 204,
			.w0_size = 3264,
			.h0_size = 2040,
			.scale_w = 3264,
			.scale_h = 2040,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 3264,
			.h1_size = 2040,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 3264,
			.h2_tg_size = 2040,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = DASHSC821CSULTRA_SENSOR_ID,
	.reg_addr_sensor_id = {0x3107, 0x3108},
	.i2c_addr_table = {0x6c, 0xFF},
	.i2c_burst_write_support = FALSE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.resolution = {3264, 2448},
	.mirror = IMAGE_NORMAL,
	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_lane_num = SENSOR_MIPI_2_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 15.75,
	.ana_gain_type = 1,
	.ana_gain_step = 1,
	.ana_gain_table = dashsc821csultra_ana_gain_table,
	.ana_gain_table_size = sizeof(dashsc821csultra_ana_gain_table),
	.tuning_iso_base = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 2,
	.exposure_max = 0x7FF0 - 8,
	.exposure_step = 1,
	.exposure_margin = 8,

	.frame_length_max = 0x7FF0,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 500000,

	.pdaf_type = PDAF_SUPPORT_NA,
	.hdr_type = HDR_SUPPORT_NA,
	.seamless_switch_support = FALSE,

	.g_gain2reg = get_gain2reg,

	.s_cali = NULL,

	.s_streaming_control = set_streaming_control,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = PARAM_UNDEFINED,
	.reg_addr_exposure = {
		{0x3e20, 0x3e00, 0x3e01, 0x3e02}
	},
	.long_exposure_support = FALSE,
	.reg_addr_exposure_lshift = PARAM_UNDEFINED,
	.reg_addr_ana_gain = {
		{0x3e08, 0x3e09},
	},
	.reg_addr_frame_length = {0x326D, 0x320e, 0x320f},

	.reg_addr_auto_extend = PARAM_UNDEFINED,
	.reg_addr_frame_count = 0x4869,
	.reg_addr_temp_en = PARAM_UNDEFINED,
	.reg_addr_temp_read = PARAM_UNDEFINED,


	.init_setting_table = dashsc821csultra_init_setting,
	.init_setting_len = ARRAY_SIZE(dashsc821csultra_init_setting),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 0,
	.chk_s_off_end = 0,
	.mi_i2c_type = 1,

	//TBD
	.checksum_value = 0xAF3E324F,
};

static struct subdrv_ops ops = {
	.init_ctx = init_ctx,
	.open = common_open,
	.get_id = dashsc821csultra_get_imgsensor_id,
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
	{HW_ID_MCLK,  {24},	   0},
	{HW_ID_RST,   {0},	   0},
	{HW_ID_DOVDD, {1800000,1800000}, 0},
	{HW_ID_DVDD,  {1200000,1200000}, 0},
	{HW_ID_AVDD,  {2800000,2800000}, 1000},
	{HW_ID_MCLK_DRIVING_CURRENT, {4}, 1000},
	{HW_ID_RST,   {1},	   5000},
};


const struct subdrv_entry dashsc821csultra_mipi_raw_entry = {
	.name = "dashsc821csultra_mipi_raw",
	.id = DASHSC821CSULTRA_SENSOR_ID,
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

static u16 get_gain2reg(u32 gain)
{
	return gain * 256 / BASEGAIN;
}

static void dashsc821csultra_gain2reg(const u64 total_analog_gain, u16* reg_gain_out, u8* analog_reg_gain, u8* digital_fine_reg_gain)
{
	u64 gain = total_analog_gain;
	u8 dig_gain = 0;

	if (gain < DASHSC821CSULTRA_SENSOR_GAIN_BASE) {
		gain = DASHSC821CSULTRA_SENSOR_GAIN_BASE;
	} else if (gain > DASHSC821CSULTRA_SENSOR_GAIN_MAX) {
		gain = DASHSC821CSULTRA_SENSOR_GAIN_MAX;
		dig_gain = 128;
	}

	if ((gain >= 1*DASHSC821CSULTRA_SENSOR_GAIN_BASE) && (gain < 2*DASHSC821CSULTRA_SENSOR_GAIN_BASE)) {
		*reg_gain_out = 0x00;
		*analog_reg_gain = gain * 32 / DASHSC821CSULTRA_SENSOR_GAIN_BASE;
		dig_gain = gain * 4 / (*analog_reg_gain);
	} else if ((gain >= 2*DASHSC821CSULTRA_SENSOR_GAIN_BASE) && (gain < 4*DASHSC821CSULTRA_SENSOR_GAIN_BASE)) {
		*reg_gain_out = 0x01;
		*analog_reg_gain = gain * 32 / (DASHSC821CSULTRA_SENSOR_GAIN_BASE * 2);
		dig_gain = gain * 2 / (*analog_reg_gain);
	} else if ((gain >= 4*DASHSC821CSULTRA_SENSOR_GAIN_BASE) && (gain < 8*DASHSC821CSULTRA_SENSOR_GAIN_BASE)) {
		*reg_gain_out = 0x03;
		*analog_reg_gain = gain * 32 / (DASHSC821CSULTRA_SENSOR_GAIN_BASE * 4);
		dig_gain = gain / (*analog_reg_gain);
	} else if ((gain >= 8*DASHSC821CSULTRA_SENSOR_GAIN_BASE) && (gain < 16*DASHSC821CSULTRA_SENSOR_GAIN_BASE)) {
		*reg_gain_out = 0x07;
		*analog_reg_gain = gain * 32 / (DASHSC821CSULTRA_SENSOR_GAIN_BASE * 8);
		dig_gain = gain / 2 / (*analog_reg_gain);
	} else {
		*reg_gain_out = 0x0f;
		*analog_reg_gain = gain * 32 / (DASHSC821CSULTRA_SENSOR_GAIN_BASE * 16);
		dig_gain = gain / 4 /(*analog_reg_gain);
	}

		*digital_fine_reg_gain = dig_gain;
}

static int dashsc821csultra_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u8 analog_reg_gain;
	u8 digital_fine_reg_gain;
	u16 reg_gain = 0;
	u64 gain = *(u64 *)para;

	dashsc821csultra_gain2reg(gain, &reg_gain, &analog_reg_gain, &digital_fine_reg_gain);
	set_i2c_buffer(ctx, 0x3E08, reg_gain);
	set_i2c_buffer(ctx, 0x3E09, analog_reg_gain);
	set_i2c_buffer(ctx, 0x3E07, digital_fine_reg_gain & 0xff);
	commit_i2c_buffer(ctx);
	DRV_LOG_MUST(ctx, "Gain_Debug: gain = %llu reg_gain 0x%x analog_gain = 0x%x, dgain = 0x%x\n",
		gain, reg_gain, analog_reg_gain, digital_fine_reg_gain);
	DRV_LOG(ctx, "Gain_Debug read again = 0x%x, dgain = 0x%x\n",
		subdrv_i2c_rd_u8(ctx, 0x3E08), subdrv_i2c_rd_u8(ctx, 0x3E07));
	return ERROR_NONE;
}
static int dashsc821csultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	return dashsc821csultra_set_shutter_frame_length(ctx, para, len);
}

static int dashsc821csultra_set_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 shutter = *((u64 *)para);
	u32 frame_length = 0;
	u32 fine_integ_line = 0;

	ctx->frame_length = frame_length ? frame_length : ctx->min_frame_length;
	check_current_scenario_id_bound(ctx);
	// check boundary of shutter
	fine_integ_line = ctx->s_ctx.mode[ctx->current_scenario_id].fine_integ_line;
	shutter = FINE_INTEG_CONVERT(shutter, fine_integ_line);
	shutter = max_t(u64, shutter, (u64)ctx->s_ctx.exposure_min);
	shutter = min_t(u64, shutter, (u64)ctx->s_ctx.exposure_max);
	// check boundary of framelength
	ctx->frame_length = max((u32)shutter + ctx->s_ctx.exposure_margin, ctx->min_frame_length);
	ctx->frame_length = min(ctx->frame_length, ctx->s_ctx.frame_length_max);
	// restore shutter
	memset(ctx->exposure, 0, sizeof(ctx->exposure));
	ctx->exposure[0] = (u32) shutter;
	// write framelength
	if (set_auto_flicker(ctx, 0) || frame_length || !ctx->s_ctx.reg_addr_auto_extend)
		write_frame_length(ctx, ctx->frame_length);

	shutter = 2 * shutter;
	// write shutter
	if (shutter > 0) {
		set_i2c_buffer(ctx, 0x3e20, (shutter >> 20) & 0x0F);
		set_i2c_buffer(ctx, 0x3e00, (shutter >> 12) & 0xFF);
		set_i2c_buffer(ctx, 0x3e01, (shutter >> 4)&0xFF);
		set_i2c_buffer(ctx, 0x3e02, (shutter<<4) & 0xF0);
	}
	DRV_LOG_MUST(ctx, "Shutter_Debug: exp[0x%x], fll(input/output):%u/%u, flick_en:%u\n",
		ctx->exposure[0], frame_length, ctx->frame_length, ctx->autoflicker_en);

	commit_i2c_buffer(ctx);
	return ERROR_NONE;
}

static int dashsc821csultra_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *) para;
	u64 *shutters = (u64 *)(* feature_data);
	u16 exp_cnt = (u16) (*(feature_data + 1));
	u16 frame_length = (u16) (*(feature_data + 2));

	int i = 0;
	int fine_integ_line = 0;
	u16 last_exp_cnt = 1;
	u32 calc_fl[4] = {0};
	int readout_diff = 0;
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
	/* - (1) previous se + previous me + current le */
	calc_fl[0] = (u32) shutters[0];
	for (i = 1; i < last_exp_cnt; i++)
		calc_fl[0] += ctx->exposure[i];
	calc_fl[0] += ctx->s_ctx.exposure_margin*exp_cnt*exp_cnt;

	/* - (2) current se + current me + current le */
	calc_fl[1] = (u32) shutters[0];
	for (i = 1; i < exp_cnt; i++)
		calc_fl[1] += (u32) shutters[i];
	calc_fl[1] += ctx->s_ctx.exposure_margin*exp_cnt*exp_cnt;

	/* - (3) readout time cannot be overlapped */
	calc_fl[2] =
		(ctx->s_ctx.mode[ctx->current_scenario_id].readout_length +
		ctx->s_ctx.mode[ctx->current_scenario_id].read_margin);
	if (last_exp_cnt == exp_cnt)
		for (i = 1; i < exp_cnt; i++) {
			readout_diff = ctx->exposure[i] - (u32) shutters[i];
			calc_fl[2] += readout_diff > 0 ? readout_diff : 0;
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
	/* write framelength */
	if (set_auto_flicker(ctx, 0) || frame_length || !ctx->s_ctx.reg_addr_auto_extend)
		write_frame_length(ctx, ctx->frame_length);
	else if (ctx->s_ctx.reg_addr_auto_extend)
		write_frame_length(ctx, ctx->min_frame_length);
	/* write shutter */

	rg_shutters[0] = (u32) shutters[0] * 2;

	if (ctx->s_ctx.reg_addr_exposure_lshift != PARAM_UNDEFINED) {
		set_i2c_buffer(ctx, ctx->s_ctx.reg_addr_exposure_lshift, 0);
		ctx->l_shift = 0;
	}
	for (i = 0; i < 3; i++) {
		if (rg_shutters[i]) {
			if (ctx->s_ctx.reg_addr_exposure[i].addr[3]) {
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[0], 
					(rg_shutters[i] >> 20) & 0x0F);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[1],
					(rg_shutters[i] >> 12) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[2],
					(rg_shutters[i] >> 4) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[3],
					rg_shutters[i] << 4 & 0xF0);
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
		return ERROR_NONE;
}

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
		mdelay(10);
		DRV_LOG_MUST(ctx,
			"MODE_SEL(%08x)\n", subdrv_i2c_rd_u8(ctx, 0x0100));
		ctx->test_pattern = 0;
	} else {
		subdrv_i2c_wr_u8(ctx, 0x0100, 0x00);// stream off
	}

	return ret;
}

static int dashsc821csultra_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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

static int dashsc821csultra_get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id)
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