/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2023 MediaTek Inc. */

#ifndef __MTK_CAM_SENINF_SENTEST_CTRL_H__
#define __MTK_CAM_SENINF_SENTEST_CTRL_H__

#include "mtk_cam-seninf.h"
#include "mtk_cam-seninf-tsrec.h"
#include "mtk_cam-seninf_control-8.h"

/******************************************************************************/
// seninf sentest call back ctrl --- function
/******************************************************************************/

int seninf_sentest_get_debug_reg_result(struct seninf_ctx *ctx, void *arg);

int seninf_sentest_watchingdog_en(struct mtk_cam_sentest_watchdog *wd, bool en);

int seninf_sentest_watchdog_init(struct mtk_cam_sentest_watchdog *wd);

int seninf_sentest_flag_init(struct seninf_ctx *ctx);

int seninf_sentest_probe_init(struct seninf_ctx *ctx);

int seninf_sentest_uninit(struct seninf_ctx *ctx);

int notify_sentest_irq(struct seninf_ctx *ctx,
			const struct mtk_cam_seninf_tsrec_irq_notify_info *p_info);

int seninf_sentest_get_csi_mipi_measure_result(struct seninf_ctx *ctx,
			struct mtk_cam_seninf_meter_info *info);

#endif
