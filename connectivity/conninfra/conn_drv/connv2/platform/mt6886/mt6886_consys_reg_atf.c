// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "../include/connsys_smc.h"
#include "../include/consys_hw.h"
#include "../include/consys_reg_mng.h"
#include "../include/plat_library.h"
#include "include/mt6886_atf.h"

int consys_print_debug_mt6886_atf(enum conninfra_bus_error_type level)
{
	int ret;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_PRINT_DEBUG_OPID, level, 0, 0, 0, 0, 0, ret);
	return ret;
}

int consys_check_conninfra_off_domain_status_mt6886_atf(void)
{
	int ret;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_CHECK_CONNINFRA_OFF_DOMAIN_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int consys_check_conninfra_irq_status_mt6886_atf(void)
{
	int ret;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_CHECK_CONNINFRA_IRQ_STATUS_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

void consys_debug_init_mt6886_atf(void)
{
	CONNSYS_SMC_CALL_VOID(SMC_CONNSYS_DEBUG_INIT_OPID, 0, 0, 0, 0, 0, 0);
}

void consys_debug_deinit_mt6886_atf(void)
{
	CONNSYS_SMC_CALL_VOID(SMC_CONNSYS_DEBUG_DEINIT_OPID, 0, 0, 0, 0, 0, 0);
}

