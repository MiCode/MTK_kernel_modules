// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include "adaptor-sentest-ioctrl.h"
#include "adaptor-sentest-ctrl.h"

/******************************************************************************/
// adaptor sentest call back ioctrl --- function
/******************************************************************************/
struct adaptor_sentest_ioctl {
	enum sentest_ctrl_id ctrl_id;
	int (*func)(struct adaptor_ctx *ctx, void *arg);
};

static int sentest_g_sensor_profile(struct adaptor_ctx *ctx, void *arg)
{
	struct subdrv_ctx *subctx;
	struct mtk_sensor_profile *user_sensor_profile = arg;
	struct mtk_sensor_profile sensor_profile;
	struct mtk_cam_sentest_cfg_info *sentest_cfg_info;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(user_sensor_profile == NULL)) {
		pr_info("[%s][ERROR] user_sensor_profile is NULL\n", __func__);
		return -EINVAL;
	}

	subctx = &ctx->subctx;
	if (unlikely(subctx == NULL)) {
		pr_info("[%s][ERROR] subctx is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_cfg_info = &ctx->sentest_cfg_info;
	if (unlikely(sentest_cfg_info == NULL)) {
		pr_info("[%s][ERROR] sentest_cfg_info is NULL\n", __func__);
		return -EINVAL;
	}

	if (!sentest_cfg_info->power_on_profile_en) {
		memset((void *)user_sensor_profile, 0, sizeof(struct mtk_sensor_profile));
		dev_info(ctx->dev,
			"[%s] power_on_profile_en is %d, init the sensor_profile\n",
				__func__, sentest_cfg_info->power_on_profile_en);

		return -EFAULT;
	}

	sensor_profile = subctx->sensor_pw_on_profile;

	user_sensor_profile->i2c_init_period = sensor_profile.i2c_init_period;
	user_sensor_profile->i2c_init_table_len = sensor_profile.i2c_init_table_len;
	user_sensor_profile->i2c_cfg_period = sensor_profile.i2c_cfg_period;
	user_sensor_profile->i2c_cfg_table_len = sensor_profile.i2c_cfg_table_len;
	user_sensor_profile->hw_power_on_period = sensor_profile.hw_power_on_period;

	dev_info(ctx->dev, "sensor_profile copy_to_user is done\n");

	return 0;
}

static int sentest_g_tsrec_info(struct adaptor_ctx *ctx, void *arg)
{
	return sentest_get_current_tsrec_info(ctx,
				(struct mtk_cam_seninf_tsrec_timestamp_info *)arg);
}

static int sentest_s_tsrec_traget_frame_id(struct adaptor_ctx *ctx, void *arg)
{
	u32 *frame_id = arg;
	struct mtk_cam_sentest_cfg_info *sentest_info;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(frame_id == NULL)) {
		pr_info("[%s][ERROR] frame_id is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_info = &ctx->sentest_cfg_info;
	if (unlikely(sentest_info == NULL)) {
		pr_info("[%s][ERROR] sentest_info is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_info->listen_tsrec_frame_id = *frame_id;

	dev_info(ctx->dev, "listen_tsrec_frame_id: %u\n",
				 sentest_info->listen_tsrec_frame_id);

	return 0;
}

static int sentest_s_sensor_profile_en(struct adaptor_ctx *ctx, void *arg)
{
	int *en = arg;
	struct mtk_cam_sentest_cfg_info *sentest_cfg_info;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(en == NULL)) {
		pr_info("[%s][ERROR] en is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_cfg_info = &ctx->sentest_cfg_info;
	if (unlikely(sentest_cfg_info == NULL)) {
		pr_info("[%s][ERROR] sentest_cfg_info is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_cfg_info->power_on_profile_en = (*en) ? true : false;

	dev_info(ctx->dev, "[%s] en: %d, power_on_profile_en is %d\n",
				__func__, *en, sentest_cfg_info->power_on_profile_en);

	return 0;
}

static int sentest_s_lbmf_delay_do_ae_en(struct adaptor_ctx *ctx, void *arg)
{
	int *en = arg;
	struct mtk_cam_sentest_cfg_info *sentest_cfg_info;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(en == NULL)) {
		pr_info("[%s][ERROR] en is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_cfg_info = &ctx->sentest_cfg_info;
	if (unlikely(sentest_cfg_info == NULL)) {
		pr_info("[%s][ERROR] sentest_cfg_info is NULL\n", __func__);
		return -EINVAL;
	}

	sentest_cfg_info->lbmf_delay_do_ae_en = (*en) ? true : false;

	dev_info(ctx->dev, "[%s] en: %d, lbmf_delay_do_ae_en is %d\n",
				__func__, *en, sentest_cfg_info->lbmf_delay_do_ae_en);

	return 0;
}

static const struct adaptor_sentest_ioctl sentest_ioctl_table[] = {
	{SENTEST_G_SENSOR_PROFILE, sentest_g_sensor_profile},
	{SENTEST_G_TSREC_TIME_STAMP, sentest_g_tsrec_info},
	{SENTEST_G_CTRL_ID_MAX, NULL},
	{SENTEST_S_SENSOR_PROFILE_EN, sentest_s_sensor_profile_en},
	{SENTEST_S_SENSOR_LBMF_DO_DELAY_AE_EN, sentest_s_lbmf_delay_do_ae_en},
	{SENTEST_S_TSREC_TRAGET_FRAME_ID, sentest_s_tsrec_traget_frame_id},

};

int sentest_ioctl_entry(struct adaptor_ctx *ctx, void *arg)
{
	int i;
	int len = ARRAY_SIZE(sentest_ioctl_table);
	struct mtk_adaptor_sentest_ctrl *ctrl_info = (struct mtk_adaptor_sentest_ctrl *)arg;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(ctrl_info == NULL)) {
		pr_info("[%s][ERROR] ctrl_info is NULL\n", __func__);
		return -EINVAL;
	}

	for (i = SENTEST_G_CTRL_ID_MIN; i < SENTEST_S_CTRL_ID_MAX && i < len; i ++) {
		if (ctrl_info->ctrl_id == sentest_ioctl_table[i].ctrl_id) {
			if (unlikely(sentest_ioctl_table[i].func == NULL))
				pr_info("[%s][ERROR] sentest_ioctl_table[i].func is NULL\n", __func__);
			else
				return sentest_ioctl_table[i].func(ctx, (void *)ctrl_info->param_ptr);
		}
	}

	pr_info("[ERROR][%s] ctrl_id %d not found\n", __func__, ctrl_info->ctrl_id);
	return -EINVAL;
}
