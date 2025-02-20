// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include "mtk_cam-seninf-sentest-ioctrl.h"
#include "mtk_cam-seninf-sentest-ctrl.h"
#include "mtk_cam-seninf_control-8.h"
#include "mtk_cam-seninf-hw.h"
#include "imgsensor-user.h"

/******************************************************************************/
// seninf sentest call back ioctrl --- function
/******************************************************************************/

struct seninf_sentest_ioctl {
	enum seninf_sentest_ctrl_id ctrl_id;
	int (*func)(struct seninf_ctx *ctx, void *arg);
};

static int s_sentest_max_isp_clk_en(struct seninf_ctx *ctx, void *arg)
{
	int *en = kmalloc(sizeof(int), GFP_KERNEL);

	if (unlikely(en == NULL)) {
		pr_info("[%s][ERROR] en is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		kfree(en);
		return -EINVAL;
	}

	if (ctx->streaming){
		dev_info(ctx->dev,
				"[ERROR][%s] set max_clk_en failed, due to streaming is %d\n",
				__func__, ctx->streaming);
		kfree(en);
		return -EINVAL;
	}

	if (copy_from_user(en, arg, sizeof(int))) {
		pr_info("[%s][ERROR] copy_from_user return failed\n", __func__);
		kfree(en);
		return -EFAULT;
	}

	ctx->sentest_adjust_isp_en = *en;

	dev_info(ctx->dev, "[%s] en: %d, sentest_adjust_isp_en is %d\n",
				__func__, *en, ctx->sentest_adjust_isp_en);

	kfree(en);
	return 0;
}

static int s_sentest_mipi_measure_en(struct seninf_ctx *ctx, void *arg)
{
	int *en = kmalloc(sizeof(int), GFP_KERNEL);

	if (unlikely(en == NULL)) {
		pr_info("[%s][ERROR] en is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		kfree(en);
		return -EINVAL;
	}

	if (ctx->streaming){
		dev_info(ctx->dev,
				"[ERROR][%s] set mipi_measure_en failed, due to streaming is %d\n",
				__func__, ctx->streaming);
		kfree(en);
		return -EINVAL;
	}

	if (copy_from_user(en, arg, sizeof(int))) {
		pr_info("[%s][ERROR] copy_from_user return failed\n", __func__);
		kfree(en);
		return -EFAULT;
	}

	ctx->sentest_mipi_measure_en = *en;

	dev_info(ctx->dev, "[%s] en: %d, sentest_mipi_measure_en is %d\n",
				__func__, *en, ctx->sentest_mipi_measure_en);

	kfree(en);
	return 0;
}

static inline int g_sentest_debug_result(struct seninf_ctx *ctx, void *arg)
{
	return seninf_sentest_get_debug_reg_result(ctx, arg);
}

static int s_sentest_seamless_ut_en(struct seninf_ctx *ctx, void *arg)
{
	int *en = kmalloc(sizeof(int), GFP_KERNEL);

	if (unlikely(en == NULL)) {
		pr_info("[%s][ERROR] en is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		kfree(en);
		return -EINVAL;
	}

	if (copy_from_user(en, arg, sizeof(int))) {
		pr_info("[%s][ERROR] copy_from_user return failed\n", __func__);
		kfree(en);
		return -EFAULT;
	}

	ctx->sentest_seamless_ut_en = *en;
	ctx->sentest_seamless_irq_ref = ctx->sentest_irq_counter;

	if (*en)
		ctx->sentest_seamless_ut_status = SENTEST_SEAMLESS_IS_DOING;
	else
		ctx->sentest_seamless_ut_status = SENTEST_SEAMLESS_IS_IDLE;

	dev_info(ctx->dev, "[%s] en: %d, sentest_seamless_ut_en is %d\n",
				__func__, *en, ctx->sentest_seamless_ut_en);

	kfree(en);
	return 0;
}

static int s_sentest_seamless_ut_cfg(struct seninf_ctx *ctx, void *arg)
{
	struct mtk_seamless_switch_param *config =
						kmalloc(sizeof(struct mtk_seamless_switch_param),
								GFP_KERNEL);

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		goto SENTEST_SEAMLESS_UT_CFG_ERR_EXIT;
	}

	memset(&ctx->sentest_seamless_cfg, 0, sizeof(struct mtk_seamless_switch_param));

	if (unlikely(config == NULL)) {
		pr_info("[%s][ERROR] config is NULL\n", __func__);
		goto SENTEST_SEAMLESS_UT_CFG_ERR_EXIT;
	}

	if (copy_from_user(config, arg, sizeof(struct mtk_seamless_switch_param))) {
		pr_info("[%s][ERROR] copy_from_user return failed\n", __func__);
		goto SENTEST_SEAMLESS_UT_CFG_ERR_EXIT;
	}

	memcpy(&ctx->sentest_seamless_cfg, config, sizeof(struct mtk_seamless_switch_param));

	kfree(config);
	return 0;

SENTEST_SEAMLESS_UT_CFG_ERR_EXIT:
	kfree(config);
	return -EFAULT;
}

static int g_sentest_seamless_current_status(struct seninf_ctx *ctx, void *arg)
{
	static enum SENTEST_SEAMLESS_STATUS current_status;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (copy_to_user(arg, &ctx->sentest_seamless_ut_status, sizeof(enum SENTEST_SEAMLESS_STATUS))) {
		pr_info("[%s][ERROR] copy_to_user return failed\n", __func__);
		return -EFAULT;
	}

	if (current_status != ctx->sentest_seamless_ut_status)
		dev_info(ctx->dev, "[%s] sentest_seamless_ut_status: %d\n",
				__func__, ctx->sentest_seamless_ut_status);

	current_status = ctx->sentest_seamless_ut_status;

	return 0;
}

static int g_sentest_sensor_meter_info(struct seninf_ctx *ctx, void *arg)
{
	struct mtk_cam_seninf_meter_info info;


	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (copy_from_user(&info, arg, sizeof(struct mtk_cam_seninf_meter_info))) {
		pr_info("[%s][ERROR] copy_to_user return failed\n", __func__);
		return -EFAULT;
	}

	if (seninf_sentest_get_csi_mipi_measure_result(ctx, &info)) {
		pr_info("[%s][ERROR] copy_to_user return failed\n", __func__);
		return -EFAULT;
	}

	if (copy_to_user(arg, &info, sizeof(struct mtk_cam_seninf_meter_info))) {
		pr_info("[%s][ERROR] copy_to_user return failed\n", __func__);
		return -EFAULT;
	}
	return 0;
}

static const struct seninf_sentest_ioctl sentest_ioctl_table[] = {
	{SENINF_SENTEST_S_MAX_ISP_EN, s_sentest_max_isp_clk_en},
	{SENINF_SENTEST_S_SINGLE_STREAM_RAW, s_sentest_mipi_measure_en},
	{SENINF_SENTEST_S_SEAMLESS_UT_EN, s_sentest_seamless_ut_en},
	{SENINF_SENTEST_S_SEAMLESS_UT_CONFIG, s_sentest_seamless_ut_cfg},

	{SENINF_SENTEST_G_DEBUG_RESULT, g_sentest_debug_result},
	{SENINF_SENTEST_G_SEAMLESS_STATUS, g_sentest_seamless_current_status},
	{SENINF_SENTEST_G_SENSOR_METER_INFO_BY_LINE, g_sentest_sensor_meter_info},
};

int seninf_sentest_ioctl_entry(struct seninf_ctx *ctx, void *arg)
{
	int i;
	struct mtk_seninf_sentest_ctrl *ctrl_info = (struct mtk_seninf_sentest_ctrl *)arg;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(ctrl_info == NULL)) {
		pr_info("[%s][ERROR] ctrl_info is NULL\n", __func__);
		return -EINVAL;
	}

	for (i = SENINF_SENTEST_G_CTRL_ID_MIN; i < SENINF_SENTEST_S_CTRL_ID_MAX; i ++) {
		if (ctrl_info->ctrl_id == sentest_ioctl_table[i].ctrl_id)
			return sentest_ioctl_table[i].func(ctx, ctrl_info->param_ptr);
	}

	pr_info("[ERROR][%s] ctrl_id %d not found\n", __func__, ctrl_info->ctrl_id);
	return -EINVAL;
}
