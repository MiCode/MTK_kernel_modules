// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

//#define DEBUG

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>

#include "kd_imgsensor_define_v4l2.h"
#include "adaptor.h"
#include "adaptor-hw.h"
#include "adaptor-profile.h"
#include "adaptor-util.h"
#include <linux/clk-provider.h>
#include "adaptor-i2c.h"

#define INST_OPS(__ctx, __field, __idx, __hw_id, __set, __unset) do {\
	if (__ctx->__field[__idx]) { \
		__ctx->hw_ops[__hw_id].set = __set; \
		__ctx->hw_ops[__hw_id].unset = __unset; \
		__ctx->hw_ops[__hw_id].data = (void *)__idx; \
	} \
} while (0)

static const char * const clk_names[] = {
	ADAPTOR_CLK_NAMES
};

static const char * const reg_names[] = {
	ADAPTOR_REGULATOR_NAMES
};

static const char * const state_names[] = {
	ADAPTOR_STATE_NAMES
};

static int pmic_wake_en;
static struct clk *get_clk_by_idx_freq(struct adaptor_ctx *ctx,
				unsigned long long idx, int freq)
{
	if (idx == CLK_MCLK) {
		switch (freq) {
		case 6:
			return ctx->clk[CLK_6M];
		case 12:
			return ctx->clk[CLK_12M];
		case 13:
			return ctx->clk[CLK_13M];
		case 19:
			return ctx->clk[CLK_19_2M];
		case 24:
			if (ctx->aov_mclk_ulposc_flag)
				return ctx->clk[CLK_26M];
			else
				return ctx->clk[CLK_24M];
		case 26:
			return ctx->clk[CLK_26M];
		case 52:
			return ctx->clk[CLK_52M];
		}
	} else if (idx == CLK1_MCLK1) {
		switch (freq) {
		case 6:
			return ctx->clk[CLK1_6M];
		case 12:
			return ctx->clk[CLK1_12M];
		case 13:
			return ctx->clk[CLK1_13M];
		case 19:
			return ctx->clk[CLK1_19_2M];
		case 24:
#if IMGSENSOR_AOV_EINT_UT
			return ctx->clk[CLK_24M];
#else
			if (ctx->aov_mclk_ulposc_flag)
				return ctx->clk[CLK1_26M_ULPOSC];
			else
				return ctx->clk[CLK1_24M];
#endif
		case 26:
			if (ctx->aov_mclk_ulposc_flag)
				return ctx->clk[CLK1_26M_ULPOSC];
			else
				return ctx->clk[CLK1_26M];
		case 52:
			return ctx->clk[CLK1_52M];
		}
	}

	return NULL;
}

static int set_mclk(struct adaptor_ctx *ctx, void *data, int val)
{
	int ret;
	struct clk *mclk, *mclk_src;
	unsigned long long idx;

	idx = (unsigned long long)data;
	mclk = ctx->clk[idx];
	mclk_src = get_clk_by_idx_freq(ctx, idx, val);

	adaptor_logm(ctx, "+ idx(%llu),val(%d)\n", idx, val);
	ret = clk_prepare_enable(mclk);
	if (ret) {
		adaptor_logi(ctx,
			"clk_prepare_enable(%s),ret(%d)(fail)\n",
			clk_names[idx], ret);
		return ret;
	}
	adaptor_logm(ctx,
		"clk_prepare_enable(%s),ret(%d)(correct)\n",
		clk_names[idx], ret);
	ret = clk_set_parent(mclk, mclk_src);
	if (ret) {
		adaptor_logi(ctx,
			"mclk(%s) clk_set_parent (%s),ret(%d)(fail)\n",
			__clk_get_name(mclk), __clk_get_name(mclk_src), ret);
		WRAP_AEE_EXCEPTION("clk_set_parent", "Err");
		return ret;
	}
	adaptor_logm(ctx,
		"- clk_set_parent(%s),ret(%d)(correct)\n",
		__clk_get_name(mclk_src), ret);
	return 0;
}

static int unset_mclk(struct adaptor_ctx *ctx, void *data, int val)
{
	struct clk *mclk, *mclk_src;
	unsigned long long idx;

	idx = (unsigned long long)data;
	mclk = ctx->clk[idx];
	mclk_src = get_clk_by_idx_freq(ctx, idx, val);

	adaptor_logm(ctx, "+ idx(%llu),val(%d)\n", idx, val);

	clk_disable_unprepare(mclk);

	adaptor_logm(ctx,
		"- clk_disable_unprepare(%s)\n",
		clk_names[idx]);
	return 0;
}

static int set_reg(struct adaptor_ctx *ctx, void *data, int val)
{
	unsigned long long ret, idx;
	struct regulator *reg;

	idx = (unsigned long long)data;


	if (ctx->regulator[idx] == NULL)
		return -EINVAL;


	reg = ctx->regulator[idx];
	adaptor_logm(ctx, "+ idx(%llu),val(%d)\n", idx, val);

	ret = regulator_set_voltage(reg, val, val);
	if (ret) {
		adaptor_loge(ctx,
			"regulator_set_voltage(%s),val(%d),ret(%llu)(fail)\n",
			reg_names[idx], val, ret);
	} else {
		adaptor_logm(ctx,
			"regulator_set_voltage(%s),val(%d),ret(%llu)(correct)\n",
			reg_names[idx], val, ret);
	}
	ret = regulator_enable(reg);
	if (ret) {
		adaptor_loge(ctx,
			"regulator_enable(%s),ret(%llu)(fail)\n",
			reg_names[idx], ret);
		return ret;
	}
	adaptor_logm(ctx,
		"- regulator_enable(%s),ret(%llu)(correct)\n",
		reg_names[idx], ret);
	return 0;
}

static int unset_reg(struct adaptor_ctx *ctx, void *data, int val)
{
	unsigned long long ret, idx;
	struct regulator *reg;

	idx = (unsigned long long)data;
	reg = ctx->regulator[idx];

	adaptor_logm(ctx, "+ idx(%llu),val(%d)\n", idx, val);
	ret = regulator_disable(reg);
	if (ret) {
		adaptor_loge(ctx,
			"disable(%s),ret(%llu)(fail)\n",
			reg_names[idx], ret);
		return ret;
	}

	adaptor_logm(ctx,
		"- disable(%s),ret(%llu)(correct)\n",
		reg_names[idx], ret);
	return 0;
}

static int set_state(struct adaptor_ctx *ctx, void *data, int val)
{
	unsigned long long idx, x;
	int ret;

	idx = (unsigned long long)data;
	x = idx + val;

	adaptor_logm(ctx, "+ idx(%llu),val(%d)\n", idx, val);
	if (!ctx) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}
	ret = pinctrl_select_state(ctx->pinctrl, ctx->state[x]);
	if (ret < 0) {
		adaptor_loge(ctx,
			"select(%s),ret(%d)(fail)\n",
			state_names[x], ret);
		return ret;
	}
	adaptor_logm(ctx,
		"- select(%s),ret(%d)(correct)\n",
		state_names[x], ret);
	return 0;
}

static int unset_state(struct adaptor_ctx *ctx, void *data, int val)
{
	return set_state(ctx, data, 0);
}

static int set_state_div2(struct adaptor_ctx *ctx, void *data, int val)
{
	return set_state(ctx, data, val >> 1);
}

static int set_state_boolean(struct adaptor_ctx *ctx, void *data, int val)
{
	return set_state(ctx, data, !!val);
}

static int set_state_mipi_switch(struct adaptor_ctx *ctx, void *data, int val)
{
	return set_state(ctx, (void *)STATE_MIPI_SWITCH_ON, 0);
}

static int unset_state_mipi_switch(struct adaptor_ctx *ctx, void *data,
	int val)
{
	return set_state(ctx, (void *)STATE_MIPI_SWITCH_OFF, 0);
}

static int set_reg_pmic_wakeup(struct adaptor_ctx *ctx, unsigned long long data)
{
	unsigned long long ret, idx;
	struct regulator *reg;

	idx = data;

	// re-get reg everytime due to pmic limitation
	 ctx->regulator[idx] = devm_regulator_get_optional(ctx->dev, reg_names[idx]);
	if (IS_ERR(ctx->regulator[idx])) {
		ctx->regulator[idx] = NULL;
		adaptor_loge(ctx, "no reg %s\n", reg_names[idx]);
		return -EINVAL;
	}

	reg = ctx->regulator[idx];
	ret = regulator_enable(reg);

	if (ret) {
		adaptor_loge(ctx,
		"regulator_enable(%s),ret(%llu)(fail)\n",
		reg_names[idx], ret);
		return ret;
	}
	ctx->pmic_on_tick = ktime_get_boottime_ns();
	adaptor_logi(ctx,
		"- regulator_enable(%s),ret(%llu)(correct)\n",
		reg_names[idx], ret);

	return 0;
}

static int set_reg_pmic_sleep(struct adaptor_ctx *ctx, unsigned long long data)
{
	unsigned long long ret, idx;
	struct regulator *reg;

	idx = data;
	reg = ctx->regulator[idx];
	adaptor_logm(ctx, "[%s]+ idx(%llu)\n", __func__, idx);
	if (IS_ERR_OR_NULL(ctx->regulator[idx])) {
		adaptor_loge(ctx, "no reg %s\n", reg_names[idx]);
		return -EINVAL;
	}
	ret = regulator_disable(reg);
	if (ret) {
		adaptor_loge(ctx,
		"disable(%s),ret(%llu)(fail)\n",
		reg_names[idx], ret);
		return ret;
	}
	// always put reg due to pmic limitation
	devm_regulator_put(ctx->regulator[idx]);
	adaptor_logm(ctx,
		"disable(%s),ret(%llu)(correct)\n",
		reg_names[idx], ret);

	return 0;
}
static int reinit_pinctrl(struct adaptor_ctx *ctx)
{
	int i;
	struct device *dev = ctx->dev;

	adaptor_logm(ctx, "+\n");
	/* pinctrl */
	ctx->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(ctx->pinctrl)) {
		adaptor_loge(ctx, "fail to get pinctrl\n");
		return PTR_ERR(ctx->pinctrl);
	}

	/* pinctrl states */
	for (i = 0; i < STATE_MAXCNT; i++) {
		ctx->state[i] = pinctrl_lookup_state(
				ctx->pinctrl, state_names[i]);
		if (IS_ERR(ctx->state[i])) {
			ctx->state[i] = NULL;
			adaptor_logd(ctx, "no state %s\n", state_names[i]);
		}
	}
	adaptor_logm(ctx, "-\n");
	return 0;
}
int do_cam_pmic_on(struct adaptor_ctx *ctx)
{
	return set_reg_pmic_wakeup(ctx, REGULATOR_BASE);
}

int adaptor_cam_pmic_on(struct adaptor_ctx *ctx)
{
	pmic_wake_en = 1;
	adaptor_logi(ctx, "pmic_wake_en = true\n");
	return do_cam_pmic_on(ctx);
}

int do_cam_pmic_off(struct adaptor_ctx *ctx)
{
	return set_reg_pmic_sleep(ctx, REGULATOR_BASE);
}

int adaptor_cam_pmic_off(struct adaptor_ctx *ctx)
{
	return do_cam_pmic_off(ctx);
}

int adaptor_pmic_ctrl(struct adaptor_ctx *ctx, bool bPmicEnable)
{
	static int pmic_enable_cnt;
	unsigned long long pmic_timedffus;

	adaptor_logi(ctx, "[%s]+ bPmicEnable:%d, pmic_enable_cnt:%d+\n", __func__, bPmicEnable,
	pmic_enable_cnt);
	if (!ctx) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}
	if (ctx->pmic_delayus == 0) {
		adaptor_logi(ctx, "add extra delay to every sensor driver. pmic_delayus:%llu\n",
			ctx->pmic_delayus);
		return 0;
	}
	if (bPmicEnable) {
		if (pmic_enable_cnt == 0) {
			if (pmic_wake_en) {
				ctx->first_sensor_power_on_tick = ktime_get_boottime_ns();
				pmic_timedffus = (ctx->first_sensor_power_on_tick -
				ctx->pmic_on_tick)/1000;
				if (pmic_timedffus < ctx->pmic_delayus) {
					udelay((ctx->pmic_delayus- pmic_timedffus));
					adaptor_logi(ctx,
						"sen_pwr_on:%llu pmic_on:%llu pmic_dff:%llu pmic_delayus:%llu\n",
					ctx->first_sensor_power_on_tick,
					ctx->pmic_on_tick,
					pmic_timedffus,
					ctx->pmic_delayus);
				} else {
					adaptor_logi(ctx,
						"no delay:sen_pwr_on:%llu pmic_on:%llu pmic_dff:%llu pmic_delayus:%llu\n",
					ctx->first_sensor_power_on_tick,
					ctx->pmic_on_tick,
					pmic_timedffus,
					ctx->pmic_delayus);
				}
			} else {
				do_cam_pmic_on(ctx);
				udelay(ctx->pmic_delayus);
				adaptor_logi(ctx, "maximum delay:power_refcnt:%d pmic_delayus:%llu\n",
				ctx->power_refcnt, ctx->pmic_delayus);
			}
		} else {
			do_cam_pmic_on(ctx);
		}
		pmic_enable_cnt++;
	} else {
		do_cam_pmic_off(ctx);
		pmic_enable_cnt--;
		if (pmic_enable_cnt == 0) {
			pmic_wake_en = 0;
			adaptor_logi(ctx, "pmic_enable_cnt:%d pmic_wake_en:%d\n",
				pmic_enable_cnt, pmic_wake_en);
		}
	}
	adaptor_logi(ctx, "[%s]- bPmicEnable:%d, pmic_enable_cnt:%d-\n", __func__, bPmicEnable,
	pmic_enable_cnt);
	return 0;
}

int do_hw_power_on(struct adaptor_ctx *ctx)
{
	int i;
	const struct subdrv_pw_seq_entry *ent;
	struct adaptor_hw_ops *op;
	struct adaptor_profile_tv tv;
	struct adaptor_log_buf buf;
	struct subdrv_ctx *subctx;
	u64 time_boot_begin = 0;

	adaptor_logm(ctx, "+\n");
	if (!ctx || !(ctx->subdrv)) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}

	if (ctx->sensor_ws) {
		if (ctx->aov_pm_ops_flag == 0) {
			ctx->aov_pm_ops_flag = 1;
			__pm_stay_awake(ctx->sensor_ws);
		}
	} else {
		adaptor_logm(ctx, "__pm_stay_awake(fail)\n");
	}

	adaptor_log_buf_init(&buf, ADAPTOR_LOG_BUF_SZ);

	/* may be released for mipi switch */
	if (!ctx->pinctrl)
		reinit_pinctrl(ctx);

	subctx = &ctx->subctx;
	op = &ctx->hw_ops[HW_ID_MIPI_SWITCH];
	if (op->set)
		op->set(ctx, op->data, 0);

	if ((subctx->power_on_profile_en != NULL) &&
		(*subctx->power_on_profile_en))
		time_boot_begin = ktime_get_boottime_ns();

	adaptor_pmic_ctrl(ctx, true);
	for (i = 0; i < ctx->subdrv->pw_seq_cnt; i++) {
		if (ctx->ctx_pw_seq)
			ent = &ctx->ctx_pw_seq[i]; // use ctx pw seq
		else
			ent = &ctx->subdrv->pw_seq[i];
		op = &ctx->hw_ops[ent->id];
		if (!op->set) {
			adaptor_logd(ctx, "cannot set comp %d val %d\n",
				ent->id, ent->val);
			continue;
		}

		ADAPTOR_PROFILE_BEGIN(&tv);
		op->set(ctx, op->data, ent->val);
		ADAPTOR_PROFILE_END(&tv);

		{
			static const char * const hw_id_names[] = {
				HW_ID_NAMES
			};

			if (ent->id >= 0 && ent->id < ARRAY_SIZE(hw_id_names)) {
				adaptor_log_buf_gather(ctx, __func__, &buf, "[%s:%lldus]",
						hw_id_names[ent->id],
						(ADAPTOR_PROFILE_G_DIFF_NS(&tv) / 1000));
			} else {
				adaptor_log_buf_gather(ctx, __func__, &buf, "[hwid%d:%lldus]",
						ent->id,
						(ADAPTOR_PROFILE_G_DIFF_NS(&tv) / 1000));
			}
		}

		adaptor_logm(ctx, "set comp %d val %d\n",
			ent->id, ent->val);

		if (ent->delay)
			udelay(ent->delay);
	}

	if ((subctx->power_on_profile_en != NULL) &&
		(*subctx->power_on_profile_en)) {
		subctx->sensor_pw_on_profile.hw_power_on_period =
			ktime_get_boottime_ns() - time_boot_begin;
	}

	if (ctx->subdrv->ops->power_on)
		subdrv_call(ctx, power_on, NULL);

	adaptor_logm(ctx, "-\n");
	adaptor_log_buf_flush(ctx, __func__, &buf);
	adaptor_log_buf_deinit(&buf);

	return 0;
}

int adaptor_hw_power_on(struct adaptor_ctx *ctx)
{
#if ALWAYS_ON_POWER
	int ret;
#endif
	adaptor_logm(ctx, "+\n");
	if (!ctx) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}
#ifndef IMGSENSOR_USE_PM_FRAMEWORK
	adaptor_logd(ctx, "power ref cnt = %d\n", ctx->power_refcnt);
	ctx->power_refcnt++;
	if (ctx->power_refcnt > 1) {
		adaptor_logd(ctx, "already powered, cnt = %d\n", ctx->power_refcnt);
		return 0;
	}
#endif
	adaptor_logm(ctx, "-\n");
#if ALWAYS_ON_POWER
	if (check_multicam_power(ctx, 0)||ctx->always_on_flag) {
		if (pm_runtime_suspended(ctx->dev)) {
			ret = pm_runtime_get_sync(ctx->dev);
			if (ret < 0) {
				pm_runtime_put_noidle(ctx->dev);
				dev_info(ctx->dev, "%s pm_runtime_get_sync fail\n", __func__);
			}
		}
		if (ctx->always_on_flag) {
			if (ctx->sensor_ws) {
				if (ctx->aov_pm_ops_flag == 0) {
					ctx->aov_pm_ops_flag = 1;
					__pm_stay_awake(ctx->sensor_ws);
				}
			} else {
				adaptor_logm(ctx, "__pm_stay_awake(fail)\n");
			}
		}
		return 0;
	}
	if (pm_runtime_suspended(ctx->dev)) {
		ret = pm_runtime_get_sync(ctx->dev);
		if (ret < 0) {
			pm_runtime_put_noidle(ctx->dev);
			dev_info(ctx->dev, "%s pm_runtime_get_sync fail\n", __func__);
		}
	}
	do_hw_power_on(ctx);
	ctx->always_on_flag = 1;
	subdrv_call(ctx, pre_open);
	imgsensor_multicam_always_on_process(ctx);
	return 0;
#else
	if (check_multicam_power(ctx, 0))
		return 0;

	return do_hw_power_on(ctx);
#endif
}
#if ALWAYS_ON_POWER
int do_hw_power_off_of_suspend(struct adaptor_ctx *ctx)
{
	int i;
	const struct subdrv_pw_seq_entry *ent;
	struct adaptor_hw_ops *op;

	adaptor_logm(ctx, "+\n");
	/* call subdrv close function if sensor is streaming */
	if (ctx->subctx.is_streaming)
		subdrv_call(ctx, close);

	if (ctx->subctx.s_ctx.mode[ctx->subctx.current_scenario_id].rosc_mode) {
		for (i = 0; i < ctx->mclk_refcnt; i++) {
			// enable mclk
			if (clk_prepare_enable(ctx->clk[CLK1_MCLK1]))
				adaptor_logi(ctx,
				"clk_prepare_enable CLK1_MCLK1(fail)\n");
		}
		adaptor_logi(ctx, "[%s] rosc_mode recover. enable aov mclk.\n", __func__);
		ctx->mclk_refcnt = 0;
	}

	if (ctx->subdrv->ops->power_off)
		subdrv_call(ctx, power_off, NULL);

	for (i = ctx->subdrv->pw_seq_cnt - 1; i >= 0; i--) {
		if (ctx->ctx_pw_seq)
			ent = &ctx->ctx_pw_seq[i]; // use ctx pw seq
		else
			ent = &ctx->subdrv->pw_seq[i];
		op = &ctx->hw_ops[ent->id];
		if (!op->unset)
			continue;
		op->unset(ctx, op->data, ent->val);
		//msleep(ent->delay);
	}
	adaptor_pmic_ctrl(ctx, false);

	op = &ctx->hw_ops[HW_ID_MIPI_SWITCH];
	if (op->unset)
		op->unset(ctx, op->data, 0);

	/* the pins of mipi switch are shared. free it for another users */
	if (ctx->state[STATE_MIPI_SWITCH_ON] ||
		ctx->state[STATE_MIPI_SWITCH_OFF] ||
		ctx->state[STATE_DOVDD_ON] ||
		ctx->state[STATE_DOVDD_OFF]) {
		devm_pinctrl_put(ctx->pinctrl);
		ctx->pinctrl = NULL;
	}

	adaptor_logm(ctx, "-\n");
	return 0;
}
#endif
int do_hw_power_off(struct adaptor_ctx *ctx)
{
	int i;
	const struct subdrv_pw_seq_entry *ent;
	struct adaptor_hw_ops *op;

	adaptor_logm(ctx, "+\n");
	if (!ctx || !(ctx->subdrv)) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}

	/* call subdrv close function if sensor is streaming */
	if (ctx->subctx.is_streaming)
		subdrv_call(ctx, close);

	if (ctx->subctx.s_ctx.mode[ctx->subctx.current_scenario_id].rosc_mode) {
		for (i = 0; i < ctx->mclk_refcnt; i++) {
			// enable mclk
			if (clk_prepare_enable(ctx->clk[CLK1_MCLK1]))
				adaptor_logi(ctx,
				"clk_prepare_enable CLK1_MCLK1(fail)\n");
		}
		adaptor_logi(ctx, "[%s] rosc_mode recover. enable aov mclk.\n", __func__);
		ctx->mclk_refcnt = 0;
	}

	if (ctx->subdrv->ops->power_off)
		subdrv_call(ctx, power_off, NULL);

	for (i = ctx->subdrv->pw_seq_cnt - 1; i >= 0; i--) {
		if (ctx->ctx_pw_seq)
			ent = &ctx->ctx_pw_seq[i]; // use ctx pw seq
		else
			ent = &ctx->subdrv->pw_seq[i];
		op = &ctx->hw_ops[ent->id];
		if (!op->unset)
			continue;
		op->unset(ctx, op->data, ent->val);
		//msleep(ent->delay);
	}
	adaptor_pmic_ctrl(ctx, false);

	op = &ctx->hw_ops[HW_ID_MIPI_SWITCH];
	if (op->unset)
		op->unset(ctx, op->data, 0);

	/* the pins of mipi switch are shared. free it for another users */
	if (ctx->state[STATE_MIPI_SWITCH_ON] ||
		ctx->state[STATE_MIPI_SWITCH_OFF] ||
		ctx->state[STATE_DOVDD_ON] ||
		ctx->state[STATE_DOVDD_OFF]) {
		devm_pinctrl_put(ctx->pinctrl);
		ctx->pinctrl = NULL;
	}

	if (ctx->sensor_ws) {
		if (ctx->aov_pm_ops_flag == 1) {
			ctx->aov_pm_ops_flag = 0;
			__pm_relax(ctx->sensor_ws);
		}
	} else {
		adaptor_logm(ctx, "__pm_relax(fail)\n");
	}

	adaptor_logm(ctx, "-\n");
	return 0;
}
int adaptor_hw_power_off(struct adaptor_ctx *ctx)
{
	adaptor_logm(ctx, "+\n");
#ifndef IMGSENSOR_USE_PM_FRAMEWORK
	if (!ctx) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}
	if (!ctx->power_refcnt) {
		adaptor_logd(ctx, "power ref cnt = %d, skip due to not power on yet\n",
			ctx->power_refcnt);
		return 0;
	}
	adaptor_logd(ctx, "power ref cnt = %d\n", ctx->power_refcnt);
	ctx->power_refcnt--;
	if (ctx->power_refcnt > 0) {
		adaptor_logd(ctx, "skip due to cnt = %d\n", ctx->power_refcnt);
		return 0;
	}
	ctx->power_refcnt = 0;
	ctx->is_sensor_inited = 0;
	ctx->is_sensor_scenario_inited = 0;
	ctx->is_streaming = 0;
#endif

	adaptor_logm(ctx, "-\n");
#if ALWAYS_ON_POWER
	if (check_multicam_power(ctx, 1)||ctx->always_on_flag) {
		if (ctx->sensor_ws) {
			if (ctx->aov_pm_ops_flag == 1) {
				ctx->aov_pm_ops_flag = 0;
				__pm_relax(ctx->sensor_ws);
			}
		} else {
			adaptor_logm(ctx, "__pm_relax(fail)\n");
		}
		return 0;
	}
#else
	if (check_multicam_power(ctx, 1))
		return 0;
#endif
	return do_hw_power_off(ctx);
}

int adaptor_hw_init(struct adaptor_ctx *ctx)
{
	int i;
	struct device *dev = ctx->dev;

	adaptor_logm(ctx, "+\n");
	if (!ctx) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}

	/* clocks */
	for (i = 0; i < CLK_MAXCNT; i++) {
		ctx->clk[i] = devm_clk_get(dev, clk_names[i]);
		if (IS_ERR(ctx->clk[i])) {
			ctx->clk[i] = NULL;
			adaptor_logd(ctx, "no clk %s\n", clk_names[i]);
		}
	}

	/* supplies */
	for (i = 0; i < REGULATOR_MAXCNT; i++) {
		ctx->regulator[i] = devm_regulator_get_optional(
				dev, reg_names[i]);
		if (IS_ERR(ctx->regulator[i])) {
			ctx->regulator[i] = NULL;
			adaptor_logd(ctx, "no reg %s\n", reg_names[i]);
		}
	}

	/* pinctrl */
	ctx->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(ctx->pinctrl)) {
		adaptor_loge(ctx, "fail to get pinctrl\n");
		ctx->pinctrl = NULL;
	//	return PTR_ERR(ctx->pinctrl);
	}
	if (ctx->pinctrl != NULL) {
		/* pinctrl states */
		for (i = 0; i < STATE_MAXCNT; i++) {
			ctx->state[i] = pinctrl_lookup_state(
				ctx->pinctrl, state_names[i]);
			if (IS_ERR(ctx->state[i])) {
				ctx->state[i] = NULL;
				adaptor_logd(ctx, "no state %s\n", state_names[i]);
			}
		}
	}
	/* install operations */

	INST_OPS(ctx, clk, CLK_MCLK, HW_ID_MCLK, set_mclk, unset_mclk);

	INST_OPS(ctx, clk, CLK1_MCLK1, HW_ID_MCLK1, set_mclk, unset_mclk);
	INST_OPS(ctx, regulator, REGULATOR_PDN, HW_ID_PDN,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_AVDD, HW_ID_AVDD,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_DVDD, HW_ID_DVDD,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_DOVDD, HW_ID_DOVDD,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_AFVDD, HW_ID_AFVDD,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_AFVDD1, HW_ID_AFVDD1,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_AVDD1, HW_ID_AVDD1,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_AVDD2, HW_ID_AVDD2,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_AVDD3, HW_ID_AVDD3,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_AVDD4, HW_ID_AVDD4,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_BASE, HW_ID_BASE,
			set_reg, unset_reg);
	INST_OPS(ctx, regulator, REGULATOR_DVDD1, HW_ID_DVDD1,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_DVDD2, HW_ID_DVDD2,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_OISVDD, HW_ID_OISVDD,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_OISEN, HW_ID_OISEN,
			set_reg, unset_reg);

	INST_OPS(ctx, regulator, REGULATOR_RST, HW_ID_RST,
			set_reg, unset_reg);

	if (ctx->state[STATE_MIPI_SWITCH_ON])
		ctx->hw_ops[HW_ID_MIPI_SWITCH].set = set_state_mipi_switch;

	if (ctx->state[STATE_MIPI_SWITCH_OFF])
		ctx->hw_ops[HW_ID_MIPI_SWITCH].unset = unset_state_mipi_switch;

	INST_OPS(ctx, state, STATE_MCLK_OFF, HW_ID_MCLK_DRIVING_CURRENT,
			set_state_div2, unset_state);

	INST_OPS(ctx, state, STATE_MCLK1_OFF, HW_ID_MCLK1_DRIVING_CURRENT,
			set_state_div2, unset_state);

	INST_OPS(ctx, state, STATE_RST_LOW, HW_ID_RST,
			set_state, unset_state);

	INST_OPS(ctx, state, STATE_PDN_LOW, HW_ID_PDN,
			set_state, unset_state);

	INST_OPS(ctx, state, STATE_AVDD_OFF, HW_ID_AVDD,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_DVDD_OFF, HW_ID_DVDD,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_DOVDD_OFF, HW_ID_DOVDD,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_AFVDD_OFF, HW_ID_AFVDD,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_AFVDD1_OFF, HW_ID_AFVDD1,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_AVDD1_OFF, HW_ID_AVDD1,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_AVDD2_OFF, HW_ID_AVDD2,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_AVDD3_OFF, HW_ID_AVDD3,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_AVDD4_OFF, HW_ID_AVDD4,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_BASE_OFF, HW_ID_BASE,
			set_state_boolean, unset_state);
	INST_OPS(ctx, state, STATE_DVDD1_OFF, HW_ID_DVDD1,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_OISVDD_OFF, HW_ID_OISVDD,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_OISEN_OFF, HW_ID_OISEN,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_DVDD2_OFF, HW_ID_DVDD2,
			set_state_boolean, unset_state);

	INST_OPS(ctx, state, STATE_RST1_LOW, HW_ID_RST1,
			set_state, unset_state);

	INST_OPS(ctx, state, STATE_PONV_LOW, HW_ID_PONV,
			set_state, unset_state);

	INST_OPS(ctx, state, STATE_SCL_AP, HW_ID_SCL,
			set_state, unset_state);

	INST_OPS(ctx, state, STATE_SDA_AP, HW_ID_SDA,
			set_state, unset_state);

	INST_OPS(ctx, state, STATE_EINT, HW_ID_EINT,
		 set_state, unset_state);
	if (ctx->pinctrl != NULL) {
		/* the pins of mipi switch are shared. free it for another users */
		if (ctx->state[STATE_MIPI_SWITCH_ON] ||
			ctx->state[STATE_MIPI_SWITCH_OFF]) {
			devm_pinctrl_put(ctx->pinctrl);
			ctx->pinctrl = NULL;
		}
	}
	adaptor_logm(ctx, "-\n");

	return 0;
}

int adaptor_hw_sensor_reset(struct adaptor_ctx *ctx)
{
	int ret = 0;

	if (!ctx || !(ctx->subdrv)) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}

	adaptor_logi(ctx, "%d|%d|%d\n",
		ctx->is_streaming,
		ctx->is_sensor_inited,
		ctx->power_refcnt);

	if (ctx->is_streaming == 1 &&
		ctx->is_sensor_inited == 1 &&
		ctx->power_refcnt > 0) {

		do_hw_power_off(ctx);
		do_hw_power_on(ctx);
		ret = adaptor_ixc_do_daa (&ctx->ixc_client);
		if (ret)
			adaptor_loge(ctx, "ixc_do_daa(ret=%d), prot= %d\n",
				ret, ctx->ixc_client.protocol);

		return 0;
	}
	adaptor_logi(ctx, "skip to reset due to either integration or else\n");

	return -1;
}


