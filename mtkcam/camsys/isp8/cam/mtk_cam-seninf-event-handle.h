/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2024 MediaTek Inc.

#ifndef __MTK_CAM_SENINF_EVENT_HANDLE_H__
#define __MTK_CAM_SENINF_EVENT_HANDLE_H__

#include "mtk_cam-seninf.h"
#include "mtk_cam-seninf-tsrec.h"


/*----------------------------------------------------------------------------*/
// => sensor event/handle
//    some APIs are declared in mtk_cam-seninf-if.h
/*----------------------------------------------------------------------------*/
u8 is_reset_by_user(struct seninf_ctx *ctx);
int reset_sensor(struct seninf_ctx *ctx);
bool has_multiple_expo_mode(struct seninf_ctx *ctx);
void mtk_cam_sensor_get_frame_cnt(struct seninf_ctx *ctx, u32 *frame_cnt);
void mtk_cam_sensor_get_glp_dt(struct seninf_ctx *ctx,
	struct seninf_glp_dt *info);


/*----------------------------------------------------------------------------*/
// => seninf event/handle
/*----------------------------------------------------------------------------*/
int seninf_get_fmeter_clk(struct seninf_core *core,
	int clk_fmeter_idx, unsigned int *out_clk);


/*----------------------------------------------------------------------------*/
// => fsync event/handle
/*----------------------------------------------------------------------------*/
void setup_fsync_vsync_src_pad(struct seninf_ctx *ctx,
	const u64 fsync_ext_vsync_pad_code);
void chk_is_fsync_vsync_src(struct seninf_ctx *ctx, const int pad_id);
int notify_fsync_listen_target(struct seninf_ctx *ctx);
void notify_fsync_listen_target_with_kthread(struct seninf_ctx *ctx,
	const unsigned int mdelay);


/*----------------------------------------------------------------------------*/
// => tsrec event/handle
/*----------------------------------------------------------------------------*/
void mtk_cam_seninf_tsrec_irq_notify(
	const struct mtk_cam_seninf_tsrec_irq_notify_info *p_info);


/*----------------------------------------------------------------------------*/
// => camsys event/handle
//    some APIs are declared in mtk_cam-seninf-if.h
/*----------------------------------------------------------------------------*/

#endif /* __MTK_CAM_SENINF_EVENT_HANDLE_H__ */
