// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "../../../../conf/include/conninfra_conf.h"
#include "../../../../include/conninfra.h"
#include "../include/clock_mng.h"
#include "../include/consys_hw.h"
#include "../include/consys_reg_util.h"
#include "../include/plat_library.h"
#include "include/mt6989.h"
#include "include/mt6989_consys_reg_offset.h"
#include "include/mt6989_pos_gen.h"
#include "include/mt6989_soc.h"

static int g_conn_infra_bus_timeout_irq = 0;
static bool g_conn_infra_bus_timeout_irq_register = false;
static struct work_struct g_conninfra_irq_rst_work;

static atomic_t g_conn_infra_bus_timeout_irq_flag;

mapped_addr g_vir_addr_systimer;

int consys_co_clock_type_mt6989(void)
{
	const struct conninfra_conf *conf;
	/* For 6989, clock for connsys is always 26M (RFCK2B).
	 * We don't need to read clock ic register to identify clock rate.
	 */
	int clock_type = CONNSYS_CLOCK_SCHEMATIC_26M_COTMS;
	unsigned char tcxo_gpio = 0;

	/* Default solution */
	conf = conninfra_conf_get_cfg();
	if (NULL == conf)
		pr_notice("[%s] Get conf fail", __func__);
	else
		tcxo_gpio = conf->tcxo_gpio;

	if (tcxo_gpio != 0 || conn_hw_env.tcxo_support)
		clock_type = CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO;
	pr_info("[%s] conf->tcxo_gpio=%d conn_hw_env.tcxo_support=%d, %s",
		__func__, tcxo_gpio, conn_hw_env.tcxo_support,
		clock_mng_get_schematic_name(clock_type));

	return clock_type;
}

int consys_clk_get_from_dts_mt6989(struct platform_device *pdev)
{
	pm_runtime_enable(&pdev->dev);
	dev_pm_syscore_device(&pdev->dev, true);

	return 0;
}

int consys_clock_buffer_ctrl_mt6989(unsigned int enable)
{
	mapped_addr vir_addr_consys_dbg_gen_topckgen_base_mt6989;
	vir_addr_consys_dbg_gen_topckgen_base_mt6989
		= ioremap(CONSYS_DBG_GEN_TOPCKGEN_BASE_ADDR, 0x100);

	if (!vir_addr_consys_dbg_gen_topckgen_base_mt6989) {
		pr_notice("topckgen_base_mt6989 is not defined\n");
		return -1;
	}

	if (enable) {
		/* Turn on f_fap2conn_host_ck
		 * write 0x1000_0048[15] = 1
		 */
		pr_info("[%s] Turn on f_fap2conn_host_ck\n", __func__);
		CONSYS_SET_BIT(vir_addr_consys_dbg_gen_topckgen_base_mt6989 +
			0x48, (0x1U << 15));
	} else {
		/* Turn off f_fap2conn_host_ck
		 * write 0x1000_0044[15] = 1
		 */
		pr_info("[%s] Turn off f_fap2conn_host_ck\n", __func__);
		CONSYS_SET_BIT(vir_addr_consys_dbg_gen_topckgen_base_mt6989 +
			0x44, (0x1U << 15));
	}

	if (vir_addr_consys_dbg_gen_topckgen_base_mt6989)
		iounmap(vir_addr_consys_dbg_gen_topckgen_base_mt6989);

	return 0;
}

unsigned int consys_soc_chipid_get_mt6989(void)
{
	return PLATFORM_SOC_CHIP_MT6989;
}

int consys_platform_spm_conn_ctrl_mt6989(unsigned int enable)
{
	int ret = 0;
	struct platform_device *pdev = get_consys_device();

	if (!pdev) {
		pr_info("get_consys_device fail.\n");
		return -1;
	}

	if (enable) {
		ret = pm_runtime_get_sync(&(pdev->dev));
		if (ret)
			pr_info("pm_runtime_get_sync() fail(%d)\n", ret);
		else
			pr_info("pm_runtime_get_sync() CONSYS ok\n");

		ret = device_init_wakeup(&(pdev->dev), true);
		if (ret)
			pr_info("device_init_wakeup(true) fail.\n");
		else
			pr_info("device_init_wakeup(true) CONSYS ok\n");
	} else {
		ret = device_init_wakeup(&(pdev->dev), false);
		if (ret)
			pr_info("device_init_wakeup(false) fail.\n");
		else
			pr_info("device_init_wakeup(false) CONSYS ok\n");

		ret = pm_runtime_put_sync(&(pdev->dev));
		if (ret)
			pr_info("pm_runtime_put_sync() fail.\n");
		else
			pr_info("pm_runtime_put_sync() CONSYS ok\n");
	}

	return ret;
}

void consys_set_if_pinmux_mt6989(unsigned int enable)
{
	// No TCXO support for mt6989
}

static void conninfra_irq_rst_handler(struct work_struct *work)
{
	pr_notice("[%s] ++++++\n", __func__);

	if (g_vir_addr_systimer) {
		pr_info("[%s] 0x1cc10008=[0x%x]\n", __func__,
			CONSYS_REG_READ(g_vir_addr_systimer + 0x8));
		pr_info("[%s] 0x1cc1000c=[0x%x]\n", __func__,
			CONSYS_REG_READ(g_vir_addr_systimer + 0xc));
	}
	conninfra_is_bus_hang();
	conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_CONNINFRA,
									"conninfra bus timeout irq handling");
	/* clear irq */
	CONSYS_SET_BIT(CONN_BUS_CR_BASE +
		CONN_BUS_CR_CONN_INFRA_OFF_BUS_TIMEOUT_CTRL_ADDR_OFFSET, (0x1U << 1));
	enable_irq(g_conn_infra_bus_timeout_irq);
	atomic_set(&g_conn_infra_bus_timeout_irq_flag, 0);
}

irqreturn_t consys_irq_handler_mt6989(int irq, void* data)
{
	if (atomic_read(&g_conn_infra_bus_timeout_irq_flag) == 0) {
		pr_notice("[%s] receive irq id=[%d]\n", __func__, irq);
		atomic_set(&g_conn_infra_bus_timeout_irq_flag, 1);
		disable_irq_nosync(g_conn_infra_bus_timeout_irq);
		schedule_work(&g_conninfra_irq_rst_work);
	}

	return IRQ_HANDLED;
}

int consys_register_irq_mt6989(struct platform_device *pdev)
{
	int ret = 0;

	pr_info("[mtk debug] ioremap systimer addr 0x1cc10000\n");
	g_vir_addr_systimer = ioremap(0x1cc10000, 0x100);

	if (!g_vir_addr_systimer)
		pr_notice("[mtk debug] g_vir_addr_systimer is not defined\n");

	g_conn_infra_bus_timeout_irq = platform_get_irq(pdev, 0);

	if (g_conn_infra_bus_timeout_irq < 0) {
		pr_notice("[%s] fail to get irq=[%d]\n", __func__,
				g_conn_infra_bus_timeout_irq);
		return -1;
	} else {
		INIT_WORK(&g_conninfra_irq_rst_work, conninfra_irq_rst_handler);

		ret = request_irq(g_conn_infra_bus_timeout_irq,
						consys_irq_handler_mt6989,
						IRQF_TRIGGER_HIGH,
						"CONN_INFRA_BUS_TIMEOUT_IRQ", NULL);
		if (ret) {
			pr_notice("[%s] register irq num=[%d] fail\n", __func__,
				g_conn_infra_bus_timeout_irq);
			return ret;
		} else {
			g_conn_infra_bus_timeout_irq_register = true;
			pr_info("[%s] register irq num=[%d] done\n", __func__,
				g_conn_infra_bus_timeout_irq);
		}

		atomic_set(&g_conn_infra_bus_timeout_irq_flag, 0);
	}

	return 0;
}

void consys_unregister_irq_mt6989(void)
{
	if (g_conn_infra_bus_timeout_irq_register) {
		free_irq(g_conn_infra_bus_timeout_irq, NULL);
	}

	if (g_vir_addr_systimer)
		iounmap(g_vir_addr_systimer);
}
