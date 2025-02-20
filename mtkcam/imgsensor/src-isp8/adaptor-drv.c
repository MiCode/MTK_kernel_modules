// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/thermal.h>
#include <linux/delay.h>
#include <linux/version.h>
#include "thermal_core.h"
#include "mtk-i3c-i2c-wrap.h"

#include "kd_imgsensor_define_v4l2.h"
#include "kd_imgsensor.h"

#include "adaptor.h"
#include "adaptor-hw.h"
#include "adaptor-i2c.h"
#include "adaptor-ctrls.h"
#include "adaptor-command.h"
#include "adaptor-fsync-ctrls.h"
#include "adaptor-ioctl.h"
#include "adaptor-trace.h"
#include "adaptor-tsrec-cb-ctrl-impl.h"
#include "adaptor-sentest-ctrl.h"
#include "imgsensor-glue/imgsensor-glue.h"
#include "virt-sensor/virt-sensor-entry.h"

#undef E
#define E(__x__) (__x__##_entry)
#define EXTERN_IMGSENSOR_SUBDRVS extern struct subdrv_entry \
	IMGSENSOR_SUBDRVS
#ifdef IMGSENSOR_SUBDRVS
EXTERN_IMGSENSOR_SUBDRVS;
#endif

#undef E
#define E(__x__) (&__x__##_entry)
static struct subdrv_entry *imgsensor_subdrvs[] = {
#ifdef IMGSENSOR_SUBDRVS
	IMGSENSOR_SUBDRVS
#endif
};

module_param(sensor_debug, uint, 0644);
module_param(set_ctrl_unlock, uint, 0644);
MODULE_PARM_DESC(sensor_debug, "imgsensor_debug");
#ifdef __XIAOMI_CAMERA__
bool parallel_setting_enable = TRUE;
module_param(parallel_setting_enable, bool, 0644);
#endif
unsigned int gSensor_num;
unsigned int is_multicam;
unsigned int is_imgsensor_fusion_test_workaround;

static void get_outfmt_code(struct adaptor_ctx *ctx)
{
	unsigned int i, outfmt;

	for (i = 0; i < SENSOR_SCENARIO_ID_MAX; i++) {

		outfmt = (ctx->subctx.s_ctx.mode == NULL
				|| i >= ctx->subctx.s_ctx.sensor_mode_num)
			? ctx->sensor_info.SensorOutputDataFormat
			: ctx->subctx.s_ctx.mode[i].sensor_output_dataformat;
		switch (outfmt) {
		case SENSOR_OUTPUT_FORMAT_RAW_B:
		case SENSOR_OUTPUT_FORMAT_RAW_IR:
		case SENSOR_OUTPUT_FORMAT_RAW_MONO:
			ctx->fmt_code[i] =  MEDIA_BUS_FMT_SBGGR10_1X10;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW_Gb:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGBRG10_1X10;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW_Gr:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGRBG10_1X10;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW_R:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SRGGB10_1X10;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_B:
		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_BAYER_B:
		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B:
			adaptor_logd(ctx, "unsupported 4cell output_format %d\n", outfmt);
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SBGGR10_1X10;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_Gb:
		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_BAYER_Gb:
		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_Gb:
			adaptor_logd(ctx, "unsupported 4cell output_format %d\n", outfmt);
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGBRG10_1X10;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_Gr:
		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_BAYER_Gr:
		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_Gr:
			adaptor_logd(ctx, "unsupported 4cell output_format %d\n", outfmt);
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGRBG10_1X10;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_R:
		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_BAYER_R:
		case SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R:
			adaptor_logd(ctx, "unsupported 4cell output_format %d\n", outfmt);
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SRGGB10_1X10;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_B:
		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_BAYER_B:
		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_HW_BAYER_B:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SBGGR12_1X12;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_Gb:
		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_BAYER_Gb:
		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_HW_BAYER_Gb:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGBRG12_1X12;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_Gr:
		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_BAYER_Gr:
		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_HW_BAYER_Gr:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGRBG12_1X12;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_R:
		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_BAYER_R:
		case SENSOR_OUTPUT_FORMAT_RAW12_4CELL_HW_BAYER_R:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SRGGB12_1X12;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_B:
		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_BAYER_B:
		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_HW_BAYER_B:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SBGGR14_1X14;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_Gb:
		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_BAYER_Gb:
		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_HW_BAYER_Gb:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGBRG14_1X14;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_Gr:
		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_BAYER_Gr:
		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_HW_BAYER_Gr:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGRBG14_1X14;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_R:
		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_BAYER_R:
		case SENSOR_OUTPUT_FORMAT_RAW14_4CELL_HW_BAYER_R:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SRGGB14_1X14;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW8_MONO:
		case SENSOR_OUTPUT_FORMAT_SENSING_MODE_RAW_MONO:
		case SENSOR_OUTPUT_FORMAT_VIEWING_MODE_RAW_MONO:
		case SENSOR_OUTPUT_FORMAT_RAW8_B:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SBGGR8_1X8;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW8_Gb:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGBRG8_1X8;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW8_Gr:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGRBG8_1X8;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW8_R:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SRGGB8_1X8;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW12_TOF:
		case SENSOR_OUTPUT_FORMAT_RAW12_B:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SBGGR12_1X12;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW12_Gb:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGBRG12_1X12;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW12_Gr:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGRBG12_1X12;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW12_R:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SRGGB12_1X12;
			break;

		case SENSOR_OUTPUT_FORMAT_RAW14_B:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SBGGR14_1X14;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW14_Gb:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGBRG14_1X14;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW14_Gr:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SGRBG14_1X14;
			break;
		case SENSOR_OUTPUT_FORMAT_RAW14_R:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SRGGB14_1X14;
			break;

		case SENSOR_OUTPUT_FORMAT_NV12:
		case SENSOR_OUTPUT_FORMAT_NV21:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SBGGR8_1X8;
			break;
		case SENSOR_OUTPUT_FORMAT_YUV_P010:
		case SENSOR_OUTPUT_FORMAT_YVU_P010:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SBGGR10_1X10;
			break;
		case SENSOR_OUTPUT_FORMAT_RGB888:
			ctx->fmt_code[i] = MEDIA_BUS_FMT_RGB888_1X24;
			break;
		default:
			adaptor_logi(ctx, "unknown output format %d\n", outfmt);
			ctx->fmt_code[i] = MEDIA_BUS_FMT_SBGGR10_1X10;
			break;
		}
		adaptor_logi(ctx, "scenario id : %d, fmt_code = 0x%04x\n", i, ctx->fmt_code[i]);
	}
}

static u32 get_active_line_num(struct adaptor_ctx *ctx, u32 scenario_id)
{
	int ret, j;
	struct mtk_mbus_frame_desc fd_tmp;
	u32 result = 0, height = 0;

	ret = subdrv_call(ctx, get_frame_desc, scenario_id, &fd_tmp);
	if (!ret) {
		for (j = 0; j < fd_tmp.num_entries; j++) {
			if (fd_tmp.entry[j].bus.csi2.is_active_line &&
			    (fd_tmp.entry[j].bus.csi2.user_data_desc != VC_STAGGER_ME) &&
			    (fd_tmp.entry[j].bus.csi2.user_data_desc != VC_STAGGER_SE) &&
			    (fd_tmp.entry[j].bus.csi2.user_data_desc != VC_PDAF_STATS_ME_PIX_1) &&
			    (fd_tmp.entry[j].bus.csi2.user_data_desc != VC_PDAF_STATS_ME_PIX_2) &&
			    (fd_tmp.entry[j].bus.csi2.user_data_desc != VC_PDAF_STATS_SE_PIX_1) &&
			    (fd_tmp.entry[j].bus.csi2.user_data_desc != VC_PDAF_STATS_SE_PIX_2)) {
				result += fd_tmp.entry[j].bus.csi2.vsize;
			}
			/*set default active line*/
			if ((fd_tmp.entry[j].bus.csi2.user_data_desc == VC_STAGGER_NE) ||
				(fd_tmp.entry[j].bus.csi2.user_data_desc == VC_RAW_DATA))
				height = fd_tmp.entry[j].bus.csi2.vsize;
		}
		if (!result)
			result = height;
	}

	return result;
}

static void add_sensor_mode(struct adaptor_ctx *ctx,
		int id, int width, int height)
{
	union feature_para para;
	u32 idx, len, llp_readout;
	u64 val;
	int val_signed;
	struct sensor_mode *mode;

	idx = ctx->mode_cnt;

	if (idx >= MODE_MAXCNT) {
		adaptor_loge(ctx, "invalid mode idx %d\n", idx);
		return;
	}

	mode = &ctx->mode[idx];
	mode->id = id;
	mode->width = width;
	mode->height = height;

	para.u64[0] = id;
	para.u64[1] = (u64)&val;

	para.u64[2] = SENSOR_GET_LINELENGTH_FOR_READOUT;
	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_PERIOD_BY_SCENARIO,
		para.u8, &len);
	llp_readout = val & 0xffff;
	para.u64[2] = 0;

	val = 0;
	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_PERIOD_BY_SCENARIO,
		para.u8, &len);

	mode->llp = val & 0xffff;
	mode->fll = val >> 16;

	if (!mode->llp || !mode->fll)
		return;

	val = 0;
	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_MIPI_PIXEL_RATE,
		para.u8, &len);

	mode->mipi_pixel_rate = val;

	val = 0;
	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_CUST_PIXEL_RATE,
		para.u8, &len);

	mode->cust_pixel_rate = val;

	val = 0;
	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO,
		para.u8, &len);

	mode->max_framerate = val;

	val = 0;
	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO,
		para.u8, &len);

	mode->pclk = val;

	val = 0;
	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_ESD_RESET_BY_USER,
		para.u8, &len);

	mode->esd_reset_by_user = val;

	para.u64[1] = (u64)&val_signed;
	val_signed = 0;

	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_FINE_INTEG_LINE_BY_SCENARIO,
		para.u8, &len);

	mode->fine_intg_line = val_signed;

	if (!mode->mipi_pixel_rate || !mode->max_framerate || !mode->pclk)
		return;

	subdrv_call(ctx, get_csi_param, mode->id, &mode->csi_param);

	/* update linetime_in_ns */
	mode->linetime_in_ns = (u64)mode->llp * 1000000 +
		(mode->pclk / 1000 - 1);
	do_div(mode->linetime_in_ns, mode->pclk / 1000);


	mode->linetime_in_ns_readout = (u64)llp_readout * 1000000 +
		(mode->pclk / 1000 - 1);
	do_div(mode->linetime_in_ns_readout, mode->pclk / 1000);


	/* Update active line */
	mode->active_line_num = get_active_line_num(ctx, mode->id);


	dev_dbg(ctx->dev, "%s [%d] id %d %dx%d %dx%d px %d fps %d tLine %lld|%lld fintl %d\n",
		__func__,
		idx, id, width, height,
		mode->llp, mode->fll,
		mode->mipi_pixel_rate, mode->max_framerate,
		mode->linetime_in_ns,
		mode->linetime_in_ns_readout,
		mode->fine_intg_line);

	ctx->mode_cnt++;
}

static int init_sensor_mode(struct adaptor_ctx *ctx)
{
	MSDK_SENSOR_RESOLUTION_INFO_STRUCT res;
	unsigned int i = 0;

	memset(&res, 0, sizeof(res));
	subdrv_call(ctx, get_resolution, &res);
	for (i = SENSOR_SCENARIO_ID_MIN; i < SENSOR_SCENARIO_ID_MAX; i++) {
		ctx->seamless_scenarios[i] = SENSOR_SCENARIO_ID_NONE;

		if (res.SensorWidth[i] > 0 && res.SensorHeight[i] > 0)
			add_sensor_mode(ctx,
			i,
			res.SensorWidth[i],
			res.SensorHeight[i]);
	}
	return 0;
}

#ifdef __XIAOMI_CAMERA__
void sensor_init_work(struct work_struct *work)
{
	struct adaptor_ctx *ctx = container_of(work, struct adaptor_ctx, init_work);

	adaptor_logm(ctx, "[%s]+\n", __func__);

	if (!ctx) {
		return;
	}

	if (ctx->init_skipped == 0 || ctx->subctx.aov_sensor_support
	|| ctx->is_sensor_inited == 0) {
		subdrv_call(ctx, open);
	} else {
		adaptor_logm(ctx, "init_skipped\n");
	}
	ctx->init_skipped = 0;
	ctx->is_sensor_inited = 1;

	adaptor_logm(ctx, "[%s]-\n", __func__);

	return;
}
void sensor_ctrl_work(struct work_struct *work)
{
	MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT image_window;
	MSDK_SENSOR_CONFIG_STRUCT sensor_config_data;
	u64 data[4];
	u32 len;
	struct adaptor_ctx *ctx = container_of(work, struct adaptor_ctx, ctrl_work);


	adaptor_logm(ctx, "[%s]+\n", __func__);

	if (!ctx) {
		return;
	}
	subdrv_call(ctx, control, ctx->cur_mode->id, &image_window, &sensor_config_data);
	data[0] = ctx->cur_mode->id;
	subdrv_call(ctx, feature_control, SENSOR_FEATURE_SET_DESKEW_CTRL, (u8 *)data, &len);
	subdrv_call(ctx, feature_control, SENSOR_FEATURE_SET_CPHY_LRTE_MODE, (u8 *)data, &len);
	ctx->is_sensor_scenario_inited = 1;

	adaptor_logm(ctx, "[%s]-\n", __func__);

	return;
}
#endif

static void control_sensor(struct adaptor_ctx *ctx)
{
#ifndef __XIAOMI_CAMERA__
	MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT image_window;
	MSDK_SENSOR_CONFIG_STRUCT sensor_config_data;
	u64 data[4];
	u32 len;
#endif
	if (ctx == NULL) {
		adaptor_loge(ctx, "null pointer ctx is invalid\n");
		return;
	}


	adaptor_logm(ctx,
		"+ is_sensor_scenario_inited(%u),is_streaming(%u)\n",
		ctx->is_sensor_scenario_inited, ctx->is_streaming);

#ifdef __XIAOMI_CAMERA__
	flush_work(&ctx->ctrl_work);
	flush_work(&ctx->init_work);
#endif

	if (!ctx->is_sensor_scenario_inited && !ctx->is_streaming) {

#ifdef __XIAOMI_CAMERA__
	queue_work(system_wq, &ctx->ctrl_work);
	if(!parallel_setting_enable){
		flush_work(&ctx->ctrl_work);
	}
#else
		subdrv_call(ctx, control,
				ctx->cur_mode->id,
				&image_window,
				&sensor_config_data);

		data[0] = ctx->cur_mode->id;
		subdrv_call(ctx, feature_control,
				SENSOR_FEATURE_SET_DESKEW_CTRL,
				(u8 *)data, &len);
		subdrv_call(ctx, feature_control,
				SENSOR_FEATURE_SET_CPHY_LRTE_MODE,
				(u8 *)data, &len);
		ctx->is_sensor_scenario_inited = 1;
#endif
	}
	if (!ctx->is_streaming) // no need to restore ae when seamless
		restore_ae_ctrl(ctx);

	adaptor_logm(ctx, "-\n");
}

static int set_sensor_mode(struct adaptor_ctx *ctx,
		struct sensor_mode *mode, char update_ctrl_defs)
{
	s64 min, max, def;

	if (ctx->cur_mode == mode) {
		if (update_ctrl_defs)
			control_sensor(ctx);
		return 0;
	}

	ctx->cur_mode = mode;
	ctx->is_sensor_scenario_inited = 0;

	subdrv_call(ctx, get_info,
			mode->id,
			&ctx->sensor_info,
			&ctx->sensor_cfg);

	if (update_ctrl_defs) {
		/* pixel rate */
		min = max = def = mode->mipi_pixel_rate;
		__v4l2_ctrl_modify_range(ctx->pixel_rate, min, max, 1, def);

		/* hblank */
		min = max = def = mode->llp - mode->width;
		__v4l2_ctrl_modify_range(ctx->hblank, min, max, 1, def);

		/* vblank */
		min = def = get_mode_vb(ctx, mode);
		max = ctx->subctx.max_frame_length - mode->height;
		__v4l2_ctrl_modify_range(ctx->vblank, min, max, 1, def);

		/* init sensor scenario setting */
		control_sensor(ctx);
	}

	adaptor_logd(ctx, "select %dx%d@%d %dx%d px %d\n",
		mode->width, mode->height, mode->max_framerate,
		mode->llp, mode->fll, mode->mipi_pixel_rate);

	return 0;
}

static int init_sensor_info(struct adaptor_ctx *ctx)
{
	init_sensor_mode(ctx);
	set_sensor_mode(ctx, &ctx->mode[0], 0);
	ctx->try_format_mode = &ctx->mode[0];
	get_outfmt_code(ctx);
	return 0;
}

static int imgsensor_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct adaptor_ctx *ctx = to_ctx(sd);
#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
	struct v4l2_mbus_framefmt *try_fmt =
		v4l2_subdev_state_get_format(fh->state, 0);
#else
	struct v4l2_mbus_framefmt *try_fmt =
		v4l2_subdev_get_try_format(sd, fh->state, 0);
#endif

	adaptor_logm(ctx, "+\n");

	mutex_lock(&ctx->mutex);

	ctx->open_refcnt++;

	/* Initialize try_fmt */
	try_fmt->width = ctx->cur_mode->width;
	try_fmt->height = ctx->cur_mode->height;
	try_fmt->code = ctx->fmt_code[ctx->cur_mode->id];
	try_fmt->field = V4L2_FIELD_NONE;

	mutex_unlock(&ctx->mutex);

	adaptor_logm(ctx, "-\n");

	return 0;
}

static int imgsensor_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct adaptor_ctx *ctx = to_ctx(sd);
	int i;

	adaptor_logm(ctx, "+\n");

	mutex_lock(&ctx->mutex);

	ctx->open_refcnt--;
	if (ctx->open_refcnt <= 0) {
		for (i = 0; ctx->power_refcnt; i++)
			adaptor_hw_power_off(ctx);
		ctx->open_refcnt = 0;
	}

	mutex_unlock(&ctx->mutex);
	adaptor_logm(ctx, "-\n");
	return 0;
}

static int imgsensor_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
				  struct v4l2_subdev_mbus_code_enum *code)
{
	struct adaptor_ctx *ctx = to_ctx(sd);

	if (code->index >= ctx->mode_cnt)
		return -EINVAL;

	code->code = to_mtk_ext_fmt_code(ctx->fmt_code[ctx->mode[code->index].id],
			ctx->mode[code->index].id);
	adaptor_logd(ctx, "enum mbus fmt code = 0x%x\n", code->code);

	return 0;
}

static int imgsensor_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	struct adaptor_ctx *ctx = to_ctx(sd);

	if (fse->index >= ctx->mode_cnt)
		return -EINVAL;

	fse->min_width = ctx->mode[fse->index].width;
	fse->max_width = fse->min_width;
	fse->min_height = ctx->mode[fse->index].height;
	fse->max_height = fse->min_height;

	return 0;
}

static int imgsensor_enum_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *state,
		struct v4l2_subdev_frame_interval_enum *fie)
{
	struct adaptor_ctx *ctx = to_ctx(sd);
	struct sensor_mode *mode;
	u32 i, index = fie->index;

	for (i = 0; i < ctx->mode_cnt; i++) {
		mode = &ctx->mode[i];
		if (fie->width != mode->width ||
			fie->height != mode->height)
			continue;
		if (index-- == 0) {
			fie->interval.numerator = 10;
			fie->interval.denominator = mode->max_framerate;
			return 0;
		}
	}

	return -EINVAL;
}

static int imgsensor_get_selection(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *state,
		struct v4l2_subdev_selection *sel)
{
	struct adaptor_ctx *ctx = to_ctx(sd);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE)
		return -EINVAL;

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = ctx->cur_mode->width;
		sel->r.height = ctx->cur_mode->height;
		return 0;
	default:
		return -EINVAL;
	}
}

static void update_pad_format(struct adaptor_ctx *ctx,
		const struct sensor_mode *mode,
		struct v4l2_subdev_format *fmt)
{
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.code = to_mtk_ext_fmt_code(ctx->fmt_code[mode->id], mode->id);
	fmt->format.field = V4L2_FIELD_NONE;
}

static int __imgsensor_get_pad_format(struct adaptor_ctx *ctx,
				   struct v4l2_subdev_state *state,
				   struct v4l2_subdev_format *fmt)
{
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
		fmt->format = *v4l2_subdev_state_get_format(state, fmt->pad);
#else
		fmt->format = *v4l2_subdev_get_try_format(&ctx->sd, state,
					  fmt->pad);
#endif
	else
		update_pad_format(ctx, ctx->cur_mode, fmt);

	return 0;
}

static int imgsensor_get_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *state,
				 struct v4l2_subdev_format *fmt)
{
	struct adaptor_ctx *ctx = to_ctx(sd);
	int ret;

	ret = __imgsensor_get_pad_format(ctx, state, fmt);
	return ret;
}

static int imgsensor_set_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *state,
				 struct v4l2_subdev_format *fmt)
{
	struct adaptor_ctx *ctx = to_ctx(sd);
	struct sensor_mode *mode;
	struct v4l2_mbus_framefmt *framefmt;
	int sensor_mode_id = 0;
	u64 start = 0, end = 0;

	start = ktime_get_boottime_ns();
	mutex_lock(&ctx->mutex);

	/* Only one raw bayer order is supported */
	set_std_parts_fmt_code(fmt->format.code, ctx->fmt_code[ctx->cur_mode->id]);


	/* Returns the best match or NULL if the Length of the array is zero */
	mode = v4l2_find_nearest_size(ctx->mode,
		ctx->mode_cnt, width, height,
		fmt->format.width, fmt->format.height);

	/* Override mode when fmt code specified sensor mode */
	if (is_mtk_ext_fmt_code(fmt->format.code)) {
		sensor_mode_id = get_sensor_mode_from_fmt_code(fmt->format.code);
		if (sensor_mode_id >= 0 && sensor_mode_id < ctx->mode_cnt)
			mode = &ctx->mode[sensor_mode_id];
	}

	if (mode == NULL) {
		adaptor_loge(ctx,
			"set fmt code = 0x%x, which %d ctx->mode_cnt = %d\n",
			fmt->format.code, fmt->which, ctx->mode_cnt);

		mutex_unlock(&ctx->mutex);
		return -EINVAL;
	}

	update_pad_format(ctx, mode, fmt);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
		framefmt = v4l2_subdev_state_get_format(state, fmt->pad);
#else
		framefmt = v4l2_subdev_get_try_format(sd, state, fmt->pad);
#endif
		*framefmt = fmt->format;

		ctx->try_format_mode = mode;
	} else {

		ADAPTOR_SYSTRACE_BEGIN("imgsensor::init_sensor");
		adaptor_sensor_init(ctx);
		ADAPTOR_SYSTRACE_END();

		ADAPTOR_SYSTRACE_BEGIN("imgsensor::set_mode_%u", mode->id);
		set_sensor_mode(ctx, mode, 1);
		ADAPTOR_SYSTRACE_END();
		end = ktime_get_boottime_ns();
		adaptor_logi(ctx,
			"set fmt code = 0x%x, which %d sensor_mode_id = %u, set_fmt time:%lld(us)\n",
			fmt->format.code, fmt->which, mode->id, (end-start)/1000);
	}
	mutex_unlock(&ctx->mutex);

	return 0;
}

static int imgsensor_set_power(struct v4l2_subdev *sd, int on)
{
	struct adaptor_ctx *ctx = to_ctx(sd);
	int ret, ret1;

	mutex_lock(&ctx->mutex);
	if (on) {
	ret = adaptor_hw_power_on(ctx);
	ret1 = adaptor_ixc_do_daa (&ctx->ixc_client);
	if (ret1)
		adaptor_loge(ctx, "ixc_do_daa(ret=%d), prot= %d\n",
				ret, ctx->ixc_client.protocol);
	} else
#ifdef __XIAOMI_CAMERA__
	ret = adaptor_hw_power_off_deferred(ctx);
#else
	ret = adaptor_hw_power_off(ctx);
#endif
	mutex_unlock(&ctx->mutex);

	return ret;
}

static int imgsensor_streaming_delay(struct adaptor_ctx *ctx)
{
	u64 sys_ts, mono_ts, ae_memento_le_ns, streaming_sensor_vsync_ts,
		streaming_sensor_fl_ns, hw_reinit_time_ns, target_timing_ns;
	u32 ae_ctrl_cit;
	long long streamon_delay_ns = 0;
	u32 tmp = 0;
	char *systrace_log = NULL;
	u32 systrace_log_len = 0;
	char namebuff[3];

	if (ctx->streamon_1sof_vsync_ts_info.vsync_ts_ns) {
		systrace_log = kzalloc(1024 + 1, GFP_KERNEL);
		sys_ts = ktime_get_boottime_ns();
		mono_ts = ktime_get_ns();
		streaming_sensor_vsync_ts = ctx->streamon_1sof_vsync_ts_info.vsync_ts_ns;
		streaming_sensor_fl_ns = (ctx->streamon_1sof_vsync_ts_info.fps)
				? (10000000000/(ctx->streamon_1sof_vsync_ts_info.fps)) : 0;
		hw_reinit_time_ns = ctx->subctx.hw_time_info[ctx->cur_mode->id].init_time_ns;
		target_timing_ns = (u64)ctx->streamon_1sof_vsync_ts_info.target_timing_us * 1000;
		if(!hw_reinit_time_ns) {
			memcpy( namebuff, (ctx->subdrv->name), 2);
			namebuff[2] = '\0';
			if (! (strcmp(namebuff, "ov"))) {
				hw_reinit_time_ns = 1.6 * 1000000;
				systrace_log_len += snprintf(systrace_log + systrace_log_len,
									1024 - systrace_log_len,
									"Latency value:%llu(ns) by-default(OV),",
									hw_reinit_time_ns);
			} else if (! (strcmp(namebuff, "im"))) {
				hw_reinit_time_ns = 3.3 * 1000000;
				systrace_log_len += snprintf(systrace_log + systrace_log_len,
									1024 - systrace_log_len,
									"Latency value:%llu(ns) by-default(Sony),",
									hw_reinit_time_ns);
			} else if (! (strcmp(namebuff, "s5"))) {
				hw_reinit_time_ns = 8 * 1000000;
				systrace_log_len += snprintf(systrace_log + systrace_log_len,
									1024 - systrace_log_len,
									"Latency value:%llu(ns) by-default(sams),",
									hw_reinit_time_ns);
			} else {
				hw_reinit_time_ns = 5 * 1000000;
				systrace_log_len += snprintf(systrace_log + systrace_log_len,
									1024 - systrace_log_len,
									"Latency value:%llu(ns) by-default(unknown),",
									hw_reinit_time_ns);
			}
		} else {
			systrace_log_len += snprintf(systrace_log + systrace_log_len,
							1024 - systrace_log_len,
							"Latency value:%llu(ns) from-table,",
							hw_reinit_time_ns);
		}
		if ((ctx->subctx.s_ctx.mode[ctx->cur_mode->id].hdr_mode == HDR_RAW_LBMF) &&
			(ctx->subctx.s_ctx.mode[ctx->cur_mode->id].exposure_order_in_lbmf
					== IMGSENSOR_LBMF_EXPOSURE_SE_FIRST)) {
			ae_ctrl_cit = FINE_INTEG_CONVERT(
					ctx->ae_memento.exposure.me_exposure,
					ctx->subctx.s_ctx.mode[ctx->cur_mode->id].fine_integ_line);
		} else {
			ae_ctrl_cit = FINE_INTEG_CONVERT(
					ctx->ae_memento.exposure.le_exposure,
					ctx->subctx.s_ctx.mode[ctx->cur_mode->id].fine_integ_line);
		}
		ae_memento_le_ns = ae_ctrl_cit * (ctx->cur_mode->linetime_in_ns);

		streamon_delay_ns = ((streaming_sensor_vsync_ts
				+ streaming_sensor_fl_ns
				- target_timing_ns)
				- mono_ts - hw_reinit_time_ns - ae_memento_le_ns) ;

		tmp = 0;
		while ((streamon_delay_ns < 0) && (streaming_sensor_fl_ns)) {
			streamon_delay_ns += streaming_sensor_fl_ns;
			tmp ++;
		}
		if (streamon_delay_ns < 0)
			streamon_delay_ns = 0;
		systrace_log_len += snprintf(systrace_log + systrace_log_len,
								1024 - systrace_log_len,
								"Sensor-ID:%u(%s), Mode-number:%u, Calculated delay time:%llu(ns)",
								ctx->dts_idx,
								ctx->subdrv->name,
								ctx->cur_mode->id,
								streamon_delay_ns);
		systrace_log_len += snprintf(systrace_log + systrace_log_len,
								1024 - systrace_log_len,
								",from streaming sensor (%llu/%llu/%llu) sys_ts:%llu mono_ts:%llu hw_reinit:%llu ae_memento_le_ns:%llu(%u*%llu)",
								streaming_sensor_vsync_ts,
								streaming_sensor_fl_ns,
								target_timing_ns,
								sys_ts,
								mono_ts,
								hw_reinit_time_ns,
								ae_memento_le_ns,
								ae_ctrl_cit,
								ctx->cur_mode->linetime_in_ns);

		ADAPTOR_SYSTRACE_BEGIN("imgsensor::streamondelay::%s",systrace_log);
		udelay(streamon_delay_ns/1000);
		// while (ktime_get_boottime_ns() < (sys_ts + streamon_delay_ns)) {
		// ;
		// }
		ADAPTOR_SYSTRACE_END();

		adaptor_logi(ctx,
					"cur_mode_id:%u (%llu/%llu/%llu) sys_ts:%llu mono_ts:%llu hw_reinit:%llu ae_memento_le_ns:%llu(%u*%llu) streamon_delay_ns:%lld(%u) [SYSTRACE: %s]\n",
					ctx->cur_mode->id,
					streaming_sensor_vsync_ts,
					streaming_sensor_fl_ns,
					target_timing_ns,
					sys_ts,
					mono_ts,
					hw_reinit_time_ns,
					ae_memento_le_ns,
					ae_ctrl_cit,
					ctx->cur_mode->linetime_in_ns,
					streamon_delay_ns,
					tmp,
					systrace_log);

		/* reset variables */
		kfree(systrace_log);
		memset(&ctx->streamon_1sof_vsync_ts_info, 0, sizeof(ctx->streamon_1sof_vsync_ts_info));
	}
	return 0;
}

/* Start streaming */
static int imgsensor_start_streaming(struct adaptor_ctx *ctx)
{
	u64 data[4];
	u32 len;

	if (ctx == NULL) {
		adaptor_loge(ctx, "null pointer ctx is invalid\n");
		return -EINVAL;
	}

	adaptor_logm(ctx, "+\n");

	adaptor_sensor_init(ctx);

	control_sensor(ctx);

	data[0] = 0; // shutter
	imgsensor_streaming_delay(ctx);
	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_SET_STREAMING_RESUME,
		(u8 *)data, &len);

	adaptor_logm(ctx, "[SENSOR_FEATURE_SET_STREAMING_RESUME] -\n");


	/* update timeout value after reset*/
	update_shutter_for_timeout_by_ae_ctrl(ctx, &ctx->ae_memento);
	/* 1st/2nd framelength */
	ctx->last_framelength = ctx->subctx.frame_length_rg;
	update_framelength_for_timeout(ctx);

	/* notify frame-sync streaming ON */
	notify_fsync_mgr_streaming(ctx, 1);

	adaptor_logm(ctx, "-\n");
	return 0;
}

/* Stop streaming */
static int imgsensor_stop_streaming(struct adaptor_ctx *ctx)
{
	u64 data[4];
	u32 len;
	union feature_para para;

	/* clear ebd record */
	mutex_lock(&ctx->ebd_lock);
	memset(&ctx->latest_ebd, 0, sizeof(ctx->latest_ebd));
	mutex_unlock(&ctx->ebd_lock);

	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_SET_STREAMING_SUSPEND,
		(u8 *)data, &len);

	para.u8[0] = 0;
	//Make sure close test pattern
	subdrv_call(ctx, feature_control,
		    SENSOR_FEATURE_SET_TEST_PATTERN,
		    para.u8, &len);

	/* notify frame-sync streaming OFF */
	notify_fsync_mgr_streaming(ctx, 0);

	/* reset variables */
	memset(&ctx->ae_memento, 0, sizeof(ctx->ae_memento));
	memset(&ctx->ae_ctrl_dbg_info, 0, sizeof(ctx->ae_ctrl_dbg_info));
	ctx->sys_ts_update_sof_cnt = 0;
	ctx->sof_cnt = 0;

	/* reset sentest flag */
	sentest_flag_init(ctx);

	return 0;
}

#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
static int imgsensor_get_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *sd_state,
		struct v4l2_subdev_frame_interval *fi)
{
	struct adaptor_ctx *ctx = to_ctx(sd);

	mutex_lock(&ctx->mutex);
	fi->interval.numerator = 10;

	if (fi->reserved[0] == V4L2_SUBDEV_FORMAT_TRY)
		fi->interval.denominator = ctx->try_format_mode->max_framerate;
	else
		fi->interval.denominator = ctx->cur_mode->max_framerate;

	mutex_unlock(&ctx->mutex);

	return 0;
}

static int imgsensor_set_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *sd_state,
		struct v4l2_subdev_frame_interval *fi)
{
	struct adaptor_ctx *ctx = to_ctx(sd);

	adaptor_logi(ctx, "set_frame_interval = %u\n", fi->interval.denominator);

	return imgsensor_get_frame_interval(sd, sd_state, fi);
}

#else
static int imgsensor_get_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_frame_interval *fi)
{
	struct adaptor_ctx *ctx = to_ctx(sd);

	mutex_lock(&ctx->mutex);
	fi->interval.numerator = 10;

	if (fi->reserved[0] == V4L2_SUBDEV_FORMAT_TRY)
		fi->interval.denominator = ctx->try_format_mode->max_framerate;
	else
		fi->interval.denominator = ctx->cur_mode->max_framerate;

	mutex_unlock(&ctx->mutex);

	return 0;
}

static int imgsensor_set_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_frame_interval *fi)
{
	struct adaptor_ctx *ctx = to_ctx(sd);

	adaptor_logi(ctx, "set_frame_interval = %u\n", fi->interval.denominator);

	return imgsensor_get_frame_interval(sd, fi);
}

#endif

static int imgsensor_set_stream(struct v4l2_subdev *sd, int enable)
{
	int ret;
	struct adaptor_ctx *ctx = to_ctx(sd);

	mutex_lock(&ctx->mutex);
	if (ctx->is_streaming == enable) {
		adaptor_logm(ctx,
			"+ is_streaming(%d)\n",
			ctx->is_streaming);
		mutex_unlock(&ctx->mutex);
		return 0;
	}

	if (enable) {
		/*
		 * Apply default & customized values
		 * and then start streaming.
		 */
		ret = imgsensor_start_streaming(ctx);
		if (ret)
			goto err_rpm_put;
	} else {
		imgsensor_stop_streaming(ctx);

	}

	ctx->is_streaming = enable;
	mutex_unlock(&ctx->mutex);

	adaptor_logi(ctx, "en %d\n", enable);

	return 0;

err_rpm_put:
	mutex_unlock(&ctx->mutex);

	return ret;
}

static int imgsensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
		struct v4l2_mbus_config *config)
{
	struct adaptor_ctx *ctx = to_ctx(sd);

	config->type = ctx->sensor_info.MIPIsensorType == MIPI_CPHY ?
		V4L2_MBUS_CSI2_CPHY : V4L2_MBUS_CSI2_DPHY;

	switch (ctx->sensor_info.SensorMIPILaneNumber) {
	case SENSOR_MIPI_1_LANE:
		config->bus.mipi_csi2.num_data_lanes = 1;
		break;
	case SENSOR_MIPI_2_LANE:
		config->bus.mipi_csi2.num_data_lanes = 2;
		break;
	case SENSOR_MIPI_3_LANE:
		config->bus.mipi_csi2.num_data_lanes = 3;
		break;
	case SENSOR_MIPI_4_LANE:
		config->bus.mipi_csi2.num_data_lanes = 4;
		break;
	}

	return 0;
}

static const struct v4l2_subdev_core_ops imgsensor_core_ops = {
	.s_power = imgsensor_set_power,
	.ioctl = adaptor_ioctl,
	.command = adaptor_command,
};

static const struct v4l2_subdev_video_ops imgsensor_video_ops = {
#if (KERNEL_VERSION(6, 7, 0) >= LINUX_VERSION_CODE)
	.g_frame_interval = imgsensor_get_frame_interval,
	.s_frame_interval = imgsensor_set_frame_interval,
#endif
	.s_stream = imgsensor_set_stream,
};

static const struct v4l2_subdev_pad_ops imgsensor_pad_ops = {
	.enum_mbus_code = imgsensor_enum_mbus_code,
	.get_fmt = imgsensor_get_pad_format,
	.set_fmt = imgsensor_set_pad_format,
	.enum_frame_size = imgsensor_enum_frame_size,
	.enum_frame_interval = imgsensor_enum_frame_interval,
	.get_selection = imgsensor_get_selection,
	.get_mbus_config = imgsensor_g_mbus_config,
#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
	.get_frame_interval = imgsensor_get_frame_interval,
	.set_frame_interval = imgsensor_set_frame_interval,
#endif
};

static const struct v4l2_subdev_ops imgsensor_subdev_ops = {
	.core = &imgsensor_core_ops,
	.video = &imgsensor_video_ops,
	.pad = &imgsensor_pad_ops,
};

static const struct v4l2_subdev_internal_ops imgsensor_internal_ops = {
	.open = imgsensor_open,
	.close = imgsensor_close,
};

static int imgsensor_get_temp(struct thermal_zone_device *tz, int *temperature)
{
	struct adaptor_ctx *ctx = tz->devdata;

	*temperature = 0;

	if (ctx->is_streaming && !ctx->is_i2c_bus_scp)
		subdrv_call(ctx, get_temp, temperature);
	else
		*temperature = THERMAL_TEMP_INVALID;

	return 0;
}

static const struct thermal_zone_device_ops imgsensor_tz_ops = {
	.get_temp = imgsensor_get_temp,
};

#define SHOW(buf, len, fmt, ...) { \
	len += snprintf(buf + len, PAGE_SIZE - len, fmt, ##__VA_ARGS__); \
}

static ssize_t debug_i2c_ops_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	int len = 0;
	struct adaptor_ctx *ctx = to_ctx(dev_get_drvdata(dev));


	SHOW(buf, len, "%s i2c read 0x%08x = 0x%08x\n",
			ctx->subdrv->name,
			ctx->sensorReg.RegAddr,
			ctx->sensorReg.RegData);
	return len;
}

enum DBG_ARG_IDX {
	DBG_ARG_IDX_I2C_ADDR,
	DBG_ARG_IDX_I2C_DATA,
	DBG_ARG_IDX_MAX_NUM,
};

static ssize_t debug_i2c_ops_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	char delim[] = " ";
	char *token = NULL;
	char *sbuf = kzalloc(sizeof(char) * (count + 1), GFP_KERNEL);
	char *s = sbuf;
	int ret;
	unsigned int num_para = 0;
	char *arg[DBG_ARG_IDX_MAX_NUM];
	struct adaptor_ctx *ctx = to_ctx(dev_get_drvdata(dev));
	u32 val;
	u32 reg;

	ctx->sensorReg.RegAddr = 0;
	ctx->sensorReg.RegData = 0;

	if (!sbuf)
		goto ERR_DEBUG_OPS_STORE;

	memcpy(sbuf, buf, count);

	token = strsep(&s, delim);
	while (token != NULL && num_para < DBG_ARG_IDX_MAX_NUM) {
		if (strlen(token)) {
			arg[num_para] = token;
			num_para++;
		}

		token = strsep(&s, delim);
	}

	if (num_para > DBG_ARG_IDX_MAX_NUM) {
		adaptor_loge(ctx, "Wrong command parameter number %u\n", num_para);
		goto ERR_DEBUG_OPS_STORE;
	}
	ret = kstrtouint(arg[DBG_ARG_IDX_I2C_ADDR], 0, &reg);
	if (ret)
		goto ERR_DEBUG_OPS_STORE;
	ctx->sensorReg.RegAddr = reg;

	if (num_para == DBG_ARG_IDX_MAX_NUM) {
		ret = kstrtouint(arg[DBG_ARG_IDX_I2C_DATA], 0, &val);
		if (ret)
			goto ERR_DEBUG_OPS_STORE;
		ctx->sensorReg.RegData = val;

		ret = subdrv_call(ctx, feature_control, SENSOR_FEATURE_SET_REGISTER,
					(MUINT8 *) &ctx->sensorReg,
					(MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
		adaptor_logi(ctx, "i2c write 0x%08x = 0x%08x ret = %d\n",
			ctx->sensorReg.RegAddr, ctx->sensorReg.RegData, ret);
	}

	ret = subdrv_call(ctx, feature_control, SENSOR_FEATURE_GET_REGISTER,
				(MUINT8 *) &ctx->sensorReg,
				(MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
		adaptor_logi(ctx, "i2c read 0x%08x = 0x%08x  ret = %d\n",
			ctx->sensorReg.RegAddr, ctx->sensorReg.RegData, ret);


ERR_DEBUG_OPS_STORE:

	kfree(sbuf);
	adaptor_logd(ctx, "exit\n");

	return count;
}


static DEVICE_ATTR_RW(debug_i2c_ops);


static const char * const hw_id_names[] = {
	HW_ID_NAMES
};

static ssize_t debug_pwr_ops_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	int len = 0;
	struct adaptor_ctx *ctx = to_ctx(dev_get_drvdata(dev));
	int i;
	const struct subdrv_pw_seq_entry *ent;

	SHOW(buf, len, "sensor%d name %s\n", ctx->idx, ctx->subdrv->name);
	for (i = 0; i < ctx->subdrv->pw_seq_cnt; i++) {
		if (ctx->ctx_pw_seq)
			ent = &ctx->ctx_pw_seq[i]; // use ctx pw seq
		else
			ent = &ctx->subdrv->pw_seq[i];
		SHOW(buf, len, "\t%s power (%d,%d) with delay %d\n",
		     hw_id_names[(unsigned int)ent->id], ent->val.para1, ent->val.para2, ent->delay);
	}

	return len;
}

enum DBG_PWR_ARG_IDX {
	DBG_PWR_ARG_IDX_SEQ_IDX,
	DBG_PWR_ARG_IDX_SEQ_VAL,
	DBG_PWR_ARG_IDX_SEQ_DELAY,
	DBG_PWR_ARG_IDX_MAX_NUM,
};

static ssize_t debug_pwr_ops_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	char delim[] = " ";
	char *token = NULL;
	char *sbuf = kzalloc(sizeof(char) * (count + 1), GFP_KERNEL);
	char *s = sbuf;
	unsigned int num_para = 0;
	char *arg[DBG_PWR_ARG_IDX_MAX_NUM];
	struct adaptor_ctx *ctx = to_ctx(dev_get_drvdata(dev));
	struct subdrv_pw_seq_entry *ent;
	unsigned int seq_idx, seq_val, seq_delay;
	int ret;

	if (!sbuf)
		goto ERR_DEBUG_PWR_OPS_STORE;

	if (!ctx->ctx_pw_seq) {
		adaptor_loge(ctx, "ctx pw seq is null\n");
		goto ERR_DEBUG_PWR_OPS_STORE;
	}

	memcpy(sbuf, buf, count);

	token = strsep(&s, delim);
	while (token != NULL && num_para < DBG_PWR_ARG_IDX_MAX_NUM) {
		if (strlen(token)) {
			arg[num_para] = token;
			num_para++;
		}

		token = strsep(&s, delim);
	}

	if (num_para > DBG_PWR_ARG_IDX_MAX_NUM) {
		adaptor_loge(ctx, "Wrong command parameter number %u\n", num_para);
		goto ERR_DEBUG_PWR_OPS_STORE;
	}
	ret = kstrtouint(arg[DBG_PWR_ARG_IDX_SEQ_IDX], 0, &seq_idx);
	if (ret)
		goto ERR_DEBUG_PWR_OPS_STORE;
	ret = kstrtouint(arg[DBG_PWR_ARG_IDX_SEQ_VAL], 0, &seq_val);
	if (ret)
		goto ERR_DEBUG_PWR_OPS_STORE;
	ret = kstrtouint(arg[DBG_PWR_ARG_IDX_SEQ_DELAY], 0, &seq_delay);
	if (ret)
		goto ERR_DEBUG_PWR_OPS_STORE;

	if (seq_idx < ctx->subdrv->pw_seq_cnt) {
		ent = &ctx->ctx_pw_seq[seq_idx];
		ent->val.para1 = seq_val;
		ent->delay = seq_delay;
	}

ERR_DEBUG_PWR_OPS_STORE:

	kfree(sbuf);

	return count;
}

static DEVICE_ATTR_RW(debug_pwr_ops);

static ssize_t debug_sensor_mode_ops_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	int len = 0;
	struct adaptor_ctx *ctx = to_ctx(dev_get_drvdata(dev));

	SHOW(buf, len, "(%s) sensor_mode_ops is (%d)\n",
			ctx->subdrv->name,
			ctx->subctx.sensor_mode_ops);
	return len;

}

static ssize_t debug_sensor_mode_ops_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	char *sbuf = kzalloc(sizeof(char) * (count + 1), GFP_KERNEL);
	struct adaptor_ctx *ctx = to_ctx(dev_get_drvdata(dev));
	unsigned int val = 0;
	int ret;

	if (!sbuf)
		goto ERR_DEBUG_SENSOR_MODE_OPS_STORE;

	memcpy(sbuf, buf, count);

	ret = kstrtouint(sbuf, 0, &val);
	if (ret)
		goto ERR_DEBUG_SENSOR_MODE_OPS_STORE;

	switch (val) {
	case AOV_MODE_CTRL_OPS_SENSING_CTRL:
	case AOV_MODE_CTRL_OPS_MONTION_DETECTION_CTRL:
		ctx->subctx.sensor_mode_ops = val;
		break;
	case AOV_MODE_CTRL_OPS_SENSING_UT_ON_SCP:
		// true for default aov sensing mode streaming control on scp
		ctx->subctx.sensor_debug_sensing_ut_on_scp = true;
		break;
	case AOV_MODE_CTRL_OPS_SENSING_UT_ON_APMCU:
		// flase for default aov sensing mode streaming control on apmcu side
		ctx->subctx.sensor_debug_sensing_ut_on_scp = false;
		break;
	case AOV_MODE_CTRL_OPS_DPHY_GLOBAL_TIMING_CONTINUOUS_CLK:
		ctx->subctx.sensor_debug_dphy_global_timing_continuous_clk = true;
		break;
	case AOV_MODE_CTRL_OPS_DPHY_GLOBAL_TIMING_NON_CONTINUOUS_CLK:
		ctx->subctx.sensor_debug_dphy_global_timing_continuous_clk = false;
		break;
	default:
		adaptor_loge(ctx, "Wrong command parameter number (%u)\n", val);
		break;
	}

ERR_DEBUG_SENSOR_MODE_OPS_STORE:

	kfree(sbuf);

	return count;
}

static DEVICE_ATTR_RW(debug_sensor_mode_ops);

void mtk_v4l2_ixc_subdev_init(struct v4l2_subdev *sd,
	struct i3c_i2c_device *client, const struct v4l2_subdev_ops *ops)
{
	struct device *dev = adaptor_ixc_get_dev(client);

	v4l2_subdev_init(sd, ops);
	if (client->protocol == I2C_PROTOCOL)
		sd->flags |= V4L2_SUBDEV_FL_IS_I2C;
	/* the owner is the same as the i2c_client's driver owner */
	sd->owner = dev->driver->owner;
	sd->dev = dev;
	/* i2c_client and v4l2_subdev point to one another */
	v4l2_set_subdevdata(sd, client);
	dev_set_drvdata(dev, sd);
}

static int search_sensor(struct adaptor_ctx *ctx)
{
	int ret, i, j, of_sensor_names_cnt, subdrvs_cnt;
	struct subdrv_entry **subdrvs, *subdrv;
	struct subdrv_entry *of_subdrvs[OF_SENSOR_NAMES_MAXCNT];
	const char *of_sensor_names[OF_SENSOR_NAMES_MAXCNT];
	int subdrv_name_ret;
	const char *of_subdrv_name;

	of_sensor_names_cnt = of_property_read_string_array(ctx->dev->of_node,
		"sensor-names", of_sensor_names, ARRAY_SIZE(of_sensor_names));

	subdrv_name_ret = of_property_read_string(ctx->dev->of_node,
						  "subdrv-name", &of_subdrv_name);

	adaptor_logi(ctx, "subdrv_name_ret %d\n", subdrv_name_ret);

	if (!subdrv_name_ret)
		adaptor_logi(ctx, "subdrv name %s found\n", of_subdrv_name);

	/* try to load custom list from DT */
	if (of_sensor_names_cnt > 0) {
		subdrvs = of_subdrvs;
		subdrvs_cnt = 0;
		for (i = 0; i < of_sensor_names_cnt; i++) {
			adaptor_logi(ctx, "sensor_name %s\n",
				of_sensor_names[i]);
			for (j = 0; j < ARRAY_SIZE(imgsensor_subdrvs); j++) {
				subdrv = imgsensor_subdrvs[j];
				if (!strcmp(subdrv->name,
					of_sensor_names[i])) {
					of_subdrvs[subdrvs_cnt++] = subdrv;
					break;
				}
			}
			if (j == ARRAY_SIZE(imgsensor_subdrvs)) {
				adaptor_logd(ctx, "%s not found\n",
					of_sensor_names[i]);
			}
		}
	} else if (!subdrv_name_ret) {
		subdrvs_cnt = 0;
		of_subdrvs[0] = vs_query_subdrv_entry(of_subdrv_name);

		if (of_subdrvs[0]) {
			subdrvs_cnt = 1;
			subdrvs = of_subdrvs;
			adaptor_logi(ctx, "subdrv name %s found\n", of_subdrv_name);
		}
	} else {
		subdrvs = imgsensor_subdrvs;
		subdrvs_cnt = ARRAY_SIZE(imgsensor_subdrvs);
	}

	for (i = 0; i < subdrvs_cnt; i++) {
		u32 sensor_id = 0xffffffff;

		ctx->subdrv = subdrvs[i];

		if (ctx->subdrv == NULL) {
			adaptor_loge(ctx, "ctx->subdrv == NULL");
			continue;
		}

		ctx->subctx.i2c_client = ctx->i2c_client;
		ctx->subctx.ixc_client = ctx->ixc_client;
		adaptor_cam_pmic_on(ctx);
		adaptor_hw_power_on(ctx);
		ret = adaptor_ixc_do_daa (&ctx->ixc_client);
		if (ret)
			adaptor_logi(ctx, "ixc_do_daa(ret=%d), prot= %d\n",
				ret, ctx->ixc_client.protocol);
		subdrv_call(ctx, init_ctx, ctx->i2c_client,
				ctx->subctx.i2c_write_id);
		ret = subdrv_call(ctx, get_id, &sensor_id);
		adaptor_hw_power_off(ctx);
		if (!ret) {
			adaptor_logi(ctx, "sensor %s found\n",
				ctx->subdrv->name);
			subdrv_call(ctx, init_ctx, ctx->i2c_client,
				ctx->subctx.i2c_write_id);
			ctx->ctx_pw_seq = kmalloc_array(ctx->subdrv->pw_seq_cnt,
					sizeof(struct subdrv_pw_seq_entry),
					GFP_KERNEL);
			if (ctx->ctx_pw_seq) {
				memcpy(ctx->ctx_pw_seq, ctx->subdrv->pw_seq,
				       ctx->subdrv->pw_seq_cnt *
				       sizeof(struct subdrv_pw_seq_entry));
			}
			if (ctx->subctx.aov_sensor_support && ctx->cust_aov_csi_clk) {
				ctx->subctx.aov_csi_clk = ctx->cust_aov_csi_clk;
				adaptor_logi(ctx,
					"aov_csi_clk:%u\n",
					ctx->subctx.aov_csi_clk);
			}
			if (ctx->subctx.aov_sensor_support && ctx->phy_ctrl_ver) {
				ctx->subctx.aov_phy_ctrl_ver = ctx->phy_ctrl_ver;
				adaptor_logi(ctx,
					"aov_phy_ctrl_ver:%s\n",
					ctx->subctx.aov_phy_ctrl_ver);
			}
			return 0;
		}
		adaptor_logi(ctx, "sensor %s not found\n",
			ctx->subdrv->name);
	}

	return -EIO;
}

static int imgsensor_probe(struct i3c_i2c_device *client)
{
	struct device *dev = adaptor_ixc_get_dev(client);
	struct device_node *endpoint;
	struct adaptor_ctx *ctx;
	int i, ret;
	unsigned int reindex;
	unsigned int pmic_delayus;
	const char *reindex_match[OF_SENSOR_NAMES_MAXCNT];
	int reindex_match_cnt;
	int forbid_index;
	struct device_node *platform_node = NULL;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	adaptor_tsrec_cb_ctrl_init(ctx);

	mutex_init(&ctx->mutex);
	mutex_init(&ctx->ebd_lock);
	mutex_init(&ctx->subctx.i2c_buffer_lock);


	if (sentest_probe_init(ctx))
		adaptor_loge(ctx, "sentest_probe_init return failed\n");

	ctx->open_refcnt = 0;
	ctx->power_refcnt = 0;
	ctx->mclk_refcnt = 0;

	ctx->ixc_client = *client;
	ctx->i2c_client = client->i2c_dev;
	ctx->dev = dev;
	ctx->sensor_debug_flag = &sensor_debug;
	ctx->p_set_ctrl_unlock_flag = &set_ctrl_unlock;
	ctx->aov_pm_ops_flag = 0;
	ctx->aov_mclk_ulposc_flag = 0;

#ifdef __XIAOMI_CAMERA__
	init_waitqueue_head(&ctx->poweroff_wq);
	INIT_WORK(&ctx->poweroff_work, adaptor_hw_power_off_work);
	ctx->poweroff_timeout_ms = poweroff_timeout_ms;
	INIT_WORK(&ctx->init_work, sensor_init_work);
	INIT_WORK(&ctx->ctrl_work, sensor_ctrl_work);
#endif

	if (!of_property_read_u32(
		dev->of_node, "cust-aov-csi-clk", &ctx->cust_aov_csi_clk))
		adaptor_logi(ctx, "cust_aov_csi_clk:%u\n", ctx->cust_aov_csi_clk);

	platform_node = of_find_compatible_node(NULL, NULL, "mediatek,seninf-core");
	if (!platform_node) {
		adaptor_loge(ctx,
			"compatiable node:(mediatek,seninf-core) not found\n");
		return -EINVAL;
	}

	if (!of_property_read_string(
		platform_node, "mtk-iomem-ver", &ctx->phy_ctrl_ver))
		adaptor_logi(ctx, "phy_ctrl_ver:%s\n", ctx->phy_ctrl_ver);

	endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (!endpoint) {
		adaptor_loge(ctx, "endpoint node not found\n");
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_parse(of_fwnode_handle(endpoint), &ctx->ep);

	of_node_put(endpoint);

	if (ret < 0) {
		adaptor_loge(ctx, "parsing endpoint node failed\n");
		return ret;
	}

	ret = adaptor_hw_init(ctx);
	if (ret) {
		adaptor_loge(ctx, "failed to init hw handles\n");
		return ret;
	}

	if (!of_property_read_u32(dev->of_node, "maximum-pmic-delay", &pmic_delayus)) {
		ctx->pmic_delayus = pmic_delayus;
		dev_info(dev, "pmic_delayus:%llu\n", ctx->pmic_delayus);
	}	else
		ctx->pmic_delayus = 0;

	ret = search_sensor(ctx);
	adaptor_hw_deinit(ctx);
	if (ret) {
		adaptor_loge(ctx, "no sensor found\n");
		return ret;
	} else {
		gSensor_num++;
		if (gSensor_num > 2)
			is_multicam = 1;
		else
			is_multicam = 0;
		if (ctx->phy_ctrl_ver) {
			if (is_multicam &&
				(!strcasecmp(ctx->phy_ctrl_ver, MT6989_PHY_CTRL_VERSIONS) ||
				!strcasecmp(ctx->phy_ctrl_ver, MT6878_PHY_CTRL_VERSIONS) ||
				!strcasecmp(ctx->phy_ctrl_ver, MT6991_PHY_CTRL_VERSIONS)))
				is_imgsensor_fusion_test_workaround = 1;
		} else
			is_imgsensor_fusion_test_workaround = 0;
		adaptor_logi(ctx,
			"Sensor_num/is_multicam/is_imgsensor_fusion_test_workaround:%u/%u/%u\n",
			gSensor_num, is_multicam,
			is_imgsensor_fusion_test_workaround);
	}

	/* read property */
	of_property_read_u32(dev->of_node, "location", &ctx->location);
	of_property_read_u32(dev->of_node, "rotation", &ctx->rotation);

	/* init sensor info */
	init_sensor_info(ctx);

	/* init subdev */
	mtk_v4l2_ixc_subdev_init(&ctx->sd, client, &imgsensor_subdev_ops);
	ctx->sd.internal_ops = &imgsensor_internal_ops;
	ctx->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	ctx->pad.flags = MEDIA_PAD_FL_SOURCE;
	ctx->sd.dev = adaptor_ixc_get_dev(client);
	ctx->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ctx->forbid_idx = -1;
	if (!of_property_read_u32(dev->of_node, "forbid-index", &forbid_index)) {
		ctx->forbid_idx = forbid_index;
		adaptor_logi(ctx, "not support to power on with sensor%d\n", ctx->forbid_idx);
	}

	ret = sscanf(dev->of_node->name, OF_SENSOR_NAME_PREFIX"%d", &ctx->dts_idx);
	if (ret != 1)
		adaptor_logi(ctx, "failed to parse %s\n", dev->of_node->name);
	ctx->idx = ctx->dts_idx;

	if (!of_property_read_u32(dev->of_node, "reindex-to", &reindex)) {
		reindex_match_cnt = of_property_read_string_array(dev->of_node,
			"reindex-match", reindex_match, ARRAY_SIZE(reindex_match));
		for (i = 0; i < reindex_match_cnt; i++) {
			if (!strncmp(reindex_match[i], ctx->subdrv->name,
				     strlen(reindex_match[i]))) {
				ctx->idx = reindex;
				adaptor_logi(ctx, "reindex to sensor%d\n", ctx->idx);
				break;
			}
		}
	}

	/* init subdev name */
	ret = snprintf(ctx->sd.name, sizeof(ctx->sd.name), "%s%d",
		OF_SENSOR_NAME_PREFIX, ctx->idx);
	if (ret <= 0) {
		adaptor_loge(ctx, "failed to snprintf subdev name\n");
		return ret;
	}


	/* init controls */
	ret = adaptor_init_ctrls(ctx);
	if (ret) {
		adaptor_loge(ctx, "failed to init controls\n");
		return ret;
	}

	ret = media_entity_pads_init(&ctx->sd.entity, 1, &ctx->pad);
	if (ret < 0) {
		adaptor_loge(ctx, "failed to init entity pads: %d", ret);
		goto free_ctrl;
	}

	ret = v4l2_async_register_subdev(&ctx->sd);
	if (ret < 0) {
		adaptor_loge(ctx, "could not register v4l2 device\n");
		goto free_entity;
	}

	device_enable_async_suspend(dev);

	/* register thermal device */
	if (ctx->subdrv->ops->get_temp) {
		struct thermal_zone_device *tzdev;

		tzdev = devm_thermal_of_zone_register(
			       dev, 0, ctx, &imgsensor_tz_ops);
		if (IS_ERR(tzdev))
			adaptor_loge(ctx, "failed to register thermal zone\n");
	}

	notify_fsync_mgr(ctx, 1);

	ret = device_create_file(dev, &dev_attr_debug_i2c_ops);
	if (ret)
		adaptor_loge(ctx, "failed to create sysfs debug_i2c_ops\n");

	ret = device_create_file(dev, &dev_attr_debug_pwr_ops);
	if (ret)
		adaptor_loge(ctx, "failed to create sysfs debug_pwr_ops\n");

	ret = device_create_file(dev, &dev_attr_debug_sensor_mode_ops);
	if (ret)
		adaptor_loge(ctx, "failed to create sysfs debug_sensor_mode_ops\n");

	ctx->sensor_ws = wakeup_source_register(dev, ctx->sd.name);

	if (!ctx->sensor_ws)
		adaptor_loge(ctx, "failed to wakeup_source_register\n");

	kthread_init_worker(&ctx->adaptor_worker);
	ctx->adaptor_kworker_task = kthread_run(kthread_worker_fn,
				&ctx->adaptor_worker,
				"adaptor_worker_sensor_idx_%d",
				ctx->idx);

	if (IS_ERR(ctx->adaptor_kworker_task)) {
		adaptor_loge(ctx, "failed to start adaptor kthread worker\n");
		ctx->adaptor_kworker_task = NULL;
	} else
		adaptor_logi(ctx, "adaptor kthread worker init done\n");

	return 0;

free_entity:
	media_entity_cleanup(&ctx->sd.entity);

free_ctrl:
	v4l2_ctrl_handler_free(&ctx->ctrls);
	mutex_destroy(&ctx->mutex);

	return ret;
}

static void imgsensor_remove(struct i3c_i2c_device *client)
{
	struct v4l2_subdev *sd = adaptor_ixc_get_clientdata(client);
	struct adaptor_ctx *ctx = to_ctx(sd);

	if (sd == NULL) {
		adaptor_loge(ctx, "invalid pointer sd\n");
		return;
	}

	if (ctx == NULL) {
		adaptor_loge(ctx, "invalid pointer ctx\n");
		return;
	}

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	notify_fsync_mgr(ctx, 0);

	if (!ctx->sensor_ws)
		wakeup_source_unregister(ctx->sensor_ws);
	ctx->sensor_ws = NULL;

	kfree(ctx->ctx_pw_seq);

	device_remove_file(ctx->dev, &dev_attr_debug_i2c_ops);
	device_remove_file(ctx->dev, &dev_attr_debug_pwr_ops);
	device_remove_file(ctx->dev, &dev_attr_debug_sensor_mode_ops);

	mutex_destroy(&ctx->mutex);

}


static const struct i2c_device_id imgsensor_id[] = {
	{"imgsensor", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, imgsensor_id);

static const struct of_device_id imgsensor_of_match[] = {
	{.compatible = "mediatek,imgsensor"},
	{}
};
MODULE_DEVICE_TABLE(of, imgsensor_of_match);
MODULE_DEVICE_TABLE(i3c, mtk_i3c_id_table);

static struct i3c_i2c_device_id mtk_ixc_ids = {
	.i3c_id_table = mtk_i3c_id_table,
	.i2c_id_table = NULL,
};

struct i3c_i2c_driver imgsensor_ixc_driver = {
	.probe = imgsensor_probe,
	.remove = imgsensor_remove,
	.driver = {
		.name = "imgsensor",
		.of_match_table = of_match_ptr(imgsensor_of_match),
	},
	.id_table = &mtk_ixc_ids,
};

static int __init adaptor_drv_init(void)
{
	gSensor_num = 0;
	is_multicam = 0;
	is_imgsensor_fusion_test_workaround = 0;

	mtk_i3c_i2c_driver_register(&imgsensor_ixc_driver);

	return 0;
}

static void __exit adaptor_drv_exit(void)
{
	gSensor_num = 0;
	is_multicam = 0;
	is_imgsensor_fusion_test_workaround = 0;

	mtk_i3c_i2c_driver_unregister(&imgsensor_ixc_driver);
}

late_initcall(adaptor_drv_init);
module_exit(adaptor_drv_exit);

MODULE_LICENSE("GPL v2");
