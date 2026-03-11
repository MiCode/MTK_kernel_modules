/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2019 MediaTek Inc. */

#ifndef __ADAPTOR_HW_H__
#define __ADAPTOR_HW_H__

int adaptor_hw_power_on(struct adaptor_ctx *ctx);
int adaptor_hw_power_off(struct adaptor_ctx *ctx);
int adaptor_hw_init(struct adaptor_ctx *ctx);
int adaptor_hw_sensor_reset(struct adaptor_ctx *ctx);
int adaptor_cam_pmic_on(struct adaptor_ctx *ctx);
int adaptor_cam_pmic_off(struct adaptor_ctx *ctx);
#if ALWAYS_ON_POWER
int do_hw_power_off(struct adaptor_ctx *ctx);
int do_hw_power_on(struct adaptor_ctx *ctx);
int do_hw_power_off_of_suspend(struct adaptor_ctx *ctx);
#endif
extern int check_multicam_power(struct adaptor_ctx *ctx, int flag);
extern void imgsensor_multicam_mutex_lock(struct adaptor_ctx *ctx);
extern void imgsensor_multicam_mutex_unlock(struct adaptor_ctx *ctx);
extern void imgsensor_multicam_mutex_lock_for_power(struct adaptor_ctx *ctx);
extern void imgsensor_multicam_mutex_unlock_for_power(struct adaptor_ctx *ctx);
extern void imgsensor_multicam_always_on_process(struct adaptor_ctx *ctx);
#endif
