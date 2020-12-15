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

#ifndef _PLATFORM_CONSYS_REG_MNG_H_
#define _PLATFORM_CONSYS_REG_MNG_H_

#include <linux/platform_device.h>

struct consys_reg_mng_ops {

	int(*consys_reg_mng_init) (struct platform_device *pdev);
	int(*consys_reg_mng_deinit) (void);
	int(*consys_reg_mng_check_reable) (void);
	int(*consys_reg_mng_is_consys_reg) (unsigned int addr);

};

int consys_reg_mng_init(struct platform_device *pdev);
int consys_reg_mng_deinit(void);

int consys_hw_reg_readable(void);
int consys_hw_is_connsys_reg(phys_addr_t addr);

#endif				/* _PLATFORM_CONSYS_REG_MNG_H_ */
