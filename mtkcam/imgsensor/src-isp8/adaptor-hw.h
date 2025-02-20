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
int adaptor_hw_deinit(struct adaptor_ctx *ctx);

#ifdef __XIAOMI_CAMERA__
#include <linux/workqueue.h>
#include "adaptor.h"
extern uint poweroff_timeout_ms;
void adaptor_hw_power_off_work(struct work_struct *work);
int adaptor_hw_power_off_deferred(struct adaptor_ctx *ctx);
void adaptor_hw_drain_power_off_work(struct adaptor_ctx *ctx);
void adaptor_hw_power_off_work(struct work_struct *work);
#endif

#endif
