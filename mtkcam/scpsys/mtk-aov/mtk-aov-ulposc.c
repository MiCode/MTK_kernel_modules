// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 *
 * Author: ChenHung Yang <chenhung.yang@mediatek.com>
 */

#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/io.h>
#include <clk-fmeter.h>

#include "mtk-aov-drv.h"
#include "mtk-aov-ulposc.h"
#include "mtk-aov-core.h"
#include "mtk-aov-regs.h"
#include "mtk-aov-log.h"

#define PROPNAME_ULPOSC_SUPPORT	"ulposc-support"
#define PROPNAME_ULPOSC_CALI_CONFIG_NUM "ulposc-cali-config-num"
#define PROPNAME_ULPOSC_CALI_REGISTER "ulposc-cali-register"
#define PROPNAME_ULPOSC_CALI_CONFIG "ulposc-cali-config"
#define PROPNAME_ULPOSC_CALI_RESULT "ulposc-cali-result"
#define PROPNAME_ULPOSC_CALI_TARGET "ulposc-cali-target"
#define PROPNAME_ULPOSC_RESULT_TARGET "ulposc-result-target"
#define PROPNAME_ULPOSC_CALI_FMETER_ID "ulposc-cali-fmeter-id"
#define PROPNAME_ULPOSC_RESULT_FMETER_ID "ulposc-result-fmeter-id"
#define PROPNAME_ULPOSC_CLK	"ulposc-clksys"

/* fmeter data */
#define FM_CNT2FREQ(cnt)	(cnt * 26 / CALI_DIV_VAL)
#define FM_FREQ2CNT(freq)	(freq * CALI_DIV_VAL / 26)
#define KHZ_TO_MHZ(freq)	(freq / 1000)

static int aov_ulposc_turn_on_off(struct mtk_aov *aov_dev, bool turnon)
{
	int ret = 0;

	if (turnon) {
		ret = aov_core_send_cmd(aov_dev, AOV_SCP_CMD_TURN_ON_ULPOSC, NULL, 0, false);
		if (ret < 0)
			dev_info(aov_dev->dev, "%s: send cmd(%d) fail", __func__, AOV_SCP_CMD_TURN_ON_ULPOSC);
		else
			dev_info(aov_dev->dev, "%s: send cmd(%d) done", __func__, AOV_SCP_CMD_TURN_ON_ULPOSC);
	} else {
		ret = aov_core_send_cmd(aov_dev, AOV_SCP_CMD_TURN_OFF_ULPOSC, NULL, 0, false);
		if (ret < 0)
			dev_info(aov_dev->dev, "%s: send cmd(%d) fail", __func__, AOV_SCP_CMD_TURN_OFF_ULPOSC);
		else
			dev_info(aov_dev->dev, "%s: send cmd(%d) done", __func__, AOV_SCP_CMD_TURN_OFF_ULPOSC);
	}

	return 0;
}

enum {
	PRE_CONFIG = 0,
	CALI_CONFIG = 1,
	CCONFIG_MAX,
};

static int aov_ulposc_set_config(struct mtk_aov *aov_dev, int type)
{
	int i = 0;

	switch (type) {
	case PRE_CONFIG:
		for (i = 0; i < aov_dev->ulposc_info.ulposc_cali_config_num; i++)
			DRV_WriteReg32((aov_dev->ulposc_info.vlp_base + aov_dev->ulposc_info.ulposc_config_register[i]),
				aov_dev->ulposc_info.ulposc_cali_config_setting[i]);
		break;
	case CALI_CONFIG:
		for (i = 0; i < aov_dev->ulposc_info.ulposc_cali_config_num; i++)
			DRV_WriteReg32((aov_dev->ulposc_info.vlp_base + aov_dev->ulposc_info.ulposc_config_register[i]),
				aov_dev->ulposc_info.ulposc_cali_result[i]);
		dev_info(aov_dev->dev,"[%s] cali_result:0x%x\n",
			__func__,
			DRV_Reg32((aov_dev->ulposc_info.vlp_base + aov_dev->ulposc_info.ulposc_config_register[0])));
		break;
	default :
		dev_info(aov_dev->dev, "%s no support config type:%d\n", __func__, type);
		break;
	};

	return 0;
}

static void aov_ulposc_set_cali_factor1_value(struct mtk_aov *aov_dev, int value)
{
	DRV_ClrReg32((aov_dev->ulposc_info.vlp_base + aov_dev->ulposc_info.ulposc_config_register[0]),
		CALI_FACTOR1_REG_MASK);
	DRV_SetReg32((aov_dev->ulposc_info.vlp_base + aov_dev->ulposc_info.ulposc_config_register[0]),
		(value << CALI_FACTOR1_REG_SHIFT));
}

static void aov_ulposc_set_cali_factor2_value(struct mtk_aov *aov_dev, int value)
{
	DRV_ClrReg32((aov_dev->ulposc_info.vlp_base + aov_dev->ulposc_info.ulposc_config_register[0]),
		CALI_FACTOR2_REG_MASK);
	DRV_SetReg32((aov_dev->ulposc_info.vlp_base + aov_dev->ulposc_info.ulposc_config_register[0]),
		(value << CALI_FACTOR2_REG_SHIFT));
}

static int aov_ulposc_get_fmeter_result(unsigned int fmeter_id)
{
	unsigned int result_freq = 0;

	result_freq = mt_get_fmeter_freq(fmeter_id, VLPCK);
	if (result_freq == 0) {
		/* result_freq is not expected to be 0 */
		pr_notice("[%s]: mt_get_fmeter_freq() return %d, pls check CCF configs\n",
			__func__, result_freq);
	}

	return result_freq;
}

static void aov_ulposc_record_cali_result(struct mtk_aov *aov_dev)
{
	aov_dev->ulposc_info.ulposc_cali_result[0] =
		DRV_Reg32((aov_dev->ulposc_info.vlp_base + aov_dev->ulposc_info.ulposc_config_register[0]));
	dev_info(aov_dev->dev, "[%s]:0x%x", __func__,
		DRV_Reg32((aov_dev->ulposc_info.vlp_base + aov_dev->ulposc_info.ulposc_config_register[0])));
}

int aov_ulposc_check_cali_result(struct mtk_aov *aov_dev)
{
	unsigned int current_val = 0;

	aov_ulposc_turn_on_off(aov_dev, 0);
	aov_ulposc_set_config(aov_dev, CALI_CONFIG);
	aov_ulposc_turn_on_off(aov_dev, 1);

	current_val = aov_ulposc_get_fmeter_result(aov_dev->ulposc_info.ulposc_result_fmeter_id);

	if (current_val < (aov_dev->ulposc_info.ulposc_cali_result_target * 1000 * (100 + CALI_MIS_RATE)/100) &&
		current_val > (aov_dev->ulposc_info.ulposc_cali_result_target * 1000 * (100 - CALI_MIS_RATE)/100)) {
		aov_dev->ulposc_info.ulposc_cali_done = true;
		dev_info(aov_dev->dev,"[%s] pass: current_val: %dMHz/%dkhz, target: %dMHz\n",
			__func__, KHZ_TO_MHZ(current_val), current_val,
			aov_dev->ulposc_info.ulposc_cali_result_target);
		return 1;
	}

	dev_info(aov_dev->dev,"[%s] fail: current_val: %dMHz/%dkhz, target: %dMHz\n",
			__func__, KHZ_TO_MHZ(current_val), current_val,
			aov_dev->ulposc_info.ulposc_cali_result_target);
	return 0;
}

static int aov_ulposc_cali_process(struct mtk_aov *aov_dev)
{
	unsigned int target_val = 0, current_val = 0, ret = 0;
	unsigned int min = 0, max = 0, mid  = 0;
	unsigned int diff_by_min = 0, diff_by_max = 0xffff;
	unsigned int factor1_cali_result = 0;

	target_val = aov_dev->ulposc_info.ulposc_cali_target;

	min = CAL_FACTOR1_MIN_VAL;
	max = CAL_FACTOR1_MAX_VAL;

	do {
		mid = (min + max) / 2;
		if (mid == min) {
			pr_debug("turning_factor1 mid(%u) == min(%u)\n", mid, min);
			break;
		}
		aov_ulposc_set_cali_factor1_value(aov_dev, mid);
		current_val = KHZ_TO_MHZ(aov_ulposc_get_fmeter_result(aov_dev->ulposc_info.ulposc_cali_fmeter_id));
		if (current_val > target_val)
			max = mid;
		else
			min = mid;
	} while (min <= max);

	aov_ulposc_set_cali_factor1_value(aov_dev, min);
	current_val = KHZ_TO_MHZ(aov_ulposc_get_fmeter_result(aov_dev->ulposc_info.ulposc_cali_fmeter_id));
	diff_by_min = (current_val > target_val) ? (current_val - target_val):(target_val - current_val);
	aov_ulposc_set_cali_factor1_value(aov_dev, max);
	current_val = KHZ_TO_MHZ(aov_ulposc_get_fmeter_result(aov_dev->ulposc_info.ulposc_cali_fmeter_id));
	diff_by_max = (current_val > target_val) ? (current_val - target_val):(target_val - current_val);
	factor1_cali_result = (diff_by_min < diff_by_max) ? min : max;
	aov_ulposc_set_cali_factor1_value(aov_dev, factor1_cali_result);

	min = CAL_FACTOR2_MIN_VAL;
	max = CAL_FACTOR2_MAX_VAL;

	do {
		mid = (min + max) / 2;
		if (mid == min) {
			pr_debug("turning_factor2 mid(%u) == min(%u)\n", mid, min);
			break;
		}
		aov_ulposc_set_cali_factor2_value(aov_dev, mid);
		current_val =  KHZ_TO_MHZ(aov_ulposc_get_fmeter_result(aov_dev->ulposc_info.ulposc_cali_fmeter_id));
		if (current_val > target_val)
			max = mid;
		else
			min = mid;
	} while (min <= max);

	aov_ulposc_set_cali_factor2_value(aov_dev, min);
	current_val = KHZ_TO_MHZ(aov_ulposc_get_fmeter_result(aov_dev->ulposc_info.ulposc_cali_fmeter_id));
	diff_by_min = (current_val > target_val) ? (current_val - target_val):(target_val - current_val);
	aov_ulposc_set_cali_factor2_value(aov_dev, max);
	current_val = KHZ_TO_MHZ(aov_ulposc_get_fmeter_result(aov_dev->ulposc_info.ulposc_cali_fmeter_id));
	diff_by_max = (current_val > target_val) ? (current_val - target_val):(target_val - current_val);
	factor1_cali_result = (diff_by_min < diff_by_max) ? min : max;
	aov_ulposc_set_cali_factor2_value(aov_dev, factor1_cali_result);

	aov_ulposc_record_cali_result(aov_dev);

	/* check calibration result */
	ret = aov_ulposc_check_cali_result(aov_dev);
	if (ret)
		dev_info(aov_dev->dev, "%s : pass,\n", __func__);
	else
		dev_info(aov_dev->dev, "%s : fail,\n", __func__);

	return 0;
}

void aov_ulposc_dts_init(struct mtk_aov *aov_dev)
{
	int ret = 0;
	unsigned int i = 0, vlp_base_pa = 0;

	aov_dev->ulposc_info.ulposc_support = of_property_read_bool(aov_dev->dev->of_node, PROPNAME_ULPOSC_SUPPORT);
	if (!aov_dev->ulposc_info.ulposc_support) {
		dev_info(aov_dev->dev, "%s: no ulposc support\n", __func__);
		return;
	}

	of_property_read_u32(aov_dev->dev->of_node,
		PROPNAME_ULPOSC_CALI_CONFIG_NUM,
		&(aov_dev->ulposc_info.ulposc_cali_config_num));
	of_property_read_u32(aov_dev->dev->of_node,
		PROPNAME_ULPOSC_CALI_TARGET,
		&(aov_dev->ulposc_info.ulposc_cali_target));
	of_property_read_u32(aov_dev->dev->of_node,
		PROPNAME_ULPOSC_RESULT_TARGET,
		&(aov_dev->ulposc_info.ulposc_cali_result_target));
	of_property_read_u32(aov_dev->dev->of_node,
		PROPNAME_ULPOSC_CALI_FMETER_ID,
		&(aov_dev->ulposc_info.ulposc_cali_fmeter_id));
	of_property_read_u32(aov_dev->dev->of_node,
		PROPNAME_ULPOSC_RESULT_FMETER_ID,
		&(aov_dev->ulposc_info.ulposc_result_fmeter_id));

	ret = of_property_count_u32_elems(aov_dev->dev->of_node, PROPNAME_ULPOSC_CALI_REGISTER);
	if (ret != aov_dev->ulposc_info.ulposc_cali_config_num) {
		dev_info(aov_dev->dev,
			"[%s]: address nums does not equals to ulposc-cali-num\n", __func__);
		return;
	}

	ret = of_property_count_u32_elems(aov_dev->dev->of_node, PROPNAME_ULPOSC_CALI_CONFIG);
	if (ret != aov_dev->ulposc_info.ulposc_cali_config_num) {
		dev_info(aov_dev->dev,
			"[%s]: config nums does not equals to ulposc-cali-num\n", __func__);
		return;
	}

	ret = of_property_count_u32_elems(aov_dev->dev->of_node, PROPNAME_ULPOSC_CALI_RESULT);
	if (ret != aov_dev->ulposc_info.ulposc_cali_config_num) {
		dev_info(aov_dev->dev,
			"[%s]: result nums does not equals to ulposc-cali-num\n", __func__);
		return;
	}

	for (i = 0; i < aov_dev->ulposc_info.ulposc_cali_config_num; i++) {
		ret = of_property_read_u32_index(aov_dev->dev->of_node,
			PROPNAME_ULPOSC_CALI_REGISTER, i, &(aov_dev->ulposc_info.ulposc_config_register[i]));
		if (ret) {
			dev_info(aov_dev->dev,
				"[%s]: find cali address failed, idx: %d\n", __func__, i);
		}
		ret = of_property_read_u32_index(aov_dev->dev->of_node,
			PROPNAME_ULPOSC_CALI_CONFIG, i, &(aov_dev->ulposc_info.ulposc_cali_config_setting[i]));
		if (ret) {
			dev_info(aov_dev->dev,
				"[%s]: find cali config failed, idx: %d\n", __func__, i);
		}
		ret = of_property_read_u32_index(aov_dev->dev->of_node,
			PROPNAME_ULPOSC_CALI_RESULT, i, &(aov_dev->ulposc_info.ulposc_cali_result[i]));
		if (ret) {
			dev_info(aov_dev->dev,
				"[%s]: find cali result failed, idx: %d\n", __func__, i);
		}
	}

	aov_dev->ulposc_info.ulposc_cali_done = false;
	if (!of_property_read_u32(aov_dev->dev->of_node, "vlp-base", &vlp_base_pa))
		aov_dev->ulposc_info.vlp_base = ioremap(vlp_base_pa, 0x1000);
	else
		aov_dev->ulposc_info.vlp_base = NULL;
}

int aov_ulposc_cali(struct mtk_aov *aov_dev)
{
	if (!aov_dev->ulposc_info.ulposc_support) {
		dev_info(aov_dev->dev, "%s:no ulposc-support", __func__);
		return 0;
	}

	if (aov_dev->ulposc_info.ulposc_cali_done) {
		dev_info(aov_dev->dev, "%s:ulposc cali done, no need to cali", __func__);
		return 0;
	}

	aov_ulposc_turn_on_off(aov_dev, 0);

	aov_ulposc_set_config(aov_dev, PRE_CONFIG);
	aov_ulposc_turn_on_off(aov_dev, 1);
	aov_ulposc_cali_process(aov_dev);
	aov_ulposc_turn_on_off(aov_dev, 0);
	return 0;
}
