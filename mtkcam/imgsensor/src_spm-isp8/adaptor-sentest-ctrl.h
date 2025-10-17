/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2023 MediaTek Inc. */

#ifndef __ADAPTOR_SENTEST_CTRL_H__
#define __ADAPTOR_SENTEST_CTRL_H__

#include "adaptor.h"
#include "adaptor-fsync-ctrls.h"
#include "adaptor-tsrec-cb-ctrl-impl.h"


/******************************************************************************/
// adaptor sentest call back ctrl --- function
/******************************************************************************/
int sentest_probe_init(struct adaptor_ctx *ctx);

int sentest_flag_init(struct adaptor_ctx *ctx);

int notify_sentest_tsrec_time_stamp(struct adaptor_ctx *ctx,
					struct mtk_cam_seninf_tsrec_timestamp_info *info);

int sentest_get_current_tsrec_info(struct adaptor_ctx *ctx,
					struct mtk_cam_seninf_tsrec_timestamp_info *info);

#endif
