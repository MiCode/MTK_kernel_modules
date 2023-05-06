// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/memblock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "consys_reg_mng.h"
#include "consys_reg_util.h"
#include "mt6895_consys_reg.h"
#include "mt6895_consys_reg_offset.h"
#include "mt6895_pos.h"
#include "mt6895_pos_gen.h"
#include "mt6895_debug_gen.h"
#include "osal.h"
#include "mt6895_pmic.h"

#define CONSYS_DUMP_BUF_SIZE 800

static int consys_reg_init(struct platform_device *pdev);
static int consys_reg_deinit(void);
static int consys_check_reg_readable(void);
static int consys_check_reg_readable_for_coredump(void);
static int __consys_check_reg_readable(int check_type);
static int consys_is_consys_reg(unsigned int addr);
static int consys_is_bus_hang(void);

struct consys_base_addr conn_reg_mt6895;

struct consys_reg_mng_ops g_dev_consys_reg_ops_mt6895 = {
	.consys_reg_mng_init = consys_reg_init,
	.consys_reg_mng_deinit = consys_reg_deinit,
	.consys_reg_mng_check_reable = consys_check_reg_readable,
	.consys_reg_mng_check_reable_for_coredump = consys_check_reg_readable_for_coredump,
	.consys_reg_mng_is_bus_hang = consys_is_bus_hang,
	.consys_reg_mng_is_consys_reg = consys_is_consys_reg,
};

static struct conn_debug_info_mt6895 *debug_info;
static char *debug_buf;

static const char* consys_base_addr_index_to_str[CONSYS_BASE_ADDR_MAX] = {
	"infracfg_ao",
	"GPIO",
	"IOCFG_RT",
	"IOCFG_RTT",
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
};

int consys_is_consys_reg(unsigned int addr)
{
	if (addr >= 0x18000000 && addr < 0x19000000)
		return 1;
	return 0;
}

static void consys_print_log(const char *title, struct conn_debug_info_mt6895 *info)
{
	char temp[13];
	int i;

	if (debug_buf == NULL)
		return;

	temp[0] = '\0';
	if (snprintf(debug_buf, CONSYS_DUMP_BUF_SIZE, "%s", title) < 0) {
		pr_notice("%s snprintf failed\n", __func__);
		return;
	}

	for (i = 0; i < info->length; i++) {
		if (snprintf(temp, sizeof(temp), "[0x%08x]", info->rd_data[i]) < 0) {
			pr_notice("%s snprintf failed\n", __func__);
			return;
		}
		if (strlen(debug_buf) + strlen(temp) < CONSYS_DUMP_BUF_SIZE)
			strncat(debug_buf, temp, strlen(temp) + 1);
		else
			pr_notice("%s debug_buf len is not enough\n", __func__);
	}
	pr_info("%s\n",debug_buf);
}

static void consys_print_power_debug(int level)
{
	pr_info("%s\n", __func__);

	if (level >= 0) {
		consys_print_power_debug_dbg_level_0_mt6895_debug_gen(level, debug_info);
		consys_print_log("[CONN_POWER_A]", debug_info);
	}
	if (level >= 1) {
		consys_print_power_debug_dbg_level_1_mt6895_debug_gen(level, debug_info);
		consys_print_log("[CONN_POWER_B]", debug_info);
	}
	if (level >= 2) {
		consys_print_power_debug_dbg_level_2_mt6895_debug_gen(level, debug_info);
		consys_print_log("[CONN_POWER_C]", debug_info);
	}
}

static void consys_print_bus_debug(int level)
{
	pr_info("%s\n", __func__);

	consys_print_bus_slpprot_debug_dbg_level_0_mt6895_debug_gen(level, debug_info);
	consys_print_log("[slpprot_a]", debug_info);

	if (level >= 1) {
		consys_print_bus_debug_dbg_level_1_mt6895_debug_gen(level, debug_info);
		consys_print_log("[CONN_BUS_B]", debug_info);
	}
	if (level >= 2) {
		consys_print_bus_debug_dbg_level_2_mt6895_debug_gen(level, debug_info);
		consys_print_log("[CONN_BUS_C]", debug_info);
		consys_print_bus_slpprot_debug_dbg_level_2_mt6895_debug_gen(level, debug_info);
		consys_print_log("[slpprot_c]", debug_info);
	}
}

int consys_print_debug_mt6895(int level)
{
	if (level < 0 || level > 2) {
		pr_info("%s level[%d] unexpected value.");
		return 0;
	}

	if (debug_info == NULL) {
		pr_notice("%s debug_info is NULL\n", __func__);
		return -1;
	}

	consys_print_power_debug(level);
	consys_print_bus_debug(level);
	consys_pmic_debug_log_mt6895();

	return 0;
}

static inline unsigned int __consys_bus_hang_clock_detect(void)
{
	unsigned int r = 0;

	unsigned int count = 0;

	/* Check conn_infra off bus clock */
	/* - write 0x1 to 0x1802_3000[0], reset clock detect */
	/* - 0x1802_3000[1]  conn_infra off bus clock (should be 1'b1 if clock exist) */
	/* - 0x1802_3000[2]  osc clock (should be 1'b1 if clock exist) */
	while (count < 4) {
		CONSYS_SET_BIT(CONN_DBG_CTL_CLOCK_DETECT_ADDR, (0x1 << 0));
		udelay(20);
		r = CONSYS_REG_READ_BIT(CONN_DBG_CTL_CLOCK_DETECT_ADDR, ((0x1 << 2) | (0x1 << 1)));
		if (r == 0x6)
			break;
		udelay(1000);
		count ++;
	}

	if (r != 0x6)
		pr_info("%s fail:0x1802_3000 = %x\n", __func__, r);
	return r;
}

static int consys_is_bus_hang(void)
{
	if (__consys_check_reg_readable(1) > 0)
		return 0;
	return 1;
}

static int consys_check_conninfra_on_domain(void)
{
	unsigned int r1, r2;

	/* AP2CONN_INFRA ON */
	/* 1. Check ap2conn gals sleep protect status */
	/*     - 0x1000_1C9C[0] / 0x1000_1C5C[12]   (rx/tx) */
	r1 = CONSYS_REG_READ_BIT(INFRACFG_AO_REG_BASE +
		CONSYS_GEN_MCU_CONNSYS_PROTECT_RDY_STA_0_OFFSET_ADDR, (0x1 << 0));
	r2 = CONSYS_REG_READ_BIT(INFRACFG_AO_REG_BASE +
		CONSYS_GEN_INFRASYS_PROTECT_RDY_STA_1_OFFSET_ADDR, (0x1 << 12));
	if (r1 || r2) {
		pr_info("%s 0x1000_1C9C[0] = %x, 0x1000_1C5C[12] = %x\n", __func__, r1, r2);
		return 0;
	}
	return 1;
}

int consys_check_conninfra_on_domain_mt6895(void)
{
	return consys_check_conninfra_on_domain();
}

static int consys_check_conninfra_off_domain(void)
{
	unsigned int r;

	r = __consys_bus_hang_clock_detect();
	if (r != 0x6)
		return 0;

	r = CONSYS_REG_READ(CONN_CFG_IP_VERSION_ADDR);
	if (r != CONN_HW_VER)
		return 0;

	return 1;
}

static int __consys_check_reg_readable(int check_type)
{
	// check_type includes:
	// 0: error
	// 1: print if no err
	// 2: coredump (can ignore bus timeout irq status)
	unsigned int r;
	int wakeup_conninfra = 0;
	int ret = 1;

	if (consys_check_conninfra_on_domain() == 0) {
		consys_print_debug_mt6895(0);
		return 0;
	}

	if (consys_check_conninfra_off_domain() == 0) {
		pr_info("%s: check conninfra off failed\n", __func__);
		consys_print_debug_mt6895(1);
		if (check_type == 0 || check_type == 2)
			return 0;

		/* wake up conninfra to read off register */
		if (consys_hw_force_conninfra_wakeup() != 0)
			return 0;

		wakeup_conninfra = 1;
		ret = 0;
	}

	/* Check conn_infra off domain bus hang irq status */
	/* - 0x1802_3400[2:0], should be 3'b000, or means conn_infra off bus might hang */
	r = CONSYS_REG_READ_BIT(CONN_DBG_CTL_CONN_INFRA_BUS_TIMEOUT_IRQ_ADDR,
			(0x1 << 0) | (0x1 << 1) | (0x1 << 2));
	if (r != 0) {
		pr_info("%s bus timeout 0x1802_3400[2:0] = 0x%x\n", __func__, r);
		consys_print_debug_mt6895(2);
		if (check_type == 2)
			return ret;

		ret = 0;
	} else if (check_type == 1)
		consys_print_debug_mt6895(2);

	if (wakeup_conninfra)
		consys_hw_force_conninfra_sleep();

	return ret;
}

static void consys_debug_init_mt6895(void)
{
	debug_info = (struct conn_debug_info_mt6895 *)osal_malloc(sizeof(struct conn_debug_info_mt6895));
	if (debug_info == NULL) {
		pr_notice("%s debug_info malloc failed\n", __func__);
		return;
	}

	debug_buf = osal_malloc(CONSYS_DUMP_BUF_SIZE);
	if (debug_buf == NULL) {
		pr_notice("%s debug_buf malloc failed\n", __func__);
		return;
	}

	consys_debug_init_mt6895_debug_gen();
}

static void consys_debug_deinit_mt6895(void)
{
	if (debug_info != NULL) {
		osal_free(debug_info);
		debug_info = NULL;
	}

	if (debug_buf != NULL) {
		osal_free(debug_buf);
		debug_buf = NULL;
	}

	consys_debug_deinit_mt6895_debug_gen();
}

static int consys_check_reg_readable(void)
{
	return __consys_check_reg_readable(0);
}

static int consys_check_reg_readable_for_coredump(void)
{
	return __consys_check_reg_readable(2);
}

int consys_reg_init(struct platform_device *pdev)
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
			base_addr = &conn_reg_mt6895.reg_base_addr[i];

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
		pr_notice("[%s] can't find CONSYS compatible node\n", __func__);
		return ret;
	}

	consys_debug_init_mt6895();

	return 0;

}

static int consys_reg_deinit(void)
{
	int i = 0;

	for (i = 0; i < CONSYS_BASE_ADDR_MAX; i++) {
		if (conn_reg_mt6895.reg_base_addr[i].vir_addr) {
			pr_info("[%d] Unmap %s (0x%zx)",
				i, consys_base_addr_index_to_str[i],
				conn_reg_mt6895.reg_base_addr[i].vir_addr);
			iounmap((void __iomem*)conn_reg_mt6895.reg_base_addr[i].vir_addr);
			conn_reg_mt6895.reg_base_addr[i].vir_addr = 0;
		}
	}

	consys_debug_deinit_mt6895();

	return 0;
}


