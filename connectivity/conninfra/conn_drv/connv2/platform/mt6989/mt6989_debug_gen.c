/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */


/* AUTO-GENERATED FILE.  DO NOT MODIFY.
 *
 * This file, mt6989_debug_gen.c was automatically generated
 * by the tool from the DEBUG data DE provided.
 * It should not be modified by hand.
 *
 * Reference debug file,
 * - [Lxxxy]connsys_power_debug.xlsx (Modified date: 2023-04-28)
 * - [Lxxxy]conn_infra_bus_debug_ctrl.xlsx (Modified date: 2023-05-19)
 */


#ifndef CFG_CONNINFRA_ON_CTP
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/io.h>
#endif
#include "consys_hw.h"
#include "consys_reg_util.h"
#include "mt6989_consys_reg_offset.h"
#include "mt6989_debug_gen.h"
#include "conninfra.h"


mapped_addr vir_addr_consys_dbg_gen_srclkenrc_base_mt6989;
mapped_addr vir_addr_consys_dbg_gen_topckgen_base_mt6989;
mapped_addr vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989;
mapped_addr vir_addr_consys_dbg_gen_conn_infra_sysram_offset_mt6989;
mapped_addr vir_addr_consys_dbg_gen_conn_infra_axi_layer_bus_bcrm_base_mt6989;

void consys_debug_init_mt6989_debug_gen(void)
{
	vir_addr_consys_dbg_gen_srclkenrc_base_mt6989
		= ioremap(CONSYS_DBG_GEN_SRCLKENRC_BASE_ADDR, 0x77C);
	vir_addr_consys_dbg_gen_topckgen_base_mt6989
		= ioremap(CONSYS_DBG_GEN_TOPCKGEN_BASE_ADDR, 0x40);
	vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989
		= ioremap(CONSYS_DBG_GEN_CONN_DBG_CTL_BASE_ADDR, 0x41c);
	vir_addr_consys_dbg_gen_conn_infra_sysram_offset_mt6989
		= ioremap(CONSYS_DBG_GEN_CONN_INFRA_SYSRAM_OFFSET_ADDR, 0xACC);
	vir_addr_consys_dbg_gen_conn_infra_axi_layer_bus_bcrm_base_mt6989
		= ioremap(CONSYS_DBG_GEN_CONN_INFRA_AXI_LAYER_BUS_BCRM_BASE_ADDR, 0x10);
}

void consys_debug_deinit_mt6989_debug_gen(void)
{
	if (vir_addr_consys_dbg_gen_srclkenrc_base_mt6989)
		iounmap(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989);

	if (vir_addr_consys_dbg_gen_topckgen_base_mt6989)
		iounmap(vir_addr_consys_dbg_gen_topckgen_base_mt6989);

	if (vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989)
		iounmap(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989);

	if (vir_addr_consys_dbg_gen_conn_infra_sysram_offset_mt6989)
		iounmap(vir_addr_consys_dbg_gen_conn_infra_sysram_offset_mt6989);

	if (vir_addr_consys_dbg_gen_conn_infra_axi_layer_bus_bcrm_base_mt6989)
		iounmap(vir_addr_consys_dbg_gen_conn_infra_axi_layer_bus_bcrm_base_mt6989);
}

void update_debug_read_info_mt6989_debug_gen(
		struct conn_debug_info_mt6989 *info,
		char *tag,
		unsigned int rd_addr,
		unsigned int rd_data)
{
	if (info == NULL)
		return;

	if (info->length >= CONN_DEBUG_INFO_SIZE)
		return;

	if (tag != NULL) {
		if (snprintf(&(info->tag[info->length][0]), DEBUG_TAG_SIZE, "%s", tag) < 0)
			pr_notice("%s snprintf fail", __func__);
	}

	info->rd_addr[info->length] = rd_addr;
	info->rd_data[info->length] = rd_data;
	info->length++;
}

void update_debug_write_info_mt6989_debug_gen(
		struct conn_debug_info_mt6989 *info,
		char *tag,
		unsigned int wr_addr,
		int wr_addr_lsb,
		int wr_addr_msb,
		unsigned int wr_data)
{
	int len = 0;

	if (info == NULL)
		return;

	if (info->wr_addr[info->length] != 0)
		info->length++;

	if (info->length >= CONN_DEBUG_INFO_SIZE)
		return;

	if (tag != NULL)
		len = snprintf(&(info->tag[info->length][0]), DEBUG_TAG_SIZE, "%s", tag);
	else
		len = snprintf(&(info->tag[info->length][0]), DEBUG_TAG_SIZE, "%s", "");

	if (len < 0)
		pr_notice("%s snprintf fail", __func__);

	info->wr_addr[info->length] = wr_addr;
	info->wr_addr_lsb[info->length] = wr_addr_lsb;
	info->wr_addr_msb[info->length] = wr_addr_msb;
	info->wr_data[info->length] = wr_data;
}

void consys_print_power_debug_dbg_level_0_mt6989_debug_gen(
		int level,
		struct conn_debug_info_mt6989 *pdbg_level_0_info)
{
	if (pdbg_level_0_info == NULL)
		return;

	memset(pdbg_level_0_info, 0, sizeof(struct conn_debug_info_mt6989));

	if (level < 0)
		return;

	if (!vir_addr_consys_dbg_gen_srclkenrc_base_mt6989) {
		pr_notice("vir_addr_consys_dbg_gen_srclkenrc_base_mt6989(%x) ioremap fail\n",
				CONSYS_DBG_GEN_SRCLKENRC_BASE_ADDR);
		return;
	}

	if (!vir_addr_consys_dbg_gen_topckgen_base_mt6989) {
		pr_notice("vir_addr_consys_dbg_gen_topckgen_base_mt6989(%x) ioremap fail\n",
				CONSYS_DBG_GEN_TOPCKGEN_BASE_ADDR);
		return;
	}

	if (SPM_BASE_ADDR == 0) {
		pr_notice("SPM_BASE_ADDR is not defined\n");
		return;
	}

	/* A0 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A0", 0x1C001000 + CONSYS_DBG_GEN_SPM_REQ_STA_4_OFFSET_ADDR,
		CONSYS_REG_READ(SPM_BASE_ADDR +
			CONSYS_DBG_GEN_SPM_REQ_STA_4_OFFSET_ADDR));

	/* A1 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A1", 0x1C00D000 + CONSYS_DBG_GEN_FSM_STA_0_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_FSM_STA_0_OFFSET_ADDR));

	/* A2 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A2", 0x1C00D000 + CONSYS_DBG_GEN_CMD_STA_0_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_CMD_STA_0_OFFSET_ADDR));

	/* A3 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A3", 0x1C00D000 + CONSYS_DBG_GEN_CMD_STA_1_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_CMD_STA_1_OFFSET_ADDR));

	/* A4 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A4", 0x1C00D000 + CONSYS_DBG_GEN_SPI_STA_0_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SPI_STA_0_OFFSET_ADDR));

	/* A5 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A5", 0x1C00D000 + CONSYS_DBG_GEN_PI_PO_STA_0_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_PI_PO_STA_0_OFFSET_ADDR));

	/* A6 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A6", 0x1C00D000 + CONSYS_DBG_GEN_PMRC_DEBUG_LATCH_CMD_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_PMRC_DEBUG_LATCH_CMD_OFFSET_ADDR));

	/* A7 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A7", 0x1C00D000 + CONSYS_DBG_GEN_PMIF_M_ERR_INFO_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_PMIF_M_ERR_INFO_OFFSET_ADDR));

	/* A8 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A8", 0x1C00D000 + CONSYS_DBG_GEN_PMIF_P_ERR_INFO_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_PMIF_P_ERR_INFO_OFFSET_ADDR));

	/* A9 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A9", 0x1C00D000 + CONSYS_DBG_GEN_PMIF_M_ERR_LASTONE_INFO_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_PMIF_M_ERR_LASTONE_INFO_OFFSET_ADDR));

	/* A10 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A10", 0x1C00D000 + CONSYS_DBG_GEN_PMIF_P_ERR_LASTONE_INFO_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_PMIF_P_ERR_LASTONE_INFO_OFFSET_ADDR));

	/* A11 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A11", 0x1C00D000 + CONSYS_DBG_GEN_SRCLKEN_SW_CONTRAL_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SRCLKEN_SW_CONTRAL_OFFSET_ADDR));

	/* A12 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A12", 0x1C00D000 + CONSYS_DBG_GEN_PMIC_STARTER_OPEN_CNT_ERR_CMD_LATCH_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_PMIC_STARTER_OPEN_CNT_ERR_CMD_LATCH_OFFSET_ADDR));

	/* A13 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A13", 0x1C00D000 + CONSYS_DBG_GEN_PMIC_STARTER_OPEN_CNT_ERR_INFO_LATCH_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_PMIC_STARTER_OPEN_CNT_ERR_INFO_LATCH_OFFSET_ADDR));

	/* A14 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A14", 0x1C00D000 + CONSYS_DBG_GEN_PMIC_STARTER_OFF_CNT_ERR_CMD_LATCH_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_PMIC_STARTER_OFF_CNT_ERR_CMD_LATCH_OFFSET_ADDR));

	/* A15 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A15", 0x1C00D000 + CONSYS_DBG_GEN_PMIC_STARTER_OFF_CNT_ERR_INFO_LATCH_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_PMIC_STARTER_OFF_CNT_ERR_INFO_LATCH_OFFSET_ADDR));

	/* A16 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A16", 0x1C00D000 + CONSYS_DBG_GEN_OPEN_CNT_ERR_SYS_TIMER_LATCH_LSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_OPEN_CNT_ERR_SYS_TIMER_LATCH_LSB_OFFSET_ADDR));

	/* A17 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A17", 0x1C00D000 + CONSYS_DBG_GEN_OPEN_CNT_ERR_SYS_TIMER_LATCH_MSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_OPEN_CNT_ERR_SYS_TIMER_LATCH_MSB_OFFSET_ADDR));

	/* A18 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A18", 0x1C00D000 + CONSYS_DBG_GEN_OFF_CNT_ERR_SYS_TIMER_LATCH_LSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_OFF_CNT_ERR_SYS_TIMER_LATCH_LSB_OFFSET_ADDR));

	/* A19 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A19", 0x1C00D000 + CONSYS_DBG_GEN_OFF_CNT_ERR_SYS_TIMER_LATCH_MSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_OFF_CNT_ERR_SYS_TIMER_LATCH_MSB_OFFSET_ADDR));

	/* A20 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A20", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_STA_0_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_STA_0_OFFSET_ADDR));

	/* A21 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A21", 0x1C00D000 + CONSYS_DBG_GEN_SPMI_P_STA_0_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SPMI_P_STA_0_OFFSET_ADDR));

	/* A22 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A22", 0x1C001000 + CONSYS_DBG_GEN_CONN_PWR_CON_OFFSET_ADDR,
		CONSYS_REG_READ(SPM_BASE_ADDR +
			CONSYS_DBG_GEN_CONN_PWR_CON_OFFSET_ADDR));

	/* A23 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A23", 0x10000000 + CONSYS_DBG_GEN_CLK_CFG_3_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_topckgen_base_mt6989 +
			CONSYS_DBG_GEN_CLK_CFG_3_OFFSET_ADDR));

	/* A24 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A24", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_0_0_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_0_0_OFFSET_ADDR));

	/* A25 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A25", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_0_1_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_0_1_OFFSET_ADDR));

	/* A26 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A26", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_0_2_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_0_2_OFFSET_ADDR));

	/* A27 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A27", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_0_3_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_0_3_OFFSET_ADDR));

	/* A28 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A28", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_1_0_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_1_0_OFFSET_ADDR));

	/* A29 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A29", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_1_1_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_1_1_OFFSET_ADDR));

	/* A30 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A30", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_1_2_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_1_2_OFFSET_ADDR));

	/* A31 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A31", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_1_3_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_1_3_OFFSET_ADDR));

	/* A32 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A32", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_2_0_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_2_0_OFFSET_ADDR));

	/* A33 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A33", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_2_1_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_2_1_OFFSET_ADDR));

	/* A34 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A34", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_2_2_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_2_2_OFFSET_ADDR));

	/* A35 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A35", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_2_3_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_2_3_OFFSET_ADDR));

	/* A36 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A36", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_3_0_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_3_0_OFFSET_ADDR));

	/* A37 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A37", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_3_1_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_3_1_OFFSET_ADDR));

	/* A38 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A38", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_3_2_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_3_2_OFFSET_ADDR));

	/* A39 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A39", 0x1C00D000 + CONSYS_DBG_GEN_DEBUG_TRACE_3_3_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_DEBUG_TRACE_3_3_OFFSET_ADDR));

	/* A40 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A40", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_0_LSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_0_LSB_OFFSET_ADDR));

	/* A41 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A41", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_0_MSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_0_MSB_OFFSET_ADDR));

	/* A42 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A42", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_1_LSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_1_LSB_OFFSET_ADDR));

	/* A43 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A43", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_1_MSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_1_MSB_OFFSET_ADDR));

	/* A44 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A44", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_2_LSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_2_LSB_OFFSET_ADDR));

	/* A45 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A45", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_2_MSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_2_MSB_OFFSET_ADDR));

	/* A46 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A46", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_3_LSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_3_LSB_OFFSET_ADDR));

	/* A47 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A47", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_3_MSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_3_MSB_OFFSET_ADDR));

	/* A48 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A48", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_4_LSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_4_LSB_OFFSET_ADDR));

	/* A49 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A49", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_4_MSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_4_MSB_OFFSET_ADDR));

	/* A50 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A50", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_5_LSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_5_LSB_OFFSET_ADDR));

	/* A51 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A51", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_5_MSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_5_MSB_OFFSET_ADDR));

	/* A52 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A52", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_6_LSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_6_LSB_OFFSET_ADDR));

	/* A53 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A53", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_6_MSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_6_MSB_OFFSET_ADDR));

	/* A54 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A54", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_7_LSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_7_LSB_OFFSET_ADDR));

	/* A55 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A55", 0x1C00D000 + CONSYS_DBG_GEN_SYS_TIMER_LATCH_7_MSB_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_srclkenrc_base_mt6989 +
			CONSYS_DBG_GEN_SYS_TIMER_LATCH_7_MSB_OFFSET_ADDR));
}

void consys_print_power_debug_dbg_level_1_mt6989_debug_gen(
		int level,
		struct conn_debug_info_mt6989 *pdbg_level_1_info)
{
	if (pdbg_level_1_info == NULL)
		return;

	memset(pdbg_level_1_info, 0, sizeof(struct conn_debug_info_mt6989));

	if (level < 1)
		return;

	if (!vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989) {
		pr_notice("vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989(%x) ioremap fail\n",
				CONSYS_DBG_GEN_CONN_DBG_CTL_BASE_ADDR);
		return;
	}

	if (CONN_HOST_CSR_TOP_BASE == 0) {
		pr_notice("CONN_HOST_CSR_TOP_BASE is not defined\n");
		return;
	}

	/* B0 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x0, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"B0", 0x18060000 + CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x0);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* B1 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x4, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"B1", 0x18060000 + CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x4);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* B2 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x5, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"B2", 0x18060000 + CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x5);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* B3 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x1, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"B3", 0x18060000 + CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x1);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* B4 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x2, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"B4", 0x18060000 + CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x2);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* B5 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x3, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"B5", 0x18060000 + CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x3);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* B6 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x6, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"B6", 0x18060000 + CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x6);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* B7 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x7, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"B7", 0x18060000 + CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x7);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* B8 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"B8", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_MONFLAG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_MONFLAG_OUT_OFFSET_ADDR));

	/* B9 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"B9", 0x18060000 + CONSYS_DBG_GEN_CONNSYS_PWR_STATES_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONNSYS_PWR_STATES_OFFSET_ADDR));
}

void consys_print_power_debug_dbg_level_2_mt6989_debug_gen(
		int level,
		struct conn_debug_info_mt6989 *pdbg_level_2_info)
{
	if (pdbg_level_2_info == NULL)
		return;

	memset(pdbg_level_2_info, 0, sizeof(struct conn_debug_info_mt6989));

	if (level < 2)
		return;

	if (!vir_addr_consys_dbg_gen_conn_infra_sysram_offset_mt6989) {
		pr_notice("vir_addr_consys_dbg_gen_conn_infra_sysram_offset_mt6989(%x) ioremap fail\n",
				CONSYS_DBG_GEN_CONN_INFRA_SYSRAM_OFFSET_ADDR);
		return;
	}

	if (CONN_CFG_BASE == 0) {
		pr_notice("CONN_CFG_BASE is not defined\n");
		return;
	}

	if (CONN_CLKGEN_TOP_BASE == 0) {
		pr_notice("CONN_CLKGEN_TOP_BASE is not defined\n");
		return;
	}

	if (CONN_CFG_ON_BASE == 0) {
		pr_notice("CONN_CFG_ON_BASE is not defined\n");
		return;
	}

	if (CONN_RGU_ON_BASE == 0) {
		pr_notice("CONN_RGU_ON_BASE is not defined\n");
		return;
	}

	if (CONN_WT_SLP_CTL_REG_BASE == 0) {
		pr_notice("CONN_WT_SLP_CTL_REG_BASE is not defined\n");
		return;
	}

	if (CONN_RF_SPI_MST_REG_BASE == 0) {
		pr_notice("CONN_RF_SPI_MST_REG_BASE is not defined\n");
		return;
	}

	/* C0 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C0", 0x18011000 + CONSYS_DBG_GEN_PLL_STATUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_BASE +
			CONSYS_DBG_GEN_PLL_STATUS_OFFSET_ADDR));

	/* C1 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C1", 0x18012000 + CONSYS_DBG_GEN_CKGEN_BUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CLKGEN_TOP_BASE +
			CONSYS_DBG_GEN_CKGEN_BUS_OFFSET_ADDR));

	/* C2 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C2", 0x18001000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_RC_STATUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_ON_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_RC_STATUS_OFFSET_ADDR));

	/* C3 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C3", 0x18000000 + CONSYS_DBG_GEN_BGFSYS_ON_TOP_PWR_ST_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_RGU_ON_BASE +
			CONSYS_DBG_GEN_BGFSYS_ON_TOP_PWR_ST_OFFSET_ADDR));

	/* C4 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C4", 0x18003000 + CONSYS_DBG_GEN_WB_CK_STA_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_BASE +
			CONSYS_DBG_GEN_WB_CK_STA_OFFSET_ADDR));

	/* C5 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C5", 0x18003000 + CONSYS_DBG_GEN_WB_SLP_TOP_CK_0_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_BASE +
			CONSYS_DBG_GEN_WB_SLP_TOP_CK_0_OFFSET_ADDR));

	/* C6 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C6", 0x18003000 + CONSYS_DBG_GEN_WB_SLP_TOP_CK_1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_BASE +
			CONSYS_DBG_GEN_WB_SLP_TOP_CK_1_OFFSET_ADDR));

	/* C7 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C7", 0x18003000 + CONSYS_DBG_GEN_WB_SLP_TOP_CK_2_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_BASE +
			CONSYS_DBG_GEN_WB_SLP_TOP_CK_2_OFFSET_ADDR));

	/* C8 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C8", 0x18003000 + CONSYS_DBG_GEN_WB_SLP_TOP_CK_3_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_BASE +
			CONSYS_DBG_GEN_WB_SLP_TOP_CK_3_OFFSET_ADDR));

	/* C9 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C9", 0x18003000 + CONSYS_DBG_GEN_WB_SLP_TOP_CK_4_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_BASE +
			CONSYS_DBG_GEN_WB_SLP_TOP_CK_4_OFFSET_ADDR));

	/* C10 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C10", 0x18003000 + CONSYS_DBG_GEN_WB_SLP_TOP_CK_5_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_BASE +
			CONSYS_DBG_GEN_WB_SLP_TOP_CK_5_OFFSET_ADDR));

	/* C11 */
	CONSYS_REG_WRITE_MASK(CONN_CFG_BASE +
		CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR, 0x0, 0x600000);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_2_info,
		"C11", 0x18011000 + CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR,
		21, 22, 0x0);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		NULL, 0x18011000 + CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_BASE +
			CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR));

	/* C12 */
	CONSYS_REG_WRITE_MASK(CONN_CFG_BASE +
		CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR, 0x200000, 0x600000);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_2_info,
		"C12", 0x18011000 + CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR,
		21, 22, 0x1);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		NULL, 0x18011000 + CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_BASE +
			CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR));

	/* C13 */
	CONSYS_REG_WRITE_MASK(CONN_CFG_BASE +
		CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR, 0x400000, 0x600000);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_2_info,
		"C13", 0x18011000 + CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR,
		21, 22, 0x2);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		NULL, 0x18011000 + CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_BASE +
			CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR));

	/* C14 */
	CONSYS_REG_WRITE_MASK(CONN_CFG_BASE +
		CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR, 0x600000, 0x600000);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_2_info,
		"C14", 0x18011000 + CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR,
		21, 22, 0x3);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		NULL, 0x18011000 + CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_BASE +
			CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR));

	/* C15 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C15", 0x18050000 + 0xAC4,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_infra_sysram_offset_mt6989 + 0xAC4));

	/* C16 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C16", 0x18050000 + 0xAC8,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_infra_sysram_offset_mt6989 + 0xAC8));

	/* C17 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C17", 0x18050000 + 0xACC,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_infra_sysram_offset_mt6989 + 0xACC));

	/* C22 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C22", 0x18042000 + CONSYS_DBG_GEN_SPI_STA_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_RF_SPI_MST_REG_BASE +
			CONSYS_DBG_GEN_SPI_STA_OFFSET_ADDR));

	/* C23 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C23", 0x18042000 + CONSYS_DBG_GEN_SPI_TOP_ADDR_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_RF_SPI_MST_REG_BASE +
			CONSYS_DBG_GEN_SPI_TOP_ADDR_OFFSET_ADDR));

	/* C24 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C24", 0x18042000 + CONSYS_DBG_GEN_SPI_TOP_WDAT_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_RF_SPI_MST_REG_BASE +
			CONSYS_DBG_GEN_SPI_TOP_WDAT_OFFSET_ADDR));

	/* C25 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C25", 0x18042000 + CONSYS_DBG_GEN_SPI_TOP_RDAT_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_RF_SPI_MST_REG_BASE +
			CONSYS_DBG_GEN_SPI_TOP_RDAT_OFFSET_ADDR));

	/* C26 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C26", 0x18042000 + CONSYS_DBG_GEN_SPI_HSCK_CTL_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_RF_SPI_MST_REG_BASE +
			CONSYS_DBG_GEN_SPI_HSCK_CTL_OFFSET_ADDR));

	/* C27 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"C27", 0x18042000 + CONSYS_DBG_GEN_SPI_CRTL_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_RF_SPI_MST_REG_BASE +
			CONSYS_DBG_GEN_SPI_CRTL_OFFSET_ADDR));
}

void consys_print_bus_debug_dbg_level_1_mt6989_debug_gen(
		int level,
		struct conn_debug_info_mt6989 *pdbg_level_1_info)
{
	if (pdbg_level_1_info == NULL)
		return;

	memset(pdbg_level_1_info, 0, sizeof(struct conn_debug_info_mt6989));

	if (level < 1)
		return;

	if (!vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989) {
		pr_notice("vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989(%x) ioremap fail\n",
				CONSYS_DBG_GEN_CONN_DBG_CTL_BASE_ADDR);
		return;
	}

	/* write 0x18023408[31:0] = 0x0 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x0);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x0);

	/* 1 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR, 0x10001);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"1", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR,
		0, 31, 0x10001);

	/* 1 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"1", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 2 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR, 0x20001);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"2", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR,
		0, 31, 0x20001);

	/* 2 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"2", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 3 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR, 0x10002);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"3", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR,
		0, 31, 0x10002);

	/* 3 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"3", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 4 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR, 0x20002);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"4", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR,
		0, 31, 0x20002);

	/* 4 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"4", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 5 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR, 0x30002);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"5", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_DEBUGSYS_CTRL_OFFSET_ADDR,
		0, 31, 0x30002);

	/* 5 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"5", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 6 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x1);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"6", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x1);

	/* 6 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"6", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 7 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x2);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"7", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x2);

	/* 7 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"7", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 8 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x3);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"8", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x3);

	/* 8 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"8", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 9 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x6);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"9", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x6);

	/* 9 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"9", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 10 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"10", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x7);

	/* 10 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"10", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 11 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x8);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"11", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x8);

	/* 11 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"11", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 12 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"12", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_BUS_TIMEOUT_IRQ_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_BUS_TIMEOUT_IRQ_OFFSET_ADDR));

	/* 13 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x9);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"13", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x9);

	/* 13 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"13", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 14 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0xA);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"14", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0xA);

	/* 14 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"14", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 15 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0xB);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"15", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0xB);

	/* 15 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"15", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 16 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0xC);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"16", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0xC);

	/* 16 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"16", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 17 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0xD);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"17", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0xD);

	/* 17 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"17", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 18 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0xE);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"18", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0xE);

	/* 18 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"18", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 19 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0xF);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"19", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0xF);

	/* 19 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"19", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 20 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x10);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"20", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x10);

	/* 20 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"20", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 21 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x11);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"21", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x11);

	/* 21 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"21", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 22 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x12);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"22", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x12);

	/* 22 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"22", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 23 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x13);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"23", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x13);

	/* 23 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"23", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 24 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x14);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"24", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x14);

	/* 24 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"24", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 25 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x15);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"25", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x15);

	/* 25 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"25", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 26 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x16);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"26", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x16);

	/* 26 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"26", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 27 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x17);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"27", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x17);

	/* 27 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"27", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 28 */
	CONSYS_REG_WRITE(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
		CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR, 0x18);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"28", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_SEL_OFFSET_ADDR,
		0, 31, 0x18);

	/* 28 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"28", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_DBG_OUT_OFFSET_ADDR));

	/* 29 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"29", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_VON_BUS_DEBUG_INFO_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_VON_BUS_DEBUG_INFO_OFFSET_ADDR));

	/* 30 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"30", 0x18023000 + CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_INFO_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_INFO_OFFSET_ADDR));

	/* 31 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"31", 0x18023000 + CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_ADDR_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_ADDR_OFFSET_ADDR));

	/* 32 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"32", 0x18023000 + CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_WDATA_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_dbg_ctl_base_mt6989 +
			CONSYS_DBG_GEN_CONN_VON_BUS_APB_TIMEOUT_WDATA_OFFSET_ADDR));
}

void consys_print_bus_debug_dbg_level_2_mt6989_debug_gen(
		int level,
		struct conn_debug_info_mt6989 *pdbg_level_2_info)
{
	if (pdbg_level_2_info == NULL)
		return;

	memset(pdbg_level_2_info, 0, sizeof(struct conn_debug_info_mt6989));

	if (level < 2)
		return;

	if (!vir_addr_consys_dbg_gen_conn_infra_axi_layer_bus_bcrm_base_mt6989) {
		pr_notice("vir_addr_consys_dbg_gen_conn_infra_axi_layer_bus_bcrm_base_mt6989(%x) ioremap fail\n",
				CONSYS_DBG_GEN_CONN_INFRA_AXI_LAYER_BUS_BCRM_BASE_ADDR);
		return;
	}

	if (CONN_BUS_CR_ON_BASE == 0) {
		pr_notice("CONN_BUS_CR_ON_BASE is not defined\n");
		return;
	}

	if (CONN_OFF_DEBUG_CTRL_AO_BASE == 0) {
		pr_notice("CONN_OFF_DEBUG_CTRL_AO_BASE is not defined\n");
		return;
	}

	/* 33 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"33", 0x1800e000 + CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_0_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_BUS_CR_ON_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_0_OFFSET_ADDR));

	/* 34 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"34", 0x1800e000 + CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_BUS_CR_ON_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_1_OFFSET_ADDR));

	/* 35 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"35", 0x1800e000 + CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_2_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_BUS_CR_ON_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_ON_BUS_APB_TIMEOUT_INFO_2_OFFSET_ADDR));

	/* 36 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"36", 0x1804c000 + CONSYS_DGEN_INFRA_AXI_LAYER_BUS_BCRM_1804c004_OFFSET,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_infra_axi_layer_bus_bcrm_base_mt6989 +
			CONSYS_DGEN_INFRA_AXI_LAYER_BUS_BCRM_1804c004_OFFSET));

	/* 37 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"37", 0x1804c000 + CONSYS_DGEN_INFRA_AXI_LAYER_BUS_BCRM_1804c00c_OFFSET,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_infra_axi_layer_bus_bcrm_base_mt6989 +
			CONSYS_DGEN_INFRA_AXI_LAYER_BUS_BCRM_1804c00c_OFFSET));

	/* 38 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"38", 0x1804d000 +
		CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_OFF_DEBUG_CTRL_AO_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL0_OFFSET_ADDR));

	/* 39 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"39", 0x1804d000 +
		CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_OFF_DEBUG_CTRL_AO_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL1_OFFSET_ADDR));

	/* 40 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"40", 0x1804d000 +
		CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL2_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_OFF_DEBUG_CTRL_AO_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_CTRL2_OFFSET_ADDR));

	/* 41 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"41", 0x1804d000 +
		CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT0_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_OFF_DEBUG_CTRL_AO_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT0_OFFSET_ADDR));

	/* 42 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"42", 0x1804d000 +
		CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_OFF_DEBUG_CTRL_AO_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT1_OFFSET_ADDR));

	/* 43 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"43", 0x1804d000 +
		CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT2_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_OFF_DEBUG_CTRL_AO_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT2_OFFSET_ADDR));

	/* 44 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"44", 0x1804d000 +
		CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT3_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_OFF_DEBUG_CTRL_AO_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT3_OFFSET_ADDR));

	/* 45 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"45", 0x1804d000 +
		CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT4_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_OFF_DEBUG_CTRL_AO_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT4_OFFSET_ADDR));

	/* 46 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"46", 0x1804d000 +
		CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT5_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_OFF_DEBUG_CTRL_AO_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT5_OFFSET_ADDR));

	/* 47 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"47", 0x1804d000 +
		CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT6_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_OFF_DEBUG_CTRL_AO_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_U_DEBUG_CTRL_AO_CONN_INFRA_OFF_RESULT6_OFFSET_ADDR));

	/* 48 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"48", 0x1804c000 + CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_AXILAYER_U_S1_CTRL_0_OFFSET_ADDR,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_infra_axi_layer_bus_bcrm_base_mt6989 +
			CONSYS_DBG_GEN_CONN_INFRA_VDNR_GEN_AXILAYER_U_S1_CTRL_0_OFFSET_ADDR));

	/* 49 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"49", 0x1804c000 + CONSYS_DGEN_INFRA_AXI_LAYER_BUS_BCRM_1804c004_OFFSET,
		CONSYS_REG_READ(vir_addr_consys_dbg_gen_conn_infra_axi_layer_bus_bcrm_base_mt6989 +
			CONSYS_DGEN_INFRA_AXI_LAYER_BUS_BCRM_1804c004_OFFSET));
}

void consys_print_bus_slpprot_debug_dbg_level_2_mt6989_debug_gen(
		int level,
		struct conn_debug_info_mt6989 *pdbg_level_2_info)
{
	if (pdbg_level_2_info == NULL)
		return;

	memset(pdbg_level_2_info, 0, sizeof(struct conn_debug_info_mt6989));

	if (level < 2)
		return;

	if (CONN_CFG_ON_BASE == 0) {
		pr_notice("CONN_CFG_ON_BASE is not defined\n");
		return;
	}

	/* 1 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"1", 0x18001000 + CONSYS_DBG_GEN_CONN_INFRA_CONN2AP_SLP_STATUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_ON_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CONN2AP_SLP_STATUS_OFFSET_ADDR));

	/* 2 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"2", 0x18001000 + CONSYS_DBG_GEN_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_ON_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_OFFSET_ADDR));

	/* 3 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"3", 0x18001000 + CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_SLP_STATUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_ON_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_OFF_BUS_SLP_STATUS_OFFSET_ADDR));

	/* 4 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"4", 0x18001000 + CONSYS_DBG_GEN_CONN_INFRA_WF_SLP_STATUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_ON_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_WF_SLP_STATUS_OFFSET_ADDR));

	/* 5 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"5", 0x18001000 + CONSYS_DBG_GEN_GALS_CONN2BT_SLP_STATUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_ON_BASE +
			CONSYS_DBG_GEN_GALS_CONN2BT_SLP_STATUS_OFFSET_ADDR));

	/* 6 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"6", 0x18001000 + CONSYS_DBG_GEN_GALS_BT2CONN_SLP_STATUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_ON_BASE +
			CONSYS_DBG_GEN_GALS_BT2CONN_SLP_STATUS_OFFSET_ADDR));

	/* 7 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"7", 0x18001000 + CONSYS_DBG_GEN_GALS_CONN2GPS_SLP_STATUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_ON_BASE +
			CONSYS_DBG_GEN_GALS_CONN2GPS_SLP_STATUS_OFFSET_ADDR));

	/* 8 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"8", 0x18001000 + CONSYS_DBG_GEN_GALS_GPS2CONN_SLP_STATUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_ON_BASE +
			CONSYS_DBG_GEN_GALS_GPS2CONN_SLP_STATUS_OFFSET_ADDR));

	/* 9 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"9", 0x18001000 + CONSYS_DBG_GEN_GALS_AXI_LAYER_GPS2CONN_SLP_STATUS_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_ON_BASE +
			CONSYS_DBG_GEN_GALS_AXI_LAYER_GPS2CONN_SLP_STATUS_OFFSET_ADDR));
}

void consys_print_bus_slpprot_debug_dbg_level_0_mt6989_debug_gen(
		int level,
		struct conn_debug_info_mt6989 *pdbg_level_0_info)
{
	if (pdbg_level_0_info == NULL)
		return;

	memset(pdbg_level_0_info, 0, sizeof(struct conn_debug_info_mt6989));

	if (level < 0)
		return;

	if (INFRABUS_AO_REG_BASE_ADDR == 0) {
		pr_notice("INFRABUS_AO_REG_BASE_ADDR is not defined\n");
		return;
	}

	/* 10 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"10", 0x1002C000 + CONSYS_DBG_GEN_INFRASYS_PROTECT_EN_SET_0_OFFSET_ADDR,
		CONSYS_REG_READ(INFRABUS_AO_REG_BASE_ADDR +
			CONSYS_DBG_GEN_INFRASYS_PROTECT_EN_SET_0_OFFSET_ADDR));

	/* 11 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"11", 0x1002C000 + CONSYS_DBG_GEN_INFRASYS_PROTECT_RDY_STA_0_OFFSET_ADDR,
		CONSYS_REG_READ(INFRABUS_AO_REG_BASE_ADDR +
			CONSYS_DBG_GEN_INFRASYS_PROTECT_RDY_STA_0_OFFSET_ADDR));
}

void consys_print_top_power_debug_dbg_level_0_mt6989_debug_gen(
		struct conn_debug_info_mt6989 *pdbg_level_0_info)
{
	if (pdbg_level_0_info == NULL)
		return;

	memset(pdbg_level_0_info, 0, sizeof(struct conn_debug_info_mt6989));

	if (CONN_HOST_CSR_TOP_BASE == 0) {
		pr_notice("CONN_HOST_CSR_TOP_BASE is not defined\n");
		return;
	}

	if (CONN_DBG_CTL_BASE == 0) {
		pr_notice("CONN_DBG_CTL_BASE is not defined\n");
		return;
	}

	/* A1 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A1", 0x18060294,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE + 0x294));

	/* A2 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		"A2", 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_WAKEPU_TOP_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_WAKEPU_TOP_ADDR));

	/* A3 */
	CONSYS_REG_WRITE(CONN_DBG_CTL_BASE, 0x1);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_0_info,
		"A3", 0x18023000, 0, 31, 0x1);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_0_info,
		NULL, 0x18023000, CONSYS_REG_READ(CONN_DBG_CTL_BASE));
}

void consys_print_top_power_debug_dbg_level_1_mt6989_debug_gen(
		struct conn_debug_info_mt6989 *pdbg_level_1_info)
{
	if (pdbg_level_1_info == NULL)
		return;

	memset(pdbg_level_1_info, 0, sizeof(struct conn_debug_info_mt6989));

	if (CONN_HOST_CSR_TOP_BASE == 0) {
		pr_notice("CONN_HOST_CSR_TOP_BASE is not defined\n");
		return;
	}

	/* A1 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"A1", 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_SYSSTRAP_OUT_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_SYSSTRAP_OUT_ADDR));

	/* A2 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		"A2", 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CLKGEN_ON_DBG_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CLKGEN_ON_DBG_ADDR));

	/* A3 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x0, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A3", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x0);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* A4 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x1, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A4", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x1);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* A5 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x2, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A5", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x2);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* A6 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x3, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A6", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x3);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* A7 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x4, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A7", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x4);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* A8 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x5, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A8", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x5);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* A9 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x6, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A9", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x6);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* A10 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x7, 0x7);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A10", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 2, 0x7);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_CFG_ON_DBG_OFFSET_ADDR));

	/* A11 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x0, 0xf);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A11", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 3, 0x0);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR));

	/* A12 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x1, 0xf);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A12", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 3, 0x1);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR));

	/* A13 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x2, 0xf);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A13", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 3, 0x2);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR));

	/* A14 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x3, 0xf);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A14", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 3, 0x3);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR));

	/* A15 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x4, 0xf);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A15", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 3, 0x4);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR));

	/* A16 */
	CONSYS_REG_WRITE_MASK(CONN_HOST_CSR_TOP_BASE +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR, 0x5, 0xf);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_1_info,
		"A16", 0x18060000 +
		CONSYS_DBG_GEN_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_OFFSET_ADDR,
		0, 3, 0x5);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_1_info,
		NULL, 0x18060000 + CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_RGU_ON_DBG_ADDR));
}

void consys_print_top_power_debug_dbg_level_2_mt6989_debug_gen(
		struct conn_debug_info_mt6989 *pdbg_level_2_info)
{
	if (pdbg_level_2_info == NULL)
		return;

	memset(pdbg_level_2_info, 0, sizeof(struct conn_debug_info_mt6989));

	if (CONN_DBG_CTL_BASE == 0) {
		pr_notice("CONN_DBG_CTL_BASE is not defined\n");
		return;
	}

	if (CONN_CFG_BASE == 0) {
		pr_notice("CONN_CFG_BASE is not defined\n");
		return;
	}

	/* B1 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"B1", 0x18023000 + CONSYS_DBG_GEN_CONN_INFRA_MONFLAG_OUT_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_DBG_CTL_BASE +
			CONSYS_DBG_GEN_CONN_INFRA_MONFLAG_OUT_OFFSET_ADDR));

	/* B2 */
	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		"B2", 0x18011000 + CONSYS_DBG_GEN_EMI_PROBE_ADDR,
		CONSYS_REG_READ(CONN_CFG_BASE + CONSYS_DBG_GEN_EMI_PROBE_ADDR));

	/* B3 */
	CONSYS_REG_WRITE_MASK(CONN_CFG_BASE +
		CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR, 0x0, 0x600000);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_2_info,
		"B3", 0x18011000 + CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR,
		21, 22, 0x0);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		NULL, 0x18011000 + CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_BASE +
			CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR));

	/* B4 */
	CONSYS_REG_WRITE_MASK(CONN_CFG_BASE +
		CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR, 0x1, 0x600000);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_2_info,
		"B4", 0x18011000 + CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR,
		21, 22, 0x1);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		NULL, 0x18011000 + CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_BASE +
			CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR));

	/* B5 */
	CONSYS_REG_WRITE_MASK(CONN_CFG_BASE +
		CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR, 0x2, 0x600000);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_2_info,
		"B5", 0x18011000 + CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR,
		21, 22, 0x2);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		NULL, 0x18011000 + CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_BASE +
			CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR));

	/* B6 */
	CONSYS_REG_WRITE_MASK(CONN_CFG_BASE +
		CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR, 0x3, 0x600000);
	update_debug_write_info_mt6989_debug_gen(pdbg_level_2_info,
		"B6", 0x18011000 + CONSYS_DBG_GEN_EMI_CTL_0_OFFSET_ADDR,
		21, 22, 0x3);

	update_debug_read_info_mt6989_debug_gen(pdbg_level_2_info,
		NULL, 0x18011000 + CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR,
		CONSYS_REG_READ(CONN_CFG_BASE +
			CONSYS_DBG_GEN_EMI_PROBE_1_OFFSET_ADDR));
}
