// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "../include/connsys_library.h"
#include "../include/consys_reg_mng.h"
#include "../include/consys_reg_util.h"
#include "../include/plat_def.h"
#include "../include/plat_library.h"

#include "include/mt6897_consys_reg_offset.h"
#include "include/mt6897_debug_gen.h"
#include "include/mt6897_pos.h"
#include "include/mt6897_pos_gen.h"

#define CONSYS_DUMP_BUF_SIZE 800

struct consys_base_addr conn_reg_mt6897;

static struct conn_debug_info_mt6897 static_debug_info;
static struct conn_debug_info_mt6897 *debug_info;
static char static_debug_buf[CONSYS_DUMP_BUF_SIZE];
static char *debug_buf;

static void consys_print_log(const char *title, struct conn_debug_info_mt6897 *info)
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
			strnlcat(debug_buf, temp, strlen(temp) + 1, CONSYS_DUMP_BUF_SIZE);
		else
			pr_notice("%s debug_buf len is not enough\n", __func__);
	}
	pr_info("%s\n", debug_buf);
}

static void consys_print_power_debug(enum conninfra_bus_error_type level)
{
	pr_info("%s\n", __func__);

	if (level >= CONNINFRA_BUS_LOG_LEVEL_HOST_ONLY) {
		consys_print_power_debug_dbg_level_0_mt6897_debug_gen(level, debug_info);
		consys_print_log("[CONN_POWER_A]", debug_info);
	}
	if (level >= CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON) {
		consys_print_power_debug_dbg_level_1_mt6897_debug_gen(level, debug_info);
		consys_print_log("[CONN_POWER_B]", debug_info);
	}
	if (level >= CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF) {
		consys_print_power_debug_dbg_level_2_mt6897_debug_gen(level, debug_info);
		consys_print_log("[CONN_POWER_C]", debug_info);
	}
}

static void consys_print_bus_debug(enum conninfra_bus_error_type level)
{
	pr_info("%s\n", __func__);

	consys_print_bus_slpprot_debug_dbg_level_0_mt6897_debug_gen(level, debug_info);
	consys_print_log("[slpprot_a]", debug_info);

	if (level >= CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON) {
		consys_print_bus_debug_dbg_level_1_mt6897_debug_gen(level, debug_info);
		consys_print_log("[CONN_BUS_B]", debug_info);
	}
	if (level >= CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF) {
		consys_print_bus_debug_dbg_level_2_mt6897_debug_gen(level, debug_info);
		consys_print_log("[CONN_BUS_C]", debug_info);
		consys_print_bus_slpprot_debug_dbg_level_2_mt6897_debug_gen(level, debug_info);
		consys_print_log("[slpprot_c]", debug_info);
	}
}

int consys_print_debug_mt6897(enum conninfra_bus_error_type level)
{
	if (level > CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF) {
		pr_info("%s level[%d] unexpected value.", __func__, level);
		return 0;
	}

	if (debug_info == NULL) {
		pr_notice("%s debug_info is NULL\n", __func__);
		return 0;
	}
	consys_print_power_debug(level);
	consys_print_bus_debug(level);

	return 0;
}

unsigned int consys_bus_hang_clock_detect_mt6897(void)
{
	unsigned int r = 0;

	/* Check conn_infra off bus clock */
	/* - write 0x1 to 0x1802_3000[0], reset clock detect */
	/* - 0x1802_3000[1] conn_infra off bus clock (should be 1'b1 if clock exist) */
	/* - 0x1802_3000[2] osc clock (should be 1'b1 if clock exist) */
	CONSYS_SET_BIT(CONN_DBG_CTL_CLOCK_DETECT_ADDR, (0x1 << 0));
	udelay(20);
	r = CONSYS_REG_READ_BIT(CONN_DBG_CTL_CLOCK_DETECT_ADDR, ((0x1 << 2) | (0x1 << 1)));

	return r;
}

int consys_check_conninfra_on_domain_status_mt6897(void)
{
	unsigned int r1, r2;
	int ret = -1;

	/* AP2CONN_INFRA ON */
	/* 1. Check ap2conn gals sleep protect status */
	/* - 0x1002_C1CC[0] / 0x1002_C00C[25] (rx/tx) */
	/* (sleep protect enable ready) both of them should be 1'b0 */
	r1 = CONSYS_REG_READ_BIT(INFRACFG_AO_REG_BASE +
		CONSYS_GEN_SLEEP_PROT_RDY_14_OFFSET_ADDR, (0x1 << 0));
	r2 = CONSYS_REG_READ_BIT(INFRACFG_AO_REG_BASE +
		CONSYS_GEN_SLEEP_PROT_RDY_0_OFFSET_ADDR, (0x1 << 25));
	if (r1 || r2) {
		pr_info("%s 0x1000_1C9C[0] = %x, 0x1000_1C5C[12] = %x\n", __func__, r1, r2);
		if (r1)
			ret = CONNINFRA_AP2CONN_RX_SLP_PROT_ERR;
		if (r2)
			ret = CONNINFRA_AP2CONN_TX_SLP_PROT_ERR;
		return ret;
	}
	return 0;
}

int consys_check_conninfra_bus_clock_status_mt6897(void)
{
	unsigned int r;
	unsigned int count = 0;

	while (count < 4) {
		r = consys_bus_hang_clock_detect_mt6897();
		if (r == 0x6)
			break;

		udelay(1000);
		count++;
	}

	if (r != 0x6) {
		pr_info("%s fail:0x1802_3000 = %x\n", __func__, r);
		return CONNINFRA_AP2CONN_CLK_ERR;
	}

	return 0;
}

int consys_check_conninfra_off_domain_status_mt6897(void)
{
	unsigned int r;

	r = CONSYS_REG_READ(CONN_CFG_IP_VERSION_ADDR);
	if (r != CONN_HW_VER)
		return CONNINFRA_AP2CONN_CLK_ERR;

	return 0;
}

int consys_check_conninfra_irq_status_mt6897(void)
{
	int ret = -1;

	ret = CONSYS_REG_READ_BIT(CONN_DBG_CTL_CONN_INFRA_BUS_TIMEOUT_IRQ_ADDR,
			(0x1 << 0) | (0x1 << 1) | (0x1 << 2));

	return (ret == 0) ? 0 : CONNINFRA_INFRA_BUS_HANG_IRQ;
}

void consys_debug_init_mt6897(void)
{
	debug_info = &static_debug_info;
	if (debug_info == NULL) {
		pr_notice("%s debug_info malloc failed\n", __func__);
		return;
	}

	debug_buf = static_debug_buf;
	if (debug_buf == NULL) {
		pr_notice("%s debug_buf malloc failed\n", __func__);
		return;
	}

	consys_debug_init_mt6897_debug_gen();
}

void consys_debug_deinit_mt6897(void)
{
	if (debug_info != NULL)
		debug_info = NULL;

	if (debug_buf != NULL)
		debug_buf = NULL;

	consys_debug_deinit_mt6897_debug_gen();
}

