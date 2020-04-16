/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

#include "consys_reg_mng.h"

struct consys_reg_mng_ops* g_consys_reg_ops = NULL;

struct consys_reg_mng_ops* __weak get_consys_reg_mng_ops(void)
{
	pr_warn("No specify project\n");
	return NULL;
}

int consys_hw_reg_readable(void)
{
	if (g_consys_reg_ops &&
		g_consys_reg_ops->consys_reg_mng_check_reable)
		return g_consys_reg_ops->consys_reg_mng_check_reable();
	return -1;
}

int consys_hw_is_connsys_reg(phys_addr_t addr)
{
	if (g_consys_reg_ops &&
		g_consys_reg_ops->consys_reg_mng_is_consys_reg)
		return g_consys_reg_ops->consys_reg_mng_is_consys_reg(addr);
	return -1;
}

int consys_reg_mng_init(struct platform_device *pdev)
{
	int ret = 0;
	if (g_consys_reg_ops == NULL)
		g_consys_reg_ops = get_consys_reg_mng_ops();

	if (g_consys_reg_ops &&
		g_consys_reg_ops->consys_reg_mng_init)
		ret = g_consys_reg_ops->consys_reg_mng_init(pdev);
	else
		ret = EFAULT;

	return ret;
}

int consys_reg_mng_deinit(void)
{
	if (g_consys_reg_ops&&
		g_consys_reg_ops->consys_reg_mng_deinit)
		g_consys_reg_ops->consys_reg_mng_deinit();

	return 0;
}

