/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2023 MediaTek Inc. */

#ifndef __ADAPTOR_TSREC_CB_IMPL_H__
#define __ADAPTOR_TSREC_CB_IMPL_H__


#include "adaptor-tsrec-cb-ctrl.h"


/******************************************************************************/
// adaptor tsrec call back ctrl --- function
/******************************************************************************/
static inline void adaptor_tsrec_cb_ctrl_init(struct adaptor_ctx *ctx)
{
	mutex_init(&ctx->tsrec_cb_ctrl.op_lock);
}

static inline void adaptor_tsrec_cb_ctrl_dbg_info_dump(struct adaptor_ctx *ctx,
	const char *caller)
{
	mutex_lock(&ctx->tsrec_cb_ctrl.op_lock);
	dev_info(ctx->dev,
		"[%s] [%s]: dump cb ctrl info:((%u)(seninf_idx%u, tsrec_no:%u, cb_func_ptr:%p))\n",
		__func__, caller,
		ctx->tsrec_cb_ctrl.cb_info.is_connected_to_tsrec,
		ctx->tsrec_cb_ctrl.cb_info.seninf_idx,
		ctx->tsrec_cb_ctrl.cb_info.tsrec_no,
		ctx->tsrec_cb_ctrl.cb_info.tsrec_cb_handler);
	mutex_unlock(&ctx->tsrec_cb_ctrl.op_lock);
}

static inline void adaptor_tsrec_cb_ctrl_info_setup(struct adaptor_ctx *ctx,
	struct mtk_cam_seninf_tsrec_cb_info *info, const unsigned int flag)
{
	mutex_lock(&ctx->tsrec_cb_ctrl.op_lock);
	if (flag) {
		/* set */
		ctx->tsrec_cb_ctrl.cb_info = *info;
	} else {
		/* clear */
		memset(&ctx->tsrec_cb_ctrl.cb_info, 0,
			sizeof(ctx->tsrec_cb_ctrl.cb_info));
	}
	mutex_unlock(&ctx->tsrec_cb_ctrl.op_lock);
}

static inline int adaptor_tsrec_cb_ctrl_execute(struct adaptor_ctx *ctx,
	const enum tsrec_cb_cmd cb_cmd, void *arg, const char *caller)
{
	int ret;

	mutex_lock(&ctx->tsrec_cb_ctrl.op_lock);

	if (unlikely(ctx->tsrec_cb_ctrl.cb_info.is_connected_to_tsrec == 0)) {
		ret = TSREC_CB_CTRL_ERR_NOT_CONNECTED_TO_TSREC;
		goto adaptor_tsrec_cb_ctrl_execute_end;
	}
	/* error case, connected to tsrec but function pointer is NULL */
	if (unlikely(ctx->tsrec_cb_ctrl.cb_info.tsrec_cb_handler == NULL)) {
		ret = TSREC_CB_CTRL_ERR_CB_FUNC_PTR_NULL;
		goto adaptor_tsrec_cb_ctrl_execute_end;
	}

	/* call back to seninf-tsrec */
	ret = ctx->tsrec_cb_ctrl.cb_info.tsrec_cb_handler(
		ctx->tsrec_cb_ctrl.cb_info.seninf_idx,
		ctx->tsrec_cb_ctrl.cb_info.tsrec_no,
		(const unsigned int)cb_cmd, arg, caller);

adaptor_tsrec_cb_ctrl_execute_end:
	mutex_unlock(&ctx->tsrec_cb_ctrl.op_lock);
	return ret;
}

#endif
