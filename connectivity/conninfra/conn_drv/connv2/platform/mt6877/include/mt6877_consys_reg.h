/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _PLATFORM_MT6877_CONSYS_REG_H_
#define _PLATFORM_MT6877_CONSYS_REG_H_

#include "consys_reg_base.h"

enum consys_base_addr_index {
	CONN_INFRA_RGU_BASE_INDEX 		= 0,	/* 0x1800_0000 conn_infra_rgu */
	CONN_INFRA_CFG_BASE_INDEX 		= 1,	/* 0x1800_1000 conn_infra_cfg */
	CONN_INFRA_CLKGEN_ON_TOP_BASE_INDEX 	= 2,	/* 0x1800_9000 conn_infra_clkgen_on_top */
	CONN_INFRA_BUS_CR_BASE_INDEX 		= 3,	/* 0x1800_e000 conn_infra_bus_cr */
	CONN_HOST_CSR_TOP_BASE_INDEX 		= 4,	/* 0x1806_0000 conn_host_csr_top */
	INFRACFG_AO_BASE_INDEX 			= 5,	/* 0x1000_1000 infracfg_ao */
	GPIO_BASE_INDEX 			= 6,	/* 0x1000_5000 GPIO */
	SPM_BASE_INDEX 				= 7,	/* 0x1000_6000 spm */
	TOP_RGU_BASE_INDEX 			= 8,	/* 0x1000_7000 top_rgu */
	CONN_WT_SLP_CTL_REG_BASE_INDEX 		= 9,	/* 0x1800_5000 conn_wt_slp_ctl_reg */
	CONN_INFRA_SYSRAM_SW_CR_BASE_INDEX 	= 10,	/* 0x1805_2800 conn_infra_sysram_sw_cr */
	CONN_AFE_CTL_BASE_INDEX 		= 11,	/* 0x1800_3000 conn_afe_ctl */
	CONN_SEMAPHORE_BASE_INDEX 		= 12,	/* 0x1807_0000 conn_semaphore */
	CONN_RF_SPI_MST_REG_BASE_INDEX		= 13,	/* 0x1800_4000 conn_rf_spi_mst_reg */
	IOCFG_RT_BASE_INDEX			= 14,	/* 0x11ea_0000 IOCFG_RT */
	CONN_THERM_CTL_BASE_INDEX		= 15,	/* 0x1800_2000 conn_therm_ctl */
	CONN_BCRM_ON_BASE_INDEX			= 16,	/* 0x1802_e000 conn_bcrm_on */
	CONSYS_BASE_ADDR_MAX
};


struct consys_base_addr {
	struct consys_reg_base_addr reg_base_addr[CONSYS_BASE_ADDR_MAX];
};

extern struct consys_base_addr conn_reg;

#define CONN_REG_CONN_INFRA_RGU_ADDR		conn_reg.reg_base_addr[CONN_INFRA_RGU_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_CFG_ADDR		conn_reg.reg_base_addr[CONN_INFRA_CFG_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_CLKGEN_ON_TOP_ADDR	conn_reg.reg_base_addr[CONN_INFRA_CLKGEN_ON_TOP_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_BUS_CR_ADDR		conn_reg.reg_base_addr[CONN_INFRA_BUS_CR_BASE_INDEX].vir_addr

#define CONN_REG_CONN_HOST_CSR_TOP_ADDR		conn_reg.reg_base_addr[CONN_HOST_CSR_TOP_BASE_INDEX].vir_addr
#define CONN_REG_INFRACFG_AO_ADDR		conn_reg.reg_base_addr[INFRACFG_AO_BASE_INDEX].vir_addr
#define CONN_REG_GPIO_ADDR			conn_reg.reg_base_addr[GPIO_BASE_INDEX].vir_addr
#define CONN_REG_SPM_ADDR			conn_reg.reg_base_addr[SPM_BASE_INDEX].vir_addr

#define CONN_REG_TOP_RGU_ADDR			conn_reg.reg_base_addr[TOP_RGU_BASE_INDEX].vir_addr
#define CONN_REG_CONN_WT_SLP_CTL_REG_ADDR	conn_reg.reg_base_addr[CONN_WT_SLP_CTL_REG_BASE_INDEX].vir_addr
#define CONN_REG_CONN_INFRA_SYSRAM_SW_CR_ADDR	conn_reg.reg_base_addr[CONN_INFRA_SYSRAM_SW_CR_BASE_INDEX].vir_addr
#define CONN_REG_CONN_AFE_CTL_ADDR		conn_reg.reg_base_addr[CONN_AFE_CTL_BASE_INDEX].vir_addr

#define CONN_REG_CONN_SEMAPHORE_ADDR		conn_reg.reg_base_addr[CONN_SEMAPHORE_BASE_INDEX].vir_addr
#define CONN_REG_CONN_RF_SPI_MST_REG_ADDR	conn_reg.reg_base_addr[CONN_RF_SPI_MST_REG_BASE_INDEX].vir_addr
#define CONN_REG_IOCFG_RT_ADDR			conn_reg.reg_base_addr[IOCFG_RT_BASE_INDEX].vir_addr
#define CONN_REG_CONN_THERM_CTL_ADDR		conn_reg.reg_base_addr[CONN_THERM_CTL_BASE_INDEX].vir_addr

#define CONN_REG_CONN_BCRM_ON_ADDR		conn_reg.reg_base_addr[CONN_BCRM_ON_BASE_INDEX].vir_addr

struct consys_base_addr* get_conn_reg_base_addr(void);

#endif /* _PLATFORM_MT6877_CONSYS_REG_H_ */

