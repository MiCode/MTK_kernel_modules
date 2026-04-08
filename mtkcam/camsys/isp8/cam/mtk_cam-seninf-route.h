/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2019 MediaTek Inc. */

#ifndef __MTK_CAM_SENINF_ROUTE_H__
#define __MTK_CAM_SENINF_ROUTE_H__


void mtk_cam_seninf_alloc_outmux(struct seninf_ctx *ctx);
void mtk_cam_seninf_outmux_put(struct seninf_ctx *ctx, struct seninf_outmux *outmux);
void mtk_cam_seninf_release_outmux(struct seninf_ctx *ctx);

void mtk_cam_seninf_get_vcinfo_test(struct seninf_ctx *ctx);

struct seninf_vc *mtk_cam_seninf_get_vc_by_pad(struct seninf_ctx *ctx, int idx);

int mtk_cam_seninf_get_vcinfo(struct seninf_ctx *ctx);

int mtk_cam_seninf_set_vc_info_to_tsrec(struct seninf_ctx *ctx,
			struct seninf_vc *vc,
			enum mtk_cam_seninf_tsrec_exp_id exp_id,
			u8 pre_latch_exp);

int mtk_cam_seninf_is_vc_enabled(struct seninf_ctx *ctx,
				 struct seninf_vc *vc);

int mtk_cam_seninf_is_di_enabled(struct seninf_ctx *ctx, u8 ch, u8 dt);

int mtk_cam_seninf_get_csi_param(struct seninf_ctx *ctx);
int mtk_cam_seninf_s_stream_mux(struct seninf_ctx *ctx);
int mtk_cam_seninf_forget_camtg_setting(struct seninf_ctx *ctx);
void mtk_cam_sensor_get_vc_info_by_scenario(struct seninf_ctx *ctx, u32 code);

#ifdef SENINF_DEBUG
int mux2mux_vr(struct seninf_ctx *ctx, int mux, int cammux, int vc_idx);
int mux_vr2mux(struct seninf_ctx *ctx, int mux_vr);
enum CAM_TYPE_ENUM cammux2camtype(struct seninf_ctx *ctx, int cammux);
#endif

int aov_switch_i2c_bus_scl_aux(struct seninf_ctx *ctx,
	enum mtk_cam_sensor_i2c_bus_scl aux);
int aov_switch_i2c_bus_sda_aux(struct seninf_ctx *ctx,
	enum mtk_cam_sensor_i2c_bus_sda aux);

int aov_switch_pm_ops(struct seninf_ctx *ctx,
	enum mtk_cam_sensor_pm_ops pm_ops);

int aov_switch_mclk_ulposc(struct seninf_ctx *ctx,
	unsigned int enable);

#endif
