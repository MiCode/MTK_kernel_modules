// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include "../../../../include/conninfra.h"
#include "../include/connsys_smc.h"
#include "../include/consys_hw.h"
#include "include/mt6899_atf.h"

unsigned int consys_emi_set_remapping_reg_mt6899_atf(
	phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr,
	phys_addr_t gps_emi_base_addr)
{
	int ret;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_EMI_SET_REMAPPING_REG_OPID,
			     0, 0, 0, 0, 0, 0, ret);
	return ret;
}

