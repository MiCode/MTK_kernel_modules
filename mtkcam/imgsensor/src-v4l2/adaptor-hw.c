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
			return ctx->clk[CLK1_26M];
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

	adaptor_logd(ctx, "E! idx(%llu),val(%d)\n", idx, val);

	ret = clk_prepare_enable(mclk);
	if (ret) {
		adaptor_logi(ctx,
			"clk_prepare_enable(%s),ret(%d)(fail)\n",
			clk_names[idx], ret);
		return ret;
	}
	adaptor_logd(ctx,
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
	adaptor_logd(ctx,
		"X! clk_set_parent(%s),ret(%d)(correct)\n",
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

	adaptor_logd(ctx, "E! idx(%llu),val(%d)\n", idx, val);

	clk_disable_unprepare(mclk);

	adaptor_logd(ctx,
		"X! clk_disable_unprepare(%s)\n", clk_names[idx]);

	return 0;
}

static int set_reg(struct adaptor_ctx *ctx, void *data, int val)
{
	unsigned long long ret, idx;
	struct regulator *reg;

	idx = (unsigned long long)data;

	// re-get reg everytime due to pmic limitation
	ctx->regulator[idx] = devm_regulator_get_optional(ctx->dev, reg_names[idx]);
	if (IS_ERR(ctx->regulator[idx])) {
		ctx->regulator[idx] = NULL;
		dev_dbg(ctx->dev,
			"[%s] no reg %s\n", __func__, reg_names[idx]);
		return -EINVAL;
	}

	reg = ctx->regulator[idx];

	adaptor_logd(ctx, "E! idx(%llu),val(%d)\n", idx, val);

	ret = regulator_set_voltage(reg, val, val);
	if (ret) {
		dev_dbg(ctx->dev,
			"[%s] regulator_set_voltage(%s),val(%d),ret(%llu)(fail)\n",
			__func__, reg_names[idx], val, ret);
	}
	adaptor_logd(ctx,
		"regulator_set_voltage(%s),val(%d),ret(%llu)(correct)\n",
		reg_names[idx], val, ret);

	ret = regulator_enable(reg);
	if (ret) {
		dev_dbg(ctx->dev,
			"[%s] regulator_enable(%s),ret(%llu)(fail)\n",
			__func__, reg_names[idx], ret);
		return ret;
	}
	adaptor_logd(ctx,
		"X! regulator_enable(%s),ret(%llu)(correct)\n",
		reg_names[idx], ret);

	return 0;
}

static int unset_reg(struct adaptor_ctx *ctx, void *data, int val)
{
	unsigned long long ret, idx;
	struct regulator *reg;

	idx = (unsigned long long)data;
	reg = ctx->regulator[idx];

	adaptor_logd(ctx, "E! idx(%llu),val(%d)\n", idx, val);

	ret = regulator_disable(reg);
	if (ret) {
		dev_dbg(ctx->dev,
			"[%s] disable(%s),ret(%llu)(fail)\n",
			__func__, reg_names[idx], ret);
		return ret;
	}
	// always put reg due to pmic limitation
	devm_regulator_put(ctx->regulator[idx]);

	adaptor_logd(ctx,
		"X! disable(%s),ret(%llu)(correct)\n", reg_names[idx], ret);

	return 0;
}

static int set_state(struct adaptor_ctx *ctx, void *data, int val)
{
	unsigned long long idx, x;
	int ret;

	idx = (unsigned long long)data;
	x = idx + val;

	adaptor_logd(ctx, "E! idx(%llu),val(%d)\n", idx, val);
	if (!ctx || !(ctx->subdrv)) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}

	ret = pinctrl_select_state(ctx->pinctrl, ctx->state[x]);
	if (ret < 0) {
		dev_info(ctx->dev,
			"[%s] select(%s),ret(%d)(fail)\n",
			__func__, state_names[x], ret);
		return ret;
	}

	adaptor_logd(ctx,
		"X! select(%s),ret(%d)(correct)\n", state_names[x], ret);

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

static int reinit_pinctrl(struct adaptor_ctx *ctx)
{
	int i;
	struct device *dev = ctx->dev;

	adaptor_logd(ctx, "E!\n");
	if (!ctx || !(ctx->subdrv)) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}

	/* pinctrl */
	ctx->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(ctx->pinctrl)) {
		dev_dbg(dev, "[%s] fail to get pinctrl\n", __func__);
		return PTR_ERR(ctx->pinctrl);
	}

	/* pinctrl states */
	for (i = 0; i < STATE_MAXCNT; i++) {
		ctx->state[i] = pinctrl_lookup_state(
				ctx->pinctrl, state_names[i]);
		if (IS_ERR(ctx->state[i])) {
			ctx->state[i] = NULL;
			dev_dbg(dev,
				"[%s] no state %s\n", __func__, state_names[i]);
		}
	}

	adaptor_logd(ctx, "X!\n");

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

	adaptor_logd(ctx, "E!\n");
	if (!ctx || !(ctx->subdrv)) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}

	if (ctx->sensor_ws) {
		if (ctx->aov_pm_ops_flag == 0) {
			ctx->aov_pm_ops_flag = 1;
			__pm_stay_awake(ctx->sensor_ws);
		}
	} else
		adaptor_logi(ctx, "__pm_stay_awake(fail)\n");

	adaptor_log_buf_init(&buf, ADAPTOR_LOG_BUF_SZ);

	/* may be released for mipi switch */
	if (!ctx->pinctrl)
		reinit_pinctrl(ctx);

	subctx = &ctx->subctx;
	op = &ctx->hw_ops[HW_ID_MIPI_SWITCH];
	if (op->set)
		op->set(ctx, op->data, 0);

	if (subctx->power_on_profile_en)
		time_boot_begin = ktime_get_boottime_ns();

	for (i = 0; i < ctx->subdrv->pw_seq_cnt; i++) {
		if (ctx->ctx_pw_seq)
			ent = &ctx->ctx_pw_seq[i]; // use ctx pw seq
		else
			ent = &ctx->subdrv->pw_seq[i];
		op = &ctx->hw_ops[ent->id];
		if (!op->set) {
			adaptor_logd(ctx,
				"cannot set comp:%d,val:%d\n", ent->id, ent->val);
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

		adaptor_logd(ctx, "set comp:%d,val:%d\n", ent->id, ent->val);

		if (ent->delay)
			mdelay(ent->delay);
	}

	if (subctx->power_on_profile_en) {
		subctx->sensor_pw_on_profile.hw_power_on_period =
			ktime_get_boottime_ns() - time_boot_begin;
	}

	if (ctx->subdrv->ops->power_on)
		subdrv_call(ctx, power_on, NULL);

	adaptor_logd(ctx, "X!\n");

	adaptor_log_buf_flush(ctx, __func__, &buf);
	adaptor_log_buf_deinit(&buf);

	return 0;
}

int adaptor_hw_power_on(struct adaptor_ctx *ctx)
{
	int ret = 0;

	adaptor_logd(ctx, "E!\n");
	if (!ctx || !(ctx->subdrv)) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}
#ifndef IMGSENSOR_USE_PM_FRAMEWORK
	adaptor_logd(ctx, "power ref cnt:%d\n", ctx->power_refcnt);
	ctx->power_refcnt++;
	if (ctx->power_refcnt > 1) {
		adaptor_logd(ctx, "already powered,cnt:%d\n", ctx->power_refcnt);
		return 0;
	}
#endif

	ret = do_hw_power_on(ctx);

	adaptor_logd(ctx, "X!\n");

	return ret;
}

int do_hw_power_off(struct adaptor_ctx *ctx)
{
	int i;
	const struct subdrv_pw_seq_entry *ent;
	struct adaptor_hw_ops *op;

	adaptor_logd(ctx, "E!\n");
	if (!ctx || !(ctx->subdrv)) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}

	/* call subdrv close function before pwr off */
	subdrv_call(ctx, close);

	if (ctx->subctx.s_ctx.mode &&
		ctx->subctx.s_ctx.mode[ctx->subctx.current_scenario_id].rosc_mode) {
		for (i = 0; i < ctx->mclk_refcnt; i++) {
			// enable mclk
			if (clk_prepare_enable(ctx->clk[CLK1_MCLK1]))
				dev_info(ctx->dev,
				"clk_prepare_enable CLK1_MCLK1(fail)\n");
		}
		dev_info(ctx->dev, "[%s] rosc_mode recover. enable aov mclk.\n", __func__);
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
	} else
		adaptor_logi(ctx, "__pm_relax(fail)\n");

	adaptor_logd(ctx, "X!\n");

	return 0;
}
int adaptor_hw_power_off(struct adaptor_ctx *ctx)
{
	int ret = 0;

	adaptor_logd(ctx, "E!\n");
	if (!ctx || !(ctx->subdrv)) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}
#ifndef IMGSENSOR_USE_PM_FRAMEWORK
	if (!ctx->power_refcnt) {
		adaptor_logd(ctx,
			"power ref cnt:%d,skip due to not power on yet\n",
			ctx->power_refcnt);
		return 0;
	}
	adaptor_logd(ctx, "power ref cnt:%d\n", ctx->power_refcnt);
	ctx->power_refcnt--;
	if (ctx->power_refcnt > 0) {
		adaptor_logd(ctx, "skip due to cnt:%d\n", ctx->power_refcnt);
		return 0;
	}
	ctx->power_refcnt = 0;
	ctx->is_sensor_inited = 0;
	ctx->is_sensor_scenario_inited = 0;
	ctx->is_streaming = 0;
#endif

	ret = do_hw_power_off(ctx);

	adaptor_logd(ctx, "X!\n");

	return ret;
}

int adaptor_hw_init(struct adaptor_ctx *ctx)
{
	int i;
	struct device *dev = ctx->dev;

	adaptor_logd(ctx, "E!\n");

	/* clocks */
	for (i = 0; i < CLK_MAXCNT; i++) {
		ctx->clk[i] = devm_clk_get(dev, clk_names[i]);
		if (IS_ERR(ctx->clk[i])) {
			ctx->clk[i] = NULL;
			dev_dbg(dev, "[%s] no clk %s\n", __func__, clk_names[i]);
		}
	}

	/* supplies */
	for (i = 0; i < REGULATOR_MAXCNT; i++) {
		ctx->regulator[i] = devm_regulator_get_optional(
				dev, reg_names[i]);
		if (IS_ERR(ctx->regulator[i])) {
			ctx->regulator[i] = NULL;
			dev_dbg(dev, "[%s] no reg %s\n", __func__, reg_names[i]);
		}
	}

	/* pinctrl */
	ctx->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(ctx->pinctrl)) {
		dev_dbg(dev, "[%s] fail to get pinctrl\n", __func__);
		return PTR_ERR(ctx->pinctrl);
	}

	/* pinctrl states */
	for (i = 0; i < STATE_MAXCNT; i++) {
		ctx->state[i] = pinctrl_lookup_state(
				ctx->pinctrl, state_names[i]);
		if (IS_ERR(ctx->state[i])) {
			ctx->state[i] = NULL;
			dev_dbg(dev, "[%s] no state %s\n", __func__, state_names[i]);
		}
	}

	/* install operations */

	INST_OPS(ctx, clk, CLK_MCLK, HW_ID_MCLK, set_mclk, unset_mclk);

	INST_OPS(ctx, clk, CLK1_MCLK1, HW_ID_MCLK1, set_mclk, unset_mclk);

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

	/* the pins of mipi switch are shared. free it for another users */
	if (ctx->state[STATE_MIPI_SWITCH_ON] ||
		ctx->state[STATE_MIPI_SWITCH_OFF]) {
		devm_pinctrl_put(ctx->pinctrl);
		ctx->pinctrl = NULL;
	}

	adaptor_logd(ctx, "X!\n");

	return 0;
}

int adaptor_hw_sensor_reset(struct adaptor_ctx *ctx)
{
	adaptor_logd(ctx,
		"E! %d|%d|%d\n",
		ctx->is_streaming,
		ctx->is_sensor_inited,
		ctx->power_refcnt);
	if (!ctx || !(ctx->subdrv)) {
		pr_info("[%s] ctx might be null!\n", __func__);
		return -EINVAL;
	}

	if (ctx->is_streaming == 1 &&
		ctx->is_sensor_inited == 1 &&
		ctx->power_refcnt > 0) {

		do_hw_power_off(ctx);
		do_hw_power_on(ctx);

		return 0;
	}
	adaptor_logd(ctx,
		"X! skip to reset due to either integration or else\n");

	return -1;
}


