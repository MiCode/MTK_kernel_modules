/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include <connectivity_build_in_adapter.h>
#include <asm/atomic.h>
#include <linux/memblock.h>
#include <linux/platform_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/mfd/mt6359p/registers.h>
#include <linux/regulator/consumer.h>
#include <linux/notifier.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/version.h>
#include "consys_hw.h"
#include "consys_reg_util.h"
#include "osal.h"
#include "mt6885.h"
#include "mt6885_pmic.h"
#include "mt6885_pos.h"
#include "mt6885_consys_reg.h"
#include "mt6885_consys_reg_offset.h"
#include "pmic_mng.h"
#if (COMMON_KERNEL_PMIC_SUPPORT)
#include <linux/regmap.h>
#else
#include <pmic_api_buck.h>
#include <linux/mfd/mt6359p/registers.h>
#endif


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

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

static int consys_plt_pmic_get_from_dts(struct platform_device *pdev, struct conninfra_dev_cb* dev_cb);

static int consys_plt_pmic_common_power_ctrl(unsigned int enable);
static int consys_plt_pmic_wifi_power_ctrl(unsigned int enable);
static int consys_plt_pmic_bt_power_ctrl(unsigned int enable);
static int consys_plt_pmic_gps_power_ctrl(unsigned int enable);
static int consys_plt_pmic_fm_power_ctrl(unsigned int enable);
/* VCN33_1 is enable when BT or Wi-Fi is on */
static int consys_pmic_vcn33_1_power_ctl(bool enable, struct regulator* reg_VCN33_1);
/* VCN33_2 is enable when Wi-Fi is on */
static int consys_pmic_vcn33_2_power_ctl(bool enable);

static int consys_plt_pmic_event_notifier(unsigned int id, unsigned int event);
static int consys_plt_pmic_raise_voltage(unsigned int drv_type, bool rase, bool onoff);
static void consys_plt_pmic_raise_voltage_timer_handler(timer_handler_arg data);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6885 = {
	.consys_pmic_get_from_dts = consys_plt_pmic_get_from_dts,
	/* vcn 18 */
	.consys_pmic_common_power_ctrl = consys_plt_pmic_common_power_ctrl,
	.consys_pmic_wifi_power_ctrl = consys_plt_pmic_wifi_power_ctrl,
	.consys_pmic_bt_power_ctrl = consys_plt_pmic_bt_power_ctrl,
	.consys_pmic_gps_power_ctrl = consys_plt_pmic_gps_power_ctrl,
	.consys_pmic_fm_power_ctrl = consys_plt_pmic_fm_power_ctrl,
	.consys_pmic_event_notifier = consys_plt_pmic_event_notifier,
	.consys_pmic_raise_voltage = consys_plt_pmic_raise_voltage,
};

static struct regulator *reg_VCN13;
static struct regulator *reg_VCN18;
static struct regulator *reg_VCN33_1_BT;
static struct regulator *reg_VCN33_1_WIFI;
static struct regulator *reg_VCN33_2_WIFI;
static struct notifier_block vcn13_nb;

static struct conninfra_dev_cb* g_dev_cb;

static atomic_t g_voltage_change_status = ATOMIC_INIT(0);
static OSAL_TIMER g_voltage_change_timer;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

static int consys_vcn13_oc_notify(struct notifier_block *nb, unsigned long event,
				  void *unused)
{
	if (event != REGULATOR_EVENT_OVER_CURRENT)
		return NOTIFY_OK;

	if (g_dev_cb != NULL && g_dev_cb->conninfra_pmic_event_notifier != NULL)
		g_dev_cb->conninfra_pmic_event_notifier(0, 0);
	return NOTIFY_OK;
}

static int consys_plt_pmic_event_notifier(unsigned int id, unsigned int event)
{
	static int oc_counter = 0;
	static int oc_dump = 0;

	oc_counter++;
	pr_info("[%s] VCN13 OC times: %d\n", __func__, oc_counter);

	if (oc_counter <= 30)
		oc_dump = 1;
	else if (oc_counter == (oc_dump * 100))
		oc_dump++;
	else
		return NOTIFY_OK;

	consys_plt_pmic_ctrl_dump("VCN13 OC");
	return NOTIFY_OK;
}

int consys_plt_pmic_ctrl_dump(const char* tag)
{
#define ATOP_DUMP_NUM 10
#define LOG_TMP_BUF_SZ 256
	int ret;
	unsigned int adie_value = 0;
	unsigned int value1 = 0, value2 = 0, value3 = 0;
	const unsigned int adie_cr_list[ATOP_DUMP_NUM] = {
		0xa10, 0x90, 0x94, 0xa0,
		0xa18, 0xa1c, 0xc8, 0x3c,
		0x0b4, 0x34c
	};
	int index;
	char tmp[LOG_TMP_BUF_SZ] = {'\0'};
	char tmp_buf[LOG_TMP_BUF_SZ] = {'\0'};

	consys_hw_is_bus_hang();
	ret = consys_hw_force_conninfra_wakeup();
	if (ret) {
		pr_info("[%s] force conninfra wakeup fail\n", __func__);
		return 0;
	}

	value1 = CONSYS_REG_READ(CON_REG_INFRA_CFG_ADDR + CONN_INFRA_CFG_ADIE_CTL);
	value2 = CONSYS_REG_READ(CON_REG_WT_SPL_CTL_ADDR + 0xa8);
	if (consys_sema_acquire_timeout_mt6885(CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_SUCCESS) {
		value3 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_BASE_ADDR + CONN_INFRA_SYSRAM_SW_CR_A_DIE_TOP_CK_EN_CTRL);
		consys_sema_release_mt6885(CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX);
		pr_info("[%s] D-die: 0x1800_1900:0x%08x 0x1800_50A8:0x%08x 0x1805_2830:0x%08x\n",
			(tag == NULL?__func__:tag), value1, value2, value3);
	} else {
		pr_info("[%s] D-die: 0x1800_1900:0x%08x 0x1800_50A8:0x%08x\n",
			(tag == NULL?__func__:tag), value1, value2);
	}

	for (index = 0; index < ATOP_DUMP_NUM; index++) {
		consys_spi_read_mt6885(SYS_SPI_TOP, adie_cr_list[index], &adie_value);
		if (snprintf(tmp, LOG_TMP_BUF_SZ, " [0x%04x: 0x%08x]", adie_cr_list[index], adie_value) >= 0)
			strncat(tmp_buf, tmp, strlen(tmp));
	}
	pr_info("[%s] ATOP:%s\n", (tag == NULL?__func__:tag), tmp_buf);
	consys_hw_force_conninfra_sleep();

	return 0;
}

int consys_plt_pmic_get_from_dts(struct platform_device *pdev, struct conninfra_dev_cb* dev_cb)
{
	int ret;

	g_dev_cb = dev_cb;
//#if CONSYS_PMIC_CTRL_ENABLE
	reg_VCN13 = devm_regulator_get_optional(&pdev->dev, "vcn13");
	if (!reg_VCN13)
		pr_err("Regulator_get VCN_13 fail\n");
	else {
		vcn13_nb.notifier_call = consys_vcn13_oc_notify;
		ret = devm_regulator_register_notifier(reg_VCN13, &vcn13_nb);
		if (ret) {
			pr_info("VCN13 regulator notifier request failed\n");
		}
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
//#endif

	g_voltage_change_timer.timeoutHandler = consys_plt_pmic_raise_voltage_timer_handler;
	osal_timer_create(&g_voltage_change_timer);
	return 0;
}

int consys_pmic_vcn33_1_power_ctl(bool enable, struct regulator *reg_VCN33_1)
{
	int ret;
	if (enable) {
		if (consys_is_rc_mode_enable_mt6885()) {
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
		if (consys_is_rc_mode_enable_mt6885()) {
			/* Do nothing */
		} else {
			regulator_disable(reg_VCN33_1);
		}
	}

	return 0;
}

int consys_pmic_vcn33_2_power_ctl(bool enable)
{
	int ret;
	if (enable) {
		if (consys_is_rc_mode_enable_mt6885()) {
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
		if (consys_is_rc_mode_enable_mt6885()) {
			/* Do nothing */
		} else {
			regulator_disable(reg_VCN33_2_WIFI);
		}
	}

	return 0;
}

int consys_plt_pmic_common_power_ctrl(unsigned int enable)
{
	int ret;

	if (enable) {
		if (consys_is_rc_mode_enable_mt6885()) {
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
			/* SW_LP =1 */
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN18_LP_ADDR,
				PMIC_RG_LDO_VCN18_LP_MASK << PMIC_RG_LDO_VCN18_LP_SHIFT,
				1 << PMIC_RG_LDO_VCN18_LP_SHIFT);
#else
			/*  PMRC_EN[7][6][5][4] HW_OP_EN = 1, HW_OP_CFG = 0 */
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN7, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN6, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN5, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn18_lp(SRCLKEN4, 0, 1, HW_OFF);
			/* SW_LP =1 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_LP, 1);
#endif
			regulator_set_voltage(reg_VCN18, 1800000, 1800000);
			ret = regulator_enable(reg_VCN18);
			if (ret)
				pr_err("Enable VCN18 fail. ret=%d\n", ret);

			/* VCN13 */
#if COMMON_KERNEL_PMIC_SUPPORT
			/*  PMRC_EN[7][6][5][4] HW_OP_EN = 1, HW_OP_CFG = 0 */
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_EN_SET_ADDR, 1 << 7);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_CFG_SET_ADDR, 0 << 7);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_EN_SET_ADDR, 1 << 6);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_CFG_SET_ADDR, 0 << 6);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_EN_SET_ADDR, 1 << 5);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_CFG_SET_ADDR, 0 << 5);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_EN_SET_ADDR, 1 << 4);
			regmap_write(g_regmap, PMIC_RG_LDO_VCN13_OP_CFG_SET_ADDR, 0 << 4);
			/* SW_LP =1 */
			regmap_update_bits(g_regmap,
				PMIC_RG_LDO_VCN13_LP_ADDR,
				PMIC_RG_LDO_VCN13_LP_MASK << PMIC_RG_LDO_VCN13_LP_SHIFT,
				1 << PMIC_RG_LDO_VCN13_LP_SHIFT);
#else
			/*  PMRC_EN[7][6][5][4] HW_OP_EN = 1, HW_OP_CFG = 0 */
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN7, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN6, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN5, 0, 1, HW_OFF);
			KERNEL_pmic_ldo_vcn13_lp(SRCLKEN4, 0, 1, HW_OFF);
			/* SW_LP =1 */
			KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN13_LP, 1);
#endif
			regulator_set_voltage(reg_VCN13, 1300000, 1300000);
			ret = regulator_enable(reg_VCN13);
			if (ret)
				pr_err("Enable VCN13 fail. ret=%d\n", ret);
		} else {
			/* Legacy mode */
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
		regulator_disable(reg_VCN13);
		regulator_disable(reg_VCN18);
	}
	return 0;
}

int consys_plt_pmic_wifi_power_ctrl(unsigned int enable)
{
	int ret;

	ret = consys_pmic_vcn33_1_power_ctl(enable, reg_VCN33_1_WIFI);
	if (ret)
		pr_err("%s VCN33_1 fail\n", (enable? "Enable" : "Disable"));
	ret = consys_pmic_vcn33_2_power_ctl(enable);
	if (ret)
		pr_err("%s VCN33_2 fail\n", (enable? "Enable" : "Disable"));
	return ret;
}

int consys_plt_pmic_bt_power_ctrl(unsigned int enable)
{
	return consys_pmic_vcn33_1_power_ctl(enable, reg_VCN33_1_BT);
}

int consys_plt_pmic_gps_power_ctrl(unsigned int enable)
{
	return 0;
}

int consys_plt_pmic_fm_power_ctrl(unsigned int enable)
{
	return 0;
}

static void consys_plt_pmic_raise_voltage_timer_handler(timer_handler_arg data)
{
	atomic_set(&g_voltage_change_status, 0);
}

enum vcn13_state {
	vcn13_1_3v = 0,
	vcn13_1_32v = 1,
	vcn13_1_37v = 2,
};

static void consys_raise_vcn13_vs2_voltage(enum vcn13_state next_state)
{
	static enum vcn13_state curr_vcn13_state = vcn13_1_3v;

	/* no change */
	if (curr_vcn13_state == next_state) {
		pr_info("[%s] curr==next_state(%d, %d), return\n", __func__, curr_vcn13_state, next_state);
		return;
	}
	pr_info("[%s] curr_vcn13_state=%d next_state=%d\n", __func__, curr_vcn13_state, next_state);
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
			/* Set VS2 to 1.4V */
			regmap_update_bits(g_regmap,
				PMIC_RG_BUCK_VS2_VOSEL_ADDR,
				PMIC_RG_BUCK_VS2_VOSEL_MASK << PMIC_RG_BUCK_VS2_VOSEL_SHIFT,
				0x35 << PMIC_RG_BUCK_VS2_VOSEL_SHIFT);
			/* request VS2 to 1.4V by VS2 VOTER (use bit 4) */
			regmap_update_bits(g_regmap,
				PMIC_RG_BUCK_VS2_VOTER_EN_SET_ADDR,
				PMIC_RG_BUCK_VS2_VOTER_EN_SET_MASK << PMIC_RG_BUCK_VS2_VOTER_EN_SET_SHIFT,
				0x10 << PMIC_RG_BUCK_VS2_VOTER_EN_SET_SHIFT);
			/* Restore VS2 sleep voltage to 1.35V */
			regmap_update_bits(g_regmap,
				PMIC_RG_BUCK_VS2_VOSEL_SLEEP_ADDR,
				PMIC_RG_BUCK_VS2_VOSEL_SLEEP_MASK << PMIC_RG_BUCK_VS2_VOSEL_SLEEP_SHIFT,
				0x2E << PMIC_RG_BUCK_VS2_VOSEL_SLEEP_SHIFT);
			/* Set VCN13 to 1.32V */
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
}

int consys_plt_pmic_raise_voltage(unsigned int drv_type, bool raise, bool onoff)
{
	static bool bt_raise = false;
	pr_info("[%s] [drv_type(%d) raise(%d) onoff(%d)][bt_raise(%d)]\n",
		__func__, drv_type, raise, onoff, bt_raise);
	if (drv_type == 0 && onoff) {
		bt_raise = raise;
	} else {
		return 0;
	}
	if (bt_raise) {
		consys_raise_vcn13_vs2_voltage(vcn13_1_37v);
	} else {
		consys_raise_vcn13_vs2_voltage(vcn13_1_3v);
	}
	return 0;
}

#if 0
int consys_plt_pmic_raise_voltage(unsigned int drv_type, bool raise, bool onoff)
{
	static bool bt_raise = false;
	static bool wifi_raise = false;
	static bool conninfra_raise = false;
	bool both_wifi_bt_on = false;
	unsigned int radio_status;

	pr_info("[%s] drv_type=[%d] raise=[%d] onoff=[%d], bt_raise=[%d] wifi_raise=[%d] conninfra_raise=[%d]",
		__func__, drv_type, raise, onoff, bt_raise, wifi_raise, conninfra_raise);

	if (drv_type == 0) {
		/* BT */
		if (onoff) {
			bt_raise = raise;
		}
	} else if (drv_type == 3) {
		/* WIFI */
		if (onoff) {
			if (!raise)
				wifi_raise = false;
		} else {
			wifi_raise = raise;
		}
	} else if (drv_type == 4 && !onoff) {
		/* conninfra, for screen on/off */
		conninfra_raise = raise;
	} else {
		return 0;
	}

	radio_status = CONSYS_REG_READ(CONN_INFRA_SYSRAM_BASE_ADDR + CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS);
	if ((radio_status & 0x9) == 0x9)
		both_wifi_bt_on = true;

	if ((bt_raise && wifi_raise) || (conninfra_raise && both_wifi_bt_on)) {
		consys_raise_vcn13_vs2_voltage(vcn13_1_37v);
	} else if (bt_raise) {
		consys_raise_vcn13_vs2_voltage(vcn13_1_32v);
	} else {
		consys_raise_vcn13_vs2_voltage(vcn13_1_3v);
	}
	return 0;
}
#endif
