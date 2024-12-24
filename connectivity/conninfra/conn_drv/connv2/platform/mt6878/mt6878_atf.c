// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "../../../../include/conninfra.h"
#include "../include/connsys_smc.h"
#include "../include/consys_hw.h"
#include "include/mt6878.h"
#include "include/mt6878_atf.h"
#include "include/mt6878_connsyslog.h"
#include "include/mt6878_consys_reg_offset.h"
#include "include/mt6878_pos.h"
#include "include/mt6878_soc.h"

unsigned int consys_emi_set_remapping_reg_mt6878_atf(
	phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr,
	phys_addr_t gps_emi_base_addr)
{
	unsigned int ret = 1;

	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_EMI_SET_REMAPPING_REG_OPID,
			     0, 0, 0, 0, 0, 0, ret);

	return ret;
}

