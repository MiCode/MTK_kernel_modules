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
#include "include/mt6989_consys_reg.h"
#include "include/mt6989_consys_reg_offset.h"
#include "include/mt6989_debug_gen.h"
#include "include/mt6989_pos.h"
#include "include/mt6989_pos_gen.h"

#define CONSYS_DUMP_BUF_SIZE 800

struct consys_base_addr g_conn_reg_mt6989;
static struct conn_debug_info_mt6989 static_debug_info;
static struct conn_debug_info_mt6989 *g_debug_info_ptr;
static struct conn_debug_info_mt6989 static_debug_top_power_info;
static struct conn_debug_info_mt6989 *g_debug_top_power_info_ptr;
static char static_debug_buf[CONSYS_DUMP_BUF_SIZE];
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
		struct conn_debug_info_mt6989 *info)
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

int consys_reg_init_mt6989(struct platform_device *pdev)
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
			base_addr = &g_conn_reg_mt6989.reg_base_addr[i];

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

int consys_reg_deinit_mt6989(void)
{
	int i = 0;

	for (i = 0; i < CONSYS_BASE_ADDR_MAX; i++) {
		if (g_conn_reg_mt6989.reg_base_addr[i].vir_addr) {
			pr_info("[%d] Unmap %s (0x%zx)",
				i, consys_base_addr_index_to_str[i],
				g_conn_reg_mt6989.reg_base_addr[i].vir_addr);
			iounmap((void __iomem*)g_conn_reg_mt6989.reg_base_addr[i].vir_addr);
			g_conn_reg_mt6989.reg_base_addr[i].vir_addr = 0;
		}
	}

	return 0;
}

int consys_check_ap2conn_infra_on_mt6989(void)
{
	unsigned int tx, rx;

	/* Check ap2conn slpprot
	 * 0x1002_C1CC[0] / 0x1002_C00C[25](rx/tx) (sleep protect enable ready)
	 * both of them should be 1'b0
	 */
	/* connsys_protect_rdy */
	rx = CONSYS_REG_READ(INFRABUS_AO_REG_BASE_ADDR + 0x1cc);
	/* infrasys_protect_rdy */
	tx = CONSYS_REG_READ(INFRABUS_AO_REG_BASE_ADDR + 0xc);

	if (rx & (0x1 << 0))
		return CONNINFRA_AP2CONN_RX_SLP_PROT_ERR;
	if (tx & (0x1 << 25))
		return CONNINFRA_AP2CONN_TX_SLP_PROT_ERR;

	return 0;
}

static int __consys_reg_clock_detect(void)
{
	unsigned int r = 0;
	unsigned int count = 0;

	/* Check conn_infra off bus clock
	 * write 0x1 to 0x1802_3000[0], reset clock detect
	 * 0x1802_3000[1] conn_infra off bus clock (should be 1'b1 if clock exist)
	 * 0x1802_3000[2] osc clock (should be 1'b1 if clock exist)
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
		pr_notice("%s fail:0x1802_3000 = %x\n", __func__, r);
		return 0;
	}
	return 1;
}

int consys_check_ap2conn_infra_off_clock_mt6989(void)
{

	/* 1.Check "AP2CONN_INFRA ON step is ok"
	 * 2. Check conn_infra off bus clock
	 * (Need to polling 4 times to confirm
	 * the correctness and polling every 1ms)
	 * write 0x1 to 0x1802_3000[0], reset clock detect
	 * 0x1802_3000[1] conn_infra off bus clock (should be 1'b1 if clock exist)
	 * 0x1802_3000[2] osc clock (should be 1'b1 if clock exist)
	 * 3. Read conn_infra IP version
	 * Read 0x1801_1000 = 0x02050500
	 */
	if (!__consys_reg_clock_detect())
		return CONNINFRA_AP2CONN_CLK_ERR;
	if (CONSYS_REG_READ(CONN_CFG_IP_VERSION_ADDR) != CONN_HW_VER)
		return CONNINFRA_AP2CONN_CLK_ERR;
	return 0;
}

int consys_check_ap2conn_infra_off_irq_mt6989(void)
{
	/* 4. Check conn_infra off domain bus hang irq status
	 * 0x1802_3400[2:0]
	 * should be 3'b000, or means conn_infra off bus might hang
	 */
	int ret = -1;

	ret = CONSYS_REG_READ_BIT(CONN_DBG_CTL_CONN_INFRA_BUS_TIMEOUT_IRQ_ADDR,
			(0x1 << 0) | (0x1 << 1) | (0x1 << 2));

	return (ret == 0) ? 0 : CONNINFRA_INFRA_BUS_HANG_IRQ;
}

int consys_check_reg_readable_mt6989(void)
{
	if (consys_check_ap2conn_infra_on_mt6989())
		return 0;
	if (consys_check_ap2conn_infra_off_clock_mt6989())
		return 0;
	if (consys_check_ap2conn_infra_off_irq_mt6989())
		return 0;
	return 1;
}

int consys_check_reg_readable_for_coredump_mt6989(void)
{
	if (consys_check_ap2conn_infra_on_mt6989())
		return 0;
	return 1;
}

int consys_is_consys_reg_mt6989(unsigned int addr)
{
	if (addr >= 0x18000000 && addr < 0x19000000)
		return 1;
	return 0;
}

int consys_is_bus_hang_mt6989(void)
{
	int ret = 0;
	int is_clock_fail;

	if (g_debug_info_ptr == NULL) {
		pr_notice("[%s] debug_info is NULL\n", __func__);
		return -1;
	}

	pr_notice("[CONN_BUS] version=%s\n", MT6989_CONN_INFRA_BUS_DUMP_VERSION);
	consys_print_power_debug_dbg_level_0_mt6989_debug_gen(
		CONNINFRA_BUS_LOG_LEVEL_HOST_ONLY, g_debug_info_ptr);
	consys_print_log("[CONN_POWER_A]", g_debug_info_ptr);

	ret = consys_check_ap2conn_infra_on_mt6989();
	if (ret) {
		pr_notice("[%s] ap2conn_infra_on fail", __func__);
		return ret;
	}

	consys_print_bus_slpprot_debug_dbg_level_0_mt6989_debug_gen(
		CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON, g_debug_info_ptr);
		consys_print_log("[slpprot_a]", g_debug_info_ptr);
	consys_print_power_debug_dbg_level_1_mt6989_debug_gen(
		CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON, g_debug_info_ptr);
	consys_print_log("[CONN_POWER_B]", g_debug_info_ptr);
	consys_print_bus_debug_dbg_level_1_mt6989_debug_gen(
		CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON, g_debug_info_ptr);
	consys_print_log("[CONN_BUS_B]", g_debug_info_ptr);

	is_clock_fail = consys_check_ap2conn_infra_off_clock_mt6989();
	if (is_clock_fail)
		pr_notice("[%s] ap2conn_infra_off clock fail", __func__);

	ret = consys_check_ap2conn_infra_off_irq_mt6989();
	if (ret)
		pr_notice("[%s] ap2conn_infra_off timeout irq fail", __func__);

	if (!is_clock_fail) {
		consys_print_power_debug_dbg_level_2_mt6989_debug_gen(
			CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF, g_debug_info_ptr);
		consys_print_log("[CONN_POWER_C]", g_debug_info_ptr);

		consys_print_bus_debug_dbg_level_2_mt6989_debug_gen(
			CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF, g_debug_info_ptr);
		consys_print_log("[CONN_BUS_C]", g_debug_info_ptr);

		consys_print_bus_slpprot_debug_dbg_level_2_mt6989_debug_gen(
			CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF, g_debug_info_ptr);
		consys_print_log("[slpprot_c]", g_debug_info_ptr);

		consys_print_bus_slpprot_debug_dbg_level_0_mt6989_debug_gen(
			CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF, g_debug_info_ptr);
		consys_print_log("[slpprot_a]", g_debug_info_ptr);
	}

	return ret;
}

int consys_debug_top_power_status_mt6989(void)
{
	int ret = 0;

	if (g_debug_top_power_info_ptr == NULL) {
		pr_notice("[%s] debug_top_power_info is NULL\n", __func__);
		return -1;
	}

	consys_print_top_power_debug_dbg_level_0_mt6989_debug_gen(
			g_debug_top_power_info_ptr);
	consys_print_log("[CONN_SUBSYS_POWER]", g_debug_top_power_info_ptr);

	consys_print_top_power_debug_dbg_level_1_mt6989_debug_gen(
			g_debug_top_power_info_ptr);
	consys_print_log("[CONN_TOP_POWER_A]", g_debug_top_power_info_ptr);

	consys_print_top_power_debug_dbg_level_2_mt6989_debug_gen(
			g_debug_top_power_info_ptr);
	consys_print_log("[CONN_TOP_POWER_B]", g_debug_top_power_info_ptr);

	return ret;
}

void consys_debug_init_mt6989(void)
{
	g_debug_info_ptr = &static_debug_info;
	if (g_debug_info_ptr == NULL) {
		pr_notice("[%s] debug_info malloc failed\n", __func__);
		return;
	}

	g_debug_buf_ptr = static_debug_buf;
	if (g_debug_buf_ptr == NULL) {
		pr_notice("[%s] debug_buf malloc failed\n", __func__);
		return;
	}

	g_debug_top_power_info_ptr = &static_debug_top_power_info;
	if (g_debug_top_power_info_ptr == NULL) {
		pr_notice("[%s] debug_top_power_info malloc failed\n", __func__);
		return;
	}

	consys_debug_init_mt6989_debug_gen();
}

void consys_debug_deinit_mt6989(void)
{
	if (g_debug_info_ptr != NULL)
		g_debug_info_ptr = NULL;

	if (g_debug_buf_ptr != NULL)
		g_debug_buf_ptr = NULL;

	if (g_debug_top_power_info_ptr != NULL)
		g_debug_top_power_info_ptr = NULL;

	consys_debug_deinit_mt6989_debug_gen();
}
