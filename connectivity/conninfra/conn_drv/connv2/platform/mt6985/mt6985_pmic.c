// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regmap.h>
#include <linux/timer.h>

#include "osal.h"
#include "consys_hw.h"
#include "consys_reg_util.h"
#include "pmic_mng.h"
#include "mt6985_pmic.h"
#include "mt6985_consys_reg_offset.h"
#include "mt6985_pos.h"


static struct conninfra_dev_cb* g_dev_cb;
static struct regulator *reg_VRFIO18; /* MT6363 workaround VCN15 -> VRFIO18 */
static struct regulator *reg_VANT18;

static int consys_plt_pmic_get_from_dts_mt6985(struct platform_device*, struct conninfra_dev_cb*);
static int consys_plt_pmic_common_power_ctrl_mt6985(unsigned int,
					unsigned int curr_status, unsigned int next_status);
static int consys_plt_pmic_gps_power_ctrl_mt6985(unsigned int);


const struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6985 = {
	.consys_pmic_get_from_dts = consys_plt_pmic_get_from_dts_mt6985,
	.consys_pmic_common_power_ctrl = consys_plt_pmic_common_power_ctrl_mt6985,
	.consys_pmic_gps_power_ctrl = consys_plt_pmic_gps_power_ctrl_mt6985,
};

static int consys_plt_pmic_common_power_ctrl_mt6985(unsigned int enable,
					unsigned int curr_status, unsigned int next_status)
{
	static bool setup = false;

	if (consys_is_rc_mode_enable_mt6985() && enable) {
		if (curr_status != 0)
			return 0;

		/* Full HW signal control, only need config it. */
		if (!setup) {
			/* 1. set PMIC VANT18 LDO PMIC HW mode control by PMRC_EN[10]
			 * 1.1. set PMIC VANT18 LDO op_mode = 0
			 * 1.2. set PMIC VANT18 LDO  HW_OP_EN = 1, HW_OP_CFG = 0
			 */
			regmap_update_bits(g_regmap_mt6373,
				MT6373_RG_LDO_VANT18_RC10_OP_MODE_ADDR, 1 << 2, 0 << 2);
			regmap_update_bits(g_regmap_mt6373,
				MT6373_RG_LDO_VANT18_RC10_OP_EN_ADDR,   1 << 2, 1 << 2);
			regmap_update_bits(g_regmap_mt6373,
				MT6373_RG_LDO_VANT18_RC10_OP_CFG_ADDR,  1 << 2, 0 << 2);
			setup = true;
		}
	}
	return 0;
}

int consys_plt_pmic_get_from_dts_mt6985(struct platform_device* pdev, struct conninfra_dev_cb* cb)
{
	g_dev_cb = cb;

	reg_VRFIO18 = devm_regulator_get(&pdev->dev, "mt6363_vrfio18");
	if (IS_ERR(reg_VRFIO18)) {
		pr_err("Regulator_get VCN_18 fail\n");
		reg_VRFIO18 = NULL;
	}

	reg_VANT18 = devm_regulator_get(&pdev->dev, "mt6373_vant18");
	if (IS_ERR(reg_VANT18)) {
		pr_err("Regulator_get VANT18 fail\n");
		reg_VANT18 = NULL;
	}

	return 0;
}

static int consys_plt_pmic_gps_power_ctl_rc_mode(unsigned int enable)
{
	/* HW control mode, only config hw signal */
	if (enable) {
		/* 1. set PMIC VRFIO18 LDO PMIC HW mode control by PMRC_EN[6]
		 * 	1.1. set PMIC VRFIO18 LDO op_mode = 0
		 * 	1.2. set PMIC VRFIO18 LDO HW_OP_EN = 1, HW_OP_CFG = 0
		 * 2. set PMIC VRFIO18 LDO SW_OP_EN =0, SW_EN = 0, SW_LP =0 (sw disable )
		 */
		regmap_update_bits(g_regmap_mt6363,
			MT6363_RG_LDO_VRFIO18_RC6_OP_MODE_ADDR, 1 << 6, 0 << 6);
		regmap_update_bits(g_regmap_mt6363,
			MT6363_RG_LDO_VRFIO18_RC6_OP_EN_ADDR,   1 << 6, 1 << 6);
		regmap_update_bits(g_regmap_mt6363,
			MT6363_RG_LDO_VRFIO18_RC6_OP_CFG_ADDR,  1 << 6, 0 << 6);
		regulator_set_mode(reg_VRFIO18, REGULATOR_MODE_NORMAL); /* SW_LP = 0 */
	}

	return 0;
}

static int consys_plt_pmic_gps_power_ctl_legacy_mode(unsigned int enable)
{
	int ret;

	if (enable) {
		/* VCN18 (MT6363 workaround VRFIO18 -> VCN18) */
		/* 1. set PMIC VRFIO18 LDO PMIC HW mode control by SRCCLKENA0
		 * 	1.1. set PMIC VRFIO18 LDO op_mode = 1
		 * 	1.2. set PMIC VRFIO18 LDO HW_OP_EN = 1, HW_OP_CFG = 0
		 */
		regmap_update_bits(g_regmap_mt6363,
			MT6363_RG_LDO_VRFIO18_HW0_OP_MODE_ADDR, 1 << 0, 1 << 0);
		regmap_update_bits(g_regmap_mt6363,
			MT6363_RG_LDO_VRFIO18_HW0_OP_EN_ADDR,   1 << 0, 1 << 0);
		regmap_update_bits(g_regmap_mt6363,
			MT6363_RG_LDO_VRFIO18_HW0_OP_CFG_ADDR,  1 << 0, 0 << 0);

		/* 2. set PMIC VRFIO18 LDO SW_OP_EN =1, SW_EN = 1, SW_LP =0 */
		regulator_set_mode(reg_VRFIO18, REGULATOR_MODE_NORMAL); /* SW_LP = 0 */
		ret = regulator_enable(reg_VRFIO18); /* SW_EN = 1 */
		if (ret)
			pr_err("Enable VRFIO18 fail. ret=%d\n", ret);
	} else {
		/* VCN18 (MT6363 workaround VRFIO18 -> VCN18) */
		/* 1. set PMIC VRFIO18 LDO HW_OP_EN = 0, HW_OP_CFG = 0 */
		regmap_update_bits(g_regmap_mt6363,
			MT6363_RG_LDO_VRFIO18_HW0_OP_EN_ADDR,   1 << 0, 0 << 0);
		regmap_update_bits(g_regmap_mt6363,
			MT6363_RG_LDO_VRFIO18_HW0_OP_CFG_ADDR,  1 << 0, 0 << 0);

		/* 2. set PMIC VRFIO18 LDO SW_OP_EN =0, SW_EN = 0, SW_LP =0 (sw disable) */
		/* set PMIC VRFIO18 LDO SW_EN = 0, SW_LP =0 (sw disable) */
		regulator_set_mode(reg_VRFIO18, REGULATOR_MODE_NORMAL);
		ret = regulator_disable(reg_VRFIO18);
		if (ret)
			pr_notice("%s regulator_disable err: %d", __func__, ret);
	}

	return 0;
}


int consys_plt_pmic_gps_power_ctrl_mt6985(unsigned int enable)
{
	/* 1. set PMIC VRFIO18 LDO 1.8V */
	regulator_set_voltage(reg_VRFIO18, 1800000, 1800000);
	pr_info("[%s] set VRFIO18 to 1.8V\n", __func__);
	/* VRFIO18 +0mV */
	regmap_write(g_regmap_mt6363, MT6363_RG_VRFIO18_VOCAL_ADDR, 0);

	if (consys_is_rc_mode_enable_mt6985())
		return consys_plt_pmic_gps_power_ctl_rc_mode(enable);
	else
		return consys_plt_pmic_gps_power_ctl_legacy_mode(enable);
}
