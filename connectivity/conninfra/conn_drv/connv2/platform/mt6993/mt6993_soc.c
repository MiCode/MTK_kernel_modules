// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include <linux/clk.h>
#include "../../../../conf/include/conninfra_conf.h"
#include "../../../../include/conninfra.h"
#include "../include/clock_mng.h"
#include "../include/consys_hw.h"
#include "../include/consys_reg_util.h"
#include "../include/plat_library.h"
#include "include/mt6993.h"
#include "include/mt6993_consys_reg_offset.h"
#include "include/mt6993_pos_gen.h"
#include "include/mt6993_soc.h"

static int g_conn_infra_bus_timeout_irq = 0;
static bool g_conn_infra_bus_timeout_irq_register = false;
static struct work_struct g_conninfra_irq_rst_work;
static atomic_t g_conn_infra_bus_timeout_irq_flag;
static struct clk *g_clk_pd;

int consys_co_clock_type_mt6993(void)
{
	const struct conninfra_conf *conf;
	static int clock_type = -1;
	unsigned char tcxo_gpio = 0;
	struct regmap *map = consys_clock_mng_get_regmap();
	int value = 0, ret;
	unsigned char co_clock_flag = 0;

	if (clock_type >= 0)
		return clock_type;

	clock_type = CONNSYS_CLOCK_SCHEMATIC_26M_COTMS;
	/* Default solution */
	conf = conninfra_conf_get_cfg();
	if (NULL == conf)
		pr_notice("[%s] Get conf fail", __func__);
	else {
		tcxo_gpio = conf->tcxo_gpio;
		co_clock_flag = conf->co_clock_flag;
	}

	if (tcxo_gpio != 0 || conn_hw_env.tcxo_support) {
		if (co_clock_flag == 3)
			clock_type = CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO;
		else
			clock_type = CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO;
	} else if (clock_mng_get_chip_id() == 0x6687 && map != NULL) {
		ret = regmap_read(map, MT6687_DCXO_XO_DIGBUF_ELR_CW0, &value);
		if (ret == 0) {
			if (value & 0x1)
				clock_type = CONNSYS_CLOCK_SCHEMATIC_52M_COTMS;
		} else {
			pr_notice("%s regmap_read failed, ret = %d\n", __func__, ret);
		}
	}
	pr_info("[%s] conf->tcxo_gpio=%d conn_hw_env.tcxo_support=%d, %s",
		__func__, tcxo_gpio, conn_hw_env.tcxo_support,
		clock_mng_get_schematic_name(clock_type));
	return clock_type;
}

int consys_clk_get_from_dts_mt6993(struct platform_device *pdev)
{
	g_clk_pd = devm_clk_get(&pdev->dev, "clk_pd_conn_connsys");
	if (IS_ERR(g_clk_pd))
		pr_notice("%s fail to get g_conn_power_clk\n", __func__);
	return 0;
}

int consys_clock_buffer_ctrl_mt6993(unsigned int enable)
{
	mapped_addr vir_addr_consys_gen_cksys_base = NULL;

	vir_addr_consys_gen_cksys_base =
		ioremap(CONSYS_GEN_CKSYS_TSE_BASE_ADDR, 0x80);

	if (!vir_addr_consys_gen_cksys_base) {
		pr_notice("vir_addr_consys_gen_cksys_base(%x) ioremap fail\n",
			CONSYS_GEN_CKSYS_TSE_BASE_ADDR);
		return -1;
	}

	if (enable) {
		/* write clk_ap2conn_host_sel = 1; main pll */
		CONSYS_SET_BIT(vir_addr_consys_gen_cksys_base +
			CONSYS_GEN_CLK_CFG_6_SET_OFFSET_ADDR, (0x1U << 24));

		udelay(20);
	} else {
		/* write clk_ap2conn_host_sel = 0; 26M */
		CONSYS_SET_BIT(vir_addr_consys_gen_cksys_base +
			CONSYS_GEN_CLK_CFG_6_CLR_OFFSET_ADDR, (0x1U << 24));
	}

	if (vir_addr_consys_gen_cksys_base)
		iounmap(vir_addr_consys_gen_cksys_base);

	return 0;
}

unsigned int consys_soc_chipid_get_mt6993(void)
{
	return PLATFORM_SOC_CHIP_MT6993;
}

int consys_platform_spm_conn_ctrl_mt6993(unsigned int enable)
{
	int ret = 0;
	int check = 0;

	if (!g_clk_pd) {
		pr_info("%s g_clk_pd is NULL.\n", __func__);
		return -1;
	}

	if (enable) {
		ret = clk_prepare_enable(g_clk_pd);
		if (ret)
			pr_info("clk_prepare_enable() fail(%d)\n", ret);
		else
			pr_info("clk_prepare_enable() ok\n");
	} else {
		// enable conn2emi gals slpprot
		CONSYS_SET_BIT(CONN_HOST_CSR_TOP_CONN_CONN2EMI_TX_SLEEP_PROTECT_CTRL_CSR_ADDR, 1);

		// polling conn2emi gals slpprot ready
		CONSYS_REG_BIT_POLLING(CONN_HOST_CSR_TOP_CONN_INFRA_AXI_LAYER_SLPPROT_STA_ADDR,
				2, 1, 100, 500, check);
		if (check != 0) {
			pr_notice("check slpprot fail, Status=0x%08x\n",
			  CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_AXI_LAYER_SLPPROT_STA_ADDR));
		}

		clk_disable_unprepare(g_clk_pd);
	}
	return ret;
}

void consys_set_if_pinmux_mt6993(unsigned int enable)
{
	// TCXO is controlled by VCN18, not by GPIO
}

static void conninfra_irq_rst_handler(struct work_struct *work)
{
	pr_notice("[%s] ++++++\n", __func__);
	conninfra_is_bus_hang();
	conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_CONNINFRA,
									"conninfra bus timeout irq handling");
	/* clear irq */
	CONSYS_SET_BIT(CONN_BUS_CR_BASE +
		CONN_BUS_CR_CONN_INFRA_OFF_BUS_TIMEOUT_CTRL_ADDR_OFFSET, (0x1U << 1));
	enable_irq(g_conn_infra_bus_timeout_irq);
	atomic_set(&g_conn_infra_bus_timeout_irq_flag, 0);
}

irqreturn_t consys_irq_handler_mt6993(int irq, void* data)
{
	if (atomic_read(&g_conn_infra_bus_timeout_irq_flag) == 0) {
		pr_notice("[%s] receive irq id=[%d]\n", __func__, irq);
		atomic_set(&g_conn_infra_bus_timeout_irq_flag, 1);
		disable_irq_nosync(g_conn_infra_bus_timeout_irq);
		schedule_work(&g_conninfra_irq_rst_work);
	}

	return IRQ_HANDLED;
}

int consys_register_irq_mt6993(struct platform_device *pdev)
{
	int ret = 0;

	g_conn_infra_bus_timeout_irq = platform_get_irq(pdev, 0);

	if (g_conn_infra_bus_timeout_irq < 0) {
		pr_notice("[%s] fail to get irq=[%d]\n", __func__,
				g_conn_infra_bus_timeout_irq);
		return -1;
	} else {
		INIT_WORK(&g_conninfra_irq_rst_work, conninfra_irq_rst_handler);

		ret = request_irq(g_conn_infra_bus_timeout_irq,
						consys_irq_handler_mt6993,
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
		if (enable_irq_wake(g_conn_infra_bus_timeout_irq) < 0) {
			pr_info("[%s] enable_irq_wake irq num=[%d] failed\n", __func__,
				g_conn_infra_bus_timeout_irq);
		}
		atomic_set(&g_conn_infra_bus_timeout_irq_flag, 0);
	}

	return 0;
}

void consys_unregister_irq_mt6993(void)
{
	if (g_conn_infra_bus_timeout_irq_register) {
		free_irq(g_conn_infra_bus_timeout_irq, NULL);
	}
}
