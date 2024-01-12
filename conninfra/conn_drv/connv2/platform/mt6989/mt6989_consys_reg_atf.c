// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/memblock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>

/* For bus hang error definition */
#include "../include/connsys_smc.h"
#include "conninfra.h"
#include "consys_reg_mng.h"
#include "consys_reg_util.h"
#include "osal.h"
#include "include/mt6989_consys_reg.h"
#include "include/mt6989_consys_reg_offset.h"
#include "include/mt6989_debug_gen.h"
#include "include/mt6989_pos.h"
#include "include/mt6989_pos_gen.h"

int consys_print_debug_mt6989_atf(enum conninfra_bus_error_type level)
{
	int ret;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_PRINT_DEBUG_OPID, level, 0, 0, 0, 0, 0, ret);
	return ret;
}
int consys_check_conninfra_off_domain_status_mt6989_atf(void)
{
	int ret;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_CHECK_CONNINFRA_OFF_DOMAIN_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}
int consys_check_conninfra_irq_status_mt6989_atf(void)
{
	int ret;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_CHECK_CONNINFRA_IRQ_STATUS_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}
void consys_debug_init_mt6989_atf(void)
{
	CONNSYS_SMC_CALL_VOID(SMC_CONNSYS_DEBUG_INIT_OPID, 0, 0, 0, 0, 0, 0);
}
void consys_debug_deinit_mt6989_atf(void)
{
	CONNSYS_SMC_CALL_VOID(SMC_CONNSYS_DEBUG_DEINIT_OPID, 0, 0, 0, 0, 0, 0);
}
