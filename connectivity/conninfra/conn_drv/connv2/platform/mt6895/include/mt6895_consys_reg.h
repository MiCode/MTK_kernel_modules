/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _PLATFORM_MT6895_CONSYS_REG_H_
#define _PLATFORM_MT6895_CONSYS_REG_H_

#include "consys_reg_base.h"

enum consys_base_addr_index {
	INFRACFG_AO_BASE_INDEX 			= 0,	/* 0x1000_1000 infracfg_ao */
	GPIO_BASE_INDEX 			= 1,	/* 0x1000_5000 GPIO */
	IOCFG_RT_BASE_INDEX			= 2,	/* 0x11c3_0000 IOCFG_RT */
	IOCFG_RTT_BASE_INDEX			= 3,	/* 0x11ea_0000 IOCFG_RTT */
	CONN_INFRA_RGU_ON_BASE_INDEX 		= 4,	/* 0x1800_0000 conn_infra_rgu_on */
	CONN_INFRA_CFG_ON_BASE_INDEX 		= 5,	/* 0x1800_1000 conn_infra_cfg_on */
	CONN_WT_SLP_CTL_REG_BASE_INDEX 		= 6,	/* 0x1800_3000 conn_wt_slp_ctl_reg */
	CONN_INFRA_BUS_CR_ON_BASE_INDEX		= 7,	/* 0x1800_e000 conn_infra_bus_cr_on */
	CONN_INFRA_CFG_BASE_INDEX 		= 8,	/* 0x1801_1000 conn_infra_cfg */
	CONN_INFRA_CLKGEN_TOP_BASE_INDEX 	= 9,	/* 0x1801_2000 conn_infra_clkgen_top */
	CONN_VON_BUS_BCRM_BASE_INDEX		= 10,	/* 0x1802_0000 conn_von_bus_bcrm */
	CONN_INFRA_DBG_CTL_BASE_INDEX		= 11,	/* 0x1802_3000 conn_dbg_ctl */
	CONN_INFRA_ON_BUS_BCRM_BASE_INDEX	= 12,	/* 0x1803_b000 conn_infra_on_bus_bcrm */
	CONN_THERM_CTL_BASE_INDEX		= 13,	/* 0x1804_0000 conn_therm_ctl */
	CONN_AFE_CTL_BASE_INDEX 		= 14,	/* 0x1804_1000 conn_afe_ctl */
	CONN_RF_SPI_MST_REG_BASE_INDEX		= 15,	/* 0x1804_2000 conn_rf_spi_mst_reg */
	CONN_INFRA_BUS_CR_BASE_INDEX 		= 16,	/* 0x1804_b000 conn_infra_bus_cr */
	CONN_INFRA_OFF_DEBUG_CTRL_AO_BASE_INDEX = 17,	/* 0x1804_d000 conn_infra_off_debug_ctrl_ao */
	CONN_INFRA_OFF_BUS_BCRM_BASE_INDEX	= 18,	/* 0x1804_f000 conn_infra_off_bus_bcrm */
	CONN_INFRA_SYSRAM_SW_CR_BASE_INDEX 	= 19,	/* 0x1805_3800 conn_infra_sysram_sw_cr */
	CONN_HOST_CSR_TOP_BASE_INDEX 		= 20,	/* 0x1806_0000 conn_host_csr_top */
	CONN_SEMAPHORE_BASE_INDEX 		= 21,	/* 0x1807_0000 conn_semaphore */
	SPM_BASE_INDEX 				= 22,	/* 0x1c00_1000 spm */
	TOP_RGU_BASE_INDEX 			= 23,	/* 0x1c00_7000 top_rgu */
	CONSYS_BASE_ADDR_MAX
};


struct consys_base_addr {
	struct consys_reg_base_addr reg_base_addr[CONSYS_BASE_ADDR_MAX];
};

extern struct consys_base_addr conn_reg_mt6895;

#define CONN_REG_INFRACFG_AO_ADDR			conn_reg_mt6895.reg_base_addr[INFRACFG_AO_BASE_INDEX].vir_addr
#define CONN_REG_GPIO_ADDR				conn_reg_mt6895.reg_base_addr[GPIO_BASE_INDEX].vir_addr
#define CONN_REG_IOCFG_RT_ADDR				conn_reg_mt6895.reg_base_addr[IOCFG_RT_BASE_INDEX].vir_addr
#define CONN_REG_IOCFG_RTT_ADDR				conn_reg_mt6895.reg_base_addr[IOCFG_RTT_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_RGU_ON_ADDR			conn_reg_mt6895.reg_base_addr[CONN_INFRA_RGU_ON_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_CFG_ON_ADDR			conn_reg_mt6895.reg_base_addr[CONN_INFRA_CFG_ON_BASE_INDEX].vir_addr
#define CONN_REG_CONN_WT_SLP_CTL_REG_ADDR		conn_reg_mt6895.reg_base_addr[CONN_WT_SLP_CTL_REG_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_BUS_CR_ON_ADDR		conn_reg_mt6895.reg_base_addr[CONN_INFRA_BUS_CR_ON_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_CFG_ADDR			conn_reg_mt6895.reg_base_addr[CONN_INFRA_CFG_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_CLKGEN_TOP_ADDR		conn_reg_mt6895.reg_base_addr[CONN_INFRA_CLKGEN_TOP_BASE_INDEX].vir_addr
#define CONN_REG_CONN_VON_BUS_BCRM_ADDR			conn_reg_mt6895.reg_base_addr[CONN_VON_BUS_BCRM_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_DBG_CTL_ADDR		conn_reg_mt6895.reg_base_addr[CONN_INFRA_DBG_CTL_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_ON_BUS_BCRM_ADDR		conn_reg_mt6895.reg_base_addr[CONN_INFRA_ON_BUS_BCRM_BASE_INDEX].vir_addr
#define CONN_REG_CONN_THERM_CTL_ADDR			conn_reg_mt6895.reg_base_addr[CONN_THERM_CTL_BASE_INDEX].vir_addr
#define CONN_REG_CONN_AFE_CTL_ADDR			conn_reg_mt6895.reg_base_addr[CONN_AFE_CTL_BASE_INDEX].vir_addr
#define CONN_REG_CONN_RF_SPI_MST_REG_ADDR		conn_reg_mt6895.reg_base_addr[CONN_RF_SPI_MST_REG_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_BUS_CR_ADDR			conn_reg_mt6895.reg_base_addr[CONN_INFRA_BUS_CR_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_OFF_DEBUG_CTRL_AO_ADDR	conn_reg_mt6895.reg_base_addr[CONN_INFRA_OFF_DEBUG_CTRL_AO_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_OFF_BUS_BCRM_ADDR		conn_reg_mt6895.reg_base_addr[CONN_INFRA_OFF_BUS_BCRM_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_SYSRAM_SW_CR_ADDR		conn_reg_mt6895.reg_base_addr[CONN_INFRA_SYSRAM_SW_CR_BASE_INDEX].vir_addr
#define CONN_REG_CONN_HOST_CSR_TOP_ADDR			conn_reg_mt6895.reg_base_addr[CONN_HOST_CSR_TOP_BASE_INDEX].vir_addr
#define CONN_REG_CONN_SEMAPHORE_ADDR			conn_reg_mt6895.reg_base_addr[CONN_SEMAPHORE_BASE_INDEX].vir_addr
#define CONN_REG_SPM_ADDR				conn_reg_mt6895.reg_base_addr[SPM_BASE_INDEX].vir_addr
#define CONN_REG_TOP_RGU_ADDR				conn_reg_mt6895.reg_base_addr[TOP_RGU_BASE_INDEX].vir_addr

int consys_check_conninfra_on_domain_mt6895(void);

#endif /* _PLATFORM_MT6895_CONSYS_REG_H_ */

