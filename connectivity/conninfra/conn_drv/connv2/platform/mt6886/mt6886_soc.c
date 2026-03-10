// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "../../../../conf/include/conninfra_conf.h"
#include "../../../../include/conninfra.h"
#include "../include/clock_mng.h"
#include "../include/consys_hw.h"
#include "../include/consys_reg_util.h"
#include "../include/plat_library.h"
#include "include/mt6886.h"
#include "include/mt6886_consys_reg_offset.h"
#include "include/mt6886_pos_gen.h"
#include "include/mt6886_soc.h"

int consys_get_co_clock_type_mt6886(void)
{
	const struct conninfra_conf *conf;
	struct regmap *map = consys_clock_mng_get_regmap();
	int value = 0, clock_type = CONNSYS_CLOCK_SCHEMATIC_26M_COTMS;
	const char *clock_name[CONNSYS_CLOCK_SCHEMATIC_MAX] = {
		"26M co-clock", "52M co-clock", "26M tcxo", "52M tcxo"};

	/* Default solution */
	conf = conninfra_conf_get_cfg();
	if (conf == NULL) {
		pr_err("[%s] Get conf fail", __func__);
		return -1;
	}

	if (conf->tcxo_gpio != 0 || conn_hw_env.tcxo_support) {
		if (conf->co_clock_flag == 3)
			clock_type = CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO;
		else
			clock_type = CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO;
	} else {
		if (!map) {
			pr_notice("%s, failed to get regmap.\n", __func__);
			return -1;
		}
		regmap_read(map, DCXO_DIGCLK_ELR, &value);
		if (value & 0x1)
			clock_type = CONNSYS_CLOCK_SCHEMATIC_52M_COTMS;
	}
	pr_info("[%s] conf->tcxo_gpio=%d conn_hw_env.tcxo_support=%d, %s",
		__func__, conf->tcxo_gpio, conn_hw_env.tcxo_support, clock_name[clock_type]);

	return clock_type;
}

int consys_clk_get_from_dts_mt6886(struct platform_device *pdev)
{
	pm_runtime_enable(&pdev->dev);
	dev_pm_syscore_device(&pdev->dev, true);

	return 0;
}

int consys_platform_spm_conn_ctrl_mt6886(unsigned int enable)
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

int consys_clock_buffer_ctrl_mt6886(unsigned int enable)
{
	/* This function call didn't work now.
	 * clock buffer is HW controlled, not SW controlled.
	 * Keep this function call to update status.
	 */
#if (!COMMON_KERNEL_CLK_SUPPORT)
	if (enable)
		KERNEL_clk_buf_ctrl(CLK_BUF_CONN, true);	/*open XO_WCN*/
	else
		KERNEL_clk_buf_ctrl(CLK_BUF_CONN, false);	/*close XO_WCN*/
#endif
	return 0;
}

unsigned int consys_soc_chipid_get_mt6886(void)
{
	return PLATFORM_SOC_CHIP;
}

void consys_clock_fail_dump_mt6886(void)
{
#if defined(KERNEL_clk_buf_show_status_info)
	KERNEL_clk_buf_show_status_info();
#endif
}

unsigned long long consys_soc_timestamp_get_mt6886(void)
{
#define TICK_PER_MS	(13000)
	void __iomem *addr = NULL;
	u32 tick_h = 0, tick_l = 0, tmp_h = 0;
	u64 timestamp = 0;

	/* 0x1c01_1000	sys_timer@13M (VLPSYS)
	 * - 0x0008	CNTCV_L	32	System counter count value low
	 * - 0x000C	CNTCV_H	32	System counter count value high
	 */
	addr = ioremap(0x1c011000, 0x10);
	if (addr) {
		do {
			tick_h = CONSYS_REG_READ(addr + 0x000c);
			tick_l = CONSYS_REG_READ(addr + 0x0008);
			tmp_h = CONSYS_REG_READ(addr + 0x000c);
		} while (tick_h != tmp_h);
		iounmap(addr);
	} else {
		pr_info("[%s] remap fail", __func__);
		return 0;
	}

	timestamp = ((((u64)tick_h << 32) & 0xFFFFFFFF00000000)
		    | ((u64)tick_l & 0x00000000FFFFFFFF));
	do_div(timestamp, TICK_PER_MS);

	return timestamp;
}

int consys_conninfra_on_power_ctrl_mt6886(unsigned int enable)
{
	int ret = 0;

#if MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE
	ret = consys_platform_spm_conn_ctrl_mt6886(enable);
#else
	ret = consys_conninfra_on_power_ctrl_mt6886_gen(enable);
#endif /* MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE */

	return ret;
}

void consys_set_if_pinmux_mt6886(unsigned int enable)
{
#ifndef CFG_CONNINFRA_ON_CTP
	struct pinctrl_state *tcxo_pinctrl_set;
	struct pinctrl_state *tcxo_pinctrl_clr;
	int ret = -1;
#endif
	int clock_type = consys_co_clock_type_mt6886();

	if (enable) {
		consys_set_if_pinmux_mt6886_gen(1);
		/* if(TCXO mode)
		 *	Set GPIO135 pinmux for TCXO mode (Aux3)(CONN_TCXOENA_REQ)
		 */

		if (clock_type == CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO ||
			clock_type == CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO) {
	#if defined(CFG_CONNINFRA_ON_CTP)
			consys_set_gpio_tcxo_mode_mt6886_gen(1, 1);
	#else
			if (IS_ERR(g_conninfra_pinctrl_ptr))
				return;

			tcxo_pinctrl_set = pinctrl_lookup_state(g_conninfra_pinctrl_ptr,
								"conninfra_tcxo_set");
			if (IS_ERR(tcxo_pinctrl_set))
				return;

			ret = pinctrl_select_state(g_conninfra_pinctrl_ptr, tcxo_pinctrl_set);
			if (ret)
				pr_err("[%s] set TCXO mode error: %d\n", __func__, ret);
	#endif /* defined(CFG_CONNINFRA_ON_CTP) */
		}
	} else {
		consys_set_if_pinmux_mt6886_gen(0);

		if (clock_type == CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO ||
			clock_type == CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO) {
	#if defined(CFG_CONNINFRA_ON_CTP)
			consys_set_gpio_tcxo_mode_mt6886_gen(1, 0);
	#else
			if (IS_ERR(g_conninfra_pinctrl_ptr))
				return;

			tcxo_pinctrl_clr = pinctrl_lookup_state(g_conninfra_pinctrl_ptr,
								"conninfra_tcxo_clr");
			if (IS_ERR(tcxo_pinctrl_clr))
				return;

			ret = pinctrl_select_state(g_conninfra_pinctrl_ptr, tcxo_pinctrl_clr);
			if (ret)
				pr_err("[%s] clear TCXO mode error: %d\n", __func__, ret);
	#endif /* defined(CFG_CONNINFRA_ON_CTP) */
		}
	}

}

int consys_is_consys_reg_mt6886(unsigned int addr)
{
	if (addr >= 0x18000000 && addr < 0x19000000)
		return 1;
	return 0;
}

void consys_print_platform_debug_mt6886(void)
{
	void __iomem *addr = NULL;
	unsigned int val[4];

	/* gals dbg: 0x1020E804 */
	/* slpport: 0x1021515C */
	/* SI3: 0x10215160 */
	/* ASL8: 0x10215168 */
	addr = ioremap(0x1020E804, 0x4);
	if (!addr) {
		pr_notice("%s remap failed");
		return;
	}
	val[0] = CONSYS_REG_READ(addr);
	iounmap(addr);

	addr = ioremap(0x10215150, 0x20);
	if (!addr) {
		pr_notice("%s remap failed");
		return;
	}
	val[1] = CONSYS_REG_READ(addr + 0x0c);
	val[2] = CONSYS_REG_READ(addr + 0x10);
	val[3] = CONSYS_REG_READ(addr + 0x18);
	iounmap(addr);

	pr_info("%s 0x1020E804=0x%x,0x1021515C=0x%x,0x10215160=0x%x,0x10215168=0x%x",
		__func__, val[0], val[1], val[2], val[3]);
}

static const char *consys_base_addr_index_to_str[CONSYS_BASE_ADDR_MAX] = {
	"infracfg_ao",
	"GPIO",
	"IOCFG_RT",
	"conn_infra_rgu_on",
	"conn_infra_cfg_on",
	"conn_wt_slp_ctl_reg",
	"conn_infra_bus_cr_on",
	"conn_infra_cfg",
	"conn_infra_clkgen_top",
	"conn_von_bus_bcrm",
	"conn_dbg_ctl",
	"conn_infra_on_bus_bcrm",
	"conn_therm_ctl",
	"conn_afe_ctl",
	"conn_rf_spi_mst_reg",
	"conn_infra_bus_cr",
	"conn_infra_off_debug_ctrl_ao",
	"conn_infra_off_bus_bcrm",
	"conn_infra_sysram_sw_cr",
	"conn_host_csr_top",
	"conn_semaphore",
	"spm",
	"top_rgu",
	"ccif_wf2ap_swirq",
	"ccif_bgf2ap_swirq",
};

int consys_reg_init_mt6886(struct platform_device *pdev)
{
	int ret = -1;
	struct device_node *node = NULL;
	struct consys_reg_base_addr *base_addr = NULL;
	struct resource res;
	int flag, i = 0;

	node = pdev->dev.of_node;
	pr_info("[%s] node=[%p]\n", __func__, node);
	if (node) {
		for (i = 0; i < CONSYS_BASE_ADDR_MAX; i++) {
			base_addr = &conn_reg_mt6886.reg_base_addr[i];

			ret = of_address_to_resource(node, i, &res);
			if (ret) {
				pr_err("Get Reg Index(%d-%s) failed",
						i, consys_base_addr_index_to_str[i]);
				continue;
			}

			base_addr->phy_addr = res.start;
			base_addr->vir_addr =
				(unsigned long) of_iomap(node, i);
			of_get_address(node, i, &(base_addr->size), &flag);

			pr_info("Get Index(%d-%s) phy(0x%zx) baseAddr=(0x%zx) size=(0x%zx)",
				i, consys_base_addr_index_to_str[i], base_addr->phy_addr,
				base_addr->vir_addr, base_addr->size);
		}

	} else {
		pr_err("[%s] can't find CONSYS compatible node\n", __func__);
		return ret;
	}

	return 0;

}

int consys_reg_deinit_mt6886(void)
{
	int i = 0;

	for (i = 0; i < CONSYS_BASE_ADDR_MAX; i++) {
		if (conn_reg_mt6886.reg_base_addr[i].vir_addr) {
			pr_info("[%d] Unmap %s (0x%zx)",
				i, consys_base_addr_index_to_str[i],
				conn_reg_mt6886.reg_base_addr[i].vir_addr);
			iounmap((void __iomem *)conn_reg_mt6886.reg_base_addr[i].vir_addr);
			conn_reg_mt6886.reg_base_addr[i].vir_addr = 0;
		}
	}

	return 0;
}



