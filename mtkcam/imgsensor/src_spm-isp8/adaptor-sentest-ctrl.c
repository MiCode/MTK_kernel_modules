// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include "adaptor-sentest-ctrl.h"

#include <linux/mutex.h>

/******************************************************************************/
// adaptor sentest call back for ioctl & adaptor framework using
/******************************************************************************/

static struct mtk_cam_seninf_tsrec_timestamp_info sentest_current_tsrec_info;

int notify_sentest_tsrec_time_stamp(struct adaptor_ctx *ctx,
					struct mtk_cam_seninf_tsrec_timestamp_info *info)
{
	static u64 current_frame_id;
	struct mtk_cam_sentest_cfg_info *sentest_info;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(info == NULL)) {
		pr_info("[%s][ERROR] info is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_info = &ctx->sentest_cfg_info;
	if (unlikely(sentest_info == NULL)) {
		pr_info("[%s][ERROR] sentest_info is NULL\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&sentest_info->sentest_update_tsrec_mutex);
	current_frame_id++;

	if (!sentest_info->listen_tsrec_frame_id) {
		memcpy(&sentest_current_tsrec_info, info,
					sizeof(sentest_current_tsrec_info));

		dev_info(ctx->dev, "[%s] store tsrec info done\n", __func__);

	} else if (sentest_info->listen_tsrec_frame_id == current_frame_id) {
		memcpy(&sentest_current_tsrec_info, info,
				sizeof(sentest_current_tsrec_info));

		dev_info(ctx->dev, "[%s] target frame %d tsrec info done\n",
					__func__, sentest_info->listen_tsrec_frame_id);
	}

	mutex_unlock(&sentest_info->sentest_update_tsrec_mutex);

	return 0;
}

int sentest_get_current_tsrec_info(struct adaptor_ctx *ctx,
					struct mtk_cam_seninf_tsrec_timestamp_info *info)
{
	struct mtk_cam_sentest_cfg_info *sentest_info;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(info == NULL)) {
		pr_info("[%s][ERROR] info is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_info = &ctx->sentest_cfg_info;
	if (unlikely(sentest_info == NULL)) {
		pr_info("[%s][ERROR] sentest_info is NULL\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&sentest_info->sentest_update_tsrec_mutex);
	memcpy(&info, &sentest_current_tsrec_info, sizeof(info));
	mutex_unlock(&sentest_info->sentest_update_tsrec_mutex);

	return 0;
}

int sentest_probe_init(struct adaptor_ctx *ctx)
{
	struct mtk_cam_sentest_cfg_info *sentest_info;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_info = &ctx->sentest_cfg_info;
	if (unlikely(sentest_info == NULL)) {
		pr_info("[%s][ERROR] sentest_info is NULL\n", __func__);
		return -EINVAL;
	}

	mutex_init(&sentest_info->sentest_update_tsrec_mutex);

	/*binding sentest related flags into subctx */
	ctx->subctx.power_on_profile_en = &sentest_info->power_on_profile_en;

	return 0;
}

int sentest_flag_init(struct adaptor_ctx *ctx)
{
	struct mtk_cam_sentest_cfg_info *sentest_info;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_info = &ctx->sentest_cfg_info;
	if (unlikely(sentest_info == NULL)) {
		pr_info("[%s][ERROR] sentest_info is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_info->listen_tsrec_frame_id = 0;
	sentest_info->power_on_profile_en = 0;
	sentest_info->lbmf_delay_do_ae_en = 0;

	return 0;
}
