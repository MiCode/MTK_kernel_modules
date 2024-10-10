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

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
static struct pinctrl *g_pinctrl_ptr = NULL;
static struct pinctrl_state *g_combo_uart_pin_init = NULL;
static struct pinctrl_state *g_combo_uart_pin_pre_on = NULL;
static struct pinctrl_state *g_combo_uart_pin_on = NULL;
static bool g_uart_init_done = false;
static void __iomem *vir_0x1000_5000 = NULL; /* GPIO */
static void __iomem *vir_0x11C3_0000 = NULL; /* IOCFG_RM */

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
int connv3_plt_pinctrl_init_mt6989(struct platform_device *pdev);
int connv3_plt_pinctrl_deinit_mt6989(void);
int connv3_plt_pinctrl_setup_pre_mt6989(void);
int connv3_plt_pinctrl_setup_done_mt6989(void);
int connv3_plt_pinctrl_remove_mt6989(void);

const struct connv3_platform_pinctrl_ops g_connv3_platform_pinctrl_ops_mt6989 = {
	.pinctrl_init = connv3_plt_pinctrl_init_mt6989,
	.pinctrl_deinit = connv3_plt_pinctrl_deinit_mt6989,
	.pinctrl_setup_pre = connv3_plt_pinctrl_setup_pre_mt6989,
	.pinctrl_setup_done = connv3_plt_pinctrl_setup_done_mt6989,
	.pinctrl_remove = connv3_plt_pinctrl_remove_mt6989,
};

static void _dump_uart_gpio_state(char* tag)
{
#define GET_BIT(V, INDEX) ((V & (0x1U << INDEX)) >> INDEX)
	/* GPIO Func		AUX
	 * 141	SCP_WB_UTXD	1
	 * 142	SCP_WB_URXD	1
	 */
	unsigned int aux, dir, scp_pd, scp_pu;

	if (vir_0x1000_5000 == NULL)
		vir_0x1000_5000 = ioremap(0x10005000, 0x1000);
	if (vir_0x11C3_0000 == NULL)
		vir_0x11C3_0000 = ioremap(0x11C30000, 0x1000);

	if (vir_0x1000_5000 == NULL || vir_0x11C3_0000 == NULL) {
		pr_notice("[%s] vir_0x1000_5000=%lx vir_0x11C3_0000=%lx",
			__func__, (unsigned long)vir_0x1000_5000, (unsigned long)vir_0x11C3_0000);
		return;
	}

	/* 0x1000_5000
	 * 	0x0040	GPIO_DIR4
	 * 	0: GPIO Dir. as Input; 1: GPIO Dir. as Output
	 * 	GPIO_DIR register for GPIO128~GPIO159:
	 * 	13: 141
	 * 	14: 142
	 * 	0x0410	GPIO_MODE17
	 * 	[22:20]	Aux mode of PAD_SCP_WB_UTXD
	 * 	[26:24]	Aux mode of PAD_SCP_WB_URXD
	 * 0x11C3_0000	IOCFG_RM
	 * 	0x0080	PD_CFG0
	 * 	9 scp_wb_urxd Control PAD_SCP_WB_URXD pull down.
	 * 	10 scp_wb_utxd Control PAD_SCP_WB_UTXD pull down.
	 * 	0x0090	PU_CFG0
	 * 	9 scp_wb_urxd Control PAD_SCP_WB_URXD pull up.
	 * 	10 scp_wb_utxd Control PAD_SCP_WB_UTXD pull up.
	 */
	aux = CONSYS_REG_READ(vir_0x1000_5000 + 0x0410);
	dir = CONSYS_REG_READ(vir_0x1000_5000 + 0x0040);
	scp_pd = CONSYS_REG_READ(vir_0x11C3_0000 + 0x0080);
	scp_pu = CONSYS_REG_READ(vir_0x11C3_0000 + 0x0090);

	pr_info("[%s][%s][0x%08x][0x%08x][0x%08x][0x%08x] GPIO 141 SCP_WB_UTXD aux=[%d] dir=[%s] PD/PU=[%d/%d] 142 SCP_WB_URXD aux=[%d] dir=[%s] PD/PU=[%d/%d]",
		__func__, tag, aux, dir, scp_pd, scp_pu,
		((aux & 0x700000) >> 20), (GET_BIT(dir, 13)? "OUT" : "IN"),
		GET_BIT(scp_pd, 10), GET_BIT(scp_pu, 10),
		((aux & 0x7000000) >> 24), (GET_BIT(dir, 14)? "OUT" : "IN"),
		GET_BIT(scp_pd, 9), GET_BIT(scp_pu, 9));
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

	_dump_uart_gpio_state("init");
	return 0;
}

int connv3_plt_pinctrl_init_mt6989(struct platform_device *pdev)
{
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
	}

	return 0;
}

int connv3_plt_pinctrl_deinit_mt6989(void)
{
	if (vir_0x1000_5000 != NULL)
		iounmap(vir_0x1000_5000);
	if (vir_0x11C3_0000 != NULL)
		iounmap(vir_0x11C3_0000);

	vir_0x1000_5000 = NULL;
	vir_0x11C3_0000 = NULL;

	return 0;
}

int connv3_plt_pinctrl_setup_pre_mt6989(void)
{
	int ret;

	if (!g_uart_init_done) {
		pr_notice("[%s] uart init fail, skip setting", __func__);
		return 0;
	}

	ret = pinctrl_select_state(g_pinctrl_ptr, g_combo_uart_pin_pre_on);
	if (ret)
		pr_notice("[%s] pinctrl pre on fail, %d", __func__, ret);

	_dump_uart_gpio_state("pre after");

	return 0;
}

int connv3_plt_pinctrl_setup_done_mt6989(void)
{
	int ret;

	if (!g_uart_init_done) {
		pr_notice("[%s] uart init fail, skip setting", __func__);
		return 0;
	}

	ret = pinctrl_select_state(g_pinctrl_ptr, g_combo_uart_pin_on);
	if (ret)
		pr_notice("[%s] pinctrl on fail, %d", __func__, ret);

	_dump_uart_gpio_state("setup done");
	return 0;
}

int connv3_plt_pinctrl_remove_mt6989(void)
{
	connv3_plt_pinctrl_initial_state();
	return 0;
}
