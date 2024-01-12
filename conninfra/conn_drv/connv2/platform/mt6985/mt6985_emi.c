// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "../include/connsys_smc.h"
#include "../include/emi_mng.h"
#include "include/mt6985.h"
#include "include/mt6985_pos.h"

/* In connac3 project, remap CR need to be written in security world.
 */
struct consys_platform_emi_ops g_consys_platform_emi_ops_mt6985 = {
	.consys_ic_emi_mpu_set_region_protection = NULL,
	.consys_ic_emi_set_remapping_reg = consys_emi_set_remapping_reg_mt6985_atf,
	.consys_ic_emi_get_md_shared_emi = NULL,
};

unsigned int consys_emi_set_remapping_reg_mt6985_atf(
	phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr,
	phys_addr_t gps_emi_base_addr)
{
	int ret;

	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_EMI_SET_REMAPPING_REG_OPID,
			     0, 0, 0, 0, 0, 0, ret);

	return ret;
}
