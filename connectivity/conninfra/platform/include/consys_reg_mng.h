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

enum conn_dump_cpupcr_type
{
	CONN_DUMP_CPUPCR_TYPE_BT = 1,
	CONN_DUMP_CPUPCR_TYPE_WF = 2,
	CONN_DUMP_CPUPCR_TYPE_ALL = 3,
};

struct consys_reg_mng_ops {

	int(*consys_reg_mng_init) (struct platform_device *pdev);
	int(*consys_reg_mng_deinit) (void);
	int(*consys_reg_mng_check_reable) (void);
	int(*consys_reg_mng_is_consys_reg) (unsigned int addr);

	int(*consys_reg_mng_is_bus_hang) (void);
	int(*consys_reg_mng_dump_bus_status) (void);
	int(*consys_reg_mng_dump_conninfra_status) (void);
	int(*consys_reg_mng_dump_cpupcr) (enum conn_dump_cpupcr_type, int times, unsigned long interval_us);

	int(*consys_reg_mng_is_host_csr) (unsigned long addr);

	/* STEP Utility func */
	unsigned long (*consys_reg_mng_validate_idx_n_offset) (unsigned int idx, unsigned long offset);
	int (*consys_reg_mng_find_can_write_reg) (unsigned int * idx, unsigned long* offset);
	unsigned long (*consys_reg_mng_get_phy_addr_by_idx) (unsigned int idx);
	unsigned long (*consys_reg_mng_get_virt_addr_by_idx) (unsigned int idx);
	int (*consys_reg_mng_get_chip_id_idx_offset) (unsigned int *idx, unsigned long *offset);
	int (*consys_reg_mng_get_reg_symbol_num) (void);
};

int consys_reg_mng_init(struct platform_device *pdev);
int consys_reg_mng_deinit(void);

int consys_reg_mng_reg_readable(void);
int consys_reg_mng_is_connsys_reg(phys_addr_t addr);
int consys_reg_mng_reg_read(unsigned long addr, unsigned int *value, unsigned int mask);
int consys_reg_mng_reg_write(unsigned long addr, unsigned int value, unsigned int mask);
int consys_reg_mng_is_bus_hang(void);
int consys_reg_mng_dump_bus_status(void);
int consys_reg_mng_dump_conninfra_status(void);
int consys_reg_mng_dump_cpupcr(enum conn_dump_cpupcr_type dump_type, int times, unsigned long interval_us);

int consys_reg_mng_is_host_csr(unsigned long addr);


/**************** step *************************/
/*********************************************
 * validate the reg base address index and offset
 * Parameters:
 *		idx: base address index
 *		offset: offset from base address
 *
 * Return
 * 		> 0 : phy address of index
 *		= 0 : validate fail
 *********************************************/
unsigned long consys_reg_mng_validate_idx_n_offset(unsigned int idx, unsigned long offset);

/* for consys_step_test */
/*********************************************
 * validate the reg base address index and offset
 * Parameters:
 *		offset : offset of base address index
 *
 * Return
 * 		> 0 : phy address of index
 *		< 0 : can't find
 *********************************************/
int consys_reg_mng_find_can_write_reg(unsigned int *idx, unsigned long* offset);
unsigned long consys_reg_mng_get_phy_addr_by_idx(unsigned int idx);
unsigned long consys_reg_mng_get_virt_addr_by_idx(unsigned int idx);

int consys_reg_mng_get_chip_id_idx_offset(unsigned int *idx, unsigned long *offset);
int consys_reg_mng_get_reg_symbol_num(void);

#endif				/* _PLATFORM_CONSYS_REG_MNG_H_ */
