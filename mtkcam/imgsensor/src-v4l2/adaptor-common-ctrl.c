// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2021 MediaTek Inc.

#include "kd_imgsensor_define_v4l2.h"
#include "imgsensor-user.h"
#include "adaptor.h"
#include "adaptor-common-ctrl.h"

#define USER_DESC_TO_IMGSENSOR_ENUM(DESC) \
				(DESC - VC_STAGGER_NE + \
				IMGSENSOR_STAGGER_EXPOSURE_LE)
#define IS_HDR_STAGGER(DESC) \
				((DESC >= VC_STAGGER_NE) && \
				 (DESC <= VC_STAGGER_SE))

int g_stagger_info(struct adaptor_ctx *ctx,
						  int scenario,
						  struct mtk_stagger_info *info)
{
	int ret = 0;
	struct mtk_mbus_frame_desc fd = {0};
	int hdr_cnt = 0;
	unsigned int i = 0;

	if (!info)
		return 0;

	if (info->scenario_id != SENSOR_SCENARIO_ID_NONE)
		scenario = info->scenario_id;

	// dev_info(ctx->dev, " %s scenario %d %d\n", __func__, scenario, info->scenario_id);

	ret = subdrv_call(ctx, get_frame_desc, scenario, &fd);

	if (!ret) {
		for (i = 0; i < fd.num_entries; ++i) {
			u16 udd =
				fd.entry[i].bus.csi2.user_data_desc;

			if (IS_HDR_STAGGER(udd)) {
				hdr_cnt++;
				info->order[i] = USER_DESC_TO_IMGSENSOR_ENUM(udd);
			}
		}
	}

	info->count = hdr_cnt;
	// dev_info(ctx->dev, " %s after %d %d\n", __func__, info->count, info->scenario_id);

	return ret;
}

int g_stagger_scenario(struct adaptor_ctx *ctx,
							  int scenario,
							  struct mtk_stagger_target_scenario *info)
{
	int ret = 0;
	union feature_para para;
	u32 len;

	if (!ctx || !info)
		return 0;

	para.u64[0] = scenario;
	para.u64[1] = (u64)info->exposure_num;
	para.u64[2] = SENSOR_SCENARIO_ID_NONE;

	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_STAGGER_TARGET_SCENARIO,
		para.u8, &len);

	info->target_scenario_id = (u32)para.u64[2];

	return ret;
}

u32 g_scenario_exposure_cnt(struct adaptor_ctx *ctx, int scenario)
{
	u32 result = 1, len = 0;
	union feature_para para;
	struct mtk_stagger_info info = {0};
	int ret = 0;

	para.u64[0] = scenario;
	para.u64[1] = 0;

	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_EXPOSURE_COUNT_BY_SCENARIO,
		para.u8, &len);
	if (para.u64[1]) {
		result = (u32) para.u64[1];
		adaptor_logd(ctx, "scenario exp count = %u\n", result);
		return result;
	}

	info.scenario_id = SENSOR_SCENARIO_ID_NONE;
	ret = g_stagger_info(ctx, scenario, &info);
	if (!ret) {
		/* non-stagger mode, the info count would be 0, it's same as 1 */
		if (info.count == 0)
			info.count = 1;
		result = info.count;
	}

	adaptor_logd(ctx, "exp count by stagger info = %u\n", result);
	return result;
}

int g_max_exposure(struct adaptor_ctx *ctx,
				   int scenario,
				   struct mtk_stagger_max_exp_time *info)
{
	u32 len = 0;
	union feature_para para;

	para.u64[0] = scenario;
	para.u64[1] = (u64)info->exposure;
	para.u64[2] = 0;

	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_STAGGER_MAX_EXP_TIME,
		para.u8, &len);

	info->max_exp_time = (u32)para.u64[2];

	return 0;
}

int g_max_exposure_line(struct adaptor_ctx *ctx,
				   int scenario,
				   struct mtk_max_exp_line *info)
{
	u32 len = 0;
	union feature_para para;

	para.u64[0] = scenario;
	para.u64[1] = (u64)info->exposure;
	para.u64[2] = 0;

	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_MAX_EXP_LINE,
		para.u8, &len);

	info->max_exp_line = (u32)para.u64[2];

	return 0;
}

u32 g_sensor_margin(struct adaptor_ctx *ctx, unsigned int scenario)
{
	union feature_para para;
	struct mtk_stagger_info info = {0};
	u32 len = 0;
	u32 mode_exp_cnt = 1;
	const struct subdrv_mode_struct *mode_st = NULL;

	// para.u64[0] = ctx->cur_mode->id;
	para.u64[0] = scenario;
	para.u64[1] = 0;
	para.u64[2] = 0;
	subdrv_call(ctx, feature_control,
		    SENSOR_FEATURE_GET_FRAME_CTRL_INFO_BY_SCENARIO,
		    para.u8, &len);
	info.scenario_id = SENSOR_SCENARIO_ID_NONE;

	/* get the mode's const pointer of the scenario_id */
	mode_st = &ctx->subctx.s_ctx.mode[scenario];

	switch (mode_st->hdr_mode) {
	case HDR_RAW_STAGGER:
		// if (!g_stagger_info(ctx, ctx->cur_mode->id, &info))
		if (!g_stagger_info(ctx, scenario, &info))
			mode_exp_cnt = info.count;

		/* no vc info case, it is 1 exposure */
		if (mode_exp_cnt == 0)
			mode_exp_cnt = 1;

		// XXX: para.u64[2] is single line and single exp based
		// convert to multi line and multi exp based
		return (para.u64[2] * mode_exp_cnt * mode_exp_cnt);
	case HDR_RAW_LBMF:
		// if (!g_stagger_info(ctx, ctx->cur_mode->id, &info))
		if (!g_stagger_info(ctx, scenario, &info))
			mode_exp_cnt = info.count;

		/* no vc info case, it is 1 exposure */
		if (mode_exp_cnt == 0)
			mode_exp_cnt = 1;

		// XXX: para.u64[2] is single line and single exp based
		// convert to single line and multi exp based
		return (para.u64[2] * mode_exp_cnt);
	default:
		return para.u64[2];
	}
}

int g_sensor_fine_integ_line(struct adaptor_ctx *ctx,
	const unsigned int scenario)
{
	union feature_para para;
	int fine_integ_line = 0;
	u32 len = 0;

	// para.u64[0] = ctx->cur_mode->id;
	para.u64[0] = scenario;
	para.u64[1] = (u64)&fine_integ_line;

	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_FINE_INTEG_LINE_BY_SCENARIO,
		para.u8, &len);

	return fine_integ_line;
}

u32 g_sensor_stagger_type(struct adaptor_ctx *ctx,
	const u32 scenario_id, enum IMGSENSOR_HDR_SUPPORT_TYPE_ENUM *p_type)
{
	if (unlikely(!chk_is_valid_scenario_id(ctx, scenario_id, __func__)))
		return 0;
	if (unlikely(ctx->subctx.s_ctx.mode == NULL))
		return 0;

	/* check for stagger mode */
	if (ctx->subctx.s_ctx.mode[scenario_id].hdr_mode != HDR_RAW_STAGGER)
		return 0;
	/* check for stagger type */
	if (ctx->subctx.s_ctx.hdr_type & HDR_SUPPORT_STAGGER_FDOL)
		*p_type = HDR_SUPPORT_STAGGER_FDOL;
	else if (ctx->subctx.s_ctx.hdr_type & HDR_SUPPORT_STAGGER_DOL)
		*p_type = HDR_SUPPORT_STAGGER_DOL;
	else if (ctx->subctx.s_ctx.hdr_type & HDR_SUPPORT_STAGGER_NDOL)
		*p_type = HDR_SUPPORT_STAGGER_NDOL;
	else
		return 0;

	return 1;
}

u32 g_sensor_dcg_property(struct adaptor_ctx *ctx, const u32 scenario_id)
{
	const struct subdrv_mode_struct *mode_st = NULL;

	if (unlikely(!chk_is_valid_scenario_id(ctx, scenario_id, __func__)))
		return 0;
	if (unlikely(ctx->subctx.s_ctx.mode == NULL))
		return 0;

	/* get the mode's const pointer of the scenario_id */
	mode_st = &ctx->subctx.s_ctx.mode[scenario_id];

	if (mode_st->dcg_info.dcg_mode == IMGSENSOR_DCG_NONE)
		return 0;

	return 1;
}

u32 g_sensor_lbmf_property(struct adaptor_ctx *ctx, const u32 scenario_id,
	struct adaptor_sensor_lbmf_property_st *prop)
{
	const struct subdrv_mode_struct *mode_st = NULL;

	if (unlikely(!chk_is_valid_scenario_id(ctx, scenario_id, __func__)))
		return 0;
	if (unlikely(ctx->subctx.s_ctx.mode == NULL))
		return 0;

	/* get the mode's const pointer of the scenario_id */
	mode_st = &ctx->subctx.s_ctx.mode[scenario_id];

	if (mode_st->hdr_mode != HDR_RAW_LBMF)
		return 0;

	/* fill in the lbmf property st */
	prop->exp_cnt = g_scenario_exposure_cnt(ctx, scenario_id);
	prop->exp_order = mode_st->exposure_order_in_lbmf;
	prop->mode_type = mode_st->mode_type_in_lbmf;

	/* checking property */
	if (unlikely(prop->exp_order == IMGSENSOR_LBMF_EXPOSURE_ORDER_SUPPORT_NONE)) {
		adaptor_logi(ctx,
			"ERROR: s_ctx.mode[%u]:(hdr_mode:%u (HDR_RAW_LBMF:%u), but exposure_order_in_lbmf:%u (SUPPORT_NONE:%u/LE:%u/SE:%u)), return 0\n",
			scenario_id,
			mode_st->hdr_mode,
			HDR_RAW_LBMF,
			mode_st->exposure_order_in_lbmf,
			IMGSENSOR_LBMF_EXPOSURE_ORDER_SUPPORT_NONE,
			IMGSENSOR_LBMF_EXPOSURE_LE_FIRST,
			IMGSENSOR_LBMF_EXPOSURE_SE_FIRST);
		return 0;
	}

	return 1;
}
