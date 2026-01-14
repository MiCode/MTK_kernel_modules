/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _PLATFORM_MT6985_CONSYS_REG_H_
#define _PLATFORM_MT6985_CONSYS_REG_H_

#include "../../include/consys_reg_base.h"
#include "../../include/consys_reg_mng.h"

enum connsys_base_addr_index {
	CONN_CFG_BASE_INDEX		= 0,	/* 0x1801_1000 conn_cfg*/
	CONN_HOST_CSR_TOP_BASE_INDEX	= 1,	/* 0x1806_0000 */
	CONN_INFRA_SYSRAM_INDEX		= 2,	/* 0x1805_0000 */
	CONN_BUS_CR_BASE_INDEX		= 3,	/* 0x1804_B000 */
	CONN_RGU_ON_BASE_INDEX		= 4,	/* 0x1800_0000 */
	CONN_WT_SLP_CTL_REG_BASE_INDEX	= 5,	/* 0x1800_3000 */
	CONN_CFG_ON_BASE_INDEX		= 6,	/* 0x1800_1000 */
	CONN_BUS_CR_ON_BASE_INDEX	= 7,	/* 0x1800_E000 */
	CONN_OFF_DEBUG_CTRL_AO_BASE_INDEX = 8,	/* 0x1804_D000 */
	CONN_CLKGEN_TOP_BASE_INDEX	= 9,	/* 0x1801_2000 */
	CONN_RF_SPI_MST_REG_BASE_INDEX	= 10,	/* 0x1804_2000 */
	INFRBUS_AO_REG_INDEX		= 11,	/* 0x1002_C000 */
	SPM_INDEX			= 12,	/* 0x1C00_1000 */
	CONN_SEMAPHORE_BASE_INDEX	= 13,	/* 0x1807_0000 */
	CONN_AFE_CTL_BASE_INDEX		= 14,	/* 0x1804_1000 */
	VLPSYS_SRCLKENRC		= 15,	/* 0x1c00_d000 */
	CONN_INFRA_DBG_CTL_BASE_INDEX	= 16,	/* 0x1802_3000 size: 0x1000 */
	CONSYS_BASE_ADDR_MAX
};

struct consys_base_addr {
	struct consys_reg_base_addr reg_base_addr[CONSYS_BASE_ADDR_MAX];
};

extern struct consys_base_addr g_conn_reg_mt6985;

#define CONN_CFG_BASE_ADDR_MT6985		g_conn_reg_mt6985.reg_base_addr[CONN_CFG_BASE_INDEX].vir_addr
#define CONN_HOST_CSR_TOP_BASE_ADDR_MT6985	g_conn_reg_mt6985.reg_base_addr[CONN_HOST_CSR_TOP_BASE_INDEX].vir_addr
#define CONN_INFRA_SYSRAM_BASE_ADDR_MT6985	g_conn_reg_mt6985.reg_base_addr[CONN_INFRA_SYSRAM_INDEX].vir_addr
#define CONN_BUS_CR_BASE_ADDR_MT6985		g_conn_reg_mt6985.reg_base_addr[CONN_BUS_CR_BASE_INDEX].vir_addr
#define CONN_RGU_ON_BASE_ADDR_MT6985		g_conn_reg_mt6985.reg_base_addr[CONN_RGU_ON_BASE_INDEX].vir_addr
#define CONN_WT_SLP_CTL_REG_BASE_ADDR_MT6985	g_conn_reg_mt6985.reg_base_addr[CONN_WT_SLP_CTL_REG_BASE_INDEX].vir_addr
#define CONN_CFG_ON_BASE_ADDR_MT6985		g_conn_reg_mt6985.reg_base_addr[CONN_CFG_ON_BASE_INDEX].vir_addr
#define CONN_BUS_CR_ON_BASE_ADDR_MT6985		g_conn_reg_mt6985.reg_base_addr[CONN_BUS_CR_ON_BASE_INDEX].vir_addr
#define CONN_OFF_DEBUG_CTRL_AO_BASE_ADDR_MT6985	g_conn_reg_mt6985.reg_base_addr[CONN_OFF_DEBUG_CTRL_AO_BASE_INDEX].vir_addr
#define CONN_CLKGEN_TOP_BASE_ADDR_MT6985	g_conn_reg_mt6985.reg_base_addr[CONN_CLKGEN_TOP_BASE_INDEX].vir_addr
#define CONN_RF_SPI_MST_REG_BASE_ADDR_MT6985	g_conn_reg_mt6985.reg_base_addr[CONN_RF_SPI_MST_REG_BASE_INDEX].vir_addr
#define INFRBUS_AO_REG_BASE_ADDR_MT6985		g_conn_reg_mt6985.reg_base_addr[INFRBUS_AO_REG_INDEX].vir_addr
#define SPM_BASE_ADDR_MT6985			g_conn_reg_mt6985.reg_base_addr[SPM_INDEX].vir_addr
#define CONN_SEMAPHORE_BASE_ADDR_MT6985		g_conn_reg_mt6985.reg_base_addr[CONN_SEMAPHORE_BASE_INDEX].vir_addr
#define CONN_AFE_CTL_ADDR_MT6985		g_conn_reg_mt6985.reg_base_addr[CONN_AFE_CTL_BASE_INDEX].vir_addr
#define VLPSYS_SRCLKENRC_MT6985			g_conn_reg_mt6985.reg_base_addr[VLPSYS_SRCLKENRC].vir_addr
#define CONN_REG_CONN_INFRA_DBG_CTL_ADDR_MT6985	g_conn_reg_mt6985.reg_base_addr[CONN_INFRA_DBG_CTL_BASE_INDEX].vir_addr

int consys_reg_init_mt6985(struct platform_device *pdev);
int consys_reg_deinit_mt6985(void);
int consys_check_reg_readable_mt6985(void);
int consys_check_reg_readable_for_coredump_mt6985(void);
int consys_is_consys_reg_mt6985(unsigned int addr);
int consys_is_bus_hang_mt6985(void);
void consys_debug_init_mt6985(void);
void consys_debug_deinit_mt6985(void);
int consys_check_ap2conn_infra_on_mt6985(void);

void consys_debug_init_mt6985_atf(void);
void consys_debug_deinit_mt6985_atf(void);
int consys_check_conninfra_off_domain_status_mt6985_atf(void);
int consys_check_conninfra_irq_status_mt6985_atf(void);
int consys_print_debug_mt6985_atf(enum conninfra_bus_error_type level);

#endif /* _PLATFORM_MT6985_CONSYS_REG_H_ */
