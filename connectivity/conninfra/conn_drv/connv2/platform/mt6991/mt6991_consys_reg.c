// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/memblock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>

/* For bus hang error definition */
#include "conninfra.h"
#include "consys_reg_mng.h"
#include "consys_reg_util.h"
#include "osal.h"
#include "include/mt6991_consys_reg.h"
#include "include/mt6991_consys_reg_offset.h"
#include "include/mt6991_debug_gen.h"
#include "include/mt6991_pos.h"
#include "include/mt6991_pos_gen.h"

#define CONSYS_DUMP_BUF_SIZE 1000
struct consys_base_addr g_conn_reg_mt6991;
static struct conn_debug_info_mt6991 *g_debug_info_ptr;
static struct conn_debug_info_mt6991 *g_debug_top_power_info_ptr;
static char *g_debug_buf_ptr;

static const char* consys_base_addr_index_to_str[CONSYS_BASE_ADDR_MAX] = {
	"conn_infra_rgu_on",
	"conn_infra_cfg_on",
	"conn_wt_slp_ctl_reg",
	"conn_infra_bus_cr_on",
	"conn_infra_cfg",
	"conn_infra_clkgen_top",
	"conn_dbg_ctl",
	"conn_afe_ctl",
	"conn_rf_spi_mst_reg",
	"conn_infra_bus_cr",
	"conn_infra_off_debug_ctrl_ao",
	"conn_infra_sysram",
	"conn_host_csr_top",
	"conn_semaphore",
	"ifrbus_ao_reg",
	"spm",
	"SRCLKENRC",
};

static void consys_print_log(const char *title,
		struct conn_debug_info_mt6991 *info)
{
	char temp[13];
	int i;

	if (g_debug_buf_ptr == NULL)
		return;

	temp[0] = '\0';
	if (snprintf(g_debug_buf_ptr, CONSYS_DUMP_BUF_SIZE, "%s", title) < 0) {
		pr_notice("[%s] snprintf failed\n", __func__);
		return;
	}

	for (i = 0; i < info->length; i++) {
		if (snprintf(temp, sizeof(temp), "[0x%08x]", info->rd_data[i]) < 0) {
			pr_notice("[%s] snprintf failed\n", __func__);
			return;
		}

		if (strlen(g_debug_buf_ptr) + strlen(temp) < CONSYS_DUMP_BUF_SIZE)
			strnlcat(g_debug_buf_ptr, temp, strlen(temp) + 1,
					CONSYS_DUMP_BUF_SIZE);
		else
			pr_notice("[%s] debug_buf len is not enough\n", __func__);
	}
	pr_notice("%s\n", g_debug_buf_ptr);
}

int consys_reg_init_mt6991(struct platform_device *pdev)
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
			base_addr = &g_conn_reg_mt6991.reg_base_addr[i];

			ret = of_address_to_resource(node, i, &res);
			if (ret) {
				pr_notice("Get Reg Index(%d-%s) failed",
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

int consys_reg_deinit_mt6991(void)
{
	int i = 0;

	for (i = 0; i < CONSYS_BASE_ADDR_MAX; i++) {
		if (g_conn_reg_mt6991.reg_base_addr[i].vir_addr) {
			pr_info("[%d] Unmap %s (0x%zx)",
				i, consys_base_addr_index_to_str[i],
				g_conn_reg_mt6991.reg_base_addr[i].vir_addr);
			iounmap((void __iomem*)g_conn_reg_mt6991.reg_base_addr[i].vir_addr);
			g_conn_reg_mt6991.reg_base_addr[i].vir_addr = 0;
		}
	}

	return 0;
}

int consys_check_ap2conn_infra_on_mt6991(void)
{
	unsigned int val;

	/* Check ap2conn slpprot
	 * 0x1002_C00C[24] (sleep protect enable ready) should be 1'b0
	 */
	val = CONSYS_REG_READ(INFRABUS_AO_REG_BASE_ADDR + 0xc);

	if (val & (0x1 << 24)) {
		pr_info("%s slp_prot is enabled. val = 0x%x\n", __func__, val);
		return CONNINFRA_AP2CONN_TX_SLP_PROT_ERR;
	}
	return 0;
}

static int __consys_reg_clock_detect(void)
{
	unsigned int r = 0;
	unsigned int count = 0;

	/* Check conn_infra off bus clock
	 * write 0x1 to 0x4002_3000[0], reset clock detect
	 * 0x4002_3000[1] conn_infra off bus clock (should be 1'b1 if clock exist)
	 * 0x4002_3000[2] osc clock (should be 1'b1 if clock exist)
	 */
	while (count < 4) {
		CONSYS_SET_BIT(CONN_DBG_CTL_CLOCK_DETECT_ADDR, (0x1 << 0));
		udelay(20);
		r = CONSYS_REG_READ_BIT(CONN_DBG_CTL_CLOCK_DETECT_ADDR, ((0x1 << 2) | (0x1 << 1)));
		if (r == 0x6)
			break;
		udelay(1000);
		count++;
	}

	if (r != 0x6) {
		pr_notice("%s fail:0x4002_3000 = %x\n", __func__, r);
		return 0;
	}
	return 1;
}

int consys_check_ap2conn_infra_off_clock_mt6991(void)
{

	/* 1.Check "AP2CONN_INFRA ON step is ok"
	 * 2. Check conn_infra off bus clock
	 * (Need to polling 4 times to confirm
	 * the correctness and polling every 1ms)
	 * write 0x1 to 0x4002_3000[0], reset clock detect
	 * 0x4002_3000[1] conn_infra off bus clock (should be 1'b1 if clock exist)
	 * 0x4002_3000[2] osc clock (should be 1'b1 if clock exist)
	 * 3. Read conn_infra IP version
	 * Read 0x4001_1000 = 0x02050601
	 */
	if (!__consys_reg_clock_detect())
		return CONNINFRA_AP2CONN_CLK_ERR;
	if (CONSYS_REG_READ(CONN_CFG_IP_VERSION_ADDR) != CONN_HW_VER)
		return CONNINFRA_AP2CONN_CLK_ERR;
	return 0;
}

int consys_check_ap2conn_infra_off_irq_mt6991(void)
{
	/* 4. Check conn_infra off domain bus hang irq status
	 * 0x4002_3400[2:0]
	 * should be 3'b000, or means conn_infra off bus might hang
	 */
	int ret = -1;

	ret = CONSYS_REG_READ_BIT(CONN_DBG_CTL_CONN_INFRA_BUS_TIMEOUT_IRQ_ADDR,
			(0x1 << 0) | (0x1 << 1) | (0x1 << 2));

	return (ret == 0) ? 0 : CONNINFRA_INFRA_BUS_HANG_IRQ;
}

int consys_check_reg_readable_mt6991(void)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	return 1;
#endif
	if (consys_check_ap2conn_infra_on_mt6991())
		return 0;
	if (consys_check_ap2conn_infra_off_clock_mt6991())
		return 0;
	if (consys_check_ap2conn_infra_off_irq_mt6991())
		return 0;
	return 1;
}

int consys_check_reg_readable_for_coredump_mt6991(void)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	if (consys_check_ap2conn_infra_on_mt6991())
		return 0;
#endif
	return 1;
}

int consys_is_consys_reg_mt6991(unsigned int addr)
{
	if (addr >= 0x40000000 && addr < 0x41000000)
		return 1;
	return 0;
}

int consys_is_bus_hang_mt6991(void)
{
	int ret = 0;
	int is_clock_fail;

	if (g_debug_info_ptr == NULL) {
		pr_notice("[%s] debug_info is NULL\n", __func__);
		return -1;
	}

#ifdef CONFIG_FPGA_EARLY_PORTING
	return 0;
#endif
	pr_notice("[CONN_BUS] version=%s\n", MT6991_CONN_INFRA_BUS_DUMP_VERSION);
	consys_print_power_debug_dbg_level_0_mt6991_debug_gen(
		CONNINFRA_BUS_LOG_LEVEL_HOST_ONLY, g_debug_info_ptr);
	consys_print_log("[CONN_POWER_A]", g_debug_info_ptr);

	ret = consys_check_ap2conn_infra_on_mt6991();
	if (ret) {
		pr_notice("[%s] ap2conn_infra_on fail", __func__);
		return ret;
	}

	consys_print_bus_slpprot_debug_dbg_level_0_mt6991_debug_gen(
		CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON, g_debug_info_ptr);
		consys_print_log("[slpprot_a]", g_debug_info_ptr);
	consys_print_power_debug_dbg_level_1_mt6991_debug_gen(
		CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON, g_debug_info_ptr);
	consys_print_log("[CONN_POWER_B]", g_debug_info_ptr);
	consys_print_bus_debug_dbg_level_1_mt6991_debug_gen(
		CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON, g_debug_info_ptr);
	consys_print_log("[CONN_BUS_B]", g_debug_info_ptr);

	is_clock_fail = consys_check_ap2conn_infra_off_clock_mt6991();
	if (is_clock_fail)
		pr_notice("[%s] ap2conn_infra_off clock fail", __func__);

	ret = consys_check_ap2conn_infra_off_irq_mt6991();
	if (ret)
		pr_notice("[%s] ap2conn_infra_off timeout irq fail", __func__);

	if (!is_clock_fail) {
		consys_print_power_debug_dbg_level_2_mt6991_debug_gen(
			CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF, g_debug_info_ptr);
		consys_print_log("[CONN_POWER_C]", g_debug_info_ptr);

		consys_print_bus_debug_dbg_level_2_mt6991_debug_gen(
			CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF, g_debug_info_ptr);
		consys_print_log("[CONN_BUS_C]", g_debug_info_ptr);

		consys_print_bus_slpprot_debug_dbg_level_2_mt6991_debug_gen(
			CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF, g_debug_info_ptr);
		consys_print_log("[slpprot_c]", g_debug_info_ptr);

		consys_print_bus_slpprot_debug_dbg_level_0_mt6991_debug_gen(
			CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF, g_debug_info_ptr);
		consys_print_log("[slpprot_a]", g_debug_info_ptr);

	#if 0 /* since pbus is not enable for this project, skip dumping related log. */
		consys_print_pbus_debug_dbg_level_2_mt6991_debug_gen(g_debug_info_ptr);
		consys_print_log("[pbus]", g_debug_info_ptr);
	#endif
	}

	return ret;
}

int consys_debug_top_power_status_mt6991(void)
{
	int ret = 0;

	if (g_debug_top_power_info_ptr == NULL) {
		pr_notice("[%s] debug_top_power_info is NULL\n", __func__);
		return -1;
	}

	consys_print_top_power_debug_dbg_level_0_mt6991_debug_gen(
			g_debug_top_power_info_ptr);
	consys_print_log("[CONN_SUBSYS_POWER]", g_debug_top_power_info_ptr);

	consys_print_top_power_debug_dbg_level_1_mt6991_debug_gen(
			g_debug_top_power_info_ptr);
	consys_print_log("[CONN_TOP_POWER_A]", g_debug_top_power_info_ptr);

	consys_print_top_power_debug_dbg_level_2_mt6991_debug_gen(
			g_debug_top_power_info_ptr);
	consys_print_log("[CONN_TOP_POWER_B]", g_debug_top_power_info_ptr);

	return ret;
}

void consys_debug_init_mt6991(void)
{
	g_debug_info_ptr =
		(struct conn_debug_info_mt6991 *)osal_malloc(sizeof(struct conn_debug_info_mt6991));
	if (g_debug_info_ptr == NULL) {
		pr_notice("[%s] debug_info malloc failed\n", __func__);
		return;
	}

	g_debug_buf_ptr = osal_malloc(CONSYS_DUMP_BUF_SIZE);
	if (g_debug_buf_ptr == NULL) {
		pr_notice("[%s] debug_buf malloc failed\n", __func__);
		return;
	}

	g_debug_top_power_info_ptr =
		(struct conn_debug_info_mt6991 *)osal_malloc(sizeof(struct conn_debug_info_mt6991));
	if (g_debug_top_power_info_ptr == NULL) {
		pr_notice("[%s] debug_top_power_info malloc failed\n", __func__);
		return;
	}

	consys_debug_init_mt6991_debug_gen();
}

void consys_debug_deinit_mt6991(void)
{
	if (g_debug_info_ptr != NULL) {
		osal_free(g_debug_info_ptr);
		g_debug_info_ptr = NULL;
	}

	if (g_debug_buf_ptr != NULL) {
		osal_free(g_debug_buf_ptr);
		g_debug_buf_ptr = NULL;
	}

	if (g_debug_top_power_info_ptr != NULL) {
		osal_free(g_debug_top_power_info_ptr);
		g_debug_top_power_info_ptr = NULL;
	}
	consys_debug_deinit_mt6991_debug_gen();
}
