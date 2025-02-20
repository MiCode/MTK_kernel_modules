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

static inline u32 g_sensor_frame_length_delay(struct adaptor_ctx *ctx,
	const u32 scenario_id, const char *caller)
{
	const u32 g_fdelay = ctx->subctx.frame_time_delay_frame;
	u32 m_fdelay; // from sensor drv mode info struct

	if (unlikely(!chk_is_valid_scenario_id(ctx, scenario_id, caller)))
		return g_fdelay;

	m_fdelay = ctx->subctx.s_ctx.mode[scenario_id].sw_fl_delay;
	return (m_fdelay) ? m_fdelay : g_fdelay;
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

int g_sensor_fine_integ_line(struct adaptor_ctx *ctx,
	const unsigned int scenario);

/* return: 0 => NON DCG; 1 => DCG */
u32 g_sensor_dcg_property(struct adaptor_ctx *ctx, const u32 scenario_id);

/* return: 0 => NON LBMF; 1 => LBMF */
u32 g_sensor_lbmf_property(struct adaptor_ctx *ctx, const u32 scenario_id,
	struct adaptor_sensor_lbmf_property_st *prop);

#endif
