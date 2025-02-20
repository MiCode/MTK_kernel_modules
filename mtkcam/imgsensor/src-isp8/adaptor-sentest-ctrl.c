// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include "adaptor-sentest-ctrl.h"

#include <linux/mutex.h>

/******************************************************************************/
// adaptor sentest call back for ioctl & adaptor framework using
/******************************************************************************/

int notify_sentest_tsrec_time_stamp(struct adaptor_ctx *ctx,
					struct mtk_cam_seninf_tsrec_timestamp_info *info)
{
	struct mtk_cam_sentest_cfg_info *sentest_info;
	static u64 prev_ts0;

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

	if (prev_ts0 == info->exp_recs[0].ts_us[0])
		return 0;

	mutex_lock(&sentest_info->sentest_update_tsrec_mutex);

	prev_ts0 = info->exp_recs[0].ts_us[0];
	sentest_info->ts.sentest_tsrec_frame_cnt++;
	sentest_info->ts.sys_time_ns = info->irq_sys_time_ns;

	memcpy(sentest_info->ts.ts_us, info->exp_recs[0].ts_us,
					(sizeof(u64) * TSREC_TS_REC_MAX_CNT));

	mutex_unlock(&sentest_info->sentest_update_tsrec_mutex);

	return 0;
}

int sentest_get_current_tsrec_info(struct adaptor_ctx *ctx,
					struct mtk_cam_seninf_sentest_ts *info)
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
	if (copy_to_user(info, &sentest_info->ts,
					sizeof(struct mtk_cam_seninf_sentest_ts))) {
		dev_info(ctx->dev,
			"[%s][ERR] copy_to_user return failed\n", __func__);
		mutex_unlock(&sentest_info->sentest_update_tsrec_mutex);
		return -EFAULT;
	}
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

	memset(&sentest_info->ts, 0, sizeof(struct mtk_cam_seninf_sentest_ts));

	return 0;
}
