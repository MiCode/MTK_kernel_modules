// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include <linux/gpio/consumer.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>

#include "connv3_hw.h"
#include "connv3_pinctrl_mng.h"

#include "consys_reg_util.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
enum uart_gpio_type {
	GPIO_COEX_UTXD,
	GPIO_COEX_URXD,
	GPIO_SCP_WB_UTXD,
	GPIO_SCP_WB_URXD,
	GPIO_UART_MAX,
};

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define CONNSYS_PIN_NAME_UART_INIT		"connsys-combo-gpio-init"
#define CONNSYS_PIN_NAME_UART_PRE_ON	"connsys-combo-gpio-pre-on"
#define CONNSYS_PIN_NAME_UART_ON		"connsys-combo-gpio-on"

#define CONNSYS_PIN_NAME_DFD_INIT	"connsys-pin-dfd-init"
#define CONNSYS_PIN_NAME_DFD_TRIGGER	"connsys-pin-dfd-trigger"
#define CONNSYS_PIN_NAME_DFD_DONE	"connsys-pin-dfd-release"

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
static struct pinctrl *g_pinctrl_ptr = NULL;
static struct pinctrl_state *g_combo_uart_pin_init = NULL;
static struct pinctrl_state *g_combo_uart_pin_pre_on = NULL;
static struct pinctrl_state *g_combo_uart_pin_on = NULL;
static bool g_uart_init_done = false;

static struct pinctrl_state *g_dfd_init = NULL;
static struct pinctrl_state *g_dfd_trigger = NULL;
static struct pinctrl_state *g_dfd_done = NULL;
static bool g_dfd_init_done = false;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
int connv3_plt_pinctrl_init_mt6991(struct platform_device *pdev);
int connv3_plt_pinctrl_deinit_mt6991(void);
int connv3_plt_pinctrl_setup_pre_mt6991(void);
int connv3_plt_pinctrl_setup_done_mt6991(void);
int connv3_plt_pinctrl_remove_mt6991(void);
int connv3_plt_pinctrl_dfd_trigger_mt6991(bool enable);

const struct connv3_platform_pinctrl_ops g_connv3_platform_pinctrl_ops_mt6991 = {
	.pinctrl_init = connv3_plt_pinctrl_init_mt6991,
	.pinctrl_deinit = connv3_plt_pinctrl_deinit_mt6991,
	.pinctrl_setup_pre = connv3_plt_pinctrl_setup_pre_mt6991,
	.pinctrl_setup_done = connv3_plt_pinctrl_setup_done_mt6991,
	.pinctrl_remove = connv3_plt_pinctrl_remove_mt6991,
	.pinctrl_dfd_trigger = connv3_plt_pinctrl_dfd_trigger_mt6991,
};

int connv3_plt_pinctrl_dfd_trigger_mt6991(bool enable)
{
	int ret;

	/* enable == 1: PU DFD pin
	 * enable == 0: PD DFD pin
	 */
	if (!g_dfd_init_done) {
		pr_notice("[%s] not init, returned\n", __func__);
		return -1;
	}

	if (enable) {
		ret = pinctrl_select_state(g_pinctrl_ptr, g_dfd_trigger);
		mdelay(30);
	} else {
		ret = pinctrl_select_state(g_pinctrl_ptr, g_dfd_done);
	}

	if (ret)
		pr_notice("[%s] enable=[%d] error, ret = %d\n", __func__, enable, ret);
	else
		pr_info("[%s] enable=[%d] done\n", __func__, enable);

	return ret;
}

static int connv3_plt_pinctrl_initial_state(void)
{
	int ret;

	if (!g_uart_init_done) {
		pr_notice("[%s] uart init fail, skip setting", __func__);
		return 0;
	}

	ret = pinctrl_select_state(g_pinctrl_ptr, g_combo_uart_pin_init);
	if (ret)
		pr_notice("[%s] pinctrl init fail, %d", __func__, ret);


	if (!g_dfd_init_done) {
		pr_notice("[%s] dfd pin not init, skip setting\n", __func__);
	} else {
		ret = pinctrl_select_state(g_pinctrl_ptr, g_dfd_init);
		if (ret)
			pr_notice("[%s] dfd pin init fail, %d\n", __func__, ret);
	}

	return 0;
}

int connv3_plt_pinctrl_init_mt6991(struct platform_device *pdev)
{
	int ret;

	g_pinctrl_ptr = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(g_pinctrl_ptr))
		pr_notice("[%s] fail to get connv3 pinctrl", __func__);
	else {
		/* Uart interface GPIO */
		g_combo_uart_pin_init = pinctrl_lookup_state(
			g_pinctrl_ptr, CONNSYS_PIN_NAME_UART_INIT);
		g_combo_uart_pin_pre_on = pinctrl_lookup_state(
			g_pinctrl_ptr, CONNSYS_PIN_NAME_UART_PRE_ON);
		g_combo_uart_pin_on = pinctrl_lookup_state(
			g_pinctrl_ptr, CONNSYS_PIN_NAME_UART_ON);
		if (IS_ERR_OR_NULL(g_combo_uart_pin_init) ||
			IS_ERR_OR_NULL(g_combo_uart_pin_pre_on) ||
			IS_ERR_OR_NULL(g_combo_uart_pin_on))
			pr_notice("[%s] get uart gpio fail: [%p][%p][%p]",
				__func__,
				g_combo_uart_pin_init, g_combo_uart_pin_pre_on, g_combo_uart_pin_on);
		else {
			g_uart_init_done = true;
			connv3_plt_pinctrl_initial_state();
		}

		g_dfd_init = pinctrl_lookup_state(
			g_pinctrl_ptr, CONNSYS_PIN_NAME_DFD_INIT);
		g_dfd_trigger = pinctrl_lookup_state(
			g_pinctrl_ptr, CONNSYS_PIN_NAME_DFD_TRIGGER);
		g_dfd_done = pinctrl_lookup_state(
			g_pinctrl_ptr, CONNSYS_PIN_NAME_DFD_DONE);
		if (IS_ERR_OR_NULL(g_dfd_init) ||
			IS_ERR_OR_NULL(g_dfd_trigger) ||
			IS_ERR_OR_NULL(g_dfd_done))
			pr_notice("[%s] get dfd gpio fail: [%p][%p][%p]\n",
				__func__, g_dfd_init, g_dfd_trigger, g_dfd_done);
		else {
			g_dfd_init_done = true;
			ret = pinctrl_select_state(g_pinctrl_ptr, g_dfd_init);
			if (ret)
				pr_notice("[%s] g_dfd_init fail, ret = %d\n", __func__, ret);
		}
	}

	return 0;
}

int connv3_plt_pinctrl_deinit_mt6991(void)
{
	return 0;
}

int connv3_plt_pinctrl_setup_pre_mt6991(void)
{
	int ret;

	if (!g_uart_init_done) {
		pr_notice("[%s] uart init fail, skip setting", __func__);
		return 0;
	}

	ret = pinctrl_select_state(g_pinctrl_ptr, g_combo_uart_pin_pre_on);
	if (ret)
		pr_notice("[%s] pinctrl pre on fail, %d", __func__, ret);

	connv3_plt_pinctrl_dfd_trigger_mt6991(false);
	return 0;
}

int connv3_plt_pinctrl_setup_done_mt6991(void)
{
	int ret;

	if (!g_uart_init_done) {
		pr_notice("[%s] uart init fail, skip setting", __func__);
		return 0;
	}

	ret = pinctrl_select_state(g_pinctrl_ptr, g_combo_uart_pin_on);
	if (ret)
		pr_notice("[%s] pinctrl on fail, %d", __func__, ret);

	return 0;
}

int connv3_plt_pinctrl_remove_mt6991(void)
{
	connv3_plt_pinctrl_initial_state();
	return 0;
}
