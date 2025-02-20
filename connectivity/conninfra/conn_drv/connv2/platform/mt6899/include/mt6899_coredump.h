/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#ifndef MT6899_COREDUMP_H
#define MT6899_COREDUMP_H

#include "connsys_debug_utility.h"

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ********************************************************************************
 */
struct coredump_hw_config *consys_plt_coredump_get_platform_config_mt6899(int conn_type);
unsigned int consys_plt_coredump_get_platform_chipid_mt6899(void);
char *consys_plt_coredump_get_task_string_mt6899(int conn_type, unsigned int task_id);
char *consys_plt_coredump_get_sys_name_mt6899(int conn_type);
bool consys_plt_coredump_is_host_view_cr_mt6899(unsigned int addr, unsigned int *host_view);
bool consys_plt_coredump_is_host_csr_readable_mt6899(void);
enum cr_category consys_plt_coredump_get_cr_category_mt6899(unsigned int addr);

unsigned int consys_plt_coredump_get_emi_offset_mt6899(int conn_type);
void consys_plt_coredump_get_emi_phy_addr_mt6899(phys_addr_t *base, unsigned int *size);
void consys_plt_coredump_get_mcif_emi_phy_addr_mt6899(phys_addr_t *base, unsigned int *size);
unsigned int consys_plt_coredump_setup_dynamic_remap_mt6899(int conn_type, unsigned int idx,
	unsigned int base, unsigned int length);
void __iomem *consys_plt_coredump_remap_mt6899(int conn_type, unsigned int base,
	unsigned int length);
void consys_plt_coredump_unmap_mt6899(void __iomem *vir_addr);
char *consys_plt_coredump_get_tag_name_mt6899(int conn_type);
bool consys_plt_coredump_is_supported_mt6899(unsigned int drv);

int consys_plt_coredump_setup_dump_region_mt6899_atf(int conn_type);
unsigned int consys_plt_coredump_setup_dynamic_remap_mt6899_atf(int conn_type, unsigned int idx,
	unsigned int base, unsigned int length);


#endif /* MT6899_COREDUMP_H */
