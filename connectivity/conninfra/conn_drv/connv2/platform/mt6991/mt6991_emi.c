// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "../include/connsys_smc.h"
#include "../include/emi_mng.h"
#include "include/mt6991.h"
#include "include/mt6991_pos.h"

/* In connac3 project, remap CR need to be written in security world.
 */
struct consys_platform_emi_ops g_consys_platform_emi_ops_mt6991 = {
	.consys_ic_emi_mpu_set_region_protection = NULL,
	.consys_ic_emi_set_remapping_reg = consys_emi_set_remapping_reg_mt6991_atf,
	.consys_ic_emi_get_md_shared_emi = NULL,
};

unsigned int consys_emi_set_remapping_reg_mt6991_atf(
	phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr,
	phys_addr_t gps_emi_base_addr)
{
	int ret = 0;

#ifndef CONFIG_FPGA_EARLY_PORTING
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_EMI_SET_REMAPPING_REG_OPID,
			     CONN_HW_VER_MT6991, CONNSYS_A_DIE_ID_MT6991,
			     0, 0, 0, 0, ret);
#endif
	return ret;
}
