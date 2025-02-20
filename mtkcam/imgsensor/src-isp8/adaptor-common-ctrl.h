/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2021 MediaTek Inc. */

#ifndef __ADAPTOR_COMMON_CTRL_H__
#define __ADAPTOR_COMMON_CTRL_H__


struct adaptor_sensor_lbmf_property_st {
	enum IMGSENSOR_LBMF_EXPOSURE_ORDER exp_order;
	enum IMGSENSOR_LBMF_MODE_TYPE mode_type;
	unsigned int exp_cnt;
};


/******************************************************************************/
// static inline function
/******************************************************************************/
static inline bool chk_is_valid_scenario_id(const struct adaptor_ctx *ctx,
	const u32 scenario_id, const char *caller)
{
	if (unlikely(scenario_id >= ctx->subctx.s_ctx.sensor_mode_num)) {
		adaptor_logi(ctx,
			"[%s] invalid scenario_id:%u (>= sensor_mode_num:%u)\n",
			caller, scenario_id, ctx->subctx.s_ctx.sensor_mode_num);
		return false;
	}
	return true;
}
/******************************************************************************/


int g_stagger_info(struct adaptor_ctx *ctx,
				   int scenario,
				   struct mtk_stagger_info *info);

u32 g_scenario_exposure_cnt(struct adaptor_ctx *ctx, int scenario);

int g_stagger_scenario(struct adaptor_ctx *ctx,
					   int scenario,
					   struct mtk_stagger_target_scenario *info);

int g_max_exposure(struct adaptor_ctx *ctx,
					   int scenario,
					   struct mtk_stagger_max_exp_time *info);

int g_max_exposure_line(struct adaptor_ctx *ctx,
					   int scenario,
					   struct mtk_max_exp_line *info);

u32 g_sensor_margin(struct adaptor_ctx *ctx, unsigned int scenario);

u32 g_sensor_frame_length_delay(struct adaptor_ctx *ctx,
	const u32 scenario_id, const char *caller);

int g_sensor_fine_integ_line(struct adaptor_ctx *ctx,
	const unsigned int scenario);

/*
 * return 1:
 *     p_type:
 *         FDOL: HDR_SUPPORT_STAGGER_FDOL
 *          DOL: HDR_SUPPORT_STAGGER_DOL
 *         NDOL: HDR_SUPPORT_STAGGER_NDOL
 * return 0:
 *     p_type: the value has not been modified.
 */
u32 g_sensor_stagger_type(struct adaptor_ctx *ctx,
	const u32 scenario_id, enum IMGSENSOR_HDR_SUPPORT_TYPE_ENUM *p_type);

/* return: 0 => NON DCG; 1 => DCG */
u32 g_sensor_dcg_property(struct adaptor_ctx *ctx, const u32 scenario_id);

/* return: 0 => NON LBMF; 1 => LBMF */
u32 g_sensor_lbmf_property(struct adaptor_ctx *ctx, const u32 scenario_id,
	struct adaptor_sensor_lbmf_property_st *prop);


int notify_imgsensor_start_streaming_delay(struct adaptor_ctx *ctx,
					struct mtk_cam_seninf_tsrec_timestamp_info *ts_info);

#endif
