/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2019 MediaTek Inc. */

#ifndef __ADAPTOR_CTRLS_H__
#define __ADAPTOR_CTRLS_H__

int adaptor_init_ctrls(struct adaptor_ctx *ctx);

void restore_ae_ctrl(struct adaptor_ctx *ctx);

void adaptor_sensor_init(struct adaptor_ctx *ctx);

u32 get_mode_vb(struct adaptor_ctx *ctx, const struct sensor_mode *mode);

int update_framelength_for_timeout(struct adaptor_ctx *ctx);

int update_shutter_for_timeout(struct adaptor_ctx *ctx);

int update_shutter_for_timeout_by_ae_ctrl(struct adaptor_ctx *ctx, struct mtk_hdr_ae *ae_ctrl);
extern int check_multicam_streaming(struct adaptor_ctx *ctx, int flag);
extern int check_multicam_sensor_init(struct adaptor_ctx *ctx);

extern void imgsensor_multicam_mutex_lock(struct adaptor_ctx *ctx);
extern void imgsensor_multicam_mutex_unlock(struct adaptor_ctx *ctx);
extern void imgsensor_multicam_mutex_lock_for_power(struct adaptor_ctx *ctx);
extern void imgsensor_multicam_mutex_unlock_for_power(struct adaptor_ctx *ctx);
#endif
