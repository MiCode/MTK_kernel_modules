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

#ifndef _PLATFORM_MT6885_H_
#define _PLATFORM_MT6885_H_

int consys_platform_spm_conn_ctrl(unsigned int enable);
int consys_co_clock_type(void);

struct consys_plat_thermal_data {
	int thermal_b;
	int slop_molecule;
	int offset;
};

void update_thermal_data(struct consys_plat_thermal_data* input);
#endif /* _PLATFORM_MT6885_H_ */
