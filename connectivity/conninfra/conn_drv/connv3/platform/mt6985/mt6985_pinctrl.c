// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
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
#define CONNSYS_PIN_NAME_EXT_32K_EN_DEFAULT	"connsys-pin-ext32k-en-default"
#define CONNSYS_PIN_NAME_EXT_32K_EN_SET		"connsys-pin-ext32k-en-set"
#define CONNSYS_PIN_NAME_EXT_32K_EN_CLR		"connsys-pin-ext32k-en-clr"
#define CONNSYS_PIN_NAME_UART_INIT		"connsys-combo-gpio-init"
#define CONNSYS_PIN_NAME_UART_PRE_ON		"connsys-combo-gpio-pre-on"
#define CONNSYS_PIN_NAME_UART_ON		"connsys-combo-gpio-on"

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
static struct pinctrl *g_pinctrl_ptr = NULL;
static struct pinctrl_state *g_ext32k_pin_state_init = NULL;
static struct pinctrl_state *g_ext32k_pin_state_on = NULL;
static struct pinctrl_state *g_ext32k_pin_state_off = NULL;
static struct pinctrl_state *g_combo_uart_pin_init = NULL;
static struct pinctrl_state *g_combo_uart_pin_pre_on = NULL;
static struct pinctrl_state *g_combo_uart_pin_on = NULL;
static bool g_ext32k_pin_init_done = false;
static bool g_uart_init_done = false;
static void __iomem *vir_0x1000_5000 = NULL; /* GPIO */
static void __iomem *vir_0x11C0_0000 = NULL; /* IOCFG_RM */
static void __iomem *vir_0x11B2_0000 = NULL; /* IOCFG_RT */

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
int connv3_plt_pinctrl_init_mt6985(struct platform_device *pdev);
int connv3_plt_pinctrl_deinit_mt6985(void);
int connv3_plt_pinctrl_setup_pre_mt6985(void);
int connv3_plt_pinctrl_setup_done_mt6985(void);
int connv3_plt_pinctrl_remove_mt6985(void);
int connv3_plt_pinctrl_ext_32k_ctrl(bool on);

const struct connv3_platform_pinctrl_ops g_connv3_platform_pinctrl_ops_mt6985 = {
	.pinctrl_init = connv3_plt_pinctrl_init_mt6985,
	.pinctrl_deinit = connv3_plt_pinctrl_deinit_mt6985,
	.pinctrl_setup_pre = connv3_plt_pinctrl_setup_pre_mt6985,
	.pinctrl_setup_done = connv3_plt_pinctrl_setup_done_mt6985,
	.pinctrl_remove = connv3_plt_pinctrl_remove_mt6985,
	.pinctrl_ext_32k_ctrl = connv3_plt_pinctrl_ext_32k_ctrl,
};

#if 0
static int _drv_map(unsigned int drv)
{
	const int drv_map[] = {2, 4, 6, 8, 10, 12, 14, 16};

	if (drv >= 8)
		return 0;
	return drv_map[drv];
}
#endif

static void _dump_uart_gpio_state(char* tag)
{
#define GET_BIT(V, INDEX) ((V & (0x1U << INDEX)) >> INDEX)
	/* GPIO Func		AUX
	 * 226	COEX_UTXD	1
	 * 227	COEX_URXD	1
	 * 228	SCP_WB_UTXD	1
	 * 229	SCP_WB_URXD	1
	 */
	unsigned int aux, dir, scp_pd, scp_pu, coex_pd, coex_pu;

	if (vir_0x1000_5000 == NULL)
		vir_0x1000_5000 = ioremap(0x10005000, 0x1000);
	if (vir_0x11C0_0000 == NULL)
		vir_0x11C0_0000 = ioremap(0x11C00000, 0x1000);
	if (vir_0x11B2_0000 == NULL)
		vir_0x11B2_0000 = ioremap(0x11B20000, 0x1000);

	if (vir_0x1000_5000 == NULL || vir_0x11C0_0000 == NULL || vir_0x11B2_0000 == NULL) {
		pr_err("[%s] vir_0x1000_5000=%lx vir_0x11C0_0000=%lx vir_0x11B2_0000=%lx",
			__func__, vir_0x1000_5000, vir_0x11C0_0000, vir_0x11B2_0000);
		return;
	}

	/* 0x1000_5000
	 * 	0x0070	GPIO_DIR7
	 * 	0: GPIO Dir. as Input; 1: GPIO Dir. as Output
	 * 	GPIO_DIR register for GPIO224~GPIO241; GPIO242~GPIO255:
	 * 	2: 226
	 * 	3: 227
	 * 	4: 228
	 * 	5: 229
	 * 	0x04c0	GPIO_MODE28
	 * 	[10:8]	Aux mode of PAD_COEX_UTXD
	 * 	[14:12]	Aux mode of PAD_COEX_URXD
	 * 	[18:16]	Aux mode of PAD_SCP_WB_UTXD
	 * 	[22:20]	Aux mode of PAD_SCP_WB_URXD
	 * 0x11B2_0000	IOCFG_RT
	 * 	0x0040	PD_CFG0
	 * 	2 coex_urxd Control PAD_COEX_URXD pull down. (0/1:Disable/Enable)
	 * 	3 coex_utxd Control PAD_COEX_UTXD pull down. (0/1:Disable/Enable)
	 * 	0x0060	PU_CFG0
	 * 	2 coex_urxd Control PAD_COEX_URXD pull up. (0/1:Disable/Enable)
	 * 	3 coex_utxd Control PAD_COEX_UTXD pull up. (0/1:Disable/Enable)
	 * 0x11C0_0000	IOCFG_RM
	 * 	0x0030	PD_CFG0
	 * 	9 scp_wb_urxd Control PAD_SCP_WB_URXD pull down.
	 * 	10 scp_wb_utxd Control PAD_SCP_WB_UTXD pull down.
	 * 	0x0040	PU_CFG0
	 * 	9 scp_wb_urxd Control PAD_SCP_WB_URXD pull up.
	 * 	10 scp_wb_utxd Control PAD_SCP_WB_UTXD pull up.
	 */
	aux = CONSYS_REG_READ(vir_0x1000_5000 + 0x04c0);
	dir = CONSYS_REG_READ(vir_0x1000_5000 + 0x0070);
	scp_pd = CONSYS_REG_READ(vir_0x11C0_0000 + 0x0030);
	scp_pu = CONSYS_REG_READ(vir_0x11C0_0000 + 0x0040);
	coex_pd = CONSYS_REG_READ(vir_0x11B2_0000 + 0x0040);
	coex_pu = CONSYS_REG_READ(vir_0x11B2_0000 + 0x0060);

	pr_info("[%s][%s][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x]",
		__func__, tag, aux, dir, scp_pd, scp_pu, coex_pd, coex_pu);
	pr_info("GPIO 228 SCP_WB_UTXD\taux=[%d]\tdir=[%s]\tPD/PU=[%d/%d]",
		((aux & 0x70000) >> 16), (GET_BIT(dir, 4)? "OUT" : "IN"),
		GET_BIT(scp_pd, 10), GET_BIT(scp_pu, 10));
	pr_info("GPIO 229 SCP_WB_URXD\taux=[%d]\tdir=[%s]\tPD/PU=[%d/%d]",
		((aux & 0x700000) >> 20), (GET_BIT(dir, 5)? "OUT" : "IN"),
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

int connv3_plt_pinctrl_init_mt6985(struct platform_device *pdev)
{
	int ret;

	g_pinctrl_ptr = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(g_pinctrl_ptr))
		pr_notice("[%s] fail to get connv3 pinctrl", __func__);
	else {
		/* External 32K gpio */
		g_ext32k_pin_state_init = pinctrl_lookup_state(
						g_pinctrl_ptr, CONNSYS_PIN_NAME_EXT_32K_EN_DEFAULT);
		g_ext32k_pin_state_on = pinctrl_lookup_state(
						g_pinctrl_ptr, CONNSYS_PIN_NAME_EXT_32K_EN_SET);
		g_ext32k_pin_state_off = pinctrl_lookup_state(
						g_pinctrl_ptr, CONNSYS_PIN_NAME_EXT_32K_EN_CLR);
		if (IS_ERR_OR_NULL(g_ext32k_pin_state_init) ||
		    IS_ERR_OR_NULL(g_ext32k_pin_state_on) ||
		    IS_ERR_OR_NULL(g_ext32k_pin_state_off))
			pr_notice("[%s] get ext32k fail: [%s]=[%p]\t[%s]=[%p]\t[%s]=[%p]",
				__func__,
				CONNSYS_PIN_NAME_EXT_32K_EN_DEFAULT,
				g_ext32k_pin_state_init,
				CONNSYS_PIN_NAME_EXT_32K_EN_SET,
				g_ext32k_pin_state_on,
				CONNSYS_PIN_NAME_EXT_32K_EN_CLR,
				g_ext32k_pin_state_off);
		else {
			g_ext32k_pin_init_done = true;
			ret = pinctrl_select_state(g_pinctrl_ptr, g_ext32k_pin_state_init);
			if (ret)
				pr_notice("[%s] ext32k init fail, %d", __func__, ret);
		}

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

int connv3_plt_pinctrl_deinit_mt6985(void)
{
	if (vir_0x1000_5000 != NULL)
		iounmap(vir_0x1000_5000);
	if (vir_0x11B2_0000 != NULL)
		iounmap(vir_0x11B2_0000);
	if (vir_0x11C0_0000 != NULL)
		iounmap(vir_0x11C0_0000);

	vir_0x1000_5000 = NULL;
	vir_0x11B2_0000 = NULL;
	vir_0x11C0_0000 = NULL;

	return 0;
}

int connv3_plt_pinctrl_setup_pre_mt6985(void)
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

int connv3_plt_pinctrl_setup_done_mt6985(void)
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

int connv3_plt_pinctrl_remove_mt6985(void)
{
	connv3_plt_pinctrl_initial_state();
	return 0;
}

int connv3_plt_pinctrl_ext_32k_ctrl(bool on)
{
	int ret = 0;

	if (!g_ext32k_pin_init_done) {
		pr_info("[%s] no ext32k pin\n", __func__);
		return 0;
	} else {
		if (on)
			ret = pinctrl_select_state(g_pinctrl_ptr, g_ext32k_pin_state_on);
		else
			ret = pinctrl_select_state(g_pinctrl_ptr, g_ext32k_pin_state_off);
	}

	if (ret)
		pr_notice("[%s][%d] ext32k control fail: ret=[%d], pin state=[%p][%p]",
			__func__, on, ret, g_ext32k_pin_state_on, g_ext32k_pin_state_off);

	return ret;
}
