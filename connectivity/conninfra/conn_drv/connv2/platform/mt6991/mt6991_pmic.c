// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regmap.h>
#include <linux/timer.h>

#include "osal.h"
#include "consys_hw.h"
#include "consys_reg_util.h"
#include "pmic_mng.h"
#include "mt6991_pmic.h"
#include "mt6991_consys_reg_offset.h"
#include "mt6991_pos.h"
#include "mt6991_soc.h"

static struct regulator *reg_VRFIO18; /* MT6363 workaround VCN15 -> VRFIO18 */

static int consys_plt_pmic_get_from_dts_mt6991(struct platform_device*, struct conninfra_dev_cb*);
static int consys_plt_pmic_common_power_ctrl_mt6991(unsigned int enable,
					unsigned int curr_status, unsigned int next_status);
static int consys_plt_pmic_common_power_low_power_mode_mt6991(unsigned int enable,
					unsigned int curr_status, unsigned int next_status);

const struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6991 = {
	.consys_pmic_get_from_dts = consys_plt_pmic_get_from_dts_mt6991,
	.consys_pmic_common_power_ctrl = consys_plt_pmic_common_power_ctrl_mt6991,
	.consys_pmic_common_power_low_power_mode = consys_plt_pmic_common_power_low_power_mode_mt6991,
};

static int consys_plt_pmic_get_from_dts_mt6991(struct platform_device* pdev, struct conninfra_dev_cb* cb)
{
	reg_VRFIO18 = devm_regulator_get(&pdev->dev, "mt6363_vrfio18");
	if (IS_ERR(reg_VRFIO18)) {
		pr_notice("[%s] Regulator_get VCN_18 fail\n", __func__);
		reg_VRFIO18 = NULL;
	}
	return 0;
}

static int consys_plt_pmic_common_power_ctrl_mt6991(unsigned int enable,
					unsigned int curr_status, unsigned int next_status)
{
	int ret = 0;

#ifndef CONFIG_FPGA_EARLY_PORTING
	if (reg_VRFIO18 == NULL) {
		pr_info("%s reg_VRFIO18 is NULL", __func__);
		return -1;
	}

	if (curr_status == 0 && next_status != 0) {
		regulator_set_voltage(reg_VRFIO18, 1800000, 1800000);

		if (consys_co_clock_type_mt6991() == CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO ||
			consys_is_rc_mode_enable_mt6991() == 0) {
			regulator_set_mode(reg_VRFIO18, REGULATOR_MODE_NORMAL);
			ret = regulator_enable(reg_VRFIO18);
			pr_info("%s set VRFIO18 SW_EN=1, ret = %d\n", __func__, ret);
		}
	}

	if (curr_status != 0 && next_status == 0 && consys_is_rc_mode_enable_mt6991() == 0) {
		ret = regulator_disable(reg_VRFIO18);
		pr_info("%s set VRFIO18 SW_EN=0 for legacy mode, ret = %d\n", __func__, ret);
	}
#endif
	return ret;
}

static int consys_plt_pmic_common_power_low_power_mode_mt6991(unsigned int enable,
					unsigned int curr_status, unsigned int next_status)
{
	int ret = 0;
#ifndef CONFIG_FPGA_EARLY_PORTING
	struct regmap *r = g_regmap_mt6363;

	if (((curr_status != 0) && (next_status != 0)) || (enable == 0))
		return 0;

	if (r == NULL) {
		pr_info("%s g_regmap_mt6363 is NULL\n", __func__);
		return -1;
	}

	if (reg_VRFIO18 == NULL) {
		pr_info("%s reg_VRFIO18 is NULL\n", __func__);
		return -1;
	}

	if (consys_is_rc_mode_enable_mt6991()) {
		regmap_update_bits(r, MT6363_RG_LDO_VRFIO18_RC9_OP_MODE_ADDR, 1 << 1, 0 << 1);
		regmap_update_bits(r, MT6363_RG_LDO_VRFIO18_RC9_OP_EN_ADDR,   1 << 1, 1 << 1);
		regmap_update_bits(r, MT6363_RG_LDO_VRFIO18_RC9_OP_CFG_ADDR,  1 << 1, 0 << 1);

		if (consys_co_clock_type_mt6991() == CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO) {
			ret = regulator_disable(reg_VRFIO18);
			pr_info("%s set reg_VRFIO18 SW_EN=0 for TCXO, ret = %d\n", __func__, ret);
		}
	}
#endif
	return ret;
}

