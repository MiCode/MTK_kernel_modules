// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *     rodinsc820csultramipiraw_Sensor.c
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
#include "rodinsc820csultramipiraw_Sensor.h"


#define DEBUG_LOG_EN 0
#define PFX "RODINSC820CSULTRA_camera_sensor"
#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_DEBUG(...) do { if ((DEBUG_LOG_EN)) LOG_INF(__VA_ARGS__); } while (0)

static int init_ctx(
	struct subdrv_ctx *ctx, struct i2c_client *i2c_client, u8 i2c_write_id);
static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);
static u16 get_gain2reg(u32 gain);
static int set_streaming_control(void *arg, bool enable);
static int rodinsc820csultra_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int rodinsc820csultra_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int rodinsc820csultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int rodinsc820csultra_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len);


#define RODINSC820CSULTRA_SENSOR_GAIN_BASE           1024
#define RODINSC820CSULTRA_SENSOR_GAIN_MAX            32*1024
#define RODINSC820CSULTRA_SENSOR_GAIN_MAX_VALID_INDEX  6

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, rodinsc820csultra_set_test_pattern},
	{SENSOR_FEATURE_SET_GAIN, rodinsc820csultra_set_gain},
	{SENSOR_FEATURE_SET_ESHUTTER, rodinsc820csultra_set_shutter},
	{SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME,rodinsc820csultra_set_multi_shutter_frame_length},
};

// mode 0: 3280x2464@30fps, normal preview
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

// mode 1: 3280x1840@30fps, normal preview
static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 4096,
			.vsize = 1840,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

// mode 2: 2592x1944@30fps, normal preview
static struct mtk_mbus_frame_desc_entry frame_desc_bokeh[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 2592,
			.vsize = 1944,
			.user_data_desc = VC_STAGGER_NE,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
		},
	},
};

static struct subdrv_mode_struct mode_struct[] = {
	{	/* mode 0 : preview 2592x1944@30fps */
		.mode_setting_table = rodinsc820csultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(rodinsc820csultra_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 264000000,
		.linelength = 3520,
		.framelength = 2500,
		.max_framerate = 300,
		.mipi_pixel_rate = 264000000,
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
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
	},
	{	/* mode 1 : preview 2592x1944@30fps */
		.mode_setting_table = rodinsc820csultra_preview_setting,
		.mode_setting_len = ARRAY_SIZE(rodinsc820csultra_preview_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 264000000,
		.linelength = 3520,
		.framelength = 2500,
		.max_framerate = 300,
		.mipi_pixel_rate = 264000000,
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
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
	},
	{	/* mode 2 : same as preview 2592x1944@30fps */
		.mode_setting_table = rodinsc820csultra_normal_video_setting,
		.mode_setting_len = ARRAY_SIZE(rodinsc820csultra_normal_video_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 264000000,
		.linelength = 3520,
		.framelength = 2500,
		.max_framerate = 300,
		.mipi_pixel_rate = 264000000,
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
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
	},
	{	/* mode 3 : same as preview 2592x1944@30fps */
		.mode_setting_table = rodinsc820csultra_bokeh_setting,
		.mode_setting_len = ARRAY_SIZE(rodinsc820csultra_bokeh_setting),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 264000000,
		.linelength = 3520,
		.framelength = 3125,
		.max_framerate = 240,
		.mipi_pixel_rate = 264000000,
		.framelength_step = 1,
		.imgsensor_winsize_info = {
			.full_w = 3264,
			.full_h = 2448,
			.x0_offset = 320,
			.y0_offset = 240,
			.w0_size = 2624,
			.h0_size = 1968,
			.scale_w = 2624,
			.scale_h = 1968,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = 2624,
			.h1_size = 1968,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = 2624,
			.h2_tg_size = 1968,
		},
		.pdaf_cap = FALSE,
		.imgsensor_pd_info = PARAM_UNDEFINED,
		.ae_binning_ratio = 1000,
		.fine_integ_line = 0,
		.delay_frame = 2,
		.frame_desc = frame_desc_bokeh,
		.num_entries = ARRAY_SIZE(frame_desc_bokeh),
	},
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = RODINSC820CSULTRA_SENSOR_ID,
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
	.ana_gain_max = BASEGAIN * 32,
	.ana_gain_type = 1,
	.ana_gain_step = 1,
	.ana_gain_table = rodinsc820csultra_ana_gain_table,
	.ana_gain_table_size = sizeof(rodinsc820csultra_ana_gain_table),
	.tuning_iso_base = 50,
	.exposure_def = 0x3D0,
	.exposure_min = 2,
	.exposure_max = 0x7FF0 - 4,
	.exposure_step = 1,
	.exposure_margin = 4,

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
		{0x3e00, 0x3e01, 0x3e02}
	},
	.long_exposure_support = FALSE,
	.reg_addr_exposure_lshift = PARAM_UNDEFINED,
	.reg_addr_ana_gain = {
		{0x3e08},
	},
	.reg_addr_frame_length = {0x320e, 0x320f},

	.reg_addr_auto_extend = PARAM_UNDEFINED,
	.reg_addr_frame_count = 0x4869,
	.reg_addr_temp_en = PARAM_UNDEFINED,
	.reg_addr_temp_read = PARAM_UNDEFINED,


	.init_setting_table = rodinsc820csultra_init_setting,
	.init_setting_len = ARRAY_SIZE(rodinsc820csultra_init_setting),
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
	{HW_ID_MCLK,  {24},	   0},
	{HW_ID_RST,   {0},	   0},
	{HW_ID_AVDD1,  {1800000,1800000}, 1000},
	{HW_ID_DOVDD, {1800000,1800000}, 0},
	{HW_ID_DVDD,  {1200000,1200000}, 0},
	{HW_ID_AVDD,  {2800000,2800000}, 1000},
	{HW_ID_MCLK_DRIVING_CURRENT, {4}, 1000},
	{HW_ID_RST,   {1},	   5000},
};


const struct subdrv_entry rodinsc820csultra_mipi_raw_entry = {
	.name = "rodinsc820csultra_mipi_raw",
	.id = RODINSC820CSULTRA_SENSOR_ID,
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

static u16 RODINSC820CSULTRA_AGC_Param[RODINSC820CSULTRA_SENSOR_GAIN_MAX_VALID_INDEX][2] = {
	{  1024,  0x00 },
	{  2048,  0x08 },
	{  4096,  0x09 },
	{  8192,  0x0B },
	{ 16384,  0x0f },
	{ 32768,  0x1f },
};

static void rodinsc820csultra_gain2reg (const u64 total_analog_gain, u8* analog_reg_gain, u8* digtal_fine_reg_gain)
{
	u16 gain_index;
	u32 temp_digtal_gain;
	u64 gain = total_analog_gain;

	if (gain < RODINSC820CSULTRA_SENSOR_GAIN_BASE)
		gain = RODINSC820CSULTRA_SENSOR_GAIN_BASE;
	else if (gain > RODINSC820CSULTRA_SENSOR_GAIN_MAX)
		gain = RODINSC820CSULTRA_SENSOR_GAIN_MAX;

	for (gain_index = RODINSC820CSULTRA_SENSOR_GAIN_MAX_VALID_INDEX - 1; gain_index > 0; gain_index--)
		if (gain >= RODINSC820CSULTRA_AGC_Param[gain_index][0])
			break;

	*analog_reg_gain = (u8) RODINSC820CSULTRA_AGC_Param[gain_index][1];

	temp_digtal_gain =  gain*RODINSC820CSULTRA_SENSOR_GAIN_BASE/RODINSC820CSULTRA_AGC_Param[gain_index][0];
	temp_digtal_gain = temp_digtal_gain >> 3;

	*digtal_fine_reg_gain = (u8)temp_digtal_gain;
}

static int rodinsc820csultra_set_gain(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u8 analog_reg_gain, digtal_fine_reg_gain;
	u64 gain = *(u64 *)para;

	rodinsc820csultra_gain2reg(gain, &analog_reg_gain, &digtal_fine_reg_gain);
	set_i2c_buffer(ctx, 0x3E08, analog_reg_gain);
	set_i2c_buffer(ctx, 0x3E06, 0);
	set_i2c_buffer(ctx, 0x3E07, digtal_fine_reg_gain); //1x-2x
	commit_i2c_buffer(ctx);
	DRV_LOG_MUST(ctx,"Gain_Debug: gain = %llu again = 0x%x, dgain = 0x%x\n", gain, analog_reg_gain, digtal_fine_reg_gain);
	DRV_LOG(ctx,"Gain_Debug read again = 0x%x, dgain = 0x%x\n",subdrv_i2c_rd_u8(ctx,0x3E08), subdrv_i2c_rd_u8(ctx,0x3E07));
	return ERROR_NONE;
}

static int rodinsc820csultra_set_shutter(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
		set_i2c_buffer(ctx, 0x3e00, (shutter >> 12) & 0x0F);
		set_i2c_buffer(ctx, 0x3e01, (shutter >> 4)&0xFF);
		set_i2c_buffer(ctx, 0x3e02, (shutter<<4) & 0xF0);
	}
	DRV_LOG_MUST(ctx, "Shutter_Debug: exp[0x%x], fll(input/output):%u/%u, flick_en:%u\n",
		ctx->exposure[0], frame_length, ctx->frame_length, ctx->autoflicker_en);

	commit_i2c_buffer(ctx);
	return ERROR_NONE;
}

static int rodinsc820csultra_set_multi_shutter_frame_length(struct subdrv_ctx *ctx, u8 *para, u32 *len)
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
			if (ctx->s_ctx.reg_addr_exposure[i].addr[2]) {
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[0],
					(rg_shutters[i] >> 12) & 0x0F);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[1],
					(rg_shutters[i] >> 4) & 0xFF);
				set_i2c_buffer(ctx,	ctx->s_ctx.reg_addr_exposure[i].addr[2],
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
		DRV_LOG_MUST(ctx,
			"MODE_SEL(%08x)\n", subdrv_i2c_rd_u8(ctx, 0x0100));
		ctx->test_pattern = 0;
	} else {
		subdrv_i2c_wr_u8(ctx, 0x0100, 0x00);// stream off
	}

	return ret;
}

static int rodinsc820csultra_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	switch (mode) {
	case 5:
		subdrv_i2c_wr_u8(ctx, 0x4501, 0xCC);
		break;
	default:
		subdrv_i2c_wr_u8(ctx, 0x4501, 0xC4);
		break;
	}

	ctx->test_pattern = mode;
	return ERROR_NONE;

}
