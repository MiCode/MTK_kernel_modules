// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include <connectivity_build_in_adapter.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include "osal.h"
#include "consys_hw.h"
#include "consys_reg_util.h"
#include "pmic_mng.h"
#if COMMON_KERNEL_PMIC_SUPPORT
#include <linux/regmap.h>
#include "mt6877_pmic.h"
#else
#include <pmic_api_buck.h>
#include <linux/mfd/mt6359p/registers.h>
#endif
#include "mt6877_consys_reg_offset.h"
#include "mt6877_pos.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
enum vcn13_state {
	vcn13_1_3v = 0,
	vcn13_1_32v = 1,
	vcn13_1_37v = 2,
};
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
static atomic_t g_voltage_change_status = ATOMIC_INIT(0);
static OSAL_TIMER g_voltage_change_timer;

static struct regulator *reg_VCN13;
static struct regulator *reg_VCN18;
static struct regulator *reg_VCN33_1_BT;
static struct regulator *reg_VCN33_1_WIFI;
static struct regulator *reg_VCN33_2_WIFI;
static struct notifier_block vcn13_nb;

static struct conninfra_dev_cb* g_dev_cb;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static int consys_plt_pmic_get_from_dts_mt6877(struct platform_device*, struct conninfra_dev_cb*);

static int consys_plt_pmic_common_power_ctrl_mt6877(unsigned int);
static int consys_plt_pmic_common_power_low_power_mode_mt6877(unsigned int);
static int consys_plt_pmic_wifi_power_ctrl_mt6877(unsigned int);
static int consys_plt_pmic_bt_power_ctrl_mt6877(unsigned int);
static int consys_plt_pmic_gps_power_ctrl_mt6877(unsigned int);
static int consys_plt_pmic_fm_power_ctrl_mt6877(unsigned int);
/* VCN33_1 is enable when BT or Wi-Fi is on */
static int consys_pmic_vcn33_1_power_ctl_mt6877(bool, struct regulator*);
/* VCN33_2 is enable when Wi-Fi is on */
static int consys_pmic_vcn33_2_power_ctl_mt6877(bool);

static int consys_plt_pmic_raise_voltage_mt6877(unsigned int, bool, bool);
static void consys_plt_pmic_raise_voltage_timer_handler_mt6877(timer_handler_arg);

static int consys_vcn13_oc_notify(struct notifier_block*, unsigned long, void*);
static int consys_plt_pmic_event_notifier_mt6877(unsigned int, unsigned int);

const struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6877 = {
	.consys_pmic_get_from_dts = consys_plt_pmic_get_from_dts_mt6877,
	/* vcn 18 */
	.consys_pmic_common_power_ctrl = consys_plt_pmic_common_power_ctrl_mt6877,
	.consys_pmic_common_power_low_power_mode = consys_plt_pmic_common_power_low_power_mode_mt6877,
	.consys_pmic_wifi_power_ctrl = consys_plt_pmic_wifi_power_ctrl_mt6877,
	.consys_pmic_bt_power_ctrl = consys_plt_pmic_bt_power_ctrl_mt6877,
	.consys_pmic_gps_power_ctrl = consys_plt_pmic_gps_power_ctrl_mt6877,
	.consys_pmic_fm_power_ctrl = consys_plt_pmic_fm_power_ctrl_mt6877,
	.consys_pmic_raise_voltage = consys_plt_pmic_raise_voltage_mt6877,
	.consys_pmic_event_notifier = consys_plt_pmic_event_notifier_mt6877,
};

int consys_plt_pmic_get_from_dts_mt6877(struct platform_device *pdev, struct conninfra_dev_cb* dev_cb)
{
	int ret;

	g_dev_cb = dev_cb;
	reg_VCN13 = devm_regulator_get_optional(&pdev->dev, "vcn13");
	if (!reg_VCN13)
		pr_err("Regulator_get VCN_13 fail\n");
	else {
		vcn13_nb.notifier_call = consys_vcn13_oc_notify;
		ret = devm_regulator_register_notifier(reg_VCN13, &vcn13_nb);
		if (ret)
			pr_info("VCN13 regulator notifier request failed\n");
	}

	reg_VCN18 = regulator_get(&pdev->dev, "vcn18");
	if (!reg_VCN18)
		pr_err("Regulator_get VCN_18 fail\n");
	reg_VCN33_1_BT = regulator_get(&pdev->dev, "vcn33_1_bt");
	if (!reg_VCN33_1_BT)
		pr_err("Regulator_get VCN33_1_BT fail\n");
	reg_VCN33_1_WIFI = regulator_get(&pdev->dev, "vcn33_1_wifi");
	if (!reg_VCN33_1_WIFI)
		pr_err("Regulator_get VCN33_1_WIFI fail\n");
	reg_VCN33_2_WIFI = regulator_get(&pdev->dev, "vcn33_2_wifi");
	if (!reg_VCN33_2_WIFI)
		pr_err("Regulator_get VCN33_WIFI fail\n");

	g_voltage_change_timer.timeoutHandler = consys_plt_pmic_raise_voltage_timer_handler_mt6877;
	osal_timer_create(&g_voltage_change_timer);
	return 0;
}

int consys_plt_pmic_common_power_ctrl_mt6877(unsigned int enable)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("[%s] not support on FPGA", __func__);
#else
	int ret;

	if (enable) {
		if (consys_is_rc_mode_enable_mt6877()) {
			/* RC mode */
			/* VCN18 */
#if COMMON_KERNEL_PMIC_SUPPORT
			/*  PMRC_EN[7][6][5][4] HW_OP_EN = 1, HW_OP_CFG = 0 */
			regmap_write(g_regmap, PMIC_RG_LDO_VCN18_OP_EN_SET_ADDR, 1 << 7);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN18_OP_CFG_SET_ADDR, 0 << 7);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN18_OP_EN_SET_ADDR, 1 << 6);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN18_OP_CFG_SET_ADDR, 0 << 6);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN18_OP_EN_SET_ADDR, 1 << 5);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN18_OP_CFG_SET_ADDR, 0 << 5);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN18_OP_EN_SET_ADDR, 1 << 4);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN18_OP_CFG_SET_ADDR, 0 << 4);
			/* SW_LP =0 */
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN18_LP_ADDR,
				PMIC_RG_LDO_VCN18_LP_MASK << PMIC_RG_LDO_VCN18_LP_SHIFT,
				0 << PMIC_RG_LDO_VCN18_LP_SHIFT);
#else
			/*  PMRC_EN[7][6][5][4] HW_OP_EN = 1, HW_OP_CFG = 0 */
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN7, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN6, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN5, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN4, 0, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_LP, 0);
#endif
			regulator_set_voltage(reg_VCN18, 1800000, 1800000);
			ret = regulator_enable(reg_VCN18);
			if (ret)
				pr_err("Enable VCN18 fail. ret=%d\n", ret);

#if COMMON_KERNEL_PMIC_SUPPORT
			/* VCN13 */
			/*  PMRC_EN[7][6][5][4] HW_OP_EN = 1, HW_OP_CFG = 0 */
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_EN_SET_ADDR, 1 << 7);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_CFG_SET_ADDR, 0 << 7);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_EN_SET_ADDR, 1 << 6);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_CFG_SET_ADDR, 0 << 6);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_EN_SET_ADDR, 1 << 5);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_CFG_SET_ADDR, 0 << 5);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_EN_SET_ADDR, 1 << 4);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_CFG_SET_ADDR, 0 << 4);
			/* SW_LP =0 */
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN13_LP_ADDR,
				PMIC_RG_LDO_VCN13_LP_MASK << PMIC_RG_LDO_VCN13_LP_SHIFT,
				0 << PMIC_RG_LDO_VCN13_LP_SHIFT);
#else
			/* VCN13 */
			/*  PMRC_EN[7][6][5][4] HW_OP_EN = 1, HW_OP_CFG = 0 */
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN7, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN6, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN5, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN4, 0, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN13_LP, 0);
#endif
			regulator_set_voltage(reg_VCN13, 1300000, 1300000);
			ret = regulator_enable(reg_VCN13);
			if (ret)
				pr_err("Enable VCN13 fail. ret=%d\n", ret);
		} else {
#if COMMON_KERNEL_PMIC_SUPPORT
			/* HW_OP_EN = 1, HW_OP_CFG = 1 */
			regmap_write(g_regmap, PMIC_RG_LDO_VCN18_OP_EN_SET_ADDR, 1 << 0);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN18_OP_CFG_SET_ADDR, 1 << 0);
			/* SW_LP=0 */
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN18_LP_ADDR,
				PMIC_RG_LDO_VCN18_LP_MASK << PMIC_RG_LDO_VCN18_LP_SHIFT,
				0 << PMIC_RG_LDO_VCN18_LP_SHIFT);
#else
			/* Legacy mode */
			/* HW_OP_EN = 1, HW_OP_CFG = 1 */
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN0, 1, 1, HW_LP);
			/* SW_LP=0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_LP, 0);
#endif
			regulator_set_voltage(reg_VCN18, 1800000, 1800000);
			/* SW_EN=1 */
			ret = regulator_enable(reg_VCN18);
			if (ret)
				pr_err("Enable VCN18 fail. ret=%d\n", ret);

#if COMMON_KERNEL_PMIC_SUPPORT
			/* HW_OP_EN = 1, HW_OP_CFG = 1 */
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_EN_SET_ADDR, 1 << 0);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_CFG_SET_ADDR, 1 << 0);
			/* SW_LP=0 */
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN13_LP_ADDR,
				PMIC_RG_LDO_VCN13_LP_MASK << PMIC_RG_LDO_VCN13_LP_SHIFT,
				0 << PMIC_RG_LDO_VCN13_LP_SHIFT);
#else
			/* HW_OP_EN = 1, HW_OP_CFG = 1 */
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN0, 1, 1, HW_LP);
			/* SW_LP=0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN13_LP, 0);
#endif
			regulator_set_voltage(reg_VCN13, 1300000, 1300000);
			/* SW_EN=1 */
			ret = regulator_enable(reg_VCN13);
			if (ret)
				pr_err("Enable VCN13 fail. ret=%d\n", ret);
		}
	} else {
		/* Add 1ms sleep to delay make sure that VCN13/18 would be turned off later then VCN33. */
		msleep(1);
		regulator_disable(reg_VCN13);
		regulator_disable(reg_VCN18);
	}
#endif
	return 0;
}

int consys_plt_pmic_common_power_low_power_mode_mt6877(unsigned int enable)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("[%s] not support on FPGA", __func__);
#else
	if (consys_is_rc_mode_enable_mt6877()) {
		if (enable) {
#if COMMON_KERNEL_PMIC_SUPPORT
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN18_LP_ADDR,
				PMIC_RG_LDO_VCN18_LP_MASK << PMIC_RG_LDO_VCN18_LP_SHIFT,
				1 << PMIC_RG_LDO_VCN18_LP_SHIFT);
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN13_LP_ADDR,
				PMIC_RG_LDO_VCN13_LP_MASK << PMIC_RG_LDO_VCN13_LP_SHIFT,
				1 << PMIC_RG_LDO_VCN13_LP_SHIFT);
#else
			/* SW_LP =1 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_LP, 1);
			/* SW_LP =1 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN13_LP, 1);
#endif
		}
	}
#endif
	return 0;
}

int consys_plt_pmic_wifi_power_ctrl_mt6877(unsigned int enable)
{
	int ret;

	ret = consys_pmic_vcn33_1_power_ctl_mt6877(enable, reg_VCN33_1_WIFI);
	if (ret)
		pr_err("%s VCN33_1 fail\n", (enable? "Enable" : "Disable"));
	ret = consys_pmic_vcn33_2_power_ctl_mt6877(enable);
	if (ret)
		pr_err("%s VCN33_2 fail\n", (enable? "Enable" : "Disable"));
	return ret;
}

int consys_plt_pmic_bt_power_ctrl_mt6877(unsigned int enable)
{
	return consys_pmic_vcn33_1_power_ctl_mt6877(enable, reg_VCN33_1_BT);
}

int consys_plt_pmic_gps_power_ctrl_mt6877(unsigned int enable)
{
	return 0;
}

int consys_plt_pmic_fm_power_ctrl_mt6877(unsigned int enable)
{
	return 0;
}

int consys_pmic_vcn33_1_power_ctl_mt6877(bool enable, struct regulator* reg_VCN33_1)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("[%s] not support on FPGA", __func__);
#else
	int ret;
	if (enable) {
		if (consys_is_rc_mode_enable_mt6877()) {
#if COMMON_KERNEL_PMIC_SUPPORT
			/*  PMRC_EN[6][5]  HW_OP_EN = 1, HW_OP_CFG = 0  */
			regmap_write(g_regmap, PMIC_RG_LDO_VCN33_1_OP_EN_SET_ADDR, 1 << 6);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN33_1_OP_CFG_SET_ADDR, 0 << 6);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN33_1_OP_EN_SET_ADDR, 1 << 5);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN33_1_OP_CFG_SET_ADDR, 0 << 5);
			/* SW_LP =0 */
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN33_1_LP_ADDR,
				PMIC_RG_LDO_VCN33_1_LP_MASK << PMIC_RG_LDO_VCN33_1_LP_SHIFT,
				0 << PMIC_RG_LDO_VCN33_1_LP_SHIFT);
#else
			/*  PMRC_EN[6][5]  HW_OP_EN = 1, HW_OP_CFG = 0  */
			KERNEL_pmic_ldo_vcn33_1_lp(SRCLKEN6, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn33_1_lp(SRCLKEN5, 0, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_LP, 0);
#endif
			regulator_set_voltage(reg_VCN33_1, 3300000, 3300000);
			/* SW_EN=0 */
			/* For RC mode, we don't have to control VCN33_1 & VCN33_2 */
		#if 0
			/* regulator_disable(reg_VCN33_1); */
		#endif
		} else {
#if COMMON_KERNEL_PMIC_SUPPORT
			/* HW_OP_EN = 1, HW_OP_CFG = 0 */
			regmap_write(g_regmap, PMIC_RG_LDO_VCN33_1_OP_EN_SET_ADDR, 1 << 0);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN33_1_OP_CFG_SET_ADDR, 0 << 0);
			/* SW_LP =0 */
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN33_1_LP_ADDR,
				PMIC_RG_LDO_VCN33_1_LP_MASK << PMIC_RG_LDO_VCN33_1_LP_SHIFT,
				0 << PMIC_RG_LDO_VCN33_1_LP_SHIFT);
#else
			/* HW_OP_EN = 1, HW_OP_CFG = 0 */
			KERNEL_pmic_ldo_vcn33_1_lp(SRCLKEN0, 1, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_LP, 0);
#endif
			regulator_set_voltage(reg_VCN33_1, 3300000, 3300000);
			/* SW_EN=1 */
			ret = regulator_enable(reg_VCN33_1);
			if (ret)
				pr_err("Enable VCN33_1 fail. ret=%d\n", ret);
		}
	} else {
		if (consys_is_rc_mode_enable_mt6877()) {
			/* Do nothing */
		} else {
			regulator_disable(reg_VCN33_1);
		}
	}
#endif
	return 0;
}

int consys_pmic_vcn33_2_power_ctl_mt6877(bool enable)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("[%s] not support on FPGA", __func__);
#else
	int ret;

	if (enable) {
		if (consys_is_rc_mode_enable_mt6877()) {
#if COMMON_KERNEL_PMIC_SUPPORT
			/*  PMRC_EN[6]  HW_OP_EN = 1, HW_OP_CFG = 0  */
			regmap_write(g_regmap, PMIC_RG_LDO_VCN33_2_OP_EN_SET_ADDR, 1 << 6);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN33_2_OP_CFG_SET_ADDR, 0 << 6);
			/* SW_LP =0 */
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN33_2_LP_ADDR,
				PMIC_RG_LDO_VCN33_2_LP_MASK << PMIC_RG_LDO_VCN33_2_LP_SHIFT,
				0 << PMIC_RG_LDO_VCN33_2_LP_SHIFT);
#else
			/*  PMRC_EN[6]  HW_OP_EN = 1, HW_OP_CFG = 0  */
			KERNEL_pmic_ldo_vcn33_2_lp(SRCLKEN6, 0, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_2_LP, 0);
#endif
			regulator_set_voltage(reg_VCN33_2_WIFI, 3300000, 3300000);
			/* SW_EN=0 */
			/* For RC mode, we don't have to control VCN33_1 & VCN33_2 */
		#if 0
			regulator_disable(reg_VCN33_2_WIFI);
		#endif
		} else  {
#if COMMON_KERNEL_PMIC_SUPPORT
			/* HW_OP_EN = 1, HW_OP_CFG = 0 */
			regmap_write(g_regmap, PMIC_RG_LDO_VCN33_2_OP_EN_SET_ADDR, 1 << 0);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN33_2_OP_CFG_SET_ADDR, 0 << 0);
			/* SW_LP =0 */
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN33_2_LP_ADDR,
				PMIC_RG_LDO_VCN33_2_LP_MASK << PMIC_RG_LDO_VCN33_2_LP_SHIFT,
				0 << PMIC_RG_LDO_VCN33_2_LP_SHIFT);
#else
			/* HW_OP_EN = 1, HW_OP_CFG = 0 */
			KERNEL_pmic_ldo_vcn33_2_lp(SRCLKEN0, 1, 1, HW_OFF);
			/* SW_LP =0 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_2_LP, 0);
#endif
			regulator_set_voltage(reg_VCN33_2_WIFI, 3300000, 3300000);
			/* SW_EN=1 */
			ret = regulator_enable(reg_VCN33_2_WIFI);
			if (ret)
				pr_err("Enable VCN33_2 fail. ret=%d\n", ret);
		}
	} else {
		if (consys_is_rc_mode_enable_mt6877()) {
			/* Do nothing */
		} else {
			regulator_disable(reg_VCN33_2_WIFI);
		}
	}
#endif

	return 0;
}


int consys_plt_pmic_raise_voltage_mt6877(unsigned int drv_type, bool raise, bool onoff)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	enum vcn13_state next_state;
	static enum vcn13_state curr_vcn13_state = vcn13_1_3v;
	static bool bt_raise = false;

	if (drv_type == 0 && onoff) {
		bt_raise = raise;
	} else {
		return 0;
	}
	if (bt_raise) {
		next_state = vcn13_1_37v;
	} else {
		next_state = vcn13_1_3v;
	}

	/* no change */
	if (curr_vcn13_state == next_state) {
		return 0;
	}
	pr_info("[%s][drv_type(%d) raise(%d) onoff(%d)][bt_raise(%d) curr_state(%d) next_state(%d)]\n",
		__func__, drv_type, raise, onoff, bt_raise, curr_vcn13_state, next_state);

	/* Check raise window, the duration to previous action should be 1 ms. */
	while (atomic_read(&g_voltage_change_status) == 1);
	pr_info("[%s] check down\n", __func__);
	curr_vcn13_state = next_state;

	switch (curr_vcn13_state) {
		case vcn13_1_3v:
#if COMMON_KERNEL_PMIC_SUPPORT
			/* restore VCN13 to 1.3V */
			regmap_update_bits(g_regmap,
				PMIC_RG_VCN13_VOCAL_ADDR,
				PMIC_RG_VCN13_VOCAL_MASK << PMIC_RG_VCN13_VOCAL_SHIFT,
				0 << PMIC_RG_VCN13_VOCAL_SHIFT);
			/* Restore VS2 sleep voltage to 1.35V */
			regmap_update_bits(g_regmap,
				PMIC_RG_BUCK_VS2_VOSEL_SLEEP_ADDR,
				PMIC_RG_BUCK_VS2_VOSEL_SLEEP_MASK << PMIC_RG_BUCK_VS2_VOSEL_SLEEP_SHIFT,
				0x2C << PMIC_RG_BUCK_VS2_VOSEL_SLEEP_SHIFT);
			/* clear bit 4 of VS2 VOTER then VS2 can restore to 1.35V */
			regmap_update_bits(g_regmap,
				PMIC_RG_BUCK_VS2_VOTER_EN_CLR_ADDR,
				PMIC_RG_BUCK_VS2_VOTER_EN_CLR_MASK << PMIC_RG_BUCK_VS2_VOTER_EN_CLR_SHIFT,
				0x10 << PMIC_RG_BUCK_VS2_VOTER_EN_CLR_SHIFT);
#else
			/* restore VCN13 to 1.3V */
			KERNEL_pmic_set_register_value(PMIC_RG_VCN13_VOCAL, 0);
			/* Restore VS2 sleep voltage to 1.35V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOSEL_SLEEP, 0x2C);
			/* clear bit 4 of VS2 VOTER then VS2 can restore to 1.35V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOTER_EN_CLR, 0x10);
#endif
			break;
		case vcn13_1_32v:
#if COMMON_KERNEL_PMIC_SUPPORT
			/* Set VS2 to 1.4V */
			regmap_update_bits(g_regmap,
				PMIC_RG_BUCK_VS2_VOSEL_ADDR,
				PMIC_RG_BUCK_VS2_VOSEL_MASK << PMIC_RG_BUCK_VS2_VOSEL_SHIFT,
				0x30 << PMIC_RG_BUCK_VS2_VOSEL_SHIFT);
			/* request VS2 to 1.4V by VS2 VOTER (use bit 4) */
			regmap_update_bits(g_regmap,
				PMIC_RG_BUCK_VS2_VOTER_EN_SET_ADDR,
				PMIC_RG_BUCK_VS2_VOTER_EN_SET_MASK << PMIC_RG_BUCK_VS2_VOTER_EN_SET_SHIFT,
				0x10 << PMIC_RG_BUCK_VS2_VOTER_EN_SET_SHIFT);
			/* Restore VS2 sleep voltage to 1.35V */
			regmap_update_bits(g_regmap,
				PMIC_RG_BUCK_VS2_VOSEL_SLEEP_ADDR,
				PMIC_RG_BUCK_VS2_VOSEL_SLEEP_MASK << PMIC_RG_BUCK_VS2_VOSEL_SLEEP_SHIFT,
				0x2C << PMIC_RG_BUCK_VS2_VOSEL_SLEEP_SHIFT);
			/* Set VCN13 to 1.32V */
			regmap_update_bits(g_regmap,
				PMIC_RG_VCN13_VOCAL_ADDR,
				PMIC_RG_VCN13_VOCAL_MASK << PMIC_RG_VCN13_VOCAL_SHIFT,
				0x2 << PMIC_RG_VCN13_VOCAL_SHIFT);
#else
			/* Set VS2 to 1.4V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOSEL, 0x30);
			/* request VS2 to 1.4V by VS2 VOTER (use bit 4) */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOTER_EN_SET, 0x10);
			/* Restore VS2 sleep voltage to 1.35V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOSEL_SLEEP, 0x2C);
			/* Set VCN13 to 1.32V */
			KERNEL_pmic_set_register_value(PMIC_RG_VCN13_VOCAL, 0x2);
#endif
			break;
		case vcn13_1_37v:
#if COMMON_KERNEL_PMIC_SUPPORT
			/* Set VS2 to 1.4625V */
			regmap_update_bits(g_regmap,
				PMIC_RG_BUCK_VS2_VOSEL_ADDR,
				PMIC_RG_BUCK_VS2_VOSEL_MASK << PMIC_RG_BUCK_VS2_VOSEL_SHIFT,
				0x35 << PMIC_RG_BUCK_VS2_VOSEL_SHIFT);
			/* request VS2 to 1.4V by VS2 VOTER (use bit 4) */
			regmap_update_bits(g_regmap,
				PMIC_RG_BUCK_VS2_VOTER_EN_SET_ADDR,
				PMIC_RG_BUCK_VS2_VOTER_EN_SET_MASK << PMIC_RG_BUCK_VS2_VOTER_EN_SET_SHIFT,
				0x10 << PMIC_RG_BUCK_VS2_VOTER_EN_SET_SHIFT);
			/* Set VS2 sleep voltage to 1.375V */
			regmap_update_bits(g_regmap,
				PMIC_RG_BUCK_VS2_VOSEL_SLEEP_ADDR,
				PMIC_RG_BUCK_VS2_VOSEL_SLEEP_MASK << PMIC_RG_BUCK_VS2_VOSEL_SLEEP_SHIFT,
				0x2E << PMIC_RG_BUCK_VS2_VOSEL_SLEEP_SHIFT);
			/* Set VCN13 to 1.37V */
			regmap_update_bits(g_regmap,
				PMIC_RG_VCN13_VOCAL_ADDR,
				PMIC_RG_VCN13_VOCAL_MASK << PMIC_RG_VCN13_VOCAL_SHIFT,
				0x7 << PMIC_RG_VCN13_VOCAL_SHIFT);
#else
			/* Set VS2 to 1.4625V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOSEL, 0x35);
			/* request VS2 to 1.4V by VS2 VOTER (use bit 4) */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOTER_EN_SET, 0x10);
			/* Set VS2 sleep voltage to 1.375V */
			KERNEL_pmic_set_register_value(PMIC_RG_BUCK_VS2_VOSEL_SLEEP, 0x2E);
			/* Set VCN13 to 1.37V */
			KERNEL_pmic_set_register_value(PMIC_RG_VCN13_VOCAL, 0x7);
#endif
			break;
	}
	udelay(50);

	/* start timer */
	atomic_set(&g_voltage_change_status, 1);
	osal_timer_modify(&g_voltage_change_timer, 1);
#endif
	return 0;
}

void consys_plt_pmic_raise_voltage_timer_handler_mt6877(timer_handler_arg data)
{
	atomic_set(&g_voltage_change_status, 0);
}

int consys_vcn13_oc_notify(struct notifier_block *nb, unsigned long event,
				  void *unused)
{
	if (event != REGULATOR_EVENT_OVER_CURRENT)
		return NOTIFY_OK;

	if (g_dev_cb != NULL && g_dev_cb->conninfra_pmic_event_notifier != NULL)
		g_dev_cb->conninfra_pmic_event_notifier(0, 0);
	return NOTIFY_OK;
}

int consys_plt_pmic_event_notifier_mt6877(unsigned int id, unsigned int event)
{
#define LOG_TMP_BUF_SZ 256
#define ATOP_DUMP_NUM 10
	static int oc_counter = 0;
	static int oc_dump = 0;
	unsigned int dump1_a, dump1_b, dump2_a, adie_value;
	void __iomem *addr = NULL;
	char tmp[LOG_TMP_BUF_SZ] = {'\0'};
	char tmp_buf[LOG_TMP_BUF_SZ] = {'\0'};
	int ret;
	const unsigned int adie_cr_list[ATOP_DUMP_NUM] = {
		0xa10, 0x90, 0x94, 0xa0,
		0xa18, 0xa1c, 0xc8, 0x3c,
		0x0b4, 0x34c
	};
	int i;

	if (event == 7)
		pr_info("[%s] Debug OC use: print a-die status\n", __func__);
	else {
		oc_counter++;
		pr_info("[%s] VCN13 OC times: %d\n", __func__, oc_counter);
	}

	if (oc_counter <= 30)
		oc_dump = 1;
	else if (oc_counter == (oc_dump * 100))
		oc_dump++;
	else
		return NOTIFY_OK;

	/* 1. Dump host csr status
	 * a. 0x1806_02CC 
	 * b. 0x1806_02C8
	 *
	 * 2. Dump R13 status
	 * a. 0x10006110
	 *
	 * 3. Dump RC status
	 * - 0x1000F928, 0x1000F92C, 0x1000F930, 0x1000F934
	 * - trace/timer
	 *
	 * 4. Wake up conninfra
	 *
	 * 5. Dump a-die ck_en
	 * - 0x1800_50a8
	 * - 0x1800_5120/0x1800_5124/0x1800_5128/0x1800_512C/0x1800_5130/0x1800_5134
	 *
	 * 6. Dump a-die status
	 * a. 0xa10
	 * b. 0x090/0x094/0x0a0
	 * c. 0xa18/0xa1c/0x0c8/0x03c
	 *
	 * 7. Make conninfra sleep
	 */
	dump1_a = CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_3_ADDR);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_DBG_SEL_CONN_INFRA_CFG_DBG_SEL,
		0x0);
	dump1_b = CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_2_ADDR);

	dump2_a = CONSYS_REG_READ(SPM_MD32PCM_SCU_STA0);
	pr_info("0x1806_02CC=[0x%08x] 0x1806_02C8=[0x%08x] 0x1000_6110=[0x%08x]",
		dump1_a, dump1_b, dump2_a);

	addr = ioremap(0x1000F900, 0x100);
	if (addr) {
		pr_info("[rc_status] [0x%08x][0x%08x][0x%08x][0x%08x]",
			CONSYS_REG_READ(addr + 0x28), CONSYS_REG_READ(addr + 0x2c),
			CONSYS_REG_READ(addr + 0x30), CONSYS_REG_READ(addr + 0x34));
		memset(tmp_buf, '\0', LOG_TMP_BUF_SZ);
		for (i = 0x50; i <= 0x94; i+= 4) {
			if (snprintf(tmp, LOG_TMP_BUF_SZ, "[0x%08x]",
				CONSYS_REG_READ(addr + i)) >= 0)
				strncat(tmp_buf, tmp, strlen(tmp));
		}
		pr_info("[rc_trace] %s", tmp_buf);

		memset(tmp_buf, '\0', LOG_TMP_BUF_SZ);
		for (i = 0x98; i <= 0xd4; i += 4) {
			if (snprintf(tmp, LOG_TMP_BUF_SZ, "[0x%08x]",
				CONSYS_REG_READ(addr + i)) >= 0)
				strncat(tmp_buf, tmp, strlen(tmp));
		}
		pr_info("[rc_timer] %s", tmp_buf);
		iounmap(addr);
	} else {
		pr_info("[%s] ioremap 0x1000_F900 fail", __func__);
	}

	ret = consys_hw_force_conninfra_wakeup();
	if (ret) {
		pr_info("[%s] force conninfra wakeup fail\n", __func__);
		return NOTIFY_OK;
	}

	memset(tmp_buf, '\0', LOG_TMP_BUF_SZ);
	for (i = 0x120; i <= 0x134; i+= 4) {
		if (snprintf(tmp, LOG_TMP_BUF_SZ, "[0x%08x]",
			CONSYS_REG_READ(CONN_REG_CONN_WT_SLP_CTL_REG_ADDR + i)) >= 0)
			strncat(tmp_buf, tmp, strlen(tmp));
	}
	pr_info("a-die ck:%s [0x%08x]", tmp_buf, CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR));

#if 0
	connsys_adie_top_ck_en_ctl_mt6877(true);
#endif
	memset(tmp_buf, '\0', LOG_TMP_BUF_SZ);
	for (i = 0; i < ATOP_DUMP_NUM; i++) {
		consys_spi_read_mt6877(SYS_SPI_TOP, adie_cr_list[i], &adie_value);
		if (snprintf(tmp, LOG_TMP_BUF_SZ, " [0x%04x: 0x%08x]", adie_cr_list[i], adie_value) >= 0)
			strncat(tmp_buf, tmp, strlen(tmp));
	}
#if 0
	connsys_adie_top_ck_en_ctl_mt6877(false);
#endif
	pr_info("ATOP:%s\n", tmp_buf);

	consys_hw_force_conninfra_sleep();
	return NOTIFY_OK;
}
